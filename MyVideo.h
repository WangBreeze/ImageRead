#ifndef MYVIDEO_H
#define MYVIDEO_H

#include <QMutex>
#include <QQuickFramebufferObject>
#include <QImage>
#include <QWaitCondition>
#include <QQueue>

/*!
    如何使用？
    内存图像数据直接连接 newData 槽函数即可在 qml 中显示该图像。

    如何在视频上绘制其他内容？
    可以在 qml 中的该元素实例添加子元素 文本啦 矩形框鸭 之类的。
*/

struct Context;
class MyVideo : public QQuickFramebufferObject
{
    Q_OBJECT
public:
    Q_PROPERTY(bool playing READ playing WRITE setPlaying NOTIFY playingChanged)
    Q_PROPERTY(int  currentNum READ currentNum WRITE setCurrentNum NOTIFY currentNumChanged)
    Q_PROPERTY(int  showNum READ  showNum WRITE setShowNum NOTIFY  showNumChanged)
    Q_PROPERTY(int  totalNum READ totalNum WRITE setTotalNum NOTIFY totalNumChanged)
    Q_PROPERTY(double fps READ fps WRITE setFps NOTIFY fpsChanged)
    Q_PROPERTY(QString  frameRate READ frameRate WRITE setFrameRate NOTIFY frameRateChanged)
    Q_PROPERTY(int  frameSet READ frameSet WRITE setFrameSet NOTIFY frameSetChanged)

    //开始播放图片
    Q_INVOKABLE void playStart();
    //设置图片路径
    Q_INVOKABLE void setImageFolder(QString string);
    Q_INVOKABLE void newData(QImage image);
    Q_INVOKABLE void preImage();
    Q_INVOKABLE void nextImage();

public:
    explicit MyVideo(QQuickItem *parent = nullptr);
    ~MyVideo();
    virtual QQuickFramebufferObject::Renderer *createRenderer() const override;
    QSharedPointer<Context> context();
    //读取图片
    bool readImage(QString imagePath,QImage & iamge);
    void readImage();
    bool  showImage();
    bool playing() const;
    void setPlaying(bool newPlaying);

    int currentNum() const;
    void setCurrentNum(int newCurrentNum);

    double fps() const;
    void setFps(double newFps);

    int totalNum() const;
    void setTotalNum(int newTotalNum);
    void ShowFrameRate(); //帧率计算
    QString frameRate() const;
    void setFrameRate(const QString &newFrameRate);

    int showNum() const;
    void setShowNum(int newShowNum);

    int frameSet() const;
    void setFrameSet(int newFrameSet);

signals:

    void playingChanged();

    void currentNumChanged();

    void fpsChanged();

    void totalNumChanged();

    void frameRateChanged();

    void showNumChanged();

    void frameSetChanged();

public slots:
    // 从文件加载图像
    void newDataPath(const QString &filename);
    // 从内存加载图像




    //读取显示图片
    void timerOut();
private:
    QSharedPointer<Context> m_ctx;
    QString   m_openFileFolder;
    QStringList  m_fileList;
    uint          m_currentPlay;
    //QTimer*     m_timer;
    bool m_playing;
    int m_currentNum;
    int m_showNum;
    double m_fps;
    int m_totalNum;
    QString m_frameRate;
    QWaitCondition    m_readImageWait;
    QWaitCondition    m_showImageWait;
    QMutex            m_imageMutex;
    QQueue<QImage>    m_imageQueue;

    int m_frameSet;
};

#endif // MYVIDEO_H
