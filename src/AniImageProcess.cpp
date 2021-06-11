#include <QPainter>
#include <QImage>
#include <AniImageProcess.h>
QImage AddProgressBar(const QImage &source, qreal val, qreal maxVal
                    , quint32 barHeight, QColor foreColor, QColor backColor)
{
    QImage dst(source.width(), source.height() + barHeight, source.format());
    QPainter p(&dst);
    p.drawImage(0, 0, source);
    QRect dRect(0, source.height(), source.width() * (val / maxVal), barHeight);
    p.fillRect(dRect, QBrush(foreColor));
    dRect.setRect(dRect.width(), source.height(), source.width() - dRect.width(), barHeight);
    p.fillRect(dRect, QBrush(backColor));
    return dst;
}