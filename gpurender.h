#ifndef GPU_RENDER_H
#define GPU_RENDER_H


#include <QOpenGLWidget>
#include <QOpenGLExtraFunctions>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>

#include <vector>

#include <opencv2/opencv.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>


//#ifndef GL_GLEXT_PROTOTYPES
//#define GL_GLEXT_PROTOTYPES 1
//#endif

//OpenGL
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>

//Capturing
#include "src_v4l2.hpp"
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <g2d.h>

// IMX8QM
#define GL_PIXEL_TYPE GL_RGBA
#define CAM_PIXEL_TYPE V4L2_PIX_FMT_RGB32

using namespace std;
using namespace cv;

/**********************************************************************************************************************
 * Types
 **********************************************************************************************************************/
struct vertices_obj
{
    GLuint	vao;
    GLuint	vbo;
    GLuint	tex;
    int		num;
};

class GpuRender : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
   explicit GpuRender(QWidget *parent = 0);
    ~GpuRender();

    enum RenderState {
        RenderBase,
        RenderLines,
        RenderGrids,
    };

    vector<int> mesh_index;

    int setProgram(uint index);

    int addCamera(int index, int width, int height);
    int addMesh(string filename);
    int runCamera(int index);
    void reloadMesh(int index, string filename);
    int changeMesh(Mat xmap, Mat ymap, int density, Point2f top, int index);
    Mat takeFrame(int index);

    int getVerticesNum(uint num) {if (num < v_obj.size()) return (v_obj[num].num); return (-1);}

    int addBuffer(GLfloat* buf, int num);
    int setBufferAsAttr(int buf_num, int prog_num, char* atr_name);
    void renderBuffer(int buf_num, int type, int vert_num);
    void updateBuffer(int buf_num, GLfloat* buf, int num);
    void setRenderState(RenderState state_) {renderState = state_;}

public slots:

protected:
    void initializeGL() Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;
    void resizeGL(int w, int h) Q_DECL_OVERRIDE;
    bool event(QEvent *e) Q_DECL_OVERRIDE;
//    void mousePressEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
//    void mouseReleaseEvent(QMouseEvent *e) Q_DECL_OVERRIDE;
//    void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;

private:
    int renderState = 0;

    int currentProgram;
    std::vector<QOpenGLShaderProgram *> renderPrograms;
    vector<vertices_obj> v_obj;
    std::vector<v4l2Camera> v4l2_cameras;

    void vLoad(GLfloat** vert, int* num, string filename);
    void bufferObjectInit(GLuint* text_vao, GLuint* text_vbo, GLfloat* vert, int num);
    void texture2dInit(GLuint* texture);

    typedef void (GL_APIENTRY *PFNGLTEXDIRECTVIVMAP)
                 (GLenum Target, GLsizei Width, GLsizei Height,
                  GLenum Format, GLvoid ** Logical, const GLuint *Physical);
    typedef void (GL_APIENTRY *PFNGLTEXDIRECTINVALIDATEVIV) (GLenum Target);
    PFNGLTEXDIRECTVIVMAP pFNglTexDirectVIVMap;
    PFNGLTEXDIRECTINVALIDATEVIV pFNglTexDirectInvalidateVIV;
};

#endif // GPU_RENDER_H
