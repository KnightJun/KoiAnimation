#include <png.h>
#include <PngEncoder.h>
#include <QDebug>
#include <ImageDiff.h>
#include <options/KoiOptionUtils.h>
using namespace KoiAPngOpt;
struct PNGParam
{
	png_structp png_ptr = nullptr;
	png_infop info_ptr = nullptr;
	png_bytepp rows = nullptr;
	QIODevice *filep = nullptr;
};
static void PNGParam_uninit(struct PNGParam* pngParam)
{
	if (pngParam) {
		if (pngParam->filep) {
			// fclose(pngParam->filep);
			pngParam->filep = nullptr;
		}
		if (pngParam->rows) {
			free(pngParam->rows);
			pngParam->rows = nullptr;
		}
		if (pngParam->png_ptr || pngParam->info_ptr) {
			png_destroy_write_struct(&pngParam->png_ptr, &pngParam->info_ptr);
			pngParam->png_ptr = nullptr;
			pngParam->info_ptr = nullptr;
		}
	}
}
void png_cb_io_write(png_structp png, png_bytep data, size_t len)
{
	QIODevice* file = (QIODevice*)png_get_io_ptr(png);
	file->write((const char *)data, len);
}
size_t png_cb_io_getpos(png_structp png)
{
	QIODevice* file = (QIODevice*)png_get_io_ptr(png);
	return file->pos();
}

void png_cb_io_setpos(png_structp png, size_t pos)
{
	QIODevice* file = (QIODevice*)png_get_io_ptr(png);
	file->seek(pos);
	return;
}

void png_cb_io_flush(png_structp png)
{
	//QIODevice* file = (QIODevice*)png_get_io_ptr(png);
	//file->flush();
	return;
}

PngEncoder::PngEncoder()
{
    mPngParam = new struct PNGParam();
}
PngEncoder::~PngEncoder()
{
    PNGParam_uninit(mPngParam);
}
bool PngEncoder::setImageSize(QSize size)
{
    Q_ASSERT(this->mFrameCount == 0);
	png_color_16 transpColor = {0};
	mPngParam->png_ptr =
		png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	mPngParam->info_ptr = png_create_info_struct(mPngParam->png_ptr);
	//setjmp(png_jmpbuf(pngParam->png_ptr));
	mPngParam->rows = (png_bytepp)malloc(size.height() * sizeof(png_bytep));
	png_set_write_fn(mPngParam->png_ptr, mPngParam->filep, png_cb_io_write, png_cb_io_flush);
	png_set_pos_fn(mPngParam->png_ptr, png_cb_io_getpos, png_cb_io_setpos);
	png_set_compression_level(mPngParam->png_ptr, opt_int(CompressLevel));
	png_set_IHDR(mPngParam->png_ptr, mPngParam->info_ptr, size.width(), size.height(), 8, PNG_COLOR_TYPE_RGB, 0, 0, 0);
	png_set_tRNS(mPngParam->png_ptr, mPngParam->info_ptr, 0, 1, &transpColor);
	png_set_acTL(mPngParam->png_ptr, mPngParam->info_ptr, PNG_UINT_31_MAX, 0);
	png_set_filter(mPngParam->png_ptr, 0, PNG_FAST_FILTERS);
	png_write_info(mPngParam->png_ptr, mPngParam->info_ptr);
	return true;
}

void PngEncoder::setDefaultOpt()
{
	opt_SetDefaultVal(ColorType, TrueColor);
	opt_SetDefaultVal(CompressLevel, 6);
}

bool PngEncoder::init(QIODevice *file)
{
    PNGParam_uninit(mPngParam);
	mLastFrame = QImage();
    mPngParam->filep = file;
	mInitPos = mPngParam->filep->pos();
	this->mFrameCount = 0;
	this->mJumpFrameCount = 0;
	this->mFinishSize=0;
	return true;
}
bool PngEncoder::encodeFrame(QImage frame, uint16_t delayTime)
{
    if(frame.format() != QImage::Format_RGB888)
	{
		frame = frame.convertToFormat(QImage::Format_RGB888);
	}
    Q_ASSERT(this->mTimestampRecord == 0);
    if(this->mFrameCount == 0){
        this->setImageSize(frame.size());
    }
	ImageTranspReplace_24bit(frame);
	__addFrameRGBA(frame, delayTime);
	mFrameCount++;
	return true;
}

bool PngEncoder::encodeTimestamp(QImage frame, uint32_t timestamp)
{
    if(frame.format() != QImage::Format_RGB888)
	{
		frame = frame.convertToFormat(QImage::Format_RGB888);
	}
    if(this->mFrameCount == 0){
        this->setImageSize(frame.size());
    }
	ImageTranspReplace_24bit(frame);
	if(frame == mLastFrame){
		mJumpFrameCount++;
		return true;
	}
    if(this->mFrameCount != 0){
		int64_t delay = timestamp - this->mTimestampRecord;
		png_rewrite_delay(mPngParam->png_ptr, delay, 1000);
	}
	__addFrameRGBA(frame, 2333);
	this->mTimestampRecord = timestamp;
	mFrameCount++;
	return true;
}

bool PngEncoder::finish(uint32_t timestamp)
{
	int64_t delay = timestamp - this->mTimestampRecord;
	if (delay > 0) {
		png_rewrite_delay(mPngParam->png_ptr, delay, 1000);
	}
// ========== 重新写帧数信息================
	png_set_acTL(mPngParam->png_ptr, mPngParam->info_ptr, mFrameCount, 0);
	png_rewrite_acTL(mPngParam->png_ptr, mFrameCount, 0);
// ======================================================
	if (setjmp(png_jmpbuf(mPngParam->png_ptr))) {
		Q_ASSERT(0);
        return false;
	}else{
	    png_write_end(mPngParam->png_ptr, mPngParam->info_ptr);
    }
	mFinishSize = this->encodedSize();
	PNGParam_uninit(mPngParam);
	this->mTimestampRecord = 0;
	return true;
}
size_t PngEncoder::encodedSize()
{
	if(mFinishSize){
		return mFinishSize;
	}
    return mPngParam->filep->pos() - mInitPos;
}
bool PngEncoder::__addFrameRGBA(QImage &add_image, const __int64 delay)
{	
	QImage image;
	if(mLastFrame.isNull()){
		image = add_image;
	}else{
		image = ImageSameTransp_24bit(mLastFrame, add_image, 150);
	}
	size_t rowbytes = png_get_rowbytes(mPngParam->png_ptr, mPngParam->info_ptr);
	for (int i = 0; i < image.height(); i++) {
		mPngParam->rows[i] = (png_byte*)image.scanLine(i);
	}
	png_write_frame_head(mPngParam->png_ptr, mPngParam->info_ptr, NULL, image.width(), image.height(), 0, 0,
		delay, 1000, PNG_DISPOSE_OP_NONE,
		PNG_BLEND_OP_OVER);
	png_write_image(mPngParam->png_ptr, mPngParam->rows);
	png_write_frame_tail(mPngParam->png_ptr, mPngParam->info_ptr);
	
	mLastFrame = add_image;
	return true;
}
