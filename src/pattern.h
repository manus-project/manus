/** Based on the ARma library: http://xanthippi.ceid.upatras.gr/people/evangelidis/arma/ */

#ifndef __MANUS_PATTERN_H
#define __MANUS_PATTERN_H

#include <memory>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>

using namespace std;
using namespace cv;

#define PATTERN_SIZE 100

namespace manus {

class Pattern
{
    public:
  		Pattern(int id, double size, const string& filename, Mat offset = cv::Mat::eye(3, 3, CV_32F));

		~Pattern(){};      

        int getIdentifier();

        double getSize();

        double match(const Mat& src, int& orientation);

    private:

        int id;
        double size;
        string filename;
        std::vector<cv::Mat> markers;
        cv::Mat offset;

};


class PatternDetection
{
	public:
		PatternDetection(int id, double size, const Mat& rotVec, const Mat& transVec, double confidence, vector<Point2f> corners);

		~PatternDetection() {};
		
		//augments image with 3D cubes. It;s too simple augmentation jsut for checking
		void draw(Mat& frame, const Mat& camMatrix, const Mat& distMatrix);

		void getExtrinsics(Mat& rotation, Mat& translation);

        int getIdentifier();

        double getConfidence();

        double getSize();

        Point2f getCorner(int i);

private:

		int id;
		int orientation;//{0,1,2,3}
		double size; //in milimeters
		double confidence;//min: -1, max: 1
		Mat rotVec, transVec;
        Point2f corners[4];

};

class PatternDetector
{
public:
    //constructor
	PatternDetector(double threshold = 5, int block_size = 45, double conf_threshold = 0.60);

	//distractor
	~PatternDetector(){};

    int loadPattern(const char* filename, double realsize = 50);


    //detect patterns in the input frame
    void detect(const Mat &frame, const Mat& cameraMatrix, const Mat& distortions, vector<PatternDetection>& foundPatterns);

private:

	//solves the exterior orientation problem between patten and camera
	void calculateExtrinsics(const double size, const Mat& cameraMatrix, const Mat& distortions, Mat& rotVec, Mat& transVec, const vector<Point2f>& vertices);

	void calculateExtrinsicsOld(const double size, const Mat& cameraMatrix, const Mat& distortions, Mat& rotVec, Mat& transVec, const vector<Point2f>& vertices);

    std::vector<shared_ptr<Pattern> > library;

	int block_size;
	double confThreshold, threshold;

	Mat binImage, grayImage, normROI;
	Point2f norm2DPts[4];

	void convertAndBinarize(const Mat& src, Mat& dst1, Mat& dst2);
	void normalizePattern(const Mat& src, const Point2f roiPoints[], Rect& rec, Mat& dst);
	int identifyPattern(const Mat& src, double& confidence, int& orientation);

};

}

#endif
