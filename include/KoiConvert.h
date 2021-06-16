#include "KoiAnimation.h"
#include "KoiMovie.h"
#include "koianimation_global.h"

class KOIANIMATION_EXPORT KoiConvert : public QObject
{
    Q_OBJECT
public:
    KoiConvert(QString srcFilename, QString dstFilename, 
    KoiAnimation* animation = nullptr, QObject *parent = nullptr);
    KoiConvert(QString srcFilename, KoiFormat dstFormat, 
    KoiAnimation* animation = nullptr, QObject *parent = nullptr);
    ~KoiConvert();
    void addProgressBar(bool enable);
    void setDuration(qint64 duration);
    void encodeNextFrame(bool sync = false);
    bool isFinish(){return mFinishFlag;};
    int frameCount();
    qreal progress();
signals:
    void sigFrameConverted();
    void sigConverted();
private:
    void init(QString srcFilename, QString dstFilename, 
    KoiAnimation* animation = nullptr);
    void encodeNextFrameProgressBar(bool sync = false);
    void encodeNextFrameNoProgressBar(bool sync = false);
    bool mProgressBar = false;
    bool mNeedDeleteAnimation = true;
    KoiAnimation *mAnimation = nullptr;
    KoiMovie     *mMoive = nullptr;
    qint64 mDuration = -1;
    bool mStopFlag = false;
    bool mFinishFlag = false;
    bool mIsLastFrame = false;
    bool mInsertFrame = false;
    qint64 mTimestamp = -1;
};
