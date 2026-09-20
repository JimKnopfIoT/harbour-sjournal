#ifndef XN_PAGE_H
#define XN_PAGE_H

#include "model/layer.h"

#include <QColor>
#include <QList>
#include <QSizeF>
#include <QString>

namespace xn {
struct Background
{
    enum Type { Solid, Pixmap, Pdf };

    Background(): type(Solid), color(Qt::white), style(QStringLiteral("plain")), pageNo(0) {}

    Type type;
    QColor color;
    QString style;
    QString filename;
    QString domain;
    int pageNo;
    QString config;
};

class Page
{
public:
    static QSizeF a4();

    explicit Page(const QSizeF &size = a4());
    ~Page();

    Layer *layerAt(int index);
    Layer *ensureLayer(int index);
    int layerCount() const { return layers.size(); }

    Layer *addLayer(const QString &name = QString());
    void insertLayer(int index, Layer *l);
    Layer *takeLayer(int index);

    QRectF rect() const { return QRectF(0, 0, width, height); }

    Page *clone() const;

    qreal width;
    qreal height;
    Background background;
    QList<Layer *> layers;

private:
    Page(const Page &);
    Page &operator=(const Page &);
};
}

#endif
