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
// TEST(NormalEncode, GifEncode) {
//     testKoiPerform(KoiFormat::GIF, "../test.webp", "result.gif");
// }

// TEST(NormalEncode, DgaEncode) {
//     testKoiPerform(KoiFormat::Dga, "test.gif", "result.Dga");
// }

// TEST(NormalEncode, DgaDecode1) {
//     testKoiPerform(KoiFormat::APNG, "result.Dga", "result_decode.png");
// }

TEST(Gif, timestamp) {
    KoiMovie mov;
    mov.setFileName("test.dga");
    int src_dur = mov.duration();
    KoiAnimation koi;
    koi.init("test2.gif", KoiFormat::GIF);
    int timestamp = 0;
    for (size_t i = 0; i < mov.frameCount(); i++)
    {
        koi.addFrameTimestamp(mov.currentImage(), timestamp);
        timestamp += mov.nextFrameDelay();
        mov.jumpToNextFrame();
        koi.waitForIdle();
    }
    koi.finish(timestamp);
    koi.waitForIdle();
    
    int enc_dur = koi.timeStamp();
    KoiMovie mov2;
    mov2.setFileName("test2.gif");
    int dec_dur = mov2.duration();
    // EXPECT_EQ(src_dur, enc_dur);
    EXPECT_EQ(enc_dur, dec_dur);
}

TEST(Dga, timestamp) {
    KoiMovie mov;
    mov.setFileName("test.gif");
    int src_dur = mov.duration();
    KoiAnimation koi;
    koi.init("test2.gif", KoiFormat::GIF);
    int timestamp = 0;
    for (size_t i = 0; i < mov.frameCount(); i++)
    {
        koi.addFrameTimestamp(mov.currentImage(), timestamp);
        timestamp += mov.nextFrameDelay();
        mov.jumpToNextFrame();
        koi.waitForIdle();
    }
    koi.finish(timestamp);
    koi.waitForIdle();
    
    int enc_dur = koi.timeStamp();
    KoiMovie mov2;
    mov2.setFileName("test2.gif");
    int dec_dur = mov2.duration();
    EXPECT_EQ(src_dur, enc_dur);
    EXPECT_EQ(src_dur, dec_dur);
}

TEST(KoiMovie, LoopTest) {
    KoiMovie mov;
    mov.setFileName("result.Dga");
    EXPECT_GT(mov.frameCount(), 1);
    for (size_t i = 0; i < mov.frameCount(); i++)
    {
        mov.jumpToNextFrame();
    }
    mov.jumpToNextFrame();
    EXPECT_EQ(0, mov.currentFrameNumber());
}

TEST(KoiConvert, DgaWebpTest) {
    KoiConvert cov("result.Dga", KoiFormat::WEBP);
    cov.addProgressBar(true);
    while (!cov.isFinish())
    {
        cov.encodeNextFrame(true);
    }
}

// TEST(NormalEncode, DgaDecode2) {
//     QFile DgaFile("result.Dga");
//     DgaFile.open(QFile::ReadOnly);
    
//     KoiAnimation koi;
//     DgaIOHandler DgaIO;
//     QImage curImage;
//     DgaIO.setDevice(&DgaFile);
//     DgaIO.canRead();
//     qDebug() << "jumpToImage(0)";
//     DgaIO.jumpToImage(0);
//     int Timestamp = 0;
//     qDebug() << "koi.init";
//     koi.init("Dga_decoder.png", KoiFormat::APNG);
//     do{
//     qDebug() << "DgaIO.read(&curImage)";
//         DgaIO.read(&curImage);
//     qDebug() << "__DgaIO.read(&curImage)";
//         koi.addFrameTimestamp(curImage, Timestamp);
//         koi.waitForIdle();
//         Timestamp += DgaIO.nextImageDelay();
//         // DgaIO.jumpToNextImage();
//     }while(DgaIO.currentImageNumber() != 0);
//     koi.finish(Timestamp);
//     koi.waitForIdle();
// }
// /*
//  * 正常测试
//  * */
// TEST(NormalEncode, PngEncode) {
//     testKoiPerform(KoiFormat::APNG, "../test.webp", "result.png");
// }

// TEST(NormalEncode, WebpEncodeTimestamp) {
//     testKoiPerform(KoiFormat::WEBP, "../test.webp", "result.webp");
// }

int main(int argc, char *argv[])
{   
    QGuiApplication a(argc, argv);
    QSettings *setting = new QSettings("KoiAnimation.ini");
    KoiAnimation::setOptionSetting(setting);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}