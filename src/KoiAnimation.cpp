#include "KoiAnimation.h"
#include <GifEncoder.h>
#include <PngEncoder.h>
#include <DgaEncoder.h>
#include <WebpEncoder.h>
#include <QDebug>
#include <QFileInfo>
#include <QSettings>
#include <QThread>
#include <QMutex>

QSettings *KoiOptions = nullptr;
void KoiAnimation::setOptionSetting(QSettings* setting)
{
	KoiOptions = setting;
	GifEncoder::setDefaultOpt();
	PngEncoder::setDefaultOpt();
	WebpEncoder::setDefaultOpt();
}
KoiAnimation::KoiAnimation()
{
	connect(this, &KoiAnimation::sigAddFrameTimestamp, 
		this, &KoiAnimation::_addFrameTimestamp);
	mRunThread = new QThread(this);
	this->moveToThread(mRunThread);
	mRunThread->start();
}

KoiAnimation::~KoiAnimation()
{
	qDebug() << "mRunThread->quit()";
	mRunThread->quit();
	mRunThread->wait();
	qDebug() << "mRunThread->terminate()";
	mRunThread->terminate();
	if (mEncoder)delete mEncoder;
}
KoiFormat KoiAnimation::format()
{
	return mFormat;
}


void KoiAnimation::init(QString fileName)
{
    QFileInfo info(fileName);
	QString suffix = info.suffix();
	KoiFormat format;
    if(suffix == GifEncoder::formatName()){
		format = GIF;
    }else if(suffix == PngEncoder::formatName()){
		format = APNG;
    }else if(suffix == WebpEncoder::formatName()){
		format = WEBP;
	}else if(suffix == DgaEncoder::formatName()){
		format = Dga;
	}else{
		qWarning() << "unknow format:" << suffix;
		return;
	}
	init(fileName, format);
}


void KoiAnimation::init(QString fileName, KoiFormat format)
{
	if (mEncoder)delete mEncoder;
	mFormat = format;
	switch (format)
	{
	case GIF:
		mEncoder = new GifEncoder();
		break;
	case APNG:
		mEncoder = new PngEncoder();
		break;
	case WEBP:
		mEncoder = new WebpEncoder();
		break;
	case Dga:
		mEncoder = new DgaEncoder();
		break;
	default:
		Q_ASSERT(0);
		break;
	}
	mLastFrameCnt = 0;
	mLastFpsCalc = 0;
	mLastFrameTime = 0;
	mFinishSize = 0;
	mTimestamp = 0;
	mFile.close();
	mFile.setFileName(fileName);
	if (!mFile.open(QIODevice::WriteOnly)) {
		qDebug() << "open file fail";
		return;
	}
	mFileName = QFileInfo(fileName).canonicalFilePath();
	mEncoder->init(&mFile);
}

bool KoiAnimation::addFrameTimestamp(const QImage frame, int Timestamp)
{
	if(mThreadBusy)return false;
	mThreadBusy = true;
	emit sigAddFrameTimestamp(frame, Timestamp);
	return true;
}
bool KoiAnimation::waitForIdle(int timeout)
{
	while (mThreadBusy)
	{
		QThread::msleep(1);
	}
	return true;
}

size_t KoiAnimation::frameCount()
{
	if (mEncoder)
		return mEncoder->frameCount();
	else
		return 0;
}
float KoiAnimation::framePerSec()
{
	if (!mEncoder)return 0;
	if (mTimestamp - mLastFrameTime > 1000)
	{
		mLastFpsCalc = (mEncoder->frameCount() - mLastFrameCnt) /
			((mTimestamp - mLastFrameTime) / 1000.0);
		mLastFrameCnt = mEncoder->frameCount();
		mLastFrameTime = mTimestamp;
	}
	return mLastFpsCalc;
}
size_t KoiAnimation::encodedSize()
{
	if(mFile.isOpen()){
		return mFile.pos();
	}
	return mFinishSize;
}
void KoiAnimation::_addFrameTimestamp(const QImage frame, int Timestamp)
{
	if (!mFile.isOpen()) {
		qDebug() << "open file fail";
		mThreadBusy = false;
		return;
	}
	if(mEncoder->frameCount() == 0){
		mFistTimeStamp = Timestamp;
	}
	Timestamp = Timestamp - mFistTimeStamp;
	if(mFormat == GIF){
		Timestamp = qRound(Timestamp / 10.0f)*10;
	}
	if(frame.isNull()){
		mEncoder->finish(Timestamp);
		mFinishSize = mFile.pos();
		mFile.close();
	}else{
		mEncoder->encodeTimestamp(frame, Timestamp);
	}
	mTimestamp = Timestamp;
	mThreadBusy = false;
	emit sigAddFrameFinish();
}

bool KoiAnimation::isBusy()
{
	return mThreadBusy;
}

void KoiAnimation::finish(int Timestamp)
{
	if(mThreadBusy)return;
	mThreadBusy = true;
	emit sigAddFrameTimestamp(QImage(), Timestamp);
	return;
}
void KoiAnimation::enablePerformStatistic(bool enable)
{
	if(mEncoder)mEncoder->enablePerformStatistic(enable);
}
const QString KoiAnimation::performStatistic()
{
	if(mEncoder){
		return mEncoder->performStatistic();
	}else{
		return QString();
	}
}

QString KoiAnimation::FormatStr(int format)
{
    switch (format)
    {
    case KoiFormat::GIF:
        return "gif";
        break;
    case KoiFormat::APNG:
        return "png";
        break;
    case KoiFormat::WEBP:
        return "webp";
        break;
    case KoiFormat::Dga:
        return "dga";
        break;
    default:
        return "unknow";
        break;
    }
}
