
#include "koianimation_global.h"
#include <QImage>
#include <QPainter>
void KOIANIMATION_EXPORT ImageDiff_32bit(QImage img1, QImage img2, size_t reduct_factor);
QImage KOIANIMATION_EXPORT ImageSameTransp_32bit(const QImage &img1, const QImage &img2);

void KOIANIMATION_EXPORT ImageTranspReplace_24bit(QImage &img1);
QImage KOIANIMATION_EXPORT ImageSameTransp_24bit(const QImage &img1, QImage &img2, uint8_t transpColor);