#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "camera.h"
#include "grid.hpp"
#include "masks.hpp"
#include "settings.h"

#include "gpurender.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    vector<Camera *> cameras;
    vector<CurvilinearGrid*> grids;	// Grids
    const std::string dataPath;
    Settings *settings;

    GpuRender *render;
    QTimer *timer;
    void saveGrids();
};

#endif // MAINWINDOW_H
