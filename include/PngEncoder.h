#pragma once
#include "AniEncoder.h"
#include "KoiAnimation.h"
#include <ImageUtils.h>

struct PNGParam;
class PngEncoder :
    public AniEncoder
{
public:
	PngEncoder();
	virtual ~PngEncoder();
	virtual bool init(QIODevice *file);
	virtual bool encodeFrame(QImage frame, uint16_t delayTime);
	virtual bool encodeTimestamp(QImage frame, uint32_t timestamp);
	virtual bool finish(uint32_t timestamp = 0);
	virtual size_t encodedSize();
	virtual size_t frameCount() { return mFrameCount + mJumpFrameCount; };
	static void setDefaultOpt();
	QString suffix(){return "png";};
	static QString formatName(){return "png";};
private:
	bool setImageSize(QSize size);
	bool __addFrameRGBA(QImage &image, const __int64 delay);
	QImage mLastFrame;
	int64_t mTimestampRecord = 0;
	int32_t mFrameCount = 0;
	int32_t mJumpFrameCount = 0;
	size_t mInitPos = 0;
	size_t mFinishSize = 0;
	struct PNGParam* mPngParam = nullptr;
};

