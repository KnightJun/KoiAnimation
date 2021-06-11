#include <QImageIOHandler>
struct DgaHeader;
struct DgaFrame;
class QZstdDecompress;
class DgaIOHandler:public QImageIOHandler
{
public:
    DgaIOHandler();
    ~DgaIOHandler();
    virtual bool canRead() const;
    virtual int	imageCount() const;
    virtual int	currentImageNumber() const;
	QVariant option(ImageOption option) const final;
	bool supportsOption(ImageOption option) const final;
    bool jumpToImage(int imageNumber);
    int loopCount() const final{return -1;};
    bool jumpToNextImage() final;
    virtual int	nextImageDelay() const;
    virtual bool read(QImage *image);
    static bool canRead(QIODevice *dev);
private:
    static void initCheck(DgaIOHandler* iohandler);
    DgaHeader* mDgaHeader = nullptr;
    DgaFrame* mDgaFrame = nullptr;
    DgaFrame* mNextDgaFrame = nullptr;
    QByteArray mFrameBuff;
    int mImgIndex = 0;
    int mImgIndexOnfile = 0;
    bool mEnd = false;
    QZstdDecompress *mDecompress = nullptr;
    QIODevice *mDev = nullptr;
};
