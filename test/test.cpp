#include <QImage>
#include <KoiAnimation.h>
#include <QDebug>
#include <KoiMovie.h>
#include <QGuiApplication>
#include <gtest/gtest.h>
#include <AniImageProcess.h>
#include <../DgaImage/DgaIOHandler.h>
#include <QSettings>
#include <ImageDiff.h>
#include <KoiConvert.h>
#include <QDateTime>
#include <QThread>
struct FrameInfo{
    QImage image;
    int delay;
};
void testKoiPerform(KoiFormat format, QString source, QString dest)
{
    KoiMovie mov;
    mov.setFileName(source);
    KoiAnimation koi;
    koi.init(dest, format);
    mov.jumpToFrame(0);
    QList<FrameInfo> frames;
    EXPECT_GT(mov.frameCount(), 0);
    int dur = 0;
    qint64 timec = QDateTime::currentMSecsSinceEpoch();
    for (size_t i = 0; i < mov.frameCount(); i++)
    {
        FrameInfo tmp = {mov.currentImage(), mov.nextFrameDelay()};
        frames.append(tmp);
        dur += mov.nextFrameDelay();
        mov.jumpToNextFrame();
    }
    timec = QDateTime::currentMSecsSinceEpoch() - timec;
    qDebug() << "encoding....";
    koi.enablePerformStatistic(true);
    timec = QDateTime::currentMSecsSinceEpoch();
    int Timestamp = 0;
    QImage img;
    for (const auto & frame : frames)
    {
        // img = AddProgressBar(frame.image, Timestamp, dur);
        koi.addFrameTimestamp(frame.image, Timestamp);
        Timestamp += frame.delay;
        koi.waitForIdle();
    }
    koi.finish(Timestamp);
    koi.waitForIdle();
    timec = QDateTime::currentMSecsSinceEpoch() - timec;
    float size_kb = koi.encodedSize() / 1024.0;
    EXPECT_GT(size_kb, 100);
    qDebug() << koi.performStatistic();
    qDebug() << "Encode time "  << timec << "size: " << size_kb << "kb frame : " << frames.size() << " fps: " << frames.size() / ((float)timec / 1000);
}

/*
 * 正常测试
 * */

// TEST(NormalEncode, DgaEncode) {
//     testKoiPerform(KoiFormat::Dga, "test.gif", "result.Dga");
// }

// TEST(NormalEncode, DgaDecode1) {
//     testKoiPerform(KoiFormat::APNG, "result.Dga", "result_decode.png");
// }

TEST(Gif, encode) {
    KoiAnimation koi;
    size_t i;
    koi.init("test.gif", KoiFormat::GIF);
    QImage img = QImage(640, 480, QImage::Format_ARGB32);
    for (i = 0; i < 250; i+=10)
    {
        for (size_t x = 0; x < img.width(); x++)
        {
            for (size_t y = 0; y < img.height(); y++)
            {
                img.setPixelColor(x, y, QColor(qRgb(i,i,i)));
            }
        }
        koi.addFrameTimestamp(img, i * 10);
    }
    koi.finish(i*10);
}

TEST(Gif, decode) {
    KoiMovie kom;
    kom.setFileName("test.gif");
    EXPECT_EQ(kom.frameCount(), 25);
    kom.jumpToFrame(0);
    for (size_t j = 0; j < 2; j++)
    {
        for (size_t i = 0; i < 250; i+=10)
        {
            QImage img = kom.currentImage();
            EXPECT_EQ(img.width(), 640);
            EXPECT_EQ(img.height(), 480);
            EXPECT_EQ(kom.timeStamp() , i*10); 
            kom.jumpToNextFrame();
        }
    }
    kom.jumpToTimeStamp(50 * 10);
    EXPECT_EQ(kom.timeStamp() , 50 * 10); 
    kom.jumpToTimeStamp(10 * 10);
    EXPECT_EQ(kom.timeStamp() , 10 * 10); 

}

TEST(Convert, GifToWebp) {
    KoiConvert koc("test.gif", "test.webp");
    while (!koc.isFinish())
    {
        koc.encodeNextFrame(true);
    }
}

int main(int argc, char *argv[])
{   
    QGuiApplication a(argc, argv);
    QSettings *setting = new QSettings("KoiAnimation.ini");
    KoiAnimation::setOptionSetting(setting);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}