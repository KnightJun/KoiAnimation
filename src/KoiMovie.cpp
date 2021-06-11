#include <KoiMovie.h>
#include <QDebug>
#include <QFileInfo>
Q_IMPORT_PLUGIN(DgaImagePlugin)
KoiMovie::KoiMovie(QObject *parent): QObject(parent)
{
    mTimer.callOnTimeout(this, &KoiMovie::onTimeout);
    mReader = new QImageReader();
};

KoiMovie::~KoiMovie()
{
    delete mReader;
}

void KoiMovie::setFileName(const QString &fileName)
{
    QFileInfo info(fileName);
    if(info.suffix() == "png"){
        mReader->setFormat("apng");
    }
    mReader->setFileName(fileName);
}
QString KoiMovie::fileName() const
{
    return mReader->fileName();
}

int KoiMovie::frameCount() const
{
    return mReader->imageCount();
}

int KoiMovie::nextFrameDelay() const
{
    return mReader->nextImageDelay();
}
int KoiMovie::currentFrameNumber() const
{
    return mFrameIndex;
}

QImage KoiMovie::currentImage() const
{
    return mCurrentImage;
}

bool KoiMovie::jumpToNextFrame()
{
    if(mReader->canRead()){
        mCurrentImage = mReader->read();
        mFrameIndex++;
        onFrameChanged(mFrameIndex);
    }else{
        this->reset();
    }
    return true;
}
void KoiMovie::setFormat(const QByteArray &format)
{
    mReader->setFormat(format);
}
QByteArray KoiMovie::format() const
{
    return mReader->format();
}

void KoiMovie::reset()
{
    QString fileName = mReader->fileName();
    QByteArray format = mReader->format();
    QIODevice *device = mReader->device();
    QColor bgColor = mReader->backgroundColor();
    QSize scaledSize = mReader->scaledSize();
    delete mReader;
    if (fileName.isEmpty())
        mReader = new QImageReader(device, format);
    else
        mReader = new QImageReader(fileName, format);
    (void)mReader->canRead(); // Provoke a device->open() call
    mReader->device()->seek(0);

    mReader->jumpToImage(0);
    mCurrentImage = mReader->read();
    mFrameIndex = 0;
    onFrameChanged(mFrameIndex);
}
void KoiMovie::onFrameChanged(int frameNumber)
{
    if(frameNumber == 0){
        mNextTimeStamp = 0;
        mTimeStamp = 0;
    }
    mTimeStamp = mNextTimeStamp;
    mNextTimeStamp += this->nextFrameDelay();
    if(mSendSig){
        emit KoiMovie::frameChanged(frameNumber);
    }
}
bool KoiMovie::jumpToTimeStamp(qint64 timeStamp)
{
    mSendSig = false;
    if(timeStamp < 0) timeStamp = 0;
    if(timeStamp < mTimeStamp){
        this->reset();
    }
    while (!(timeStamp >= mTimeStamp && timeStamp < mNextTimeStamp)
        && this->frameCount() != currentFrameNumber() +1)
    {
        this->jumpToNextFrame();
    }
    mSendSig = true;
    
    int remainTime = mNextTimeStamp - timeStamp;
    if(remainTime < 1) remainTime = 1;
    if(mState == Running){
        mTimer.setInterval(remainTime);
    }else if(mState == Paused){
        mPauseRemainTime = remainTime;
    }
    emit KoiMovie::frameChanged(this->currentFrameNumber());
    return true;
}
bool KoiMovie::jumpToFrame(int frameNumber)
{
    if(frameNumber == this->currentFrameNumber()){
        return true;
    }
    if(frameNumber >= this->frameCount()){
        return false;
    }
    mSendSig = false;
    if(frameNumber < this->currentFrameNumber()){
        this->reset();
    }
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
    mTimeStamp = 0;
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
    mTimer.setInterval(this->nextFrameDelay());
}

qint64 KoiMovie::nextTimeStamp()
{
    return mNextTimeStamp;
}

qint64 KoiMovie::timeStamp()
{
    switch (mState)
    {
    case NotRunning:
        return mNextTimeStamp - this->nextFrameDelay();
    case Running:
        return mNextTimeStamp - mTimer.remainingTime();
    case Paused:
        return mNextTimeStamp - mPauseRemainTime;
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
    if(mDuration > -1)return mDuration;
    mSendSig = false;
    this->reset();
    int cNum = 0;
    mDuration = 0;
    do
    {
        mDuration += mReader->nextImageDelay();
        this->jumpToNextFrame();
    }while (this->currentFrameNumber() != cNum);
    this->reset();
    mSendSig = true;
    return mDuration;
}

void KoiMovie::setPaused(bool pause)
{
    if(mState == NotRunning)return;
    if(pause && mState == Paused)return;
    if(!pause && mState == Running)return;
    if(pause){
        mPauseRemainTime = mTimer.remainingTime();
        mTimer.stop();
        mState = Paused;
    }else
    {
        mTimer.start(mPauseRemainTime);
        mState = Running;
    }
    
}