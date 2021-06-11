#include <zstd.h>
#include <QIODevice>
#include <QBuffer>
class QZstdCompress
{
private:
    /* data */
    QIODevice *mDev = nullptr;
    size_t  	mOutSize;
    quint8* 	mBuffOut;

    size_t  	mInSize;
    size_t  	mInPos = 0;
    quint8* 	mBuffIn;
    ZSTD_CCtx* mCctx = nullptr;
public:
    QZstdCompress(QIODevice *dev);
    size_t write(unsigned char* data, size_t len, bool finish = false);
    ~QZstdCompress();
private:
    void writeTrunk(bool finish = false);
};

class QZstdDecompress
{
private:
    /* data */
    QIODevice *mDev = nullptr;
    size_t  	mOutSize;
    quint8* 	mBuffOut;

    size_t  	mInSize;
    quint8* 	mBuffIn;
    ZSTD_DCtx* mDctx = nullptr;
public:
    QZstdDecompress(QIODevice *dev);
    void reset();
    size_t read(char* data, size_t len);
    QByteArray read(size_t len);
    ~QZstdDecompress();
private:
    size_t readTrunk();
    bool deBuffFinish = false;
    QByteArray mCache;
    QBuffer mCacheBuffer;
};


