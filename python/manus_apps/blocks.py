#!/usr/bin/python

import numpy as np
import scipy.spatial.distance
import cv2
import sys, os
import math
from numpy.linalg import inv

has_yaml = False
try:
    import yaml

    # A yaml constructor is for loading from a yaml node.
    # This is taken from: http://stackoverflow.com/a/15942429
    def opencv_matrix_constructor(loader, node):
        mapping = loader.construct_mapping(node, deep=True)
        mat = np.array(mapping["data"])
        mat.resize(mapping["rows"], mapping["cols"])
        return mat
    yaml.add_constructor(u"tag:yaml.org,2002:opencv-matrix", opencv_matrix_constructor)

    # A yaml representer is for dumping structs into a yaml node.
    # So for an opencv_matrix type (to be compatible with c++'s FileStorage) we save the rows, cols, type and flattened-data
    def opencv_matrix_representer(dumper, mat):
        mapping = {'rows': mat.shape[0], 'cols': mat.shape[1], 'dt': 'd', 'data': mat.reshape(-1).tolist()}
        return dumper.represent_mapping(u"tag:yaml.org,2002:opencv-matrix", mapping)
    yaml.add_representer(np.ndarray, opencv_matrix_representer)
    has_yaml = True
except ImportError:
    pass

def block_color_name(block):

    h, s, v = block.color

    print h, s, v

    color = 'unknown'
    if v < 70:
        color = 'black'
    elif v > 210 and s < 40:
        color = 'white'
    elif s > 100 and (h < 17 or h > 160):
        color = 'red'
    elif s > 100 and (h > 90 and h < 125):
        color = 'blue'
    elif s > 100 and (h > 35 and h < 70):
        color = 'green'

    print color

    return color

def block_color(block):

    name = block_color_name(block)

    if name == 'unknown':
        return (100, 100, 100)
    elif name == 'black':
        return (0, 0, 0)
    elif name == 'white':
        return (255, 255, 255)
    elif name == 'red':
        return (255, 0, 0)
    elif name == 'green':
        return (0, 255, 0)
    elif name == 'blue':
        return (0, 0, 255)

class Block(object):

    def __init__(self, position, rotation, size, color):
        self.position = position
        self.rotation = rotation
        self.size = size
        self.color = color


class SavedCamera(object):

    def __init__(self, prefix):
        from scipy.io import loadmat

        self.image = cv2.imread(prefix + '.jpg')
        if os.path.isfile(prefix + '.mat'):
            c = loadmat(prefix + '.mat')
            self.intrinsics = c['camera'][0][0][1].astype('float32')
            self.distortion = c['camera'][0][0][2].astype('float32')
            self.rotation = c['view'][0][0][1].astype('float32')
            self.translation = c['view'][0][0][2].astype('float32')
            self.homography = c['view'][0][0][0].astype('float32')
        else:
            with open(prefix + '.yaml', 'r') as f:
                c = yaml.load(f)
                self.intrinsics = c['intrinsics'].astype('float32')
                self.distortion = c['distortion'].astype('float32')
                self.rotation = c['rotation'].astype('float32')
                self.translation = c['translation'].astype('float32')
                self.homography = c['homography'].astype('float32')

    def get_image(self):
        return self.image

    def get_homography(self):
        return self.homography

    def get_rotation(self):
        return self.rotation

    def get_translation(self):
        return self.translation

    def get_intrinsics(self):
        return self.intrinsics

    def get_distortion(self):
        return self.distortion

    def get_width(self):
        return self.image.shape[1]

    def get_height(self):
        return self.image.shape[0]

    @staticmethod
    def save_camera(path, camera):
        if not has_yaml:
            return
        cv2.imwrite(os.path.join(path,"image.jpg"), camera.get_image())
        with open(os.path.join(path, 'image.yaml'), 'w') as f:
            yaml.dump({"rotation": camera.get_rotation(),
                 "translation": camera.get_translation(),
                 "distortion": camera.get_distortion(),
                 "intrinsics": camera.get_intrinsics(),
                 "homography": camera.get_homography()}, f)


class BlockDetector(object):

    def __init__(self, block_size=20, bounds=None):
        self.block_size = block_size
        self.bounds = None
        if not bounds is None:
            self.bounds = np.array(bounds, dtype='float32')
        self.cube = np.transpose(np.array([[-1, -1, -1], [-1, -1, 1], [-1, 1, 1], [-1, 1, -1], [
                                 1, -1, -1], [1, -1, 1], [1, 1, 1], [1, 1, -1]], dtype='float32')) * 0.5 * block_size
        self.cube_homogenous = np.concatenate(
            (self.cube, np.ones((1, 8))), axis=0)


    def _estimate_block_area(self, camera):
        center = np.dot(camera.get_homography(), np.transpose(
            np.array([camera.get_width()/2, camera.get_height()/2, 1], dtype='float32')))
        center = center / center[2]
        D = np.diag((self.block_size * 0.5, self.block_size * 0.5, 1))
        D[0, 2] = center[0]
        D[1, 2] = center[1]
        points = np.array([[-1, -1, 1, 1], [-1, 1, 1, -1],
                           [1, 1, 1, 1]], dtype='float32')
        points = np.dot(np.dot(camera.get_homography(), D), points)
        points = points / np.tile(points[2, :], (3, 1))
        points = np.transpose(points[0:2, :])
        return cv2.contourArea(points.astype('float32'))

    def detect(self, camera, workspace=None):

        image = camera.get_image()
        rotation, _ = cv2.Rodrigues(camera.get_rotation())
        translation = camera.get_translation()
        intrinsics = camera.get_intrinsics()
        distortion = camera.get_distortion()
        homography = camera.get_homography()

        estimated_size = self._estimate_block_area(camera)
        interest_area = math.sqrt(estimated_size) * 2

        params = cv2.SimpleBlobDetector_Params()
        # Filter by Area.
        params.filterByColor = True
        params.minArea = estimated_size * 0.90
        params.maxArea = params.minArea * 2.2
        # Filter by Convexity
        params.filterByConvexity = True
        params.minConvexity = 0.8

        detector = cv2.SimpleBlobDetector(params)
        image_hsv = cv2.cvtColor(image, cv2.COLOR_BGR2HSV)
        candidates = detector.detect(image)
        if not self.bounds is None:
            projected = np.transpose(
                np.matrix([[p[0], p[1], 1] for p in self.bounds]))
            projected = homography.dot(projected)
            points = np.array([[p[0, 0] / p[0, 2], p[0, 1] / p[0, 2]]
                               for p in projected.T], dtype='float32')
            candidates = [c for c in candidates if cv2.pointPolygonTest(points, c.pt, False) >= 0]
        M = np.zeros(image.shape[0:2])

        for i, c in enumerate(candidates):
            M[int(c.pt[1]), int(c.pt[0])] = i + 1

        estimated_width = math.sqrt(estimated_size)
        M1 = cv2.dilate(M, np.ones(
            (int(estimated_width * 0.2), int(estimated_width * 0.2))))
        M2 = cv2.dilate(M, np.ones(
            (int(estimated_width * 2.0), int(estimated_width * 2.0))))

        M = M1
        M = M + 1
        M[np.logical_and(M2 > 0, M1 == 0)] = 0

        S = M.astype('int32')
        cv2.watershed(image, S)

        # cv2.imshow("test", (M * 50).astype('uint8'))
        # cv2.waitKey(0)

        S[:, [1, -1]] = 1
        S[[1, -1], :] = 1

        M = np.logical_not(S == 1)

        H_h, H_s, H_v = cv2.split(image_hsv)
        blocks = []

        Hi = inv(camera.get_homography())

        for c in candidates:

            P = np.dot(Hi, np.transpose(np.array([c.pt[0], c.pt[1], 1])))

            P = P / P[2]
            scores = []

            x1 = int(max(0, c.pt[0] - interest_area))
            x2 = int(min(M.shape[1], c.pt[0] + interest_area))
            y1 = int(max(0, c.pt[1] - interest_area))
            y2 = int(min(M.shape[0], c.pt[1] + interest_area))
            C = M[y1:y2, x1:x2]
            H = np.zeros((y2-y1, x2-x1), dtype='uint8')
            angles = np.linspace(0, math.pi/2, 20)
            for a in angles:
                T = np.matrix([[math.cos(a), -math.sin(a), 0, P[0]], [math.sin(a),
                                                                      math.cos(a), 0, P[1]], [0, 0, 1, 0], [0, 0, 0, 1]])
                Pt = np.dot(T, self.cube_homogenous)
                Pp, _ = cv2.projectPoints(np.transpose(
                    Pt[0:3, :]), rotation, translation, intrinsics, distortion)
                hull = cv2.convexHull(Pp.astype('float32'))
                H.fill(0)
                cv2.fillConvexPoly(H, np.array(
                    [[p[0, 0]-x1+1, p[0, 1]-y1+1] for p in hull], dtype='int32'), 1)
                scores.append(float(np.sum(C[H > 0])) / float(np.sum(H)))
            angles2 = np.linspace(0, math.pi/2, 90)
            scores2 = cv2.GaussianBlur(
                np.interp(angles2, angles, scores), (5, 1), 3)

            k = np.argmax(scores2)
            if scores2[k] > 0.85:
                a = angles2[k]
                k = S[int(c.pt[1]), int(c.pt[0])]

                h = np.median(H_h[S == k])
                s = np.median(H_s[S == k])
                v = np.median(H_v[S == k])

                blocks.append(Block((P[0], P[
                              1], self.block_size / 2), (0, 0, a), (self.block_size, self.block_size, self.block_size), (h, s, v)))

        return blocks

    def draw(self, camera, blocks):
        image = camera.get_image()
        rotation, _ = cv2.Rodrigues(camera.get_rotation())
        translation = camera.get_translation()
        intrinsics = camera.get_intrinsics()
        distortion = camera.get_distortion()

        for b in blocks:
            a = b.rotation[2]
            T = np.matrix([[math.cos(a), -math.sin(a), 0, b.position[0]], [math.sin(a),
                                                                           math.cos(a), 0, b.position[1]], [0, 0, 1, b.position[2]], [0, 0, 0, 1]])
            Pt = np.dot(T, np.array([[0, 0], [0, 10], [0, 0], [1, 1]]))
            Pp, _ = cv2.projectPoints(np.transpose(
                Pt[0:3, :]), rotation, translation, intrinsics, distortion)
            # print Pp, Pt
            cv2.circle(image, (int(Pp[0, 0, 0]), int(
                Pp[0, 0, 1])), 3, (0, 255, 0))
            cv2.line(image, (int(Pp[0, 0, 0]), int(Pp[0, 0, 1])), (int(
                Pp[1, 0, 0]), int(Pp[1, 0, 1])), (0, 255, 0), 1)

        return image

if __name__ == '__main__':
    camera = SavedCamera(sys.argv[1])

    detector = BlockDetector()
    blocks = detector.detect(camera)

    image = detector.draw(camera, blocks)
    cv2.imshow("Blocks", image)
    cv2.waitKey(0)
