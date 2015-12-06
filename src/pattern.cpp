
#include "pattern.h"
#include <iostream>
#include <iostream>
#include <stdio.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>

using namespace cv;
using namespace std;

#define embedded_header_only
#include "embedded.c"

#include "debug.h"

namespace manus {

Pattern::Pattern(int id, double size, const string& filename, Mat offset): id(id), size(size), filename(filename), offset(offset) {

    char* buffer;
    size_t buffer_size;

    if (!embedded_copy_resource(filename.c_str(), &buffer, &buffer_size))
        throw runtime_error("Unable to load image");

    _InputArray data(buffer, buffer_size);

    Mat img = imdecode(data, CV_LOAD_IMAGE_GRAYSCALE);

    free(buffer);

    //Mat img = imread(filename, 0);
	
	if(img.cols!=img.rows) {
		throw runtime_error("Not a square pattern");
	}

	int msize = PATTERN_SIZE; 

	Mat src(msize, msize, CV_8UC1);
	Point2f center((msize-1)/2.0f,(msize-1)/2.0f);
	Mat rot_mat(2,3,CV_32F);
	
	resize(img, src, Size(msize,msize));
	Mat subImg = src(Range(msize / 4,3 * msize/4), Range(msize / 4,3 * msize / 4));
	markers.push_back(subImg);

	rot_mat = getRotationMatrix2D(center, 90, 1.0);

	for (int i=1; i<4; i++){
		Mat dst= Mat(msize, msize, CV_8UC1);
		rot_mat = getRotationMatrix2D( center, -i * 90, 1.0);
		warpAffine( src, dst , rot_mat, Size(msize,msize));
		Mat subImg = dst(Range(msize/4,3*msize/4), Range(msize/4,3*msize/4));
		markers.push_back(subImg);	
	}

}

int Pattern::getIdentifier() {
    return id;
}

double Pattern::getSize() {
    return size;
}

double Pattern::match(const Mat& src, int& orientation) {

	int i;
	double tempsim;
	double N = (double)(PATTERN_SIZE*PATTERN_SIZE/4);
	double nom, den;

	Scalar mean_ext, std_ext, mean_int, std_int;

    Mat interior = src(cv::Range(PATTERN_SIZE/4,3*PATTERN_SIZE/4), cv::Range(PATTERN_SIZE/4, 3*PATTERN_SIZE/4));

	meanStdDev(src, mean_ext, std_ext);
	meanStdDev(interior, mean_int, std_int);

    //printf("ext: %f int: %f \n", mean_ext.val[0], mean_int.val[0]);

	if ((mean_ext.val[0]>mean_int.val[0]))
		return -1;

	double normSrcSq = pow(norm(interior),2);

	//zero_mean_mode;
	int zero_mean_mode = 1;
	
	//use correlation coefficient as a robust similarity measure
	double confidence = -1.0;
	for(i=0; i < markers.size(); i++){
		
		double const nnn = pow(norm(markers.at(i)),2);

		if (zero_mean_mode ==1){

			double const mmm = mean(markers.at(i)).val[0];
		
			nom = interior.dot(markers.at(i)) - (N * mean_int.val[0] * mmm);
			den = sqrt( (normSrcSq - (N * mean_int.val[0] * mean_int.val[0]) ) * (nnn - (N * mmm * mmm)));
			tempsim = nom/den;
		}
		else 
		{
		    tempsim = interior.dot(markers.at(i))/(sqrt(normSrcSq*nnn));
		}

		if(tempsim > confidence){
			confidence = tempsim;
			orientation = i;
		}
	}
	
    return confidence;
}


PatternDetection::PatternDetection(int id, double size, const Mat& rotVec, const Mat& transVec, double confidence, vector<Point2f> corners) : id(id), size(size), confidence(confidence), rotVec(rotVec), transVec(transVec) {

	orientation = -1;
    this->corners[0] = corners.at(0);
    this->corners[1] = corners.at(1);
    this->corners[2] = corners.at(2);
    this->corners[3] = corners.at(3);
}

void PatternDetection::getExtrinsics(Mat& rotation, Mat& translation) {
	Rodrigues(rotVec, rotation);

    transVec.copyTo(translation);
}

int PatternDetection::getIdentifier() {
    return id;
}

double PatternDetection::getConfidence() {
    return confidence;
}

Point2f PatternDetection::getCorner(int i)
{
    return corners[i];
}

double PatternDetection::getSize()
{
    return size;
}

void PatternDetection::draw(Mat& frame, const Mat& camMatrix, const Mat& distMatrix)
{

	CvScalar color = cvScalar(255,0,255);

	//model 3D points: they must be projected to the image plane
	Mat modelPts = (Mat_<float>(4,3) << 0, 0, 0, size, 0, 0, 0, size, 0, 0, 0, size );

	std::vector<cv::Point2f> model2ImagePts;
	/* project model 3D points to the image. Points through the transformation matrix 
	(defined by rotVec and transVec) are "transfered" from the pattern CS to the 
	camera CS, and then, points are projected using camera parameters 
	(camera matrix, distortion matrix) from the camera 3D CS to its image plane
	*/
	projectPoints(modelPts, rotVec, transVec, camMatrix, distMatrix, model2ImagePts); 

    cv::line(frame, model2ImagePts.at(0), model2ImagePts.at(1), cvScalar(0,0,255), 3);
    cv::line(frame, model2ImagePts.at(0), model2ImagePts.at(2), cvScalar(0,255,0), 3);
	cv::line(frame, model2ImagePts.at(0), model2ImagePts.at(3), cvScalar(255,0,0), 3);

	model2ImagePts.clear();

}

PatternDetector::PatternDetector(double threshold, int block_size, double conf_threshold) {


	this->threshold = threshold;//for image thresholding
	this->block_size = block_size;//for adaptive image thresholding
	this->confThreshold = conf_threshold;//bound for accepted similarities between detected patterns and loaded patterns
	normROI = Mat(PATTERN_SIZE, PATTERN_SIZE, CV_8UC1);//normalized ROI
	
	//corner of normalized area
	norm2DPts[0] = Point2f(0, 0);
	norm2DPts[1] = Point2f(PATTERN_SIZE-1, 0);
	norm2DPts[2] = Point2f(PATTERN_SIZE-1, PATTERN_SIZE-1);
	norm2DPts[3] = Point2f(0, PATTERN_SIZE-1);

}

int PatternDetector::loadPattern(const char* filename, double size) {

    library.push_back(make_shared<Pattern>(library.size(), size, filename));
	
    return library.size()-1;
}


void PatternDetector::detect(const Mat& frame, const Mat& cameraMatrix, const Mat& distortions, vector<PatternDetection>& foundPatterns)
{

	Point2f roi2DPts[4];
	Mat binImage2;

	//binarize image
	convertAndBinarize(frame, binImage, grayImage);
	binImage.copyTo(binImage2);

	int avsize = (binImage.rows + binImage.cols) / 2;

	vector<vector<Point> > contours;
	vector<Point> polycont;

	//find contours in binary image
	cv::findContours(binImage2, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);

	unsigned int i;
	Point p;
	int pMinX, pMinY, pMaxY, pMaxX;

	for(i=0; i<contours.size(); i++){
		Mat contourMat = Mat (contours[i]);
		const double per = arcLength( contourMat, true);
		//check the perimeter
		if (per>(avsize/4) && per<(4*avsize)) {
			polycont.clear();
			approxPolyDP( contourMat, polycont, per*0.02, true);

			//check rectangularity and convexity
			if (polycont.size()==4 && isContourConvex(Mat (polycont))){

				//locate the 2D box of contour,
				p = polycont.at(0);
				pMinX = pMaxX = p.x;
				pMinY = pMaxY = p.y;
				int j;
				for(j=1; j<4; j++){
					p = polycont.at(j);
					if (p.x<pMinX){
						pMinX = p.x;
						}
					if (p.x>pMaxX){
						pMaxX = p.x;
						}
					if (p.y<pMinY){
						pMinY = p.y;
						}
					if (p.y>pMaxY){
						pMaxY = p.y;
						}
				}
				Rect box(pMinX, pMinY, pMaxX-pMinX+1, pMaxY-pMinY+1);
				
				//find the upper left vertex
				double d;
				double dmin=(4*avsize*avsize);
				int v1=-1;
				for (j=0; j<4; j++){
					d = norm(polycont.at(j));
					if (d<dmin) {
					dmin=d;
					v1=j;
					}
				}

				//store vertices in refinedVertices and enable sub-pixel refinement if you want
				vector<Point2f> refinedVertices;
				refinedVertices.clear();
				for(j=0; j<4; j++){
					refinedVertices.push_back(polycont.at(j));
				}

				//refine corners
				cornerSubPix(grayImage, refinedVertices, Size(3,3), Size(-1,-1), TermCriteria(1, 3, 1));
				
				//rotate vertices based on upper left vertex; this gives you the most trivial homogrpahy 
				for(j=0; j<4;j++){
					roi2DPts[j] = Point2f(refinedVertices.at((4+v1-j)%4).x - pMinX, refinedVertices.at((4+v1-j)%4).y - pMinY);
				}

				//normalize the ROI (find homography and warp the ROI)
				normalizePattern(grayImage, roi2DPts, box, normROI);

                double confidence = 0;
                int orientation;

				int id = identifyPattern(normROI, confidence, orientation);

				//push-back pattern in the stack of foundPatterns and find its extrinsics
				if (id >= 0) {
					
                    vector<Point2f> candidateCorners;
                    Mat rotVec = (Mat_<float>(3,1) << 0, 0, 0);
                    Mat transVec = (Mat_<float>(3,1) << 0, 0, 0);

					for (j=0; j<4; j++){
						candidateCorners.push_back(refinedVertices.at((8-orientation+v1-j)%4));
					}

					//find the transformation (from camera CS to pattern CS)
					calculateExtrinsics(library[id]->getSize(), cameraMatrix, distortions, rotVec, transVec, candidateCorners);

                    PatternDetection patCand(library[id]->getIdentifier(), library[id]->getSize(), rotVec, transVec, confidence, candidateCorners);
					foundPatterns.push_back(patCand);

				}
			}
		}
	}
}


void PatternDetector::convertAndBinarize(const Mat& src, Mat& dst1, Mat& dst2)
{

	//dst1: binary image
	//dst2: grayscale image

	if (src.channels()==3){
		cvtColor(src, dst2, CV_BGR2GRAY);
	}
	else {
		src.copyTo(dst2);
	}
	
	adaptiveThreshold( dst2, dst1, 255, CV_ADAPTIVE_THRESH_GAUSSIAN_C, CV_THRESH_BINARY_INV, block_size, threshold);
	
	dilate(dst1, dst1, Mat());
}


void PatternDetector::normalizePattern(const Mat& src, const Point2f roiPoints[], Rect& rec, Mat& dst)
{
	

	//compute the homography
	Mat Homo(3,3,CV_32F);
	Homo = getPerspectiveTransform( roiPoints, norm2DPts);
	
	cv::Mat subImg = src(cv::Range(rec.y, rec.y+rec.height), cv::Range(rec.x, rec.x+rec.width));

	//warp the input based on the homography model to get the normalized ROI
	cv::warpPerspective( subImg, dst, Homo, Size(dst.cols, dst.rows));

}

int PatternDetector::identifyPattern(const Mat& src, double& confidence, int& orientation)
{
	if (library.size()<1){
		return -1;
	}

	unsigned int j;
    orientation = 0;
    int match = -1;

	//use correlation coefficient as a robust similarity measure
	confidence = confThreshold;
	for (j=0; j < library.size(); j++){
		
        double m = library[j]->match(src, orientation);

        if (m > confidence) {
            confidence = m;
            match = j;
        }

	}

	return match;

}

void PatternDetector::calculateExtrinsics(const double size, const Mat& cameraMatrix, const Mat& distortions, Mat& rotVec, Mat& transVec, const vector<Point2f>& imagePoints)
{

	//3D points in pattern coordinate system
    vector<Point3f> objectPoints;
    objectPoints.push_back(Point3f(0, 0, 0));
    objectPoints.push_back(Point3f(size, 0, 0));
    objectPoints.push_back(Point3f(size, size, 0));
    objectPoints.push_back(Point3f(0, size, 0));

    solvePnP(objectPoints, imagePoints, cameraMatrix, distortions, rotVec, transVec);
    rotVec.convertTo(rotVec, CV_32F);
    transVec.convertTo(transVec, CV_32F);

	//find extrinsic parameters
	//cvFindExtrinsicCameraParams2(&objectPts, &imagePts, &intrinsics, &distCoeff, &rot, &tra);
}


}
