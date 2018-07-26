#ifndef SETTINGS_H
#define SETTINGS_H

#include <opencv2/opencv.hpp>
#include <memory>

class Settings
{
public:
    struct CamParam {
        float sf;
        float roi;
        int contourMinSize;
        int chessboardNum;
    };

    cv::Size chessboardSize;
    float squareSize;
    int cameraNum = 0;
    std::vector<std::shared_ptr<CamParam>> camparams;

    int angles;
    int startAngle;
    int nopZ;
    float stepX;
    float radiusScale;

    float smoothAngle;

    Settings(const std::string &path);
    ~Settings();
    int load(const std::string &path);
};

#endif // SETTINGS_H
