#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "gpurender.h"

#include <QFile>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    dataPath(QApplication::applicationDirPath().toStdString() + "/data/")
{
    ui->setupUi(this);

    settings = new Settings(dataPath + "settings.xml");

    for(uint i = 0; i < settings->camparams.size(); i++) {
        std::string calibResTxt = dataPath + "calib_results_"
                + std::to_string(i + 1) + ".txt";
        Camera *pcam = new Camera(calibResTxt, i, settings->camparams[i]->sf,
                                  settings->camparams[i]->roi,
                                  settings->camparams[i]->contourMinSize);
        pcam->setIntrinsic(dataPath, "frame" + std::to_string(i + 1) + "_",
                           settings->camparams[i]->chessboardNum,
                           settings->chessboardSize);
        pcam->setTemplate(dataPath + "template_" + std::to_string(i + 1) + ".txt");

        cameras.push_back(pcam);
    }
    Camera::normTemplate(cameras);

    //extrinsic
    for(uint i = 0; i < cameras.size(); i++) {
        cv::Mat dist = cv::imread(dataPath + "ex" + std::to_string(i + 1) + ".jpg");
        cameras[i]->setExtrinsic(dist);
    }

    int nopZ = settings->nopZ;
    for(Camera *pcam : cameras) {
        int tmp = pcam->getBowlHeight(
                    settings->radiusScale * pcam->getBaseRadius(),
                    settings->stepX);
        nopZ = std::min(nopZ, tmp);
    }

    for(uint i = 0; i < cameras.size(); i++) {
        CurvilinearGrid *pgrid =
                new CurvilinearGrid(settings->angles, settings->startAngle,
                                    nopZ, settings->stepX);
        pgrid->createGrid(cameras[i],
                          settings->radiusScale * cameras[i]->getBaseRadius());
        grids.push_back(pgrid);
    }

//    saveGrids();

//    render = new GpuRender;
//    setCentralWidget(render);

//    timer = new QTimer(this);
//    connect(timer, SIGNAL(timeout()), render, SLOT(update()));
//    timer->start(30);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete settings;
    for(Camera *pcam : cameras) {
        delete pcam;
    }

    for(CurvilinearGrid *pgrid : grids) {
        delete pgrid;
    }
}

void MainWindow::saveGrids()
{
    vector< vector<Point3f> > seams;
    vector<Point3f> seam_points;

    for (uint i = 0; i < grids.size(); i++)
    {
        grids[i]->saveGrid(cameras[i]);
        grids[i]->getSeamPoints(seam_points);	// Get grid seams
        seams.push_back(seam_points);
    }

    Masks masks;
    masks.createMasks(cameras, seams, settings->smoothAngle); // Calculate masks for blending
    masks.splitGrids();

//    Compensator compensator(Size(param.disp_width, param.disp_height)); // Exposure correction
//    compensator.feed(cameras, seams);
//    compensator.save((char*)"./compensator");
}
