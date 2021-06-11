#include "GifEncoder.h"
#include <QDebug>
#include <OcTree.h>
#include <QFile>
#include <assert.h>
#include "options/KoiOptionUtils.h"
using namespace KoiGifOpt;

struct QRgbMap {
    inline QRgbMap() : used(0) { }
    uchar  pix;
    uchar used;
    QRgb  rgb;
};

struct GifLzwNode
{
    uint16_t m_next[256];
};

void write_leshort(QIODevice* f, uint16_t c)
{
    static uint8_t buf[2];
    buf[0] = c & 0xff;
    buf[1] = c >> 8;
    f->write((char *)buf, 2);
    return;
}

void GifEncoder::setDefaultOpt()
{
    opt_SetDefaultVal(PattleType, EncodeOcTreePalette);
    opt_SetDefaultVal(DitherType, DitherType::Ordered);
}

GifEncoder::GifEncoder()
{
    mCodetree = new GifLzwNode[4096];
}

GifEncoder::~GifEncoder()
{
    if (mTransMaskData) delete[] mTransMaskData;
    if (mImageIndexData) delete[] mImageIndexData;
    if (mCodetree) delete[] mCodetree;
}

bool GifEncoder::init(QIODevice *file)
{
    mEncodeing = true;
	mFile = file;
    frameCnt = 0;
    mTimestamp = 0;
    memset(mTableStat, 0, 256 * sizeof(struct TableStat));
    if (opt_int(PattleType) == EncodeOcTreePalette) {
        mOcTree = new OcTree(255);
    }
	return false;
}
bool GifEncoder::encodeFrame(QImage frame, uint16_t delayTime)
{
    return _encodeFrame(frame, delayTime);
}

// Simple structure to write out the LZW-compressed portion of the image
// one bit at a time
struct GifBitStatus
{
    uint8_t bitIndex;  // how many bits in the partial byte written so far
    uint8_t byte;      // current partial byte

    uint8_t chunkIndex;
    uint8_t chunk[256];   // bytes are written in here until we have 256 of them, then written to the file
};
// insert a single bit
void GifWriteBit(GifBitStatus& stat, uint32_t bit)
{
    bit = bit & 1;
    bit = bit << stat.bitIndex;
    stat.byte |= bit;

    ++stat.bitIndex;
    if (stat.bitIndex > 7)
    {
        // move the newly-finished byte to the chunk buffer
        stat.chunk[stat.chunkIndex++] = stat.byte;
        // and start a new byte
        stat.bitIndex = 0;
        stat.byte = 0;
    }
}

// write all bytes so far to the file
void GifWriteChunk(QIODevice* f, GifBitStatus& stat)
{
    uint8_t tmp = stat.chunkIndex;
    //f->write((char*)&tmp, 1);
    f->write((char*)&stat.chunkIndex, 1 + stat.chunkIndex);

    stat.bitIndex = 0;
    stat.byte = 0;
    stat.chunkIndex = 0;
}

void GifWriteCode(QIODevice* f, GifBitStatus& stat, uint32_t code, uint32_t length)
{
    for (uint32_t ii = 0; ii < length; ++ii)
    {
        GifWriteBit(stat, code);
        code = code >> 1;

        if (stat.chunkIndex == 255)
        {
            GifWriteChunk(f, stat);
        }
    }
}

void GifEncoder::put_image(uint16_t w, uint16_t h, uint16_t x, uint16_t y)
{
    const int minCodeSize = 8;
    const uint32_t clearCode = 1 << 8;

    fputc(minCodeSize); // min code size 8 bits

    memset(mCodetree, 0, sizeof(GifLzwNode) * 4096);
    int32_t curCode = -1;
    uint32_t codeSize = (uint32_t)minCodeSize + 1;
    uint32_t maxCode = clearCode + 1;

    GifBitStatus stat;
    stat.byte = 0;
    stat.bitIndex = 0;
    stat.chunkIndex = 0;

    GifWriteCode(mFile, stat, clearCode, codeSize);  // start with a fresh LZW dictionary

    for (uint32_t yy = y; yy < y + h; ++yy)
    {
        uint8_t* linePtr = mImageIndexData + yy * mWidth;
        for (uint32_t xx = x; xx < x+w; ++xx)
        {
            // top-left origin
            uint8_t nextValue = linePtr[xx]; //linePtr[xx];

            // "loser mode" - no compression, every single code is followed immediately by a clear
            //WriteCode( f, stat, nextValue, codeSize );
            //WriteCode( f, stat, 256, codeSize );

            if (curCode < 0)
            {
                // first value in a new run
                curCode = nextValue;
            }
            else if (mCodetree[curCode].m_next[nextValue])
            {
                // current run already in the dictionary
                curCode = mCodetree[curCode].m_next[nextValue];
            }
            else
            {
                // finish the current run, write a code
                GifWriteCode(mFile, stat, (uint32_t)curCode, codeSize);

                // insert the new run into the dictionary
                mCodetree[curCode].m_next[nextValue] = (uint16_t)++maxCode;

                if (maxCode >= (1ul << codeSize))
                {
                    // dictionary entry count has broken a size barrier,
                    // we need more bits for codes
                    codeSize++;
                }
                if (maxCode == 4095)
                {
                    // the dictionary is full, clear it out and begin anew
                    GifWriteCode(mFile, stat, clearCode, codeSize); // clear tree

                    memset(mCodetree, 0, sizeof(GifLzwNode) * 4096);
                    codeSize = (uint32_t)(minCodeSize + 1);
                    maxCode = clearCode + 1;
                }

                curCode = nextValue;
            }
        }
    }

    // compression footer
    GifWriteCode(mFile, stat, (uint32_t)curCode, codeSize);
    GifWriteCode(mFile, stat, clearCode, codeSize);
    GifWriteCode(mFile, stat, clearCode + 1, (uint32_t)minCodeSize + 1);

    // write out the last partial chunk
    while (stat.bitIndex) GifWriteBit(stat, 0);
    if (stat.chunkIndex) GifWriteChunk(mFile, stat);

    fputc(0); // image block terminator

}

void GifEncoder::gifHead(const QImage& image)
{

    if (mWidth != image.width() || image.height() != mHeight) {
        mWidth = image.width();
        mHeight = image.height();
        if (mTransMaskData) delete[] mTransMaskData;
        if (mImageIndexData) delete[] mImageIndexData;
        mTransMaskData = new uint8_t[mWidth * (long)mHeight];
        mImageIndexData = new uint8_t[mWidth * (long)mHeight];
    }
    fputs("GIF89a");

    // screen descriptor
    if (opt_int(PattleType) == EncodeGloPalette) {
        putLogicalScreenDescriptor(GetCube216ColorTable(), 256);
    }
    else if (EncodeOcTreePalette == opt_int(PattleType)) {
        putLogicalScreenDescriptor(nullptr, 0);
    }
    else
    {
        assert(0);
    }

    // animation header
    fputc(0x21); // extension
    fputc(0xff); // application specific
    fputc(11); // length 11
    fputs("NETSCAPE2.0"); // yes, really
    fputc(3); // 3 bytes of NETSCAPE2.0 data

    fputc(1); // JUST BECAUSE
    fputc(0); // loop infinitely (byte 0)
    fputc(0); // loop infinitely (byte 1)

    fputc(0); // block terminator
}

void GifEncoder::fputc(uint8_t c)
{
    mFile->write((char*)&c, 1);
}

void GifEncoder::fputs(const char* s)
{
    mFile->write(s, strlen(s));
}

void GifEncoder::adjustLastDelay(uint16_t delayTime)
{
    qint64 tmpPos = mFile->pos();
    mFile->seek(lastDelayPos);
    write_leshort(mFile, delayTime / 10);
    lastDelay = delayTime;
    mFile->seek(tmpPos);
    return;
}

// #define COLOR_EQ(a, b) (((a) & 0xffffffff) == ((b) & 0xffffffff))
#define COLOR_EQ(a, b) ((a) == (b))
/* 
    找到一行像素的差异边界
    差异部分把oldbuf改为newbuf
    相同部分把newbuf置0
*/ 
static bool memFindDiff(uint32_t const* newBuf, uint32_t* oldBuf, uint8_t* transBuf,
    size_t _Size, int *left, int *right)
{
    size_t i = 0;
    for (; i < _Size; i++) {
        if (!COLOR_EQ(*newBuf, *oldBuf))
            break;
        newBuf++;
        oldBuf++;
        transBuf++;
    }
    if(i == _Size) return false;
    *left = i;
    for (; i < _Size; i++) {
        if (!COLOR_EQ(*newBuf, *oldBuf)) {
            *(oldBuf) = *(newBuf);
            *transBuf = 1;
            *right = i;
        }
        newBuf++;
        oldBuf++;
        transBuf++;
    }
    return true;
}

QRect ImageDiff(QImage &oldFrame, QImage &newFrame, uint8_t* mTransMaskData)
{
    int bytePrePixel = (newFrame.depth() / 8);
    int lineSize = bytePrePixel * newFrame.width();
    QRect mDiffRect(-1, -1, -1, -1);
    size_t y;
    int left = newFrame.width();
    int right = 0;
    int tmpLeft = 0, tmpRight = 0;
    memset(mTransMaskData, 0, newFrame.width() * newFrame.height());
    for (y = 0; y < newFrame.height(); y++)
    {
        uint32_t* linePtrNew = (uint32_t*)newFrame.scanLine(y);
        uint32_t* linePtrOld = (uint32_t*)oldFrame.scanLine(y);
        uint8_t* linePtrTrans = mTransMaskData + y * newFrame.width();
        if (memFindDiff(linePtrNew, linePtrOld, linePtrTrans, newFrame.width(), &tmpLeft, &tmpRight)) {
            if (mDiffRect.y() < 0) mDiffRect.setTop(y);
            mDiffRect.setBottom(y);
            left = std::min(left, tmpLeft);
            right = std::max(right, tmpRight);
        }
    }
    if (mDiffRect.y() < 0) {
        return mDiffRect;
    }
    mDiffRect.setLeft(left);
    mDiffRect.setRight(right);
    return mDiffRect;
}

void GifEncoder::CalcDiffRect(QImage &newFrame)
{
    statisticBegin(DifferImage);
    mDiffRect = ImageDiff(lastFrame, newFrame, mTransMaskData);
    statisticEnd(DifferImage);
    return;
}

void GifEncoder::debugDrawDiffRect()
{
    uint8_t *indexPtr = mImageIndexData;
    indexPtr = mImageIndexData + mWidth * mDiffRect.y() + mDiffRect.x();
    memset(indexPtr, 0xfe, mDiffRect.width());
    for (size_t i = mDiffRect.y(); i < mDiffRect.bottom(); i++)
    {
        indexPtr = mImageIndexData + mWidth * i + mDiffRect.x();
        *indexPtr = 0xfe;
        indexPtr = mImageIndexData + mWidth * i + mDiffRect.right();
        *indexPtr = 0xfe;
    }
    indexPtr = mImageIndexData + mWidth * mDiffRect.bottom() + mDiffRect.x();
    memset(indexPtr, 0xfe, mDiffRect.width());
}

EncodeResult GifEncoder::_encodeFrame(QImage frame, uint16_t delayTime)
{
	if (frameCnt == 0) {
        gifHead(frame);
        mDiffRect.setRect(0, 0, mWidth, mHeight);
        memset(mTransMaskData, 1, mWidth * mHeight);
        lastFrame = frame.copy();
	}
	else {
		if (lastFrame.size() != frame.size())return EncodeFail;
        CalcDiffRect(frame);
        if (mDiffRect.y() < 0) {
            adjustLastDelay(lastDelay + delayTime);
            frameCnt++;
            return EncodeSkip;
        }
	}

    if (EncodeGloPalette == opt_int(PattleType)) {
        statisticBegin(ColorQuant);
        Dither(opt_int(DitherType), mImageIndexData, frame.bits(), mTransMaskData,
            frame.bytesPerLine(), mWidth, mHeight, mDiffRect);
        statisticEnd(ColorQuant);
        putGraphicControlExtension(delayTime, TransIndex);
        putImageDescriptor(nullptr, 0);
    }
    else if(EncodeOcTreePalette == opt_int(PattleType)){
        statisticBegin(ColorQuant);
        mOcTree->ResetOctree(255);
        mOcTree->BuildOctree(frame, mDiffRect, nullptr);
        RGB imgPalette[256];
        int palSize = mOcTree->ExtractOctreePalette(imgPalette) + 1;
        int transIndex = palSize - 1;
        mOcTree->DumpIndexData(mImageIndexData, frame, mDiffRect,
            frameCnt == 0 ? nullptr : mTransMaskData, transIndex);
        statisticEnd(ColorQuant);
        putGraphicControlExtension(delayTime, transIndex);
        putImageDescriptor((uint8_t *)imgPalette, palSize);
    }


    if (mDebugShowCurRect) {
        debugDrawDiffRect();
    };
    statisticBegin(LZWCompress);
	put_image(mDiffRect.width(), mDiffRect.height(), mDiffRect.x(), mDiffRect.y());
    statisticEnd(LZWCompress);
    frameCnt++;
	return EncodeFinish;
}

inline static int SizeToDepth(int localTableSize)
{
    int nb = 1;
    while (nb < 8 && (1 << nb) < localTableSize) nb++;

    return nb;
}

void GifEncoder::putLogicalScreenDescriptor(const uint8_t* localTable, int localTableSize)
{
    int colorDepth = SizeToDepth(localTableSize);
    fputc(mWidth & 0xff);
    fputc((mWidth >> 8) & 0xff);
    fputc(mHeight & 0xff);
    fputc((mHeight >> 8) & 0xff);
    uint8_t flag = 70;
    if (localTable) {
        flag |= 0x80;
        flag |= (colorDepth - 1);
    }
    fputc(flag);  // there is an unsorted global color table of 2 entries
    fputc(0);     // background color
    fputc(0);     // pixels are square (we need to specify this because it's 1989)

    // now the "global" palette (really just a dummy palette)
    if (localTable) {
        mGloTablePos = mFile->pos();
        mFile->write((char*)localTable, (1 << colorDepth) * 3);
    }
}

void GifEncoder::putGraphicControlExtension(int delayTime, int transIndex)
{
    // Graphic Control Extension
    uint8_t buffer[] = { 0x21, 0xf9, 0x04, 0x04 };
    if (!mDebugShowTrans) {
        buffer[3] |= 1;
    }
    mFile->write((char*)buffer, 4);
    // delay time
    lastDelayPos = mFile->pos();
    lastDelay = delayTime;
    write_leshort(mFile, delayTime / 10);
    // transparent color index
    buffer[0] = transIndex;
    // end
    buffer[1] = 00;
    mFile->write((char*)buffer, 2);
}

void GifEncoder::putImageDescriptor(const uint8_t* localTable, int localTableSize)
{
    int colorDepth = SizeToDepth(localTableSize);
    fputc(0x2c);
    write_leshort(mFile, mDiffRect.x());
    write_leshort(mFile, mDiffRect.y());
    write_leshort(mFile, mDiffRect.width());
    write_leshort(mFile, mDiffRect.height());
    uint8_t packedField = 0;
    if (localTable) {
        packedField = 0x80 | (colorDepth - 1);
    }
    fputc(packedField);
    if (localTable) {
        mFile->write((char*)localTable, (1 << colorDepth) * 3);
    }
}

bool GifEncoder::encodeTimestamp(QImage frame, uint32_t timestamp)
{
    int errCode = 0;
    if (frameCnt > 0) {
        adjustLastDelay(timestamp - mTimestamp);
    }
    errCode = _encodeFrame(frame, 2333);
    if (errCode == EncodeFinish) {
        mTimestamp = timestamp;
    }
    return true;
}

bool GifEncoder::finish(uint32_t timestamp)
{
    if (timestamp > 0) {
        adjustLastDelay(timestamp - mTimestamp);
    }
    fputc(0x3b);
    mFileSize = mFile->pos();
    if (mOcTree)delete mOcTree;
    mEncodeing = false;
	return false;
}

size_t GifEncoder::encodedSize()
{
    if (mEncodeing) {
        return mFile->pos();
    }
    else {
        return mFileSize;
    }
}
QString GifEncoder::performStatistic()
{
    QString statisStr;
    statisStr = QString("FormatConvert:%1, ColorQuant:%2, DifferImage:%3, LZWCompress:%4")
                .arg(mStatisticTimes[FormatConvert].count)
                .arg(mStatisticTimes[ColorQuant].count)
                .arg(mStatisticTimes[DifferImage].count)
                .arg(mStatisticTimes[LZWCompress].count);
    return statisStr;
}