#include "settings.h"

Settings::Settings(const std::string &path)
{
    load(path);
}

Settings::~Settings()
{
}

int Settings::load(const std::string &path)
{
    cv::FileStorage fs(path, cv::FileStorage::READ);

    cv::FileNode n;
    n = fs["chessboardSetting"];
    n["width"] >> chessboardSize.width;
    n["height"] >> chessboardSize.height;
    n["squareSize"] >> squareSize;

    n = fs["camera"];
    n["number"] >> cameraNum;
    camparams.clear();
    for(int i = 0; i < cameraNum; i++) {
        cv::FileNode sn = n["camera" + std::to_string(i + 1)];
        std::shared_ptr<CamParam> pcam(new CamParam);
        sn["sf"] >> pcam->sf;
        sn["roi"] >> pcam->roi;
        sn["contour_min_size"] >> pcam->contourMinSize;
        sn["chessboard_num"] >> pcam->chessboardNum;
        camparams.push_back(pcam);
    }

    n = fs["grid"];
    n["angles"] >> angles;
    n["start_angle"] >> startAngle;
    n["nop_z"] >> nopZ;
    n["step_x"] >> stepX;
    n["radius_scale"] >> radiusScale;

    n = fs["mask"];
    n["smooth_angle"] >> smoothAngle;

    return 0;
}
