#include "KoiConvert.h"
#include <AniImageProcess.h>
#include <QFileInfo>
#include <QDebug>
KoiConvert::KoiConvert(QString srcFilename, QString dstFilename, 
    KoiAnimation* animation, QObject *parent):QObject(parent)
{
    init(srcFilename, dstFilename, animation);
}

KoiConvert::KoiConvert(QString srcFilename, KoiFormat dstFormat, 
    KoiAnimation* animation, QObject *parent):QObject(parent)
{
    QString dstFilename;
    QFileInfo file(srcFilename);
    dstFilename = file.path() + "/" + file.baseName() + "." + KoiAnimation::FormatStr((KoiFormat)dstFormat);
    init(srcFilename, dstFilename, animation);
}

void KoiConvert::init(QString srcFilename, QString dstFilename, 
    KoiAnimation* animation)
{
    mMoive = new KoiMovie(this);
    mMoive->setFileName(srcFilename);
    mMoive->jumpToNextFrame();
    
    mAnimation = animation ? animation : new KoiAnimation();
    mNeedDeleteAnimation = !animation;
    mAnimation->init(dstFilename);
    mTimestamp = -1;
    connect(mAnimation, &KoiAnimation::sigAddFrameFinish, this, &KoiConvert::sigFrameConverted);
}


void KoiConvert::encodeNextFrameProgressBar(bool sync)
{
    // static int lastTs = 0;
    if(mFinishFlag || mAnimation->isBusy()){
        return;
    }
    if(mProgressBar && mDuration < 0){
        mDuration = mMoive->duration();
    }
    QImage image;
    bool isLastFrame = mMoive->currentFrameNumber() + 1 == mMoive->frameCount();
    int nextTimeStamp = mMoive->nextTimeStamp();
    if(mAnimation->format() == GIF){
        nextTimeStamp = qRound(mMoive->nextTimeStamp()/10.0) * 10;
    }
    if(isLastFrame){
        mDuration = nextTimeStamp;
    }
    if(nextTimeStamp - this->mTimestamp <= 50){
        if(isLastFrame){
            mFinishFlag = true;
            mAnimation->finish(mMoive->nextTimeStamp());
            if(sync) mAnimation->waitForIdle();
            return;
        }
        mMoive->jumpToNextFrame();
        this->mTimestamp = mMoive->timeStamp();
        if(mAnimation->format() == GIF){
            this->mTimestamp = qRound(mMoive->timeStamp()/10.0) * 10;
        }
    } else {
        this->mTimestamp += 30;
    }
    image = AddProgressBar(mMoive->currentImage(), this->mTimestamp, mDuration);
    mAnimation->addFrameTimestamp(image, this->mTimestamp);
    if(sync) mAnimation->waitForIdle();
}

void KoiConvert::encodeNextFrameNoProgressBar(bool sync)
{
    if(mFinishFlag || mAnimation->isBusy()){
        return;
    }
    bool isLastFrame = mMoive->currentFrameNumber() + 1 == mMoive->frameCount();
    if(isLastFrame){
        mFinishFlag = true;
        mAnimation->finish(mMoive->nextTimeStamp());
        if(sync) mAnimation->waitForIdle();
        return;
    }
    this->mTimestamp = mMoive->timeStamp();
    mAnimation->addFrameTimestamp(mMoive->currentImage(), this->mTimestamp);
    mMoive->jumpToNextFrame();
    if(sync) mAnimation->waitForIdle();
}

void KoiConvert::encodeNextFrame(bool sync)
{
    if(mProgressBar){
        encodeNextFrameProgressBar(sync);
    }else{
        encodeNextFrameNoProgressBar(sync);
    }
}

void KoiConvert::addProgressBar(bool enable)
{
    mProgressBar = true;
}

void KoiConvert::setDuration(qint64 duration)
{
    mDuration = duration;
}

int KoiConvert::frameCount()
{
    return mMoive->frameCount();
}

qreal KoiConvert::progress()
{
    if(mProgressBar){
        return this->mTimestamp / (qreal) this->mDuration;
    }
    return (mMoive->currentFrameNumber() + 1) / (qreal)mMoive->frameCount();
}

KoiConvert::~KoiConvert()
{
    delete mMoive;
    if(mNeedDeleteAnimation) delete mAnimation;
}