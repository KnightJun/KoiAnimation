#pragma once
#include <QImage>
#include "koianimation_global.h"

QImage KOIANIMATION_EXPORT AddProgressBar(const QImage &source, qreal val, qreal maxVal
                    , quint32 barHeight = 6, QColor foreColor = QColor(230, 230, 230), QColor backColor = QColor(30,30,30));