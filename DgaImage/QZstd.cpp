
#include "QZstd.h"
#include <QDebug>
#define CHECK_ZSTD(fn, ...)                                      \
    do {                                                         \
        size_t const err = (fn);                                 \
        CHECK(!ZSTD_isError(err), "%s", ZSTD_getErrorName(err)); \
    } while (0)
#define CHECK(cond, ...)                        \
    do {                                        \
        if (!(cond)) {                          \
            fprintf(stderr,                     \
                    "%s:%d CHECK(%s) failed: ", \
                    __FILE__,                   \
                    __LINE__,                   \
                    #cond);                     \
            fprintf(stderr, "" __VA_ARGS__);    \
            fprintf(stderr, "\n");              \
            exit(1);                            \
        }                                       \
    } while (0)

QZstdCompress::QZstdCompress(QIODevice *dev)
{
    mDev = dev;
    mCctx = ZSTD_createCCtx();
    mInSize = ZSTD_CStreamInSize();
    mOutSize = ZSTD_CStreamOutSize();
    mBuffIn = new quint8[mInSize];
    mBuffOut = new quint8[mOutSize];
}

void QZstdCompress::writeTrunk(bool finish)
{
    ZSTD_inBuffer input = {mBuffIn, mInPos, 0 };
    ZSTD_EndDirective const mode = finish ? ZSTD_e_end : ZSTD_e_continue;
    int finished;
    do {
        ZSTD_outBuffer output = {mBuffOut, mOutSize, 0 };
        size_t const remaining = ZSTD_compressStream2(mCctx, &output , &input, mode);
        CHECK_ZSTD(remaining);
        mDev->write((const char *)mBuffOut, output.pos);
        finished = finish ? (remaining == 0) : (input.pos == input.size);
    } while (!finished);
}

size_t QZstdCompress::write(unsigned char* data, size_t len, bool finish)
{
    while(mInPos + len >= mInSize){
        int cSize = mInSize - mInPos;
        memcpy(mBuffIn + mInPos, data, cSize);
        mInPos = mInSize;
        writeTrunk();
        data += cSize;
        len -= cSize;
        mInPos = 0;
    }
    if(len){
        memcpy(mBuffIn + mInPos, data, len);
        mInPos += len;
    }
    if(finish){
        writeTrunk(finish);
        mInPos = 0;
    }
    return len;
}

QZstdCompress::~QZstdCompress()
{
    ZSTD_freeCCtx(mCctx);
    delete[] mBuffIn;
    delete[] mBuffOut;
}


QZstdDecompress::QZstdDecompress(QIODevice *dev)
{
    mDev = dev;
    mDctx = ZSTD_createDCtx();
    mInSize = ZSTD_DStreamInSize();
    mOutSize = ZSTD_DStreamOutSize();
    mBuffIn = new quint8[mInSize];
    mBuffOut = new quint8[mOutSize];
    mCacheBuffer.setBuffer(&mCache);
    mCacheBuffer.open(QIODevice::ReadWrite);
}

QZstdDecompress::~QZstdDecompress()
{
    ZSTD_freeDCtx(mDctx);
    delete[] mBuffIn;
    delete[] mBuffOut;
}

size_t QZstdDecompress::read(char* data, size_t len)
{
    int readLen = mCacheBuffer.read((char *)data, len);
    if(readLen == len){
        return readLen;
    }
    int remainLen = len - readLen;
    while (readLen < len)
    {
        if(deBuffFinish)return readLen;
        readTrunk();
        readLen += mCacheBuffer.read((char *)(data + readLen), remainLen);
        remainLen = len - readLen;
    }
    return readLen;
}
void QZstdDecompress::reset()
{
    ZSTD_DCtx_reset(mDctx, ZSTD_reset_session_and_parameters);
    mCacheBuffer.seek(0);
    mCache.clear();
    deBuffFinish = false;
}
QByteArray QZstdDecompress::read(size_t len)
{
    QByteArray ret(len, Qt::Uninitialized);
    size_t readLen = this->read(ret.data(), len);
    if(readLen != len)
    {
        ret.resize(readLen);
    }
    return ret;
}

/* fill mBuffOut(mBuffOut must be empty) */
size_t QZstdDecompress::readTrunk()
{
    mCacheBuffer.seek(0);
    mCache.clear();
    int readSize = mDev->read((char*)mBuffIn, mInSize);
    if(readSize < mInSize){
        deBuffFinish = true;
    };
    ZSTD_inBuffer input = {mBuffIn, readSize, 0 };
    while (input.pos < input.size) {
        ZSTD_outBuffer output = { mBuffOut, mOutSize, 0 };
        size_t const ret = ZSTD_decompressStream(mDctx, &output , &input);
        CHECK_ZSTD(ret);
        mCacheBuffer.write((char*)mBuffOut, output.pos);
    }
    mCacheBuffer.seek(0);
    return mCacheBuffer.pos();
}