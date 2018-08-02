#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "gpurender.h"
#include "exposure_compensator.hpp"
#include "svgpurender.h"

#include <QFile>
#include <QTimer>
#include <QWizard>
#include <QMessageBox>
#include <QLayout>
#include <QSpacerItem>
#include <QDesktopWidget>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    appPath(QApplication::applicationDirPath().toStdString() + "/"),
    contentPath(appPath + "/Content/")
{
    ui->setupUi(this);

    settings = new Settings(contentPath + "settings.xml");

    std::string cameraModelPath = contentPath + "camera_models/";
    for(uint i = 0; i < settings->camparams.size(); i++) {
        std::string calibResTxt = cameraModelPath + "calib_results_"
                + std::to_string(i + 1) + ".txt";
        CameraCalibrator *pcam = new CameraCalibrator(calibResTxt, i, settings->camparams[i]->sf,
                                  settings->camparams[i]->roi,
                                  settings->camparams[i]->contourMinSize);
        if (pcam->setIntrinsic(cameraModelPath + "chessboard_" + std::to_string(i + 1) + "/",
                               "frame" + std::to_string(i + 1) + "_",
                               settings->camparams[i]->chessboardNum,
                               settings->chessboardSize)) {
            QMessageBox::critical(this, " ", "set intrinsic error",
                                  QMessageBox::Cancel);
            exit(-1);
        }
        if (pcam->setTemplate(contentPath + "template/" +
                              "template_" + std::to_string(i + 1) + ".txt")) {
            QMessageBox::critical(this, " ", "set template error",
                                  QMessageBox::Cancel);
            exit(-1);
        }

        camCalibs.push_back(pcam);

        if (addCamera(settings->camparams[i]->devId,
                      settings->camparams[i]->width,
                      settings->camparams[i]->height)) {
            std::cout << "add camera error" << std::endl;
        }
    }
    CameraCalibrator::normTemplate(camCalibs);

    if(runCamera()) {
        std::cout << "camera start failed" << std::endl;
    }

    ui->statusBar->showMessage("fisheye view");

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateRender);
    timer->start(50);
}

MainWindow::~MainWindow()
{
//    cv::FileStorage fs(dataPath + "status.xml", cv::FileStorage::WRITE);
//    fs << "readyToShow" << readyToShow;

    delete ui;
    delete settings;

    for(v4l2Camera &cap : v4l2_cameras) {
        cap.stopCapturing();
    }

    for(CameraCalibrator *pcam : camCalibs) {
        delete pcam;
    }

    for(CurvilinearGrid *pgrid : grids) {
        delete pgrid;
    }
}

int MainWindow::getContours(float **gl_lines)
{
    float** contours = (float**)calloc(camCalibs.size(), sizeof(float*)); // Contour arrays for each camera
    int array_num[camCalibs.size()] = {0}; // Number of array elements for each camera
    int sum_num = 0;
    int index = 0;

    for (uint i = 0; i < camCalibs.size(); i++)
    {
        if(searchContours(i) == 0)
        {
            array_num[i] = camCalibs[i]->getContours(&contours[i]);
            sum_num += array_num[i];
        }
    }
    *gl_lines = new float[sum_num];
    for (uint i = 0; i < camCalibs.size(); i++)
    {
        for (int j = 0; j < array_num[i]; j++)
        {
            (*gl_lines)[index] = contours[i][j];
            index++;
        }
    }
    for (int i = camCalibs.size() - 1; i >= 0; i--)
        if (contours[i]) free(contours[i]);
    if (contours) free(contours);

    return sum_num;
}

int MainWindow::getGrids(float **gl_grid)
{
    if(camCalibs.size() == 0) return 0;
    float** grids_data = (float**)calloc(camCalibs.size(), sizeof(float*)); // Contour arrays for each camera
    if (!grids_data)
    {
        cout << "Cannot allocate memory" << endl;
        return(0);
    }
    int array_num[camCalibs.size()] = {0}; // Number of array elements for each camera
    int sum_num = 0;
    int index = 0;

    int nopZ = settings->nopZ;
    for(CameraCalibrator *pcam : camCalibs) {
        int tmp = pcam->getBowlHeight(
                    settings->radiusScale * pcam->getBaseRadius(),
                    settings->stepX);
        nopZ = std::min(nopZ, tmp);
    }

    vector< vector<Point3f> > seam;
    for(uint i = 0; i < camCalibs.size(); i++) {
        CurvilinearGrid *pgrid =
                new CurvilinearGrid(settings->angles, settings->startAngle,
                                    nopZ, settings->stepX);
        pgrid->createGrid(camCalibs[i],
                          settings->radiusScale * camCalibs[i]->getBaseRadius());
        grids.push_back(pgrid);
        array_num[i] = pgrid->getGrid(&grids_data[i]);
        sum_num += array_num[i];
    }

    *gl_grid = new float[sum_num];
    for (uint i = 0; i < camCalibs.size(); i++)
    {
        for (int j = 0; j < array_num[i]; j++)
        {
            (*gl_grid)[index] = grids_data[i][j];
            index++;
        }
    }

    for (int i = camCalibs.size() - 1; i >= 0; i--)
        if (grids_data[i]) free(grids_data[i]);
    if (grids_data) free(grids_data);

    return sum_num;
}

int MainWindow::searchContours(int index)
{
    Mat img = takeFrame(index);
    if (camCalibs[index]->setExtrinsic(img) != 0)
        return (-1);

    return 0;
}

void MainWindow::updateRender()
{

    ui->glRender->update();
}

void MainWindow::saveGrids()
{
    vector< vector<Point3f> > seams;
    vector<Point3f> seam_points;

    for (uint i = 0; i < grids.size(); i++)
    {
        grids[i]->saveGrid(camCalibs[i]);
        grids[i]->getSeamPoints(seam_points);	// Get grid seams
        seams.push_back(seam_points);
    }

    Masks masks;
    masks.createMasks(camCalibs, seams, settings->smoothAngle, appPath); // Calculate masks for blending
    masks.splitGrids(appPath);

    QRect rec = QApplication::desktop()->screenGeometry();

    Compensator compensator(Size(rec.width(), rec.height())); // Exposure correction
    compensator.feed(camCalibs, seams);
    compensator.save((appPath + "/compensator").c_str());
}

void MainWindow::switchState(viewStates new_state)
{
    float* data;
    int data_num;

    switch (new_state) {
    case fisheye_view:
        ui->statusBar->showMessage("fisheye view");
        for(uint i = 0; i < ui->glRender->mesh_index.size(); i++) {
            std::string fileName = contentPath + "meshes/original/mesh" +
                    std::to_string(i + 1);
            ui->glRender->reloadMesh(ui->glRender->mesh_index.at(i), fileName);
        }
        ui->glRender->setRenderState(GpuRender::RenderBase);
        break;
    case defisheye_view:
        ui->statusBar->showMessage("defisheye view");
        for(uint i = 0; i < ui->glRender->mesh_index.size(); i++) {
            ui->glRender->changeMesh(camCalibs[i]->xmap, camCalibs[i]->ymap,
                                    10, Point2f(((i & 1) - 1.0), ((~i >> 1) & 1)),
                                    i);
        }
        ui->glRender->setRenderState(GpuRender::RenderBase);
        break;
    case contours_view:
        ui->statusBar->showMessage("contours view");
        data_num = getContours(&data);

        if(contoursVaoIndex == 0) {
            contoursVaoIndex = ui->glRender->addBuffer(&data[0], data_num / 3);
        } else {
            ui->glRender->updateBuffer(contoursVaoIndex, &data[0], data_num / 3);
        }
        ui->glRender->setBufferAsAttr(contoursVaoIndex, 1, (char*)"vPosition");
        if (data) free(data);
        ui->glRender->setRenderState(GpuRender::RenderLines);
        break;
    case grids_view:
        ui->statusBar->showMessage("grids view");
        data_num = getGrids(&data);
        if(gridsVaoIndex == 0) {
            gridsVaoIndex = ui->glRender->addBuffer(&data[0], data_num / 3);
        } else {
            ui->glRender->updateBuffer(gridsVaoIndex, &data[0], data_num / 3);
        }
        ui->glRender->setBufferAsAttr(gridsVaoIndex, 1, (char*)"vPosition");
        if (data) free(data);
        ui->glRender->setRenderState(GpuRender::RenderGrids);
        break;
    case result_view:
    {
        ui->statusBar->showMessage("result view");
        saveGrids();
        timer->stop();
        SvGpuRender *svRender = new SvGpuRender(&ui->glRender->v4l2_cameras);
        svRender->setPath(appPath);
        svRender->setParam(settings->cameraNum, camCalibs.at(0)->model.model.img_size.width,
                     camCalibs.at(0)->model.model.img_size.height,
                     settings->model_scale);
        svRender->showFullScreen();

        break;
    }
    default:
        break;
    }
}

int MainWindow::addCamera(int devId, int width, int height)
{
    string dev_name = "/dev/video" + to_string(devId);

    v4l2Camera v4l2_camera(width, height, CAM_PIXEL_TYPE, V4L2_MEMORY_MMAP, dev_name.c_str());
    v4l2_cameras.push_back(v4l2_camera);

    int current_index = v4l2_cameras.size() - 1;

    if (v4l2_cameras[current_index].captureSetup() == -1)
    {
        cout << "v4l_capture_setup failed camera " << index << endl;
        return (-1);
    }

    return 0;
}

int MainWindow::runCamera()
{
    for(v4l2Camera &v4l : v4l2_cameras) {
        if(v4l.startCapturing())
            return -1;
        if(v4l.getFrame())
            return -1;
    }
    return 0;
}

Mat MainWindow::takeFrame(int index)
{
    Mat out;

    // Lock the camera frame
    pthread_mutex_lock(&v4l2_cameras[index].th_mutex);

    Mat rgba(v4l2_cameras[index].getHeight(),
             v4l2_cameras[index].getWidth(),
             CV_8UC4, (char*)v4l2_cameras[index].buffers[v4l2_cameras[index].fill_buffer_inx].start);
    cvtColor(rgba, out, CV_RGBA2RGB);

    pthread_mutex_unlock(&v4l2_cameras[index].th_mutex);

    return out;
}

void MainWindow::on_backButton_clicked()
{
    if(state > fisheye_view) {
        state = (viewStates)(state - 1);
        switchState(state);
    }
}

void MainWindow::on_nextButton_clicked()
{
    if(state < result_view) {
        state = (viewStates)(state + 1);
        switchState(state);
    }
}
