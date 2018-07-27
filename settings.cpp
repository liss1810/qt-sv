#include "settings.h"
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

Settings::Settings(const std::string &path) :
    chessboardSize(8, 6)
{
    if(load(path)) {
        QMessageBox msgBox;

        msgBox.setText("error with setting file load!");
        msgBox.exec();
        save(path);
    }
}

Settings::~Settings()
{
}

int Settings::load(const std::string &path)
{
    cv::FileStorage fs(path, cv::FileStorage::READ);
    if(!fs.isOpened()) {
        std::cout << path << " not found" << std::endl;
        return -1;
    }

    cv::FileNode n;
    fs["readyToShow"] >> readyToShow;

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

    n = fs["car_model"];
    n["x_scale"]  >> model_scale[0];
    n["y_scale"] >> model_scale[1];
    n["z_scale"] >> model_scale[2];

    return 0;
}

void Settings::save(const std::string &path)
{
    cv::FileStorage fs(path, cv::FileStorage::WRITE);

    if(!fs.isOpened()) {
        std::cout << "can not open " << path << std::endl;
        return;
    }

    fs << "readyToShow" << readyToShow;

    fs << "chessboardSetting" << "{"
       << "width" << chessboardSize.width
       << "height" << chessboardSize.height
       << "squareSize" << squareSize
       << "}";

    if(camparams.empty()) {
        for(uint i = 0; i < cameraNum; i++) {
            std::shared_ptr<CamParam> pcam(new CamParam);
            camparams.push_back(pcam);
        }
    }

    fs << "camera" << "{"
       << "number" << cameraNum;
    for(uint i = 0; i < camparams.size(); i++) {
        fs << "camera" + std::to_string(i + 1) << "{"
           << "sf" << camparams[i]->sf
           << "roi" << camparams[i]->roi
           << "contour_min_size" << camparams[i]->contourMinSize
           << "chessboard_num" << camparams[i]->chessboardNum
           << "}";
    }
    fs << "}";

    fs << "grid" << "{"
       << "angles" << angles
       << "start_angle" << startAngle
       << "nop_z" << nopZ
       << "step_x" << stepX
       << "radius_scale" << radiusScale
       << "}";

    fs << "mask" << "{"
       << "smooth_angle" << smoothAngle
       << "}";

    fs << "car_model" << "{"
       << "x_scale" <<  model_scale[0]
       << "y_scale" << model_scale[1]
       << "z_scale" << model_scale[2]
       << "}";
}
