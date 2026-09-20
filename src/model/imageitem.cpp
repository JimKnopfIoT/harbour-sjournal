#include "model/imageitem.h"

#include <qmath.h>

namespace xn {
ImageItem::ImageItem()
{
}

Element *ImageItem::clone() const
{
    return new ImageItem(*this);
}

void ImageItem::scale(const QPointF &origin, qreal sx, qreal sy)
{
    const qreal x1 = origin.x() + (rect.left() - origin.x()) * sx;
    const qreal x2 = origin.x() + (rect.right() - origin.x()) * sx;
    const qreal y1 = origin.y() + (rect.top() - origin.y()) * sy;
    const qreal y2 = origin.y() + (rect.bottom() - origin.y()) * sy;
    rect = QRectF(QPointF(qMin(x1, x2), qMin(y1, y2)), QPointF(qMax(x1, x2), qMax(y1, y2)));
    if (!image.isNull() && (sx < 0 || sy < 0))
        image = image.mirrored(sx < 0, sy < 0);
}

void ImageItem::fitInto(const QRectF &area)
{
    if (image.isNull() || image.height() == 0 || area.isEmpty())
        return;

    const qreal aspect = qreal(image.width()) / qreal(image.height());
    qreal w = area.width();
    qreal h = w / aspect;
    if (h > area.height()) {
        h = area.height();
        w = h * aspect;
    }
    rect = QRectF(area.x() + (area.width() - w) / 2,
                  area.y() + (area.height() - h) / 2, w, h);
}
}
