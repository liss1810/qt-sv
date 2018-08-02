#-------------------------------------------------
#
# Project created by QtCreator 2018-07-23T14:53:32
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = nxp-mysv-autocalib
TEMPLATE = app

QT_CONFIG -= no-pkg-config
CONFIG += link_pkgconfig
PKGCONFIG += opencv glm assimp

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    calibration/src_contours.cpp \
    calibration/defisheye.cpp \
    calibration/grid.cpp \
    calibration/masks.cpp \
    calibration/cameracalibrator.cpp \
    common/settings.cpp \
    common/src_v4l2.cpp \
    render/gpurender.cpp \
    common/exposure_compensator.cpp \
    render/model_loader/Material.cpp \
    render/model_loader/ModelLoader.cpp \
    render/MRT.cpp \
    render/model_loader/VBO.cpp \
    render/svgpurender.cpp

HEADERS += \
        mainwindow.h \
    calibration/src_contours.hpp \
    calibration/defisheye.hpp \
    calibration/grid.hpp \
    calibration/masks.hpp \
    calibration/cameracalibrator.h \
    common/lines.hpp \
    common/settings.h \
    common/src_v4l2.hpp \
    render/gpurender.h \
    common/exposure_compensator.hpp \
    render/model_loader/Material.hpp \
    render/model_loader/ModelLoader.hpp \
    render/MRT.hpp \
    render/model_loader/VBO.hpp \
    render/svgpurender.h

FORMS += \
        mainwindow.ui

RESOURCES += \
    resources.qrc

DISTFILES +=

target.path = /home/root/nxp-mysv-autocalib
INSTALLS += target
