#ifndef XN_IMAGEITEM_H
#define XN_IMAGEITEM_H

#include "model/element.h"

#include <QImage>

namespace xn {
class ImageItem : public Element
{
public:
    ImageItem();

    Type type() const { return ImageType; }
    QRectF bounds() const { return rect; }
    Element *clone() const;
    void translate(qreal dx, qreal dy) { rect.translate(dx, dy); }
    void scale(const QPointF &origin, qreal sx, qreal sy);

    void fitInto(const QRectF &area);

    QImage image;
    QRectF rect;
};
}

#endif
