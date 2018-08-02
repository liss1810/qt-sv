#include "gpurender.h"
#include "shaders.hpp"

#include <glm/gtx/string_cast.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <QElapsedTimer>
#include <QDebug>
#include <QFile>
#include <QEvent>
#include <QGesture>
#include <QtMath>
#include <QApplication>

#include <GLES/egl.h>

#ifndef GL_VIV_direct_texture
#define GL_VIV_YV12                     0x8FC0
#define GL_VIV_NV12                     0x8FC1
#define GL_VIV_YUY2                     0x8FC2
#define GL_VIV_UYVY                     0x8FC3
#define GL_VIV_NV21                     0x8FC4
#endif

/* GL_VIV_clamp_to_border */
#ifndef GL_VIV_clamp_to_border
#define GL_VIV_clamp_to_border 1
#define GL_CLAMP_TO_BORDER_VIV         0x812D
#endif

#define CAR_ORIENTATION_X 90.0f
#define CAR_ORIENTATION_Y 270.0f
#define CAM_LIMIT_RY_MIN -1.57f
#define CAM_LIMIT_RY_MAX 0.0f
#define CAM_LIMIT_ZOOM_MIN -11.5f
#define CAM_LIMIT_ZOOM_MAX -2.5f

GpuRender::GpuRender(QWidget *parent) :
    QOpenGLWidget(parent)
//    pFNglTexDirectVIVMap(NULL),
//    pFNglTexDirectInvalidateVIV(NULL),
//    camParas(NULL),
//    paintFlag(false)
{
//    setAttribute(Qt::WA_AlwaysStackOnTop);
    setAttribute(Qt::WA_AcceptTouchEvents);

    pFNglTexDirectVIVMap = (PFNGLTEXDIRECTVIVMAP)
            eglGetProcAddress("glTexDirectVIVMap");
    if(pFNglTexDirectVIVMap == NULL) {
        qInfo() << "error no glTexDirectVIVMap function";
    }

    pFNglTexDirectInvalidateVIV = (PFNGLTEXDIRECTINVALIDATEVIV)
            eglGetProcAddress("glTexDirectInvalidateVIV");
    if(pFNglTexDirectInvalidateVIV == NULL) {
        qInfo() << "error no glTexDirectInvalidateVIV function";
    }

//    camera3D = new Camera3D;

//    gain = new Gains(param.disp_width, param.disp_height);
//    gain->updateGains();
}

GpuRender::~GpuRender()
{
    makeCurrent();
    for(QOpenGLShaderProgram *m_program : renderPrograms)
        delete m_program;

    for(vertices_obj &obj : v_obj) {
        glDeleteTextures(1, &obj.tex);
        glDeleteBuffers(1, &obj.vbo);
    }
    doneCurrent();
}

int GpuRender::setProgram(uint index)
{
    currentProgram = index;
}

int GpuRender::addMesh(string filename)
{
    makeCurrent();

    ///////////////////////////////// Load vertices arrays ///////////////////////////////
    vertices_obj vo_tmp;
    GLfloat* vert;
    vLoad(&vert, &vo_tmp.num, filename);

    //////////////////////// Camera textures initialization /////////////////////////////
    glGenVertexArrays(1, &vo_tmp.vao);
    glGenBuffers(1, &vo_tmp.vbo);

    bufferObjectInit(&vo_tmp.vao, &vo_tmp.vbo, vert, vo_tmp.num);
    texture2dInit(&vo_tmp.tex);

    v_obj.push_back(vo_tmp);

    if(vert) free(vert);

    doneCurrent();

    mesh_index.push_back(v_obj.size() - 1);

    return (v_obj.size() - 1);
}

void GpuRender::reloadMesh(int index, string filename)
{
    makeCurrent();

    GLfloat* vert;
    vLoad(&vert, &v_obj[index].num, filename);
    glBindBuffer(GL_ARRAY_BUFFER, v_obj[index].vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 5 * v_obj[index].num, &vert[0], GL_DYNAMIC_DRAW);
    if(vert) free(vert);

    doneCurrent();
}

int GpuRender::changeMesh(Mat xmap, Mat ymap, int density, Point2f top, int index)
{
    makeCurrent();

    if ((xmap.rows == 0) || (xmap.cols == 0) || (ymap.rows == 0) || (ymap.cols == 0))
    {
        cout << "Mesh was not generated. LUTs are empty" << endl;
        return (-1);
    }

    int rows = xmap.rows / density;
    int cols = xmap.cols / density;

    GLfloat* vert = NULL;
    v_obj[index].num = 6 * rows * cols;
    vert = (GLfloat*)calloc((int)(v_obj[index].num * 5), sizeof(GLfloat));

    if (vert == NULL) {
        cout << "Memory allocation did not complete successfully" << endl;
        return(-1);
    }

    float x_norm = 1.0 / xmap.cols;
    float y_norm = 1.0 / xmap.rows;


    int k =  0;
    for (int row = 1; row < rows; row++)
        for (int col = 1; col < cols; col++)
        {
            /****************************************** Get triangles *********************************************
             *   							  v3 _  v2
             *   Triangles orientation: 		| /|		1 triangle (v4-v1-v2)
             *   								|/_|		2 triangle (v4-v2-v3)
             *   							  v4   v1
             *******************************************************************************************************/
            // Vertices
            Point2f v1 = Point2f(col * density, row * density);
            Point2f v2 = Point2f(col * density, (row - 1) * density);
            Point2f v3 = Point2f((col - 1) * density, (row - 1) * density);
            Point2f v4 = Point2f((col - 1) * density, row * density);

                                // Texels
            Point2f p1 = Point2f(xmap.at<float>(v1), ymap.at<float>(v1));
            Point2f p2 = Point2f(xmap.at<float>(v2), ymap.at<float>(v2));
            Point2f p3 = Point2f(xmap.at<float>(v3), ymap.at<float>(v3));
            Point2f p4 = Point2f(xmap.at<float>(v4), ymap.at<float>(v4));

            if ((p2.x > 0) && (p2.y > 0) && (p2.x < xmap.cols) && (p2.y < xmap.rows) &&	// Check if p2 belongs to the input frame
               (p4.x > 0) && (p4.y > 0) && (p4.x < xmap.cols) && (p4.y < xmap.rows))		// Check if p4 belongs to the input frame
            {
                // Save triangle points to the output file
                /*******************************************************************************************************
                 *   							  		v2
                 *   1 triangle (v4-v1-v2): 		  /|
                 *   								 /_|
                 *   							  v4   v1
                 *******************************************************************************************************/
                if ((p1.x >= 0) && (p1.y >= 0) && (p1.x < xmap.cols) && (p1.y < xmap.rows))	// Check if p1 belongs to the input frame
                {
                    vert[k] = v1.x * x_norm + top.x;
                    vert[k + 1] = (top.y - v1.y * y_norm);
                    vert[k + 2] = 0;
                    vert[k + 3] = p1.x * x_norm;
                    vert[k + 4] = p1.y * y_norm;

                    vert[k + 5] = v2.x * x_norm + top.x;
                    vert[k + 6] = (top.y - v2.y * y_norm);
                    vert[k + 7] = 0;
                    vert[k + 8] = p2.x * x_norm;
                    vert[k + 9] = p2.y * y_norm;

                    vert[k + 10] = v4.x * x_norm + top.x;
                    vert[k + 11] = (top.y - v4.y * y_norm);
                    vert[k + 12] = 0;
                    vert[k + 13] = p4.x * x_norm;
                    vert[k + 14] = p4.y * y_norm;

                    k += 15;
                }

                /*******************************************************************************************************
                 *   							  v3 _	v2
                 *   2 triangle (v4-v2-v3): 		| /
                 *   								|/
                 *   							  v4
                 *******************************************************************************************************/
                if ((p3.x > 0) && (p3.y > 0) && (p3.x < xmap.cols) && (p3.y < xmap.rows))	// Check if p3 belongs to the input frame)
                {
                    vert[k] = v4.x * x_norm + top.x;
                    vert[k + 1] = (top.y - v4.y * y_norm);
                    vert[k + 2] = 0;
                    vert[k + 3] = p4.x * x_norm;
                    vert[k + 4] = p4.y * y_norm;

                    vert[k + 5] = v2.x * x_norm + top.x;
                    vert[k + 6] = (top.y - v2.y * y_norm);
                    vert[k + 7] = 0;
                    vert[k + 8] = p2.x * x_norm;
                    vert[k + 9] = p2.y * y_norm;

                    vert[k + 10] = v3.x * x_norm + top.x;
                    vert[k + 11] = (top.y - v3.y * y_norm);
                    vert[k + 12] = 0;
                    vert[k + 13] = p3.x * x_norm;
                    vert[k + 14] = p3.y * y_norm;

                    k += 15;
                }
            }
    }


    glBindBuffer(GL_ARRAY_BUFFER, v_obj[index].vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 5 * v_obj[index].num, &vert[0], GL_DYNAMIC_DRAW);
    if(vert) free(vert);
    doneCurrent();

    return (0);
}

int GpuRender::addBuffer(GLfloat *buf, int num)
{
    makeCurrent();

    ///////////////////////////////// Load vertices arrays ///////////////////////////////
    vertices_obj vo_tmp;
    vo_tmp.num = num;

    //////////////////////// Camera textures initialization /////////////////////////////
    glGenVertexArrays(1, &vo_tmp.vao);
    glGenBuffers(1, &vo_tmp.vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vo_tmp.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * num, &buf[0], GL_DYNAMIC_DRAW);

    v_obj.push_back(vo_tmp);

    doneCurrent();

    return (v_obj.size() - 1);
}

int GpuRender::setBufferAsAttr(int buf_num, int prog_num, char *atr_name)
{
    if ((buf_num < 0) || (buf_num >= (int)v_obj.size()) || (prog_num < 0) || (prog_num >= (int)renderPrograms.size()))
        return (-1);

    makeCurrent();

    glBindBuffer(GL_ARRAY_BUFFER, v_obj[buf_num].vbo);
    glBindVertexArray(v_obj[buf_num].vao);
    GLint position_attribute = glGetAttribLocation(renderPrograms.at(prog_num)->programId(), atr_name);
    glVertexAttribPointer(position_attribute, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(position_attribute);

    doneCurrent();

    return (0);
}

void GpuRender::renderBuffer(int buf_num, int type, int vert_num)
{
    makeCurrent();

    renderPrograms.at(1)->bind();

    glBindVertexArray(v_obj[buf_num].vao);
    switch (type)
    {
    case 0:
        glLineWidth(2.0);
        glDrawArrays(GL_LINES, 0, vert_num);
        break;
    case 1:
        glBeginTransformFeedback(GL_POINTS);
        glDrawArrays(GL_POINTS, 0, vert_num);
        glEndTransformFeedback();
        break;
    default:
        break;
    }

    doneCurrent();
}

void GpuRender::updateBuffer(int buf_num, GLfloat *buf, int num)
{
    v_obj[buf_num].num = num;
    glBindBuffer(GL_ARRAY_BUFFER, v_obj[buf_num].vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * num, &buf[0], GL_DYNAMIC_DRAW);
}

void GpuRender::initializeGL()
{
    initializeOpenGLFunctions();

    QOpenGLShaderProgram *m_program = new QOpenGLShaderProgram;
    m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/vshader.vsh");
    m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/fshader.fsh");
    m_program->link();
    renderPrograms.push_back(m_program);

    m_program = new QOpenGLShaderProgram;
    m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/line.vsh");
    m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/line.fsh");
    m_program->link();
    renderPrograms.push_back(m_program);

    currentProgram = 0;

    for(uint i = 0; i < 4; i++) {
        addMesh(QApplication::applicationDirPath().toStdString() + "/Content/" +
                "/meshes/original/mesh" +
                to_string(i + 1));
    }
}

void GpuRender::paintGL()
{
    // Clear background.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int i;
    renderPrograms.at(0)->bind();


    for(uint j = 0; j < v4l2_cameras.size(); j++) {
        pthread_mutex_lock(&v4l2_cameras[j].th_mutex);

        // Get index of the newes camera buffer
        if (v4l2_cameras[j].fill_buffer_inx == -1) i = 0;
        else  i = v4l2_cameras[j].fill_buffer_inx;

        glBindVertexArray(v_obj[j].vao);
        glActiveTexture(GL_TEXTURE0);

        glBindTexture(GL_TEXTURE_2D, v_obj[j].tex);
        glUniform1i(renderPrograms.at(0)->uniformLocation("myTexture"), 0);

        (*pFNglTexDirectVIVMap)(GL_TEXTURE_2D, v4l2_cameras[j].getWidth(),
                                v4l2_cameras[j].getHeight(),
                                GL_PIXEL_TYPE,
                                (GLvoid **)& v4l2_cameras[j].buffers[i].start,
                                (const GLuint *)(&v4l2_cameras[j].buffers[i].offset));
        (*pFNglTexDirectInvalidateVIV)(GL_TEXTURE_2D);

        glDrawArrays(GL_TRIANGLES, 0, v_obj[j].num);

        glBindVertexArray(0);
        pthread_mutex_unlock(&v4l2_cameras[j].th_mutex);
    }

    renderPrograms.at(1)->bind();
    if(renderState == RenderLines) {
        if(v_obj.size() < 5)
            return;
        glBindVertexArray(v_obj[4].vao);
        glLineWidth(2.0);
        glDrawArrays(GL_LINES, 0, v_obj[4].num);
    } else if(renderState == RenderGrids) {
        if(v_obj.size() < 6)
            return;
        glBindVertexArray(v_obj[5].vao);
        glBeginTransformFeedback(GL_POINTS);
        glDrawArrays(GL_POINTS, 0, v_obj[5].num);
        glEndTransformFeedback();
    }
}

void GpuRender::resizeGL(int w, int h)
{
    // Calculate aspect ratio
    float aspect = float(w) / float(h ? h : 1);
//    camera3D->setAspectRatio(aspect);
}

//bool GpuRender::event(QEvent *e)
//{
//    if(e->type() == QEvent::TouchBegin) {
//        return true;
//    }
//    if(e->type() == QEvent::TouchEnd) {
//        return true;
//    }
//    if(e->type() == QEvent::TouchUpdate) {
////        qInfo() << "touch update";
//        QTouchEvent *touchEvent = static_cast<QTouchEvent*>(e);
//        auto points = touchEvent->touchPoints();
//        auto sizePoints = points.size();

//        if(sizePoints == 1) {
//            QVector2D diff = QVector2D(points[0].pos() - points[0].lastPos());
////            camera3D->viewRotate(QVector2D(diff));
//            diff *= 0.1;
//            rx += glm::radians(diff.x());
//            ry += glm::radians(diff.y());
//            ry = glm::clamp(ry, CAM_LIMIT_RY_MIN, CAM_LIMIT_RY_MAX);
//            return true;
//        }

//        if(points.size() == 2) {
//            QPointF p1(points[0].pos() - points[1].pos());
//            QPointF p2(points[0].lastPos() - points[1].lastPos());
//            float len1 = qSqrt(qPow(p1.x(), 2) + qPow(p1.y(), 2));
//            float len2 = qSqrt(qPow(p2.x(), 2) + qPow(p2.y(), 2));
////            camera3D->setFovBias(len2 - len1);
//            float bias = len2 - len1;
//            bias *= 0.01;
//            pz -= bias;
//            pz = glm::clamp(pz, CAM_LIMIT_ZOOM_MIN, CAM_LIMIT_ZOOM_MAX);
//            return true;
//        }
//        return true;
//    }
//    return QOpenGLWidget::event(e);
//}

//void CalibGpuRender::mousePressEvent(QMouseEvent *e)
//{
//    mousePressPos = QVector2D(e->localPos());
//}

//void CalibGpuRender::mouseReleaseEvent(QMouseEvent *e)
//{
//    QVector2D diff = QVector2D(e->localPos()) - mousePressPos;
////            camera3D->viewRotate(QVector2D(diff));
//    diff *= 0.1;
//    rx += glm::radians(diff.x());
//    ry += glm::radians(diff.y());
//    ry = glm::clamp(ry, CAM_LIMIT_RY_MIN, CAM_LIMIT_RY_MAX);
//}

//void CalibGpuRender::wheelEvent(QWheelEvent *event)
//{
//    float bias = event->angleDelta().y();
//    bias *= 0.01;
//    pz += bias;
//    pz = glm::clamp(pz, CAM_LIMIT_ZOOM_MIN, CAM_LIMIT_ZOOM_MAX);
//}

void GpuRender::vLoad(GLfloat **vert, int *num, string filename)
{
    ifstream input(filename.c_str());
    *num = count(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>(), '\n'); // Get line number from the array file
    input.clear();
    input.seekg(0, ios::beg); // Returning to the beginning of fstream

    *vert = NULL;
    *vert = (GLfloat*)malloc((*num) * 5 * sizeof(GLfloat));
    if (*vert == NULL) 	{
        cout << "Memory allocation did not complete successfully" << endl;
    }
    for (int k = 0; k < (*num) * 5; k++)
    {
        input >> (*vert)[k];
    }
    input.close();
}

void GpuRender::bufferObjectInit(GLuint *text_vao, GLuint *text_vbo, GLfloat *vert, int num)
{
    makeCurrent();

    // rectangle
    glBindBuffer(GL_ARRAY_BUFFER, *text_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 5 * num, &vert[0], GL_STATIC_DRAW);
    glBindVertexArray(*text_vao);
    glBindBuffer(GL_ARRAY_BUFFER, *text_vbo);
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
    // TexCoord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    doneCurrent();
}

void GpuRender::texture2dInit(GLuint *texture)
{
    makeCurrent();

    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    doneCurrent();
}
