#ifndef XN_ELEMENT_H
#define XN_ELEMENT_H

#include <QPointF>
#include <QRectF>

namespace xn {
class Element
{
public:
    enum Type { StrokeType, TextType, ImageType, PathType };

    Element(): group(0) {}
    virtual ~Element() {}

    virtual Type type() const = 0;
    virtual QRectF bounds() const = 0;
    virtual Element *clone() const = 0;

    virtual void translate(qreal dx, qreal dy) = 0;

    // Axis aligned so an ImageItem stays a rect; a flip is a scale by -1.
    virtual void scale(const QPointF &origin, qreal sx, qreal sy) = 0;

    int group;
};
}

#endif
