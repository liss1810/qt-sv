#ifndef SV_GPU_RENDER_H
#define SV_GPU_RENDER_H

#include <QOpenGLWindow>
#include <QOpenGLExtraFunctions>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>

#include <vector>

//#include "v4l2capture.h"
//#include "model.h"
//#include "cameraparameter.h"
//#include "camera3d.h"

#include "render/model_loader/ModelLoader.hpp"
#include "MRT.hpp"
#include "common/src_v4l2.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

QT_BEGIN_NAMESPACE
class QGestureEvent;
QT_END_NAMESPACE

#define CAMERA_NUM  4
#define VAO_NUM 8 // 2 * CAMERA_NUM

extern "C"
{
#include <pthread.h>
}

class SvGpuRender : public QOpenGLWindow, protected QOpenGLExtraFunctions
{
    Q_OBJECT

public:
   explicit SvGpuRender(vector<v4l2Camera> *v4lCams, QWindow *parent = 0);
    ~SvGpuRender();
    int setParam(int camNum, int camWidth, int camHeight, float modelScale[]);
    void setPath(const std::string &p) {path = p;}

public slots:

protected:
    void initializeGL() Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;
    void resizeGL(int w, int h) Q_DECL_OVERRIDE;
    bool event(QEvent *e) Q_DECL_OVERRIDE;
    void mouseDoubleClickEvent(QMouseEvent *e) Q_DECL_OVERRIDE;

#define PLANES 4

private:
    typedef void (GL_APIENTRY *PFNGLTEXDIRECTVIVMAP)
                 (GLenum Target, GLsizei Width, GLsizei Height,
                  GLenum Format, GLvoid ** Logical, const GLuint *Physical);
    typedef void (GL_APIENTRY *PFNGLTEXDIRECTINVALIDATEVIV) (GLenum Target);
    PFNGLTEXDIRECTVIVMAP pFNglTexDirectVIVMap;
    PFNGLTEXDIRECTINVALIDATEVIV pFNglTexDirectInvalidateVIV;

//    Camera3D *camera3D;

    //=============================== nxp sv
    std::string path;
    int camera_num;
    vector<int> g_in_width;				// Input frame width
    vector<int> g_in_height;			// Input frame height
    //Model Loader
    glm::mat4 gProjection;	// Initialization is needed
    float rx = 0.0f, ry = 0.0f, px = 0.0f, py = 0.0f, pz = -10.0f;
    glm::vec3 car_scale = glm::vec3(1.0f, 1.0f, 1.0f);
    QOpenGLShaderProgram renderProgram;
    QOpenGLShaderProgram exposureCorrectionProgram;
    QOpenGLShaderProgram renderProgramWB;
    QOpenGLShaderProgram carModelProgram;
    QOpenGLShaderProgram showTexProgram;
    GLuint mvpUniform, mvUniform, mnUniform;
    vector<v4l2Camera> *v4l2_cameras;	// Camera buffers
    GLuint VAO[VAO_NUM];
    vector<int> vertices;

    // Cameras mapping
    GLuint gTexObj[VAO_NUM] = {0};		// Camera textures
    GLuint txtMask[CAMERA_NUM] = {0};	// Camera masks textures
    vector<Mat> mask;					// Mask images

    ModelLoader modelLoader;
    MRT* mrt = NULL;	// Initialization is needed

//    Gains* gain = NULL;
    QTimer *timer;
    int programsInit();
    bool RenderInit();
    void camTexInit();
    void vLoad(GLfloat** vert, int* num, string filename);
    void bufferObjectInit(GLuint* text_vao, GLuint* text_vbo, GLfloat* vert, int num);
    void texture2dInit(GLuint* texture);
    void ecTexInit();
    void mapFrame(int buf_index, int camera);
};

#endif // SV_GPU_RENDER_H
