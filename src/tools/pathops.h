#ifndef XN_PATHOPS_H
#define XN_PATHOPS_H

#include <QPainterPath>
#include <QVector>

namespace xn {
class Element;
class Path;
class TextItem;

// Elements to QPainterPath and back; Qt's united() and subtracted() do the rest.
class PathOps
{
public:
    static QPainterPath toPainterPath(const Element *e);
    // One shape with its holes, the way a QPainterPath means it.
    static Path *fromPainterPath(const QPainterPath &path, const Element *style);
    // One element per contour, for breaking a shape apart.
    static QVector<Path *> splitSubpaths(const QPainterPath &path, const Element *style);

    static QVector<Path *> unite(const QVector<Element *> &elements);
    static QVector<Path *> subtract(const QVector<Element *> &elements);
    static QVector<Path *> breakApart(const Element *element);
    static QVector<Path *> textToPath(const TextItem *text);

    static Path *join(const QVector<Element *> &elements);

    static bool isConvertible(const Element *e);
};
}

#endif
