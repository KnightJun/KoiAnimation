#include <KoiMovie.h>
#include <QDebug>
#include <QFileInfo>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <QImage>
#include <QImageReader>
Q_IMPORT_PLUGIN(DgaImagePlugin)

QMutex mutex;
class ReaderThread : public QThread
{
public:
    KoiMovie::FrameInfo getFrame()
    {
        KoiMovie::FrameInfo fInfo;
        mutex.lock();
        while (mFrameQueue.isEmpty()){
            mStatWaitNotEmpty++;
            bufferIsNotEmpty.wait(&mutex);
        }
        fInfo = mFrameQueue.front();

        // qInfo() << "pop " << fInfo.image << fInfo.frameIndex << fInfo.timeStamp << fInfo.nextTimeStamp;
        mFrameQueue.pop_front();
        bufferIsNotFull.wakeAll();
        mutex.unlock();
        return fInfo;
    }
    void reset()
    {
        mutex.lock();
        mFrameQueue.clear();
        mResetRequest = true;
        bufferIsNotFull.wakeAll();
        mutex.unlock();
    }

    void setFileName(const QString &fileName)
    {
        QFileInfo info(fileName);
        mFileName = fileName;
        if (mReader)
            delete mReader;
        mReader = new QImageReader();
        if (info.suffix() == "png")
        {
            mReader->setFormat("apng");
        }
        mReader->setFileName(fileName);
        mImageCount = mReader->imageCount();
        mFormat = mReader->format();
        qInfo() << "ReaderThread setFileName on thread : " << currentThreadId();
        qInfo() << "    mImageCount:" << mImageCount;
        this->start();
    }
    QByteArray format() { return mFormat; };
    ~ReaderThread()
    {
        qInfo() << "ReaderThread destoryed.";
        mutex.lock();
        isEnd = true;
        mFrameQueue.clear();
        bufferIsNotFull.wakeAll();
        mutex.unlock();
        this->wait();
        qInfo() << "mStatWaitNotEmpty : " << mStatWaitNotEmpty;
    }
    int imageCount() { return mImageCount; }
    QString fileName() { return mFileName; }
    int mBufferSize = 20;
    QImageReader *mReader = nullptr;
    bool isEnd = false;

protected:
    void run()
    {
        bool resetRequest = false;
        qInfo() << "ReaderThread run on thread : " << currentThreadId();
        while (1)
        {
            if (isEnd)
            {
                qInfo() << "thread " << currentThreadId() << " exit by end flag";
                return;
            }
            if (resetRequest || !mReader->canRead())
            {
                resetReader();
                resetRequest = false;
            }
            KoiMovie::FrameInfo fInfo;
            fInfo.image = mReader->read();
            fInfo.frameIndex = mFrameIndex++;
            fInfo.timeStamp = mTimeStamp;
            mTimeStamp += mReader->nextImageDelay();
            fInfo.nextTimeStamp = mTimeStamp;
            mutex.lock();
            if (fInfo.frameIndex == 0) mResetRequest = false;
            if (mResetRequest)
            {
                resetRequest = true;
            }
            else
            {
                if (mFrameQueue.size() == mBufferSize || mFrameQueue.size() == mImageCount)
                {
                    bufferIsNotFull.wait(&mutex);
                }
                mFrameQueue.push_back(fInfo);
                bufferIsNotEmpty.wakeAll();
            }
            mutex.unlock();
        }
    }

private:
    QQueue<KoiMovie::FrameInfo> mFrameQueue;
    QWaitCondition bufferIsNotFull;
    QWaitCondition bufferIsNotEmpty;
    int mTimeStamp = 0;
    int mFrameIndex = 0;
    int mImageCount;
    QString mFileName;
    QByteArray mFormat;
    bool mResetRequest = false;
    int mStatWaitNotEmpty=0;
    void resetReader()
    {
        QString fileName = mReader->fileName();
        QByteArray format = mReader->format();
        QIODevice *device = mReader->device();
        delete mReader;
        if (fileName.isEmpty())
            mReader = new QImageReader(device, format);
        else
            mReader = new QImageReader(fileName, format);
        (void)mReader->canRead(); // Provoke a device->open() call
        mReader->device()->seek(0);
        mReader->jumpToImage(0);
        mTimeStamp = 0;
        mFrameIndex = 0;
    }
};

KoiMovie::KoiMovie(QObject *parent) : QObject(parent)
{
    mTimer.callOnTimeout(this, &KoiMovie::onTimeout);
    mReaderThread = new ReaderThread();
};

KoiMovie::~KoiMovie()
{
    delete mReaderThread;
}

void KoiMovie::setFileName(const QString &fileName)
{
    mReaderThread->setFileName(fileName);
}
QString KoiMovie::fileName() const
{
    return mReaderThread->fileName();
}

int KoiMovie::frameCount() const
{
    return mReaderThread->imageCount();
}

int KoiMovie::nextFrameDelay() const
{
    return mFrameInfo.nextTimeStamp - mFrameInfo.timeStamp;
}
int KoiMovie::currentFrameNumber() const
{
    return mFrameInfo.frameIndex;
}

QImage KoiMovie::currentImage() const
{
    return mFrameInfo.image;
}

bool KoiMovie::jumpToNextFrame()
{
    mFrameInfo = mReaderThread->getFrame();
    onFrameChanged();
    return true;
}

QByteArray KoiMovie::format() const
{
    return mReaderThread->format();
}

void KoiMovie::onFrameChanged()
{
    if (mSendSig)
    {
        emit KoiMovie::frameChanged(mFrameInfo.frameIndex);
    }
}
bool KoiMovie::jumpToTimeStamp(qint64 timeStamp)
{
    mSendSig = false;
    if (timeStamp < 0)
        timeStamp = 0;
    if(timeStamp < mFrameInfo.timeStamp){
        mReaderThread->reset();
        this->jumpToNextFrame();
    }
    while (!(timeStamp >= mFrameInfo.timeStamp && timeStamp < mFrameInfo.nextTimeStamp) && this->frameCount() != this->currentFrameNumber() + 1)
    {
        this->jumpToNextFrame();
    }
    mSendSig = true;

    int remainTime = this->nextTimeStamp() - timeStamp;
    if (remainTime < 1)
        remainTime = 1;
    if (mState == Running)
    {
        mTimer.setInterval(remainTime);
    }
    else if (mState == Paused)
    {
        mPauseRemainTime = remainTime;
    }
    emit KoiMovie::frameChanged(this->currentFrameNumber());
    return true;
}
bool KoiMovie::jumpToFrame(int frameNumber)
{
    if (frameNumber == this->currentFrameNumber())
    {
        return true;
    }
    if (frameNumber >= this->frameCount())
    {
        return false;
    }
    if (frameNumber < this->currentFrameNumber())
    {
        mReaderThread->reset();
    }
    mSendSig = false;
    while (frameNumber != this->currentFrameNumber())
    {
        this->jumpToNextFrame();
    }
    mSendSig = true;
    emit KoiMovie::frameChanged(this->currentFrameNumber());
    return true;
}

void KoiMovie::start()
{
    this->jumpToFrame(0);
    mTimer.start(this->nextFrameDelay());
    mPauseRemainTime = 0;
    mState = Running;
}

void KoiMovie::stop()
{
    mTimer.stop();
    mState = NotRunning;
}

void KoiMovie::onTimeout()
{
    this->jumpToNextFrame();
    // qInfo() << "setInterval " << this->nextFrameDelay();
    mTimer.setInterval(this->nextFrameDelay());
}

qint64 KoiMovie::nextTimeStamp()
{
    return mFrameInfo.nextTimeStamp;
}

qint64 KoiMovie::timeStamp()
{
    switch (mState)
    {
    case NotRunning:
        return mFrameInfo.timeStamp;
    case Running:
        return nextTimeStamp() - mTimer.remainingTime();
    case Paused:
        return nextTimeStamp() - mPauseRemainTime;
    default:
        break;
    }
    return -1;
}

KoiMovie::MovieState KoiMovie::state()
{
    return mState;
}

qint64 KoiMovie::duration()
{
    qInfo() << "get duration";
    if (mDuration > -1)
        return mDuration;
    mSendSig = false;
    int cNum = this->currentFrameNumber();
    mDuration = 0;
    do
    {
        mDuration += this->nextFrameDelay();
        this->jumpToNextFrame();
    } while (this->currentFrameNumber() != cNum);
    mSendSig = true;
    return mDuration;
}

void KoiMovie::setPaused(bool pause)
{
    if (mState == NotRunning)
        return;
    if (pause && mState == Paused)
        return;
    if (!pause && mState == Running)
        return;
    if (pause)
    {
        mPauseRemainTime = mTimer.remainingTime();
        mTimer.stop();
        mState = Paused;
    }
    else
    {
        mTimer.start(mPauseRemainTime);
        mState = Running;
    }
}