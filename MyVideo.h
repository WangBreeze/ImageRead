﻿#ifndef MYVIDEO_H
#define MYVIDEO_H

#include <QQuickFramebufferObject>
#include <QImage>

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
    //开始播放图片
    Q_INVOKABLE void playStart();
    //设置图片路径
    Q_INVOKABLE void setImageFolder(QString string);

public:
    explicit MyVideo(QQuickItem *parent = nullptr);
    ~MyVideo();
    virtual QQuickFramebufferObject::Renderer *createRenderer() const override;
    QSharedPointer<Context> context();
    bool readImage(QString imagePath,QImage & iamge);
    bool  showImage();
    bool playing() const;
    void setPlaying(bool newPlaying);

signals:

    void playingChanged();

public slots:
    // 从文件加载图像
    void newData(const QString &filename);
    // 从内存加载图像
    void newData(const QImage &image);



    //读取显示图片
    void timerOut();
private:
    QSharedPointer<Context> m_ctx;
    QString   m_openFileFolder;
    QStringList  m_fileList;
    uint          m_currentPlay;
    QTimer*     m_timer;
    bool m_playing;
};

#endif // MYVIDEO_H
