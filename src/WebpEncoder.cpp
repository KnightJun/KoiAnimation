#include <WebpEncoder.h>
#include <webp/encode.h>
#include <webp/mux.h>
#include <ImageDiff.h>
#include <QDebug>
#include <options/KoiOptionUtils.h>
using namespace KoiWebpOpt;

struct WEBPParam
{
	WebPAnimEncoderOptions anim_config;
	WebPConfig config;
	WebPPicture pic;
	WebPData webp_data;
	WebPAnimEncoder* enc = NULL;
	size_t timestamp = 0;
	QIODevice *filep = nullptr;
};

int WebPWriterCallback(const uint8_t* data, size_t data_size,
                                  const WebPPicture* picture)
{
    QIODevice *file = (QIODevice *)picture->custom_ptr;
	qDebug() << "write:" << data_size;
    return file->write((const char *)data, data_size) == data_size;
}

void WebpEncoder::setDefaultOpt()
{
	opt_SetDefaultVal(Lossless, 1);
	opt_SetDefaultVal(Quality, 75);
	opt_SetDefaultVal(MultiThread, 1);
	opt_SetDefaultVal(MinimizeSize, 0);
	opt_SetDefaultVal(AllowMixed, 0);
}
WebpEncoder::WebpEncoder()
{
    webpParam = new struct WEBPParam();
	WebPDataInit(&webpParam->webp_data);
	if (!WebPAnimEncoderOptionsInit(&webpParam->anim_config)){
		Q_ASSERT(0);
		return;
	}
	if (!WebPConfigInit(&webpParam->config)){
		Q_ASSERT(0);
		return;
	}
	if (!WebPPictureInit(&webpParam->pic)) {
		Q_ASSERT(0);
		return;
	}
	webpParam->config.lossless = opt_int(Lossless);
	webpParam->anim_config.minimize_size = opt_int(MinimizeSize);
	webpParam->anim_config.allow_mixed = opt_int(AllowMixed);
	webpParam->config.quality = opt_int(Quality);
	webpParam->config.thread_level = opt_int(MultiThread);
	if (!WebPValidateConfig(&webpParam->config))
	{
		Q_ASSERT(false);
		return;
	}
}

void initImageInfo(struct WEBPParam* webpParam, int width, int height)
{
    webpParam->enc = WebPAnimEncoderNew(width, height, &webpParam->anim_config);
	webpParam->pic.use_argb = 1;
	webpParam->pic.argb_stride = width;
	webpParam->pic.width = width;
	webpParam->pic.height = height;
	webpParam->timestamp = 0;
	
}
bool WebpEncoder::init(QIODevice *file)
{
    webpParam->pic.custom_ptr = file;
	webpParam->filep = file;
    webpParam->pic.writer = WebPWriterCallback;
	mFrameCount = 0;
	mLastDelay = 0;
	return true;
}

WebpEncoder::~WebpEncoder()
{
	WebPDataClear(&webpParam->webp_data);
	delete webpParam;
}

bool WebpEncoder::encodeTimestamp(QImage frame, uint32_t timestamp)
{
	QImage frame32;
	if(frame.format() != QImage::Format_RGB32){
		frame32 = frame.convertToFormat(QImage::Format_RGB32);
	}else{
		frame32 = frame;
	}
	QImage dimg;
	// if(mLastFrame.isNull()){
	// 	dimg = frame32;
	// }else{
	// 	dimg = ImageSameTransp_32bit(frame32, mLastFrame);
	// }
	dimg = frame32;
	if(mFrameCount == 0){
		initImageInfo(webpParam, frame.width(), frame.height());
	}
	webpParam->pic.argb = (uint32_t*)dimg.constBits();
	webpParam->timestamp = timestamp;
	WebPAnimEncoderAdd(webpParam->enc, &webpParam->pic, webpParam->timestamp, &webpParam->config);
	mFrameCount++;
	mLastFrame = frame32;
	return true;
}

bool WebpEncoder::encodeFrame(QImage frame, uint16_t delayTime)
{
	webpParam->timestamp += mLastDelay;
	mLastDelay = delayTime;
	return encodeTimestamp(frame, webpParam->timestamp);
}

bool WebpEncoder::finish(uint32_t timestamp)
{
	if(timestamp){
		webpParam->timestamp = timestamp;
	}else{
		webpParam->timestamp += mLastDelay;
	}
	WebPAnimEncoderAdd(webpParam->enc, NULL, webpParam->timestamp, NULL);
	WebPAnimEncoderAssemble(webpParam->enc, &webpParam->webp_data);
	webpParam->filep->write((const char*)webpParam->webp_data.bytes, webpParam->webp_data.size);
	WebPDataClear(&webpParam->webp_data);
	WebPAnimEncoderDelete(webpParam->enc);
	return false;
}