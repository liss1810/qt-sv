#ifndef SETTINGS_H
#define SETTINGS_H

#include <opencv2/opencv.hpp>
#include <memory>

class Settings
{
public:
    struct CamParam {
        float sf = 7;
        float roi = 0.5;
        int contourMinSize = 200;
        int chessboardNum = 3;
    };

    cv::Size chessboardSize;
    float squareSize = 24;
    int cameraNum = 4;
    std::vector<std::shared_ptr<CamParam>> camparams;

    int angles = 60;
    int startAngle = 4;
    int nopZ = 30;
    float stepX = 0.2;
    float radiusScale = 1.5;

    float smoothAngle = 0.2;

    float model_scale[3] = {0.5, 0.5 , 0.5};
    bool readyToShow = false;

    Settings(const std::string &path);
    ~Settings();
    int load(const std::string &path);
    void save(const std::string &path);
};

#endif // SETTINGS_H
