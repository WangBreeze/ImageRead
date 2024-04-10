#include "MyVideo.h"
#include "GL/glut.h"
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
#include <QtConcurrent>
#include <QMetaObject>



#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>

//计算帧率
static int frame=0,stime=0,timebase=0;
static int lastTime = 0;
static QString f_frame;

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
        image = QImage((const unsigned char*)mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
        image = image.rgbSwapped(); // RGB转为BGR //这里要转一次
        // Qt5.14增加了Format_BGR888
        //image = QImage((const unsigned char*)mat.data,  mat.cols,mat.rows,  mat.step, QImage::Format_BGR888); //这种方式读取显示不成功，待研究：：初步研究是QImage 只是格式容器的创建，而实际数据还是原始数据
       
        //qDebug() << u8"图片格式转换为BRG888";
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

void ShowFrameRate()
{
    char s[100]={0};
    frame++;
    //glutGet获取 glutinit 函数到
    lastTime = stime;
    stime= glutGet(GLUT_ELAPSED_TIME);
    //qDebug() << u8"调用时间====》" << stime - lastTime;
    qDebug() << u8"渲染时间间隔---->" << stime - lastTime;
    if (stime - timebase > 1000) {
        sprintf(s,"FPS : %4.2f",
                frame*1000.0/(stime-timebase));
        timebase = stime;
        frame = 0;
        f_frame = s;
    }

}



// 数据上下文
struct Context {
    //cv::Mat   imgMat;
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
        //计算帧率
        ShowFrameRate();

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
        texture.setFormat(QOpenGLTexture::RGB8_UNorm); //无符号8bit
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
    setCurrentNum(0);
    setFrameSet(2);
    setShowNum(0);
    setPlaying(false);
    setFps(0.0);
    setTotalNum(0);
//    m_timer = new QTimer(this);
//    m_timer->setInterval(20);
//    connect(m_timer,&QTimer::timeout,this,&MyVideo::timerOut);

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
    //qDebug() << u8"读取图片:" << imagePath;
    //默认以3通道BGR方式读取，因为mat的存储格式默认就是BGR
    cv::Mat image = cv::imread(imagePath.toStdString(),cv::IMREAD_COLOR);
    if (image.empty())
    {
        return false;
    }
    iamge =  cvMat2QImage(image);
    return true;
}

void MyVideo::readImage()
{
    int currentNum = m_currentNum;
    while(m_playing)
    {
        m_imageMutex.lock(); //加锁

        QThread::msleep(m_frameSet);  //控制帧率
        if (currentNum > m_fileList.count() -1)
        {
            setPlaying(false);
            m_imageMutex.unlock();
            break;
        }
        if (m_imageQueue.count() > 50)
        {
            //队列满了等待
            qDebug() << u8"读取图片等待";
            m_readImageWait.wait(&m_imageMutex);
        }
        QString imagePath = m_openFileFolder + "/" + m_fileList.at(currentNum++);
        QImage image;
        if (readImage(imagePath,image))
        {
            m_imageQueue.enqueue(image); //加入队列
            qDebug() << u8"加入队列::" << m_imageQueue.count();
            m_showImageWait.wakeAll();

        }
        m_imageMutex.unlock();
    }
    setCurrentNum(currentNum);
}




void MyVideo::newDataPath(const QString &filename)
{
    auto img = QImage(filename);
    if(!img.isNull()){
        newData(img);
    }
}

void MyVideo::newData( QImage image)
{
    m_ctx->image = image.convertToFormat(QImage::Format_BGR888); //一定要在这里转换
    //qDebug() << u8"显示图片";
    //m_ctx->image = image;
    update();
    setFrameRate(f_frame);
}

void MyVideo::preImage()
{
    if (m_currentNum == 0)
    {
        return;
    }
    QString imagePath = m_openFileFolder + "/" + m_fileList.at(m_currentNum - 1);
    newDataPath(imagePath);
//    QImage img;
//    if (readImage(imagePath,img))
//    {
//        newData(img);
        setCurrentNum(m_currentNum -1);
        setShowNum(m_currentNum);
//    }

}

void MyVideo::nextImage()
{
    if (m_currentNum == m_fileList.count() - 1)
    {
        return;
    }
    QString imagePath = m_openFileFolder + "/" + m_fileList.at(m_currentNum + 1);
    newDataPath(imagePath);

    //QImage img;
    //if (readImage(imagePath,img))
    //{
    //    newData(img);
        setCurrentNum(m_currentNum +1);
        setShowNum(m_currentNum);
    //}

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
    setCurrentNum(0);
    QStringList filters;
    filters << "*.png";
    filters << "*.jpg";

    m_fileList =  dir.entryList(filters,QDir::Files);
    setTotalNum(m_fileList.count());
    //qDebug() << u8"获取的文件有" << m_fileList;
}

void MyVideo::playStart()
{
    if (m_currentNum >= m_fileList.count() -1 && m_fileList.count() > 0)
    {
        setCurrentNum(0);
        setShowNum(0);
        //重新播放
    }
    if (m_playing)
    {
        setPlaying(false);
    }
    else
    {
        setPlaying(true);
    }

    //启动多线程
    QtConcurrent::run(this,&MyVideo::readImage);
    QtConcurrent::run(this,&MyVideo::showImage);
//    if (m_timer->isActive())
//    {
//        m_timer->stop();
//        setPlaying(false);
//        return;
//    }
//    if (m_fileList.count() <= 0)
//    {
//        return ;
//    }


//    //showImage();
//    m_timer->start();

}

void MyVideo::timerOut()
{
    if (m_currentNum >= m_fileList.count() - 1)
    {
        //m_timer->stop();
        setPlaying(false);
    }
    else
    {
        setCurrentNum(m_currentNum+1);
    }

    if (showImage() == false)
    {
        //m_timer->stop();
        setPlaying(false);
    }
    setFrameRate(f_frame);

}
bool MyVideo::showImage()
{
    while(m_playing)
    {
        m_imageMutex.lock();
        if (m_showNum >= m_totalNum)
        {
            setPlaying(false);
            m_imageMutex.unlock();
            break;
        }
        if (m_imageQueue.count() == 0 ) //防止播放结束，进入等待程序
        {
            qDebug() << u8"显示图片等待";
            m_showImageWait.wait(&m_imageMutex);
        }
        QImage image = m_imageQueue.dequeue();
        m_readImageWait.wakeAll();
        setShowNum(m_showNum+1);
        qDebug() << u8"弹出队列::" << m_imageQueue.count();
        QMetaObject::invokeMethod(this,"newData",Qt::QueuedConnection,Q_ARG(QImage,image));

        m_imageMutex.unlock();
    }
//    QString imagePath = m_openFileFolder + "/" + m_fileList.at(m_currentNum);
//    QImage image;
//    if (readImage(imagePath,image))

    return true;
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

int MyVideo::currentNum() const
{
    return m_currentNum;
}

void MyVideo::setCurrentNum(int newCurrentNum)
{
    if (m_currentNum == newCurrentNum)
        return;
    m_currentNum = newCurrentNum;
    emit currentNumChanged();
}

double MyVideo::fps() const
{
    return m_fps;
}

void MyVideo::setFps(double newFps)
{
    if (qFuzzyCompare(m_fps, newFps))
        return;
    m_fps = newFps;
    emit fpsChanged();
}

int MyVideo::totalNum() const
{
    return m_totalNum;
}

void MyVideo::setTotalNum(int newTotalNum)
{
    if (m_totalNum == newTotalNum)
        return;
    m_totalNum = newTotalNum;
    emit totalNumChanged();
}

QString MyVideo::frameRate() const
{
    return m_frameRate;
}

void MyVideo::setFrameRate(const QString &newFrameRate)
{
    if (m_frameRate == newFrameRate)
        return;
    m_frameRate = newFrameRate;
    emit frameRateChanged();
}

Q_DECLARE_METATYPE(QImage);

int MyVideo::showNum() const
{
    return m_showNum;
}

void MyVideo::setShowNum(int newShowNum)
{
    if (m_showNum == newShowNum)
        return;
    m_showNum = newShowNum;
    emit showNumChanged();
}

int MyVideo::frameSet() const
{
    return m_frameSet;
}

void MyVideo::setFrameSet(int newFrameSet)
{
    if (m_frameSet == newFrameSet)
        return;
    m_frameSet = newFrameSet;
    emit frameSetChanged();
}
