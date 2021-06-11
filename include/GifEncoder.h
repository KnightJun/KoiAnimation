#pragma once
#include "AniEncoder.h"
#include "KoiAnimation.h"
#include <ImageUtils.h>

class OcTree;
struct GifLzwNode;
class GifEncoder :
    public AniEncoder
{
public:
	GifEncoder();
	virtual ~GifEncoder();
	virtual bool init(QIODevice *file);
	virtual bool encodeFrame(QImage frame, uint16_t delayTime);
	virtual bool encodeTimestamp(QImage frame, uint32_t timestamp);
	virtual bool finish(uint32_t timestamp = 0);
	virtual size_t encodedSize();
	virtual size_t frameCount() { return frameCnt; };
	virtual QString performStatistic();
	static void setDefaultOpt();
	virtual QString suffix(){return "gif";};
	static QString formatName(){return "gif";};
private:
	enum StatisticItem{
		FormatConvert,
		ColorQuant,
		DifferImage,
		LZWCompress,
		StatisticItemMax
	};
	virtual size_t statisticItemCount(){return StatisticItemMax;};
	EncodeResult _encodeFrame(QImage frame, uint16_t delayTime);
	void putLogicalScreenDescriptor(const uint8_t* localTable, int localTableSize);
	void putGraphicControlExtension(int delayTime, int transIndex);
	void putImageDescriptor(const uint8_t* localTable, int localTableSize);
	void CalcDiffRect(QImage& newFrame);
	void debugDrawDiffRect();
	bool mEncodeing = false;
	size_t mFileSize = 0;
	// int mPaletteType = EncodeOcTreePalette;
	// DitherType mDitherType = DitherType::Diffuse;
	OcTree *mOcTree = nullptr;
	int mDebugShowTrans = false;
	int mDebugShowCurRect = false;
	int mWidth = -1;
	int mHeight = -1;
	uint32_t mTimestamp;
	uint8_t* mTransMaskData = nullptr;
	uint8_t* mImageIndexData = nullptr;
	qint64 mGloTablePos;
	QRect mDiffRect;
	QIODevice *mFile;
	size_t frameCnt = 0;
	QImage lastFrame;
	struct TableStat mTableStat[256];
	int mTransIndex;
	void put_image(uint16_t w, uint16_t h, uint16_t x, uint16_t y);
	void gifHead(const QImage &image);
	void fputc(uint8_t c);
	void fputs(const char* s);
	qint64 lastDelayPos = 0;
	uint16_t lastDelay = 0;
	void adjustLastDelay(uint16_t delayTime);
	GifLzwNode* mCodetree = nullptr;
};

