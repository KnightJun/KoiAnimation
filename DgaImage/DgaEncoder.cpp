#include "DgaEncoder.h"
#include "DgaFormat.h"
#include "QZstd.h"
#include <QDebug>

const bool useXor = false;
bool DgaEncoder::init(QIODevice *file){
    mFile = file;
	mFrameCount = 0;
	mJumpFrameCount = 0;
    mLastFrame = QImage();
    mCompress = new QZstdCompress(file);
    return true;
}

void DgaEncoder::encodeHeader(const QSize &imgSize)
{
    mHeader->magicNum = 0xD19;
    mHeader->version = 1;
    mHeader->imgWidth = imgSize.width();
    mHeader->imgHeight = imgSize.height();
    mHeader->frameCount = 0;
    mFile->write((const char *)&mHeader, sizeof(DgaHeader));
}

bool ImageDiffXor(QImage frame1, QImage frame2, quint8 *dest)
{
    assert(frame2.format() == QImage::Format_ARGB32);
    quint8 isDiff = 0;
    const quint8 *src1 = frame1.constBits();
    const quint8 *src2 = frame2.constBits();
    int count = frame2.height() * frame2.bytesPerLine();
    for (size_t i = 0; i < count; i++)
    {
        *(dest) = *(src1++) ^ *(src2++);
        isDiff |= *(dest);
        dest++;
    }
    return isDiff;
}

void DgaEncoder::writeFrame(const QImage &image, quint32 timeStamp, int x, int y)
{
    DgaFrame dFrame;
    dFrame.imgformat = image.format();
    dFrame.timeStamp = timeStamp;
    dFrame.imgX = x;
    dFrame.imgY = y;
    dFrame.imgHeight = image.height();
    dFrame.imgWidth = image.width();
    dFrame.dataLen = image.height() * image.bytesPerLine();
    if(mFrameCount == 0 || useXor == false){
        dFrame.opt = OptCopy;
        mCompress->write((quint8 *)&dFrame, sizeof(DgaFrame));
        mCompress->write((quint8 *)image.constBits(), dFrame.dataLen);
    }else{
        dFrame.opt = OptXor;
        mCompress->write((quint8 *)&dFrame, sizeof(DgaFrame));
        mCompress->write(mXorDstBuff, dFrame.dataLen);
    }
}

bool DgaEncoder::encodeTimestamp(QImage frame, uint32_t timestamp)
{
    // frame = frame.convertToFormat(QImage::Format_BGR888);
    if(mFrameCount == 0)
    {
        if(mXorDstBuff) delete[] mXorDstBuff;
        mXorDstBuff = new quint8[frame.height() * frame.bytesPerLine()];
        qDebug() << "encodeHeader " << frame;
        encodeHeader(frame.size());
    }else{
        if(useXor){
            if(!ImageDiffXor(mLastFrame, frame, mXorDstBuff)){
                mJumpFrameCount++;
                return true;
            }
        }else {
            if(mLastFrame == frame){
                mJumpFrameCount++;
                return true;
            }
        }
    }
    writeFrame(frame, timestamp);
    mFrameCount++;
    mLastFrame = frame;
    return true;
}

bool DgaEncoder::finish(uint32_t timestamp)
{
    DgaFrame dFrame;
    dFrame.imgformat = 0xff;
    dFrame.timeStamp = timestamp;
    mCompress->write((quint8 *)&dFrame, sizeof(DgaFrame), true);
    qint64 tmpPos = mFile->pos();
    if(mFile->seek(0)){
        mHeader->frameCount = mFrameCount;
        mFile->write((const char *)mHeader, sizeof(DgaHeader));
        mFile->seek(tmpPos);
    }
    delete mCompress;
    mCompress = nullptr;
    return true;
}
DgaEncoder::DgaEncoder()
{
    mHeader = new DgaHeader();
}
DgaEncoder::~DgaEncoder()
{
    if(mXorDstBuff) delete[] mXorDstBuff;
    delete mHeader;
    if(mCompress) delete mCompress;
}
size_t DgaEncoder::encodedSize()
{
    return mFile->pos();
}