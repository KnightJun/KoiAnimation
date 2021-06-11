#pragma once

#include "koianimation_global.h"
#include <QObject>
#include <QFile>
#include <QImage>

enum KoiFormat
{
	GIF,
	APNG,
	WEBP,
	Dga,
	Format_Count
};

class AniEncoder;
class QSettings;
class QThread;
class KOIANIMATION_EXPORT KoiAnimation : public QObject
{
	Q_OBJECT
public:
    KoiAnimation();
	~KoiAnimation();
    void init(QString fileName, KoiFormat forma);
	void init(QString fileName);
	bool addFrameTimestamp(const QImage frame, int Timestamp);
	void finish(int Timestamp = 0);
	bool isBusy();
	bool waitForIdle(int timeout = 0);
	KoiFormat format();
	QString fileName() { return mFileName; };
	int timeStamp() {return mTimestamp;};
	size_t encodedSize();
	float framePerSec();
	size_t frameCount();
	static void setOptionSetting(QSettings* setting);
	void enablePerformStatistic(bool enable);
	const QString performStatistic();
	static QString FormatStr(int format);
signals:
	void sigAddFrameTimestamp(const QImage frame, int Timestamp);
	void sigAddFrameFinish();

private:
	void _addFrameTimestamp(const QImage frame, int Timestamp);
	int mLastFrameCnt = 0;
	float mLastFpsCalc = 0;
	QString mFileName;
	int mLastFrameTime = 0;
	int mTimestamp = 0;
	AniEncoder* mEncoder = nullptr;
	QFile mFile;
	size_t mFinishSize = 0;
	KoiFormat mFormat = GIF;
	bool mThreadBusy=false;
	QThread *mRunThread = nullptr;
	int mFistTimeStamp = 0;
};
