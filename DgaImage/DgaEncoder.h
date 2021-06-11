#pragma once
#include <AniEncoder.h>
#include <ImageUtils.h>
#include <QBuffer>

struct PNGParam;
struct DgaHeader;
class QZstdCompress;
class DgaEncoder :
    public AniEncoder
{
public:
	DgaEncoder();
	virtual ~DgaEncoder();
	virtual bool init(QIODevice *file);
	bool encodeFrame(QImage frame, uint16_t delayTime){return true;};
	virtual bool encodeTimestamp(QImage frame, uint32_t timestamp);
	virtual bool finish(uint32_t timestamp = 0);
	virtual size_t encodedSize();
	virtual size_t frameCount() { return mFrameCount + mJumpFrameCount; };
	static void setDefaultOpt(){};
	virtual QString suffix(){return "Dga";};
	static QString formatName(){return "Dga";};
private:
    void encodeHeader(const QSize &imgSize);
    void writeFrame(const QImage &image, quint32 timeStamp, int x = 0, int y = 0);
	void compressWrite(const quint8* data, size_t len, bool finish = false);
	QImage mLastFrame;
	int32_t mFrameCount = 0;
	int32_t mJumpFrameCount = 0;
    QIODevice *mFile = nullptr;
    quint8 *mXorDstBuff = nullptr;
    DgaHeader *mHeader = nullptr;
	QZstdCompress* mCompress = nullptr;

};
