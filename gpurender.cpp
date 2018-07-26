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

#define EGL_API_FB
#include <EGL/egl.h>

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

#define GL_PIXEL_TYPE GL_RGBA
#define CAM_PIXEL_TYPE V4L2_PIX_FMT_RGB32

//Model Loader
glm::mat4 gProjection;	// Initialization is needed
float rx = 0.0f, ry = 0.0f, px = 0.0f, py = 0.0f, pz = -10.0f;
glm::vec3 car_scale = glm::vec3(1.0f, 1.0f, 1.0f);

#define CAR_ORIENTATION_X 90.0f
#define CAR_ORIENTATION_Y 270.0f
#define CAM_LIMIT_RY_MIN -1.57f
#define CAM_LIMIT_RY_MAX 0.0f
#define CAM_LIMIT_ZOOM_MIN -11.5f
#define CAM_LIMIT_ZOOM_MAX -2.5f

GpuRender::GpuRender(QWidget *parent) :
    QOpenGLWidget(parent),
//    pFNglTexDirectVIVMap(NULL),
//    pFNglTexDirectInvalidateVIV(NULL),
//    camParas(NULL),
    m_program(0),
    pblockBuf(NULL)
//    paintFlag(false)
{
    setAttribute(Qt::WA_AlwaysStackOnTop);
    setAttribute(Qt::WA_AcceptTouchEvents);

//    pFNglTexDirectVIVMap = (PFNGLTEXDIRECTVIVMAP)
//            eglGetProcAddress("glTexDirectVIVMap");
//    if(pFNglTexDirectVIVMap == NULL) {
//        qInfo() << "error no glTexDirectVIVMap function";
//    }

//    pFNglTexDirectInvalidateVIV = (PFNGLTEXDIRECTINVALIDATEVIV)
//            eglGetProcAddress("glTexDirectInvalidateVIV");
//    if(pFNglTexDirectInvalidateVIV == NULL) {
//        qInfo() << "error no glTexDirectInvalidateVIV function";
//    }

    for(unsigned char i = 0; i < PLANES; i++) {
        alphaMasks[i] = NULL;
    }

//    camera3D = new Camera3D;

//    gain = new Gains(param.disp_width, param.disp_height);
//    gain->updateGains();
}

GpuRender::~GpuRender()
{
    makeCurrent();

    for(unsigned char i = 0; i < PLANES; i++) {
        if(alphaMasks[i]) {
            delete [] alphaMasks[i];
        }
    }
    delete m_program;

//    foreach (auto &item, objModels) {
//        delete item;
//    }
//    delete camera3D;
    doneCurrent();
}

//void GpuRender::allocate_buffer(uint num)
//{
//    buffers.fill(NULL, num);
////    pTexBuffers.assign(num, NULL);
//}

//void GpuRender::setBuf(QVector<V4l2Capture::CapBuffers *> &pbuf)
//{
//    for(int i = 0; i < buffers.size(); ++i) {
//        buffers[i] = pbuf[i];
//    }

////    for(uint i = 0; i < pTexBuffers.size(); ++i) {
////        pTexBuffers[i] = pbuf[i]->start;
//    //    }
//}

//void GpuRender::enablePaint()
//{
//    paintFlag = 1;
//}

//void GpuRender::setCameraPara(QVector<CameraParameter> &p)
//{
//    camParas = &p;
//}

static const GLint eglTexture[4] = {
    GL_TEXTURE0, GL_TEXTURE1, GL_TEXTURE2, GL_TEXTURE3
};

void GpuRender::initializeGL()
{
    initializeOpenGLFunctions();

//    m_program = new QOpenGLShaderProgram;
//    m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/vshader.vsh");
//    m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/fshader.fsh");
//    m_program->link();
//    m_program->bind();
//    programId = m_program->programId();

//    textureLoc = m_program->uniformLocation("s_texture");
//    glGenTextures(1, &textureCam);
//    glUniform1i(textureLoc, 0);

//    glBindTexture(GL_TEXTURE_2D, textureCam);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


//    extrinsicLoc = glGetUniformLocation(programId, "extrinsic");

//    QString modelDir(QApplication::applicationDirPath() + "/data/");
//    QVector<QString> modelName;
//    modelName << "bowlfront.obj" << "bowlleft.obj"
//              << "bowlback.obj" << "bowlright.obj";
//    for(int i = 0; i < modelName.size(); i++) {
//        objModels.append(new Model( (modelDir + modelName[i]).toStdString() ));
//    }


//    glEnable(GL_BLEND);
//    glBlendFunc(GL_ONE, GL_ONE);
//    glBlendColor(0.0f, 0.0f, 0.0f, 0.0f);

    /* =========================================== */

//    if(setParam(&param) == -1)
//        exit(-1);

    if (programsInit() == -1)
    {
        exit(-1);
    }

    if(!RenderInit()) {
        qInfo() << "render init error";
        return;
    }
    for(int i = 0; i < 4; i++) {
        textures.append(new QOpenGLTexture((QImage(":/images/ex" + QString::number(i + 1) + ".jpg")),
                           QOpenGLTexture::DontGenerateMipMaps));
    }
}

void GpuRender::paintGL()
{
//    if(paintFlag) {

//        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
//        glClear(GL_COLOR_BUFFER_BIT);
//    //    glViewport(0,0,720, 480);

//        m_program->setUniformValue("projection", camera3D->perspectMatrix());
//        m_program->setUniformValue("view", camera3D->viewMatrix());
//    //    for(uint i = 0; i < pTexBuffers.size(); i++) {

//        for(int i = 0; i < buffers.size(); i++) {

//            GLuint phy = ~0U;
//    //        pTexBuffers[i] = NULL;
//            glActiveTexture(GL_TEXTURE0);
//            glBindTexture(GL_TEXTURE_2D, textureCam);

//            (*pFNglTexDirectVIVMap)(GL_TEXTURE_2D, 720, 480, GL_VIV_UYVY,
//                                    /*&pTexBuffers[i]*/&(buffers[i]->start),
//                                    &phy);
//            (*pFNglTexDirectInvalidateVIV)(GL_TEXTURE_2D);
//            glActiveTexture(GL_TEXTURE1);
//            glBindTexture(GL_TEXTURE_2D, maskTextureID[i]);

//            m_program->bind();
//            m_program->setUniformValue("intrinsic", camParas->at(i).qIntrinsic);
//            m_program->setUniformValue("distCoeffs", camParas->at(i).qDistCoeffs);
//            glUniformMatrix4x3fv(extrinsicLoc, 1, GL_FALSE,
//                                 camParas->at(i).qExtrinsic.data());
//            objModels.at(i)->Draw(programId);
//        }
//    }

    // Calculate ModelViewProjection matrix
    glm::mat4 mv = glm::rotate(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(px, py, pz)), ry, glm::vec3(1, 0, 0)), rx, glm::vec3(0, 0, 1));
    glm::mat4 mvp = gProjection*mv;
    glm::mat3 mn = glm::mat3(glm::rotate(glm::rotate(glm::mat4(1.0f), ry, glm::vec3(1, 0, 0)), rx, glm::vec3(0, 1, 0)));

    GLuint mrtFBO = 0;
//    if (mrt->isEnabled())
//    {
//        mrtFBO = mrt->getFBO();
//        glBindFramebuffer(GL_FRAMEBUFFER, mrtFBO);
//        GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
//        glDrawBuffers(1, drawBuffers); // "1" is the size of DrawBuffers
//    }

//    if (expcor < 600)
//    {
        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);

        // Clear background.
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render camera frames
        int i;

        // Render overlap regions of camera frame with blending
//        glUseProgram(renderProgram.);
        renderProgram.bind();
        for (int camera = 0; camera < CAMERA_NUM; camera++)
        {
            // Lock the camera frame
//            pthread_mutex_lock(&v4l2_cameras[camera].th_mutex);

            // Get index of the newes camera buffer
//            if (v4l2_cameras[camera].fill_buffer_inx == -1) i = 0;
//            else  i = v4l2_cameras[camera].fill_buffer_inx;


            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, txtMask[camera]);
            glUniform1i(glGetUniformLocation(renderProgram.programId(), "myMask"), 1);

            // Set gain value for the camera
//            glUniform4f(locGain[0], gain->Gains::gain[camera][0], gain->Gains::gain[camera][1], gain->Gains::gain[camera][2], 1.0);

            // Render overlap regions of camera frame with blending
            glBindVertexArray(VAO[2 * camera]);
            glActiveTexture(GL_TEXTURE0);
//            glBindTexture(GL_TEXTURE_2D, gTexObj[2 * camera]);
            textures.at(camera)->bind();
            glUniform1i(glGetUniformLocation(renderProgram.programId(), "myTexture"), 0);
//            mapFrame(i, camera);

            GLint mvpLoc = glGetUniformLocation(renderProgram.programId(), "mvp");
            glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

            glDrawArrays(GL_TRIANGLES, 0, vertices[2 * camera]);
            glBindVertexArray(0);

            // Release camera frame
//            pthread_mutex_unlock(&v4l2_cameras[camera].th_mutex);
        }





        // Render non-overlap region of camera frame without blending
//        glUseProgram(renderProgramWB.getHandle()); 	// Use fragment shader without blending
        renderProgramWB.bind();
        glDisable(GL_BLEND);
        for (int camera = 0; camera < CAMERA_NUM; camera++)
        {
            // Lock the camera frame
//            pthread_mutex_lock(&v4l2_cameras[camera].th_mutex);

            // Get index of the newes camera buffer
//            if (v4l2_cameras[camera].fill_buffer_inx == -1) i = 0;
//            else  i = v4l2_cameras[camera].fill_buffer_inx;

            // Set gain value for the camera
//            glUniform4f(locGain[1], gain->Gains::gain[camera][0], gain->Gains::gain[camera][1], gain->Gains::gain[camera][2], 1.0); // Set gain value for the camera

            // Render non-overlap region of camera frame without blending
            glBindVertexArray(VAO[2 * camera + 1]);
            glActiveTexture(GL_TEXTURE0);
//            glBindTexture(GL_TEXTURE_2D, gTexObj[2 * camera+ 1]);
            textures.at(camera)->bind();
            glUniform1i(glGetUniformLocation(renderProgramWB.programId(), "myTexture"), 0);
//            mapFrame(i, camera);

            GLint mvpLoc = glGetUniformLocation(renderProgramWB.programId(), "mvp");
            glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

            glDrawArrays(GL_TRIANGLES, 0, vertices[2 * camera+ 1]);	// Draw texture
            glBindVertexArray(0);

            // Release camera frame
//            pthread_mutex_unlock(&v4l2_cameras[camera].th_mutex);
        }

//        // Render car model
//        glm::mat4 carModelMatrix = glm::rotate(glm::rotate(glm::scale(glm::mat4(1.0f), car_scale), glm::radians(CAR_ORIENTATION_X), glm::vec3(1, 0, 0)),
//            glm::radians(CAR_ORIENTATION_Y), glm::vec3(0, 1, 0));
//        mvp = gProjection * mv * carModelMatrix;
//        mn = glm::mat3(glm::rotate(glm::rotate(glm::rotate(glm::rotate(glm::mat4(1.0f), ry, glm::vec3(1, 0, 0)), rx, glm::vec3(0, 0, 1)),
//                glm::radians(CAR_ORIENTATION_X), glm::vec3(1, 0, 0)), glm::radians(CAR_ORIENTATION_Y), glm::vec3(0, 1, 0)));

////        glUseProgram(carModelProgram.getHandle());
//        carModelProgram.bind();
//        glEnable(GL_DEPTH_TEST);

        // Set matrices
        glUniformMatrix4fv(mvpUniform, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniformMatrix4fv(mvUniform, 1, GL_FALSE, glm::value_ptr(mv));
        glUniformMatrix3fv(mnUniform, 1, GL_FALSE, glm::value_ptr(mn));

//        modelLoader.Draw(carModelProgram.programId());

        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
//        fpsValue = report_fps();
//        expcor++;
//    }
//    else
//    {
//        // Try to lock gain mutex
//        if (pthread_mutex_trylock(&gain->Gains::th_mutex) == 0)
//        {
//            glUseProgram(exposureCorrectionProgram.getHandle());
//            glBindFramebuffer(GL_FRAMEBUFFER, fbo);	// Change frame buffer

//            glEnable(GL_BLEND);

//            int i;
//            // Render camera overlap regions
//            for (int camera = 0; camera < CAMERA_NUM; camera++)
//            {
//                // Lock the camera frame
//                pthread_mutex_lock(&v4l2_cameras[camera].th_mutex);

//                // Get index of the newes camera buffer
//                if (v4l2_cameras[camera].fill_buffer_inx == -1) i = 0;
//                else  i = v4l2_cameras[camera].fill_buffer_inx;

//                // Clear background.
//                glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
//                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//                // Render camera overlap regions
//                glBindVertexArray(VAO_EC[camera]);
//                glActiveTexture(GL_TEXTURE0);
//                glBindTexture(GL_TEXTURE_2D, gTexObj[2 * camera]);
//                glUniform1i(glGetUniformLocation(exposureCorrectionProgram.getHandle(), "myTexture"), 0);
//                mapFrame(i, camera);

//                glDrawArrays(GL_TRIANGLES, 0, vertices_ec[camera]);
//                glBindVertexArray(0);

//                //char rt[10];
//                //sprintf(rt, "PrScr%d.jpg", camera);
//                //Mat PrScr(viewport[3], viewport[2], CV_8UC(4));
//                //glReadPixels(0, 0, viewport[2], viewport[3], GL_RGBA, GL_UNSIGNED_BYTE, PrScr.data);
//                //imwrite(rt, PrScr);

//                glReadPixels(gain->compensator->getFlipROI(camera).x,
//                    gain->compensator->getFlipROI(camera).y,
//                    gain->compensator->getFlipROI(camera).width,
//                    gain->compensator->getFlipROI(camera).height,
//                    GL_RGBA,
//                    GL_UNSIGNED_BYTE,
//                    gain->Gains::overlap_roi[camera][0].data);
//                int next = NEXT(camera, camera_num - 1);
//                glReadPixels(gain->compensator->getFlipROI(next).x,
//                    gain->compensator->getFlipROI(next).y,
//                    gain->compensator->getFlipROI(next).width,
//                    gain->compensator->getFlipROI(next).height,
//                    GL_RGBA,
//                    GL_UNSIGNED_BYTE,
//                    gain->Gains::overlap_roi[camera][1].data);

//                //char name[10];
//                //sprintf(name, "%d.jpg", camera);
//                //imwrite(name, overlap_roi[camera][0]);
//                //sprintf(name, "%d_.jpg", camera);
//                //imwrite(name, overlap_roi[camera][1]);


//                // Release camera frame
//                pthread_mutex_unlock(&v4l2_cameras[camera].th_mutex);
//            }

//            // Release gain mutex
//            pthread_mutex_unlock(&gain->Gains::th_mutex);
//            // Release gain semaphore
//            sem_post(&gain->Gains::th_semaphore);
//            glBindFramebuffer(GL_FRAMEBUFFER, mrtFBO); // Reset framebuffer
//        }
//        expcor = 0;
//    }

    glDisable(GL_BLEND);

//    if (mrt->isEnabled())
//    {
//        glBindFramebuffer(GL_FRAMEBUFFER, 0);

//        glViewport(0, 0, width(), height());
//        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//        mrt->RenderSmallQuad(showTexProgram.programId());
//    }

//    stringstream ss;
//    ss << std::fixed << std::setprecision(2) << fpsValue;
//    string fpsText = "FPS: " + ss.str();
//    fontRenderer->RenderText(fpsText.c_str(), 5, 10);
}

void GpuRender::resizeGL(int w, int h)
{
    // Calculate aspect ratio
    float aspect = float(w) / float(h ? h : 1);
    gProjection = glm::perspective(45.0f, aspect, 0.1f, 100.0f);
//    camera3D->setAspectRatio(aspect);
}

bool GpuRender::event(QEvent *e)
{
    if(e->type() == QEvent::TouchBegin) {
        return true;
    }
    if(e->type() == QEvent::TouchEnd) {
        return true;
    }
    if(e->type() == QEvent::TouchUpdate) {
//        qInfo() << "touch update";
        QTouchEvent *touchEvent = static_cast<QTouchEvent*>(e);
        auto points = touchEvent->touchPoints();
        auto sizePoints = points.size();

        if(sizePoints == 1) {
            QVector2D diff = QVector2D(points[0].pos() - points[0].lastPos());
//            camera3D->viewRotate(QVector2D(diff));
            diff *= 0.1;
            rx += glm::radians(diff.x());
            ry += glm::radians(diff.y());
            ry = glm::clamp(ry, CAM_LIMIT_RY_MIN, CAM_LIMIT_RY_MAX);
            return true;
        }

        if(points.size() == 2) {
            QPointF p1(points[0].pos() - points[1].pos());
            QPointF p2(points[0].lastPos() - points[1].lastPos());
            float len1 = qSqrt(qPow(p1.x(), 2) + qPow(p1.y(), 2));
            float len2 = qSqrt(qPow(p2.x(), 2) + qPow(p2.y(), 2));
//            camera3D->setFovBias(len2 - len1);
            float bias = len2 - len1;
            bias *= 0.01;
            pz -= bias;
            pz = glm::clamp(pz, CAM_LIMIT_ZOOM_MIN, CAM_LIMIT_ZOOM_MAX);
            return true;
        }
        return true;
    }

    return QOpenGLWidget::event(e);
}

void GpuRender::mousePressEvent(QMouseEvent *e)
{
    mousePressPos = QVector2D(e->localPos());
}

void GpuRender::mouseReleaseEvent(QMouseEvent *e)
{
    QVector2D diff = QVector2D(e->localPos()) - mousePressPos;
//            camera3D->viewRotate(QVector2D(diff));
    diff *= 0.1;
    rx += glm::radians(diff.x());
    ry += glm::radians(diff.y());
    ry = glm::clamp(ry, CAM_LIMIT_RY_MIN, CAM_LIMIT_RY_MAX);
}

void GpuRender::wheelEvent(QWheelEvent *event)
{
    float bias = event->angleDelta().y();
    bias *= 0.01;
    pz += bias;
    pz = glm::clamp(pz, CAM_LIMIT_ZOOM_MIN, CAM_LIMIT_ZOOM_MAX);
}

//int GpuRender::setParam(XMLParameters *xml_param)
//{
//    camera_num = xml_param->camera_num;
//    if(camera_num > CAMERA_NUM) camera_num = CAMERA_NUM;

//    for (int i = 0; i < camera_num; i++)
//    {
//        g_in_width.push_back(xml_param->cameras[i].width);
//        g_in_height.push_back(xml_param->cameras[i].height);
//    }

//    gProjection = glm::perspective(45.0f, (float)width() / (float)height(), 0.1f, 100.0f);
//    car_scale = glm::vec3(xml_param->model_scale[0], xml_param->model_scale[0], xml_param->model_scale[0]);
//    mrt = new MRT(width(), height());

//    return(0);
//}

int GpuRender::programsInit()
{
    renderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/vshader.vsh");
    renderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/overlap.fsh");
    renderProgram.link();

//    exposureCorrectionProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, s_v_shader);
//    exposureCorrectionProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, s_f_shader);
//    exposureCorrectionProgram.link();

    renderProgramWB.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/vshader.vsh");
    renderProgramWB.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/nonoverlap.fsh");
    renderProgramWB.link();

//    carModelProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, s_v_shader_model);
//    carModelProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, s_f_shader_model);
//    carModelProgram.link();

//    mvpUniform = carModelProgram.uniformLocation("mvp");
//    mvUniform = carModelProgram.uniformLocation("mv");
//    mnUniform = carModelProgram.uniformLocation("mn");

//    showTexProgram.addShaderFromSourceCode(QOpenGLShader::Vertex, s_v_shader_tex);
//    showTexProgram.addShaderFromSourceCode(QOpenGLShader::Fragment, s_f_shader_tex);
//    showTexProgram.link();

    return (0);
}

bool GpuRender::RenderInit()
{
    if(camerasInit() == -1) return(false);	// Camera capturing

    camTexInit();	// Camera textures initialization
//    ecTexInit();	// Exposure correction

    glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);  // Enable blending
    glEnable(GL_CULL_FACE);  // Cull back face
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Set framebuffer

    // We have to initialize all utils here, because they need OpenGL context
//    if (!modelLoader.Initialize()) cout << "Car model was not initialized" << endl;
//    mrt->Initialize();
//    fontRenderer->Initialize();
//    fontRenderer->SetShader(fontProgram.getHandle());
//    return (GL_NO_ERROR == glGetError());
    return true;
}

int GpuRender::camerasInit()
{
//    for (int i = 0; i < CAMERA_NUM; i++)
//    {
//        string dev_name = "/dev/video" + to_string(i);

//        v4l2Camera v4l2_camera(g_in_width[i], g_in_height[i], CAM_PIXEL_TYPE, V4L2_MEMORY_MMAP, dev_name.c_str());
//        v4l2_cameras.push_back(v4l2_camera);
//        if (v4l2_cameras[i].captureSetup() == -1)
//        {
//            cout << "v4l_capture_setup failed camera " << i << endl;
//            return(-1);
//        }
//    }

//    for (int i = 0; i < CAMERA_NUM; i++) // Start capturing
//        if (v4l2_cameras[i].startCapturing() == -1) return(-1);

//    for (int i = 0; i < CAMERA_NUM; i++) // Get frames from cameras
//        if (v4l2_cameras[i].getFrame() == -1) return(-1);

    return(0);
}

void GpuRender::camTexInit()
{
    ///////////////////////////////// Load vertices arrays ///////////////////////////////
    GLfloat* vVertices[VAO_NUM];
    for (int j = 0; j < VAO_NUM; j++)
    {
        int vrt;
        string array = "./array" + to_string((int)(j / 2) + 1) + to_string(j % 2 + 1);
        vLoad(&vVertices[j], &vrt, array);
        vertices.push_back(vrt);
    }

    //////////////////////// Camera textures initialization /////////////////////////////

    GLuint VBO[VAO_NUM];
    glGenVertexArrays(VAO_NUM, VAO);
    glGenBuffers(VAO_NUM, VBO);

    for (int j = 0; j < VAO_NUM; j++)
    {
        bufferObjectInit(&VAO[j], &VBO[j], vVertices[j], vertices[j]);
        texture2dInit(&gTexObj[j]);
    }

    for (int j = 0; j < CAMERA_NUM; j++)
    {
        // j camera mask init
        texture2dInit(&txtMask[j]);
        string mask_name = "./mask" + to_string(j) + ".jpg";
        Mat camera_mask = imread(mask_name, CV_LOAD_IMAGE_GRAYSCALE);
        mask.push_back(camera_mask);

        glBindTexture(GL_TEXTURE_2D, txtMask[j]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, mask[j].cols, mask[j].rows, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, (uchar*)mask[j].data);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

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
}

void GpuRender::texture2dInit(GLuint *texture)
{
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GpuRender::ecTexInit()
{

}

inline void GpuRender::mapFrame(int buf_index, int camera)
{
//    (*pFNglTexDirectVIVMap)(GL_TEXTURE_2D, g_in_width[camera], g_in_height[camera], GL_PIXEL_TYPE, (GLvoid **)& v4l2_cameras[camera].buffers[buf_index].start, (const GLuint *)(&v4l2_cameras[camera].buffers[buf_index].offset));
//    (*pFNglTexDirectInvalidateVIV)(GL_TEXTURE_2D);
}
