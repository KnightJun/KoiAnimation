#include "DgaIOHandler.h"
#include "DgaFormat.h"
#include <QImage>
#include <QDebug>
#include <QFile>
#include "QZstd.h"
bool DgaIOHandler::canRead() const
{
    initCheck((DgaIOHandler *)this);
    bool ret = this->mDgaHeader->magicNum == 0xD19 && mImgIndex < mDgaHeader->frameCount;
    return ret;
}

void DgaIOHandler::initCheck(DgaIOHandler* iohandler)
{
    if(iohandler->mDev == iohandler->device()) return;
    
    iohandler->mDev = iohandler->device();
    if(!iohandler->mDev->isOpen())
    {
        qDebug() << "DgaIOHandler::Device not open!";    
    }
    iohandler->mDev->peek((char *)iohandler->mDgaHeader, sizeof(DgaHeader));
    iohandler->mEnd = true;
    iohandler->mDecompress = new QZstdDecompress(iohandler->mDev);
    return;
}

bool DgaIOHandler::canRead(QIODevice *dev)
{
    DgaHeader DgaHeader;
    dev->peek((char *)&DgaHeader, sizeof(DgaHeader));
    return DgaHeader.magicNum == 0xD19;
}

QVariant DgaIOHandler::option(ImageOption option) const
{
    initCheck((DgaIOHandler *)this);
	switch(option) {
	case QImageIOHandler::Size:
		return QSize(mDgaHeader->imgWidth, mDgaHeader->imgHeight);
	case QImageIOHandler::Animation:
		return true;
	default:
		return QVariant();
	}
}
bool DgaIOHandler::supportsOption(QImageIOHandler::ImageOption option) const
{
	switch(option) {
	case QImageIOHandler::Size:
	case QImageIOHandler::Animation:
		return true;
	default:
		return false;
	}
}

DgaIOHandler::DgaIOHandler()
{
    mDgaHeader = new DgaHeader;
    mDgaFrame = new DgaFrame;
    mNextDgaFrame = new DgaFrame;
}

DgaIOHandler::~DgaIOHandler()
{
    delete mDgaHeader;
    delete mDgaFrame;
    delete mNextDgaFrame;
    if(mDecompress) delete mDecompress;
}

bool DgaIOHandler::jumpToImage(int imageNumber)
{
    initCheck((DgaIOHandler *)this);
    mImgIndex = imageNumber;
	return (mImgIndex < mDgaHeader->frameCount);
}

int DgaIOHandler::nextImageDelay() const
{
    return mNextDgaFrame->timeStamp - mDgaFrame->timeStamp;
}

int	DgaIOHandler::currentImageNumber() const
{
    qDebug() << "currentImageNumber : " << mImgIndex;
    return mImgIndex;
}

int	DgaIOHandler::imageCount() const
{
    initCheck((DgaIOHandler *)this);
    return mDgaHeader->frameCount;
}

bool DgaIOHandler::jumpToNextImage()
{
	return (++mImgIndex < mDgaHeader->frameCount);
}

void ImageXor(quint8 *src1, quint8 *src2, int count)
{
    for (size_t i = 0; i < count; i++)
    {
        *(src1) = *(src1) ^ *(src2);
        src1++;
        src2++;
    }
    return;
}

bool DgaIOHandler::read(QImage *image)
{    
    initCheck((DgaIOHandler *)this);
    if(mImgIndex >= mDgaHeader->frameCount)return false;

    QIODevice *dev = this->device();
    if(mImgIndex == 0 ||  mImgIndex < mImgIndexOnfile)
    {
        dev->seek(sizeof(DgaHeader));
        mDecompress->reset();
        mDecompress->read((char *)mNextDgaFrame, sizeof(DgaFrame));
        mImgIndexOnfile = -1;
    }
    while (mImgIndex != mImgIndexOnfile)
    {
        *mDgaFrame = *mNextDgaFrame;
        mFrameBuff = mDecompress->read(mDgaFrame->dataLen);
        mDecompress->read((char *)mNextDgaFrame, sizeof(DgaFrame));
        mImgIndexOnfile++;
    }
    
    if(mDgaFrame->opt == OptCopy){
        *image = QImage((uchar *)mFrameBuff.data(), 
            mDgaFrame->imgWidth, mDgaFrame->imgHeight, 
            (QImage::Format)mDgaFrame->imgformat).copy();
    }else if(mDgaFrame->opt == OptXor){
        ImageXor(image->bits(), (quint8 *)mFrameBuff.data(), 
            image->bytesPerLine() * image->height());
    }
    mImgIndex++;
    return true;
}
