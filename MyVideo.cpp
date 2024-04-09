#include "MyVideo.h"
#include <QVector3D>
#include <QMatrix4x4>
#include <QOpenGLTexture>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFramebufferObjectFormat>



#include <QDir>
#include <QFileInfo>
#include <QTimer>



#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>


//本文件调用
QImage cvMat2QImage(const cv::Mat &mat)
{
    QImage image;
    switch(mat.type())
    {
    case CV_8UC1:
        // QImage构造：数据，宽度，高度，每行多少字节，存储结构
        image = QImage((const unsigned char*)mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8);
        break;
    case CV_8UC3:
        QImage((const unsigned char*)mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
        image = image.rgbSwapped(); // RGB转为BGR
        // Qt5.14增加了Format_BGR888
        //image = QImage((const unsigned char*)mat.data, mat.cols, mat.rows, mat.cols * 3, QImage::Format_BGR888);
        qDebug() << u8"图片格式转换为BRG888";
        break;
    case CV_8UC4:
        image = QImage((const unsigned char*)mat.data, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
        break;
    case CV_16UC4:
        image = QImage((const unsigned char*)mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGBA64);
        image = image.rgbSwapped(); // BRG转为RGB
        break;
    }
    return image;
}
cv::Mat QImage2cvMat(const QImage &image)
{
    cv::Mat mat;
    switch(image.format())
    {
    case QImage::Format_Grayscale8: // 灰度图，每个像素点1个字节（8位）
        // Mat构造：行数，列数，存储结构，数据，step每行多少字节
        mat = cv::Mat(image.height(), image.width(), CV_8UC1, (void*)image.constBits(), image.bytesPerLine());
        break;
    case QImage::Format_ARGB32: // uint32存储0xAARRGGBB，pc一般小端存储低位在前，所以字节顺序就成了BGRA
    case QImage::Format_RGB32: // Alpha为FF
    case QImage::Format_ARGB32_Premultiplied:
        mat = cv::Mat(image.height(), image.width(), CV_8UC4, (void*)image.constBits(), image.bytesPerLine());
        break;
    case QImage::Format_RGB888: // RR,GG,BB字节顺序存储
        mat = cv::Mat(image.height(), image.width(), CV_8UC3, (void*)image.constBits(), image.bytesPerLine());
        // opencv需要转为BGR的字节顺序
        cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
        break;
    case QImage::Format_RGBA64: // uint64存储，顺序和Format_ARGB32相反，RGBA
        mat = cv::Mat(image.height(), image.width(), CV_16UC4, (void*)image.constBits(), image.bytesPerLine());
        // opencv需要转为BGRA的字节顺序
        cv::cvtColor(mat, mat, cv::COLOR_RGBA2BGRA);
        break;
    default :
        break;
    }
    return mat;
}

// 数据上下文
struct Context {
    QImage image;
};

// 顶点矩阵 Vertex matrix
static const GLfloat vertexVertices[] = {
    -1.0f, -1.0f,
    1.0f, -1.0f,
    -1.0f, 1.0f,
    1.0f, 1.0f,
};

// 纹理矩阵 Texture matrix
static const GLfloat textureVertices[] = {
    0.0f,  1.0f,
    1.0f,  1.0f,
    0.0f,  0.0f,
    1.0f,  0.0f,
};

// OpenGL 渲染
class MyVideoRenderer : public QQuickFramebufferObject::Renderer , protected QOpenGLFunctions
{
public:
    MyVideoRenderer():texture(QOpenGLTexture::Target2D){
        // 初始化 OpenGL 上下文
        initialize();
        texture.create();
    }
    ~MyVideoRenderer(){
        texture.destroy();
    }
    void initialize(){
        initializeOpenGLFunctions();
        glClearColor(1.0f,1.0f,1.0f,1.0f);
        if(!shaderProgram.addCacheableShaderFromSourceFile(QOpenGLShader::Vertex,":/vertexshader.vert")){
            qWarning() << QString("[ERROR] Vertex shader: %1").arg(shaderProgram.log());
        }
        if(!shaderProgram.addCacheableShaderFromSourceFile(QOpenGLShader::Fragment,":/fragmentshader.frag")){
            qWarning() << QString("[ERROR] Vertex shader: %1").arg(shaderProgram.log());
        }

        shaderProgram.bindAttributeLocation("qt_Vertex",indexVvertex);
        shaderProgram.bindAttributeLocation("qt_MultiTexCoord0",indexTexture);

        if(!shaderProgram.link()){
            qWarning() << QString("[ERROR] ShaderProgram link: %1").arg(shaderProgram.log());
        }

        sampler2D = shaderProgram.uniformLocation("qt_Texture0");
        projectionMatrix = shaderProgram.uniformLocation("qt_ModelViewProjectionMatrix");
    }
    virtual QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override {
        QOpenGLFramebufferObjectFormat format;
        format.setSamples(4);
        return new QOpenGLFramebufferObject(size,format);
    }

    // 渲染
    virtual void render() override {
        glClear(GL_DEPTH_BUFFER_BIT);

        constexpr const float angle = 0.0f;
        constexpr const float scale = 1.0f;
        QMatrix4x4 modelview;
        modelview.rotate(angle, 0.0f, 1.0f, 0.0f);//y
        modelview.rotate(angle, 1.0f, 0.0f, 0.0f);//x
        modelview.rotate(angle, 0.0f, 0.0f, 1.0f);//z
        modelview.scale(scale);
        modelview.translate(0.0f, 0.0f, 0.0f);

        if(!shaderProgram.bind()){
            qWarning() << QString("[ERROR] ShaderProgram bind: %1").arg(shaderProgram.log());
        }

        shaderProgram.setUniformValue(sampler2D,0);
        shaderProgram.setUniformValue(projectionMatrix,modelview);

        shaderProgram.setAttributeArray(indexVvertex,vertexVertices,2,0);
        shaderProgram.setAttributeArray(indexTexture,textureVertices,2,0);

        shaderProgram.enableAttributeArray(indexVvertex);
        shaderProgram.enableAttributeArray(indexTexture);

        texture.bind();

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        shaderProgram.disableAttributeArray(indexVvertex);
        shaderProgram.disableAttributeArray(indexTexture);

        shaderProgram.release();
    }

    // 同步数据
    void synchronize(QQuickFramebufferObject *item) override{
        MyVideo *obj = static_cast<MyVideo *>(item);
        if(obj){
            auto img = obj->context()->image;
            if( img.width() != texture.width() || img.height() != texture.height()){
                // 大小已改变 更新纹理
                textureResize(img.width(),img.height());
                texture.setData(QOpenGLTexture::BGR,QOpenGLTexture::UInt8,img.constBits());
            }else{
                texture.setData(QOpenGLTexture::BGR,QOpenGLTexture::UInt8,img.constBits());
            }
        }
    }
    // 重置纹理
    inline void textureResize(int width,int height){
        texture.destroy();
        texture.create();
        texture.setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
        texture.setSize(width,height);
        texture.setFormat(QOpenGLTexture::RGB8_UNorm);
        texture.allocateStorage();
    }
    //
    QOpenGLShaderProgram shaderProgram;
    GLint sampler2D = -1;
    GLint projectionMatrix = -1;
    QOpenGLTexture texture;
    //顶点&纹理索引
    GLuint indexVvertex = 0;
    GLuint indexTexture = 1;
};

MyVideo::MyVideo(QQuickItem *parent) : QQuickFramebufferObject(parent),m_ctx(new Context)
{
    setMirrorVertically(true);
    m_currentPlay = 0;
    setPlaying(false);
    m_timer = new QTimer(this);
    m_timer->setInterval(20);
    connect(m_timer,&QTimer::timeout,this,&MyVideo::timerOut);

}

MyVideo::~MyVideo()
{

}

QQuickFramebufferObject::Renderer *MyVideo::createRenderer() const
{
    return new MyVideoRenderer();
}

QSharedPointer<Context> MyVideo::context()
{
    return m_ctx;
}

bool MyVideo::readImage(QString imagePath, QImage &iamge)
{
    if (imagePath.isEmpty())
    {
        return false;
    }
    qDebug() << u8"读取图片:" << imagePath;
    //默认以3通道BGR方式读取
    cv::Mat image = cv::imread(imagePath.toStdString(),cv::IMREAD_COLOR);
    if (image.empty())
    {
        return false;
    }
    iamge =  cvMat2QImage(image);
    return true;
}




void MyVideo::newData(const QString &filename)
{
    auto img = QImage(filename);
    if(!img.isNull()){
        newData(img);
    }
}

void MyVideo::newData(const QImage &image)
{
    //m_ctx->image = image.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
    qDebug() << u8"显示图片";
    m_ctx->image = image;
    update();
}

void MyVideo::setImageFolder(QString string)
{
    m_openFileFolder = string;
    QDir dir(m_openFileFolder);

    if (!dir.exists())
    {
        return ;
    }
    m_fileList.clear();

    QStringList filters;
    filters << "*.png";
    filters << "*.jpg";

    m_fileList =  dir.entryList(filters,QDir::Files);
    qDebug() << u8"获取的文件有" << m_fileList;
}

void MyVideo::playStart()
{

    if (m_timer->isActive())
    {
        m_timer->stop();
        setPlaying(false);
        return;
    }
    if (m_fileList.count() <= 0)
    {
        return ;
    }


    //showImage();
    m_timer->start();
    setPlaying(true);

}

void MyVideo::timerOut()
{
    if (m_currentPlay >= m_fileList.count() - 1)
    {
        m_timer->stop();
        setPlaying(false);
    }
    else
    {
        m_currentPlay++;
    }

    if (showImage() == false)
    {
        m_timer->stop();
        setPlaying(false);
    }

}
bool MyVideo::showImage()
{
    QString imagePath = m_openFileFolder + "/" + m_fileList.at(m_currentPlay);
    QImage image;
    if (readImage(imagePath,image))
    {
        newData(image);
        return true;
    }
    return false;
}

bool MyVideo::playing() const
{
    return m_playing;
}

void MyVideo::setPlaying(bool newPlaying)
{
    if (m_playing == newPlaying)
        return;
    m_playing = newPlaying;
    emit playingChanged();
}
