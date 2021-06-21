#pragma once
#include <QImageReader>
#include "koianimation_global.h"
#include <QTimer>
class ReaderThread;
class KOIANIMATION_EXPORT KoiMovie : public QObject
{
    Q_OBJECT
public:
    struct FrameInfo
    {
        QImage image;
        int timeStamp;
        int nextTimeStamp;
        int frameIndex = -1;
    };
    enum MovieState {
        NotRunning,
        Paused,
        Running
    };

    KoiMovie(QObject *parent = nullptr);
    ~KoiMovie();

    void setFileName(const QString &fileName);
    QString fileName() const;
    QByteArray format() const;
    int frameCount() const;
    int nextFrameDelay() const;
    int currentFrameNumber() const;
    QImage currentImage() const;
    bool jumpToNextFrame();
    bool jumpToTimeStamp(qint64 timeStamp);
    bool jumpToFrame(int frameNumber);
    qint64 timeStamp();
    qint64 nextTimeStamp();
    void start();
    void stop();
    qint64 duration();
    void setPaused(bool pause);
    MovieState state();
Q_SIGNALS:
    void frameChanged(int frameNumber);

private:
    /* data */
    void onFrameChanged();
    void onTimeout();
    bool mSendSig = true;
    QTimer mTimer;
    MovieState mState = NotRunning;
    int mPauseRemainTime = 0;
    qint64 mDuration = -1;
    FrameInfo mFrameInfo;
    ReaderThread* mReaderThread = nullptr;
};
