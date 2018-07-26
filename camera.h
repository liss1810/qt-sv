#ifndef CAMERA_H
#define CAMERA_H

#include "v4l2capture.h"
#include "defisheye.hpp"

#include <opencv2/opencv.hpp>

#include <QGenericMatrix>
#include <QVector4D>

class Camera
{
public:
    struct Parameters
    {
        cv::Mat K;
        cv::Mat distCoeffs;
        cv::Mat rvec;
        cv::Mat tvec;
    };

    struct Template {
        static cv::Size posterSize;
        float maxX;
        QVector<cv::Point3f> ref_points;
    };

    Camera();
    Camera(const std::string &calibFilePath, int index_ = -1, float sf_ = 10,
           float roi_ = 0.5, int cntrMinSize_ = 200);

    int setIntrinsic(const std::string &path, const std::string &name,
                     int img_num, cv::Size patternSize);
    int setTemplate(const std::string &tempPath);
    static void normTemplate(std::vector<Camera *> &cameras);
    int setExtrinsic(const cv::Mat &img);
    void updateLUT(float scale);
    void setCntr_min_size(int value) { cntrMinSize = value; }
    void defisheye(Mat &img, Mat &out) {remap(img, out, xmap, ymap, cv::INTER_LINEAR);}
    double getBaseRadius() {return radius;}
    int getBowlHeight(double radius, double step_x);

    Mat getK() {Mat M; param.K.copyTo(M); return M;} // Get camera matrix
    Mat getDistCoeffs() {Mat M; param.distCoeffs.copyTo(M); return M;} // Get distortion coefficients
    Mat getRvec() {Mat M; param.rvec.copyTo(M); return M;} // Get rotation vector
    Mat getTvec() {Mat M; param.tvec.copyTo(M); return M;} // Get translation vector

    Defisheye model;
    cv::Mat xmap, ymap;
    int index;
    Template temp;

private:
    float sf;
    Parameters param;
    std::vector<cv::Point2f> img_p;
    double radius;
    float roi;
    int cntrMinSize;
    V4l2Capture *capture;

    int getImagePoints(cv::Mat &undist_img, uint num,
                       std::vector<cv::Point2f> &img_points);
};

#endif // CAMERA_H
