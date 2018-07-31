#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>

#include "cameracalibrator.h"
#include "grid.hpp"
#include "masks.hpp"
#include "settings.h"

#include "gpurender.h"

namespace Ui {
class MainWindow;
}

/**********************************************************************************************************************
 * Types
 **********************************************************************************************************************/
struct camera_view
{
    int camera_index;
    vector<int> mesh_index;
};

enum viewStates {fisheye_view = 0,
                 defisheye_view,
                 contours_view,
                 grids_view,
                 result_view};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    int getContours(float** gl_lines);
    int getGrids(float** gl_grid);
    int searchContours(int index);
    void updateRender();

private slots:
    void on_backButton_clicked();
    void on_nextButton_clicked();

private:
    Ui::MainWindow *ui;
    vector<camera_view> cam_views;	// View indexes
    vector<CameraCalibrator *> camCalibs;
    vector<CurvilinearGrid*> grids;	// Grids
    const std::string dataPath;
    Settings *settings;
    viewStates state = fisheye_view;
    QTimer *timer;

    int contours_buf = 0;
    int grid_buf = 0;

    void saveGrids();
    void switchState(viewStates new_state);
};

#endif // MAINWINDOW_H
