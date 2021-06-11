#include <ImageDiff.h>
#include <QImage>
#include <QDebug>
#include <QPainter>
QRgb genNextColor(QRgb c)
{
    c = (c >> (32 - 8)) | (c << 8);
    return c;
}
struct RGB24
{
    uint8_t R;
    uint8_t G;
    uint8_t B;
};

/* 图像大小必须为4/8的倍数 */
void ImageDiff_32bit(QImage img1, QImage img2, size_t reduct_factor)
{
    QVector<QRect> diffRects(0);
    QImage rredu_img1 = img1.scaled(img1.size() / reduct_factor, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QImage rredu_img2 = img2.scaled(img2.size() / reduct_factor, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    uint32_t *diffres = new uint32_t[rredu_img1.width() * rredu_img1.height()];
    /* 计算差异部分 */
    for (size_t y = 0; y < rredu_img1.height(); y++)
    {
        uint32_t *src_row1 = (uint32_t *)rredu_img1.scanLine(y);
        uint32_t *src_row2 = (uint32_t *)rredu_img2.scanLine(y);
        uint32_t *dst_row = diffres + rredu_img1.width() * y;
        for (size_t x = 0; x < rredu_img1.width(); x++)
        {
            dst_row[x] = (src_row1[x] ^ src_row2[x]);
        }
    }
    QPainter painter(&img2);
    QRgb color = qRgb(255,0,0);
    /* 渲染差异结果 */
        qDebug() << "diffRects" << diffRects.size();
    for (size_t y = 0; y < rredu_img1.height(); y++)
    {
        QRect diffRect;
        diffRect.setRect(-1, y*reduct_factor, 0, reduct_factor);
        uint32_t *dst_row = diffres + rredu_img1.width() * y;
        for (size_t x = 0; x < rredu_img1.width(); x++)
        {
            if(dst_row[x]){
                //diffRects.append(QRect(x * reduct_factor, y*reduct_factor, reduct_factor, reduct_factor));
                if(diffRect.x() < 0){
                    diffRect.setX(x * reduct_factor);
                    diffRect.setWidth(0);
                }
                diffRect.setWidth(diffRect.width() + reduct_factor);
            } else {
                if(diffRect.x() >= 0){
                    diffRects.append(diffRect);
                    diffRect.setX(-1);
                    diffRect.setWidth(0);
                }
            }
        }
        if(diffRect.x() >= 0){
            diffRects.append(diffRect);
        }
    }
    qDebug() << "diffRects" << diffRects.size();
    for (const QRect &t : diffRects)
    {
        painter.fillRect(t, color);
        color = genNextColor(color);
    }
    painter.end();
    img2.save("diffres.png");
}


void ImageTranRect_32bit(QImage &img2, QRect rect, uint8_t channels, uint8_t transVal)
{
    for (size_t y = rect.top(); y < rect.bottom(); y++)
    {
        uint8_t *rowPtr = (uint8_t *)img2.scanLine(y);
        memset(rowPtr + channels * rect.left(), 0, channels * rect.width());
    }
}

void ImageTranspReplace_24bit(QImage &img1)
{
    RGB24 transp_avoid = {1, 1, 1};
    for (size_t y = 0; y < img1.height(); y++)
    {
        RGB24 *src_row1 = (RGB24 *)img1.scanLine(y);
        for (size_t x = 0; x < img1.width(); x++)
        {
            if(src_row1[x].R == 0 &&
            src_row1[x].G == 0 &&
            src_row1[x].B == 0){
                src_row1[x] = transp_avoid;
            }
        }
    }
    return;
}

QImage ImageSameTransp_24bit(const QImage &img1, QImage &img2, uint8_t transpColor)
{
    QImage newImg = img2.copy();
    RGB24 transp = {0, 0, 0};
    for (size_t y = 0; y < img1.height(); y++)
    {
        RGB24 *src_row1 = (RGB24 *)img1.scanLine(y);
        RGB24 *src_row2 = (RGB24 *)img2.scanLine(y);
        RGB24 *dst_row = (RGB24 *)newImg.scanLine(y);
        for (size_t x = 0; x < img1.width(); x++)
        {
            if(src_row1[x].R == src_row2[x].R &&
                src_row1[x].G == src_row2[x].G &&
                src_row1[x].B == src_row2[x].B){
                dst_row[x] = transp;
            }
        }
    }
    return newImg;
}

/* 图像大小必须为4/8的倍数 */
QImage ImageSameTransp_32bit(const QImage &img1, const QImage &img2)
{
    QImage newImg(img1.width(), img1.height(), img1.format());
    newImg.fill(0);
    for (size_t y = 0; y < img1.height(); y++)
    {
        uint32_t *src_row1 = (uint32_t *)img1.scanLine(y);
        uint32_t *src_row2 = (uint32_t *)img2.scanLine(y);
        uint32_t *dst_row = (uint32_t *)newImg.scanLine(y);
        for (size_t x = 0; x < img1.width(); x++)
        {
            if(src_row1[x] != src_row2[x]){
                dst_row[x] = src_row2[x];
            }
        }
    }
    return newImg;
}