#pragma once
#include "AniEncoder.h"
#include "KoiAnimation.h"
#include <ImageUtils.h>

struct WEBPParam;
class WebpEncoder :
    public AniEncoder
{
public:
	WebpEncoder();
	virtual ~WebpEncoder();
	virtual bool init(QIODevice *file);
	virtual bool encodeFrame(QImage frame, uint16_t delayTime);
	virtual bool encodeTimestamp(QImage frame, uint32_t timestamp);
	virtual bool finish(uint32_t timestamp = 0);
	virtual size_t frameCount() { return mFrameCount; };
	static void setDefaultOpt();
	QString suffix(){return "webp";};
	static QString formatName(){return "webp";};
private:
    size_t mFrameCount = 0;
	size_t mLastDelay = 0;
	QImage mLastFrame;
	int32_t width, height;
	struct WEBPParam* webpParam;
};
