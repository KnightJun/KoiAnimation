#pragma once
#include <QImage>
#include <QFile>
#include <QDateTime>

enum EncodeResult
{
	EncodeFail = 0,
	EncodeSkip,
	EncodeFinish
};
class AniEncoder
{
public:
	virtual ~AniEncoder(){
		if(mStatisticTimes) delete[] mStatisticTimes;
	};
	virtual bool init(QIODevice *file) = 0;
	virtual bool encodeFrame(QImage frame, uint16_t delayTime) = 0;
	virtual bool encodeTimestamp(QImage frame, uint32_t timestamp) = 0;
	virtual bool finish(uint32_t timestamp = 0) = 0;
	virtual size_t encodedSize(){return 0;};
	virtual size_t frameCount() = 0;
	virtual QString suffix() = 0;
	void enablePerformStatistic(bool enable){
		mPerformStatistic = enable;
		if(mPerformStatistic){
			mStatisticTimes = new TimeCount[statisticItemCount()]{};
		}else{
			delete[] mStatisticTimes;
			mStatisticTimes = 0;
		}
	};
	virtual QString performStatistic(){return QString();};
protected:
	struct TimeCount
	{
		int64_t start = 0;
		int64_t count = 0;
	};
	TimeCount* mStatisticTimes = nullptr;
	bool mPerformStatistic = false;
	virtual size_t statisticItemCount(){return 0;};
	void statisticBegin(int ItemId){
		if(!mPerformStatistic) return;
		mStatisticTimes[ItemId].start = QDateTime::currentMSecsSinceEpoch();
	};
	void statisticEnd(int ItemId){
		if(!mPerformStatistic) return;
		mStatisticTimes[ItemId].count += QDateTime::currentMSecsSinceEpoch() - mStatisticTimes[ItemId].start;
	};
};