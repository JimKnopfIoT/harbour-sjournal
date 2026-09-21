#include "model/page.h"

#include <QCoreApplication>

namespace xn {
QSizeF Page::a4()
{
    return QSizeF(595.275590551, 841.88976378);
}

Page::Page(const QSizeF &size)
    : width(size.width())
    , height(size.height())
{
    addLayer();
}

Page::~Page()
{
    qDeleteAll(layers);
    layers.clear();
}

Layer *Page::layerAt(int index)
{
    if (index < 0 || index >= layers.size())
        return 0;
    return layers.at(index);
}

Layer *Page::ensureLayer(int index)
{
    while (layers.size() <= index)
        addLayer();
    return layers.at(qMax(0, index));
}

Layer *Page::addLayer(const QString &name)
{
    Layer *l = new Layer;
    l->name = name.isEmpty()
            ? QCoreApplication::translate("Page", "Layer %1").arg(layers.size() + 1)
            : name;
    layers.append(l);
    return l;
}

void Page::insertLayer(int index, Layer *l)
{
    if (!l)
        return;
    if (index < 0 || index > layers.size())
        index = layers.size();
    layers.insert(index, l);
}

Layer *Page::takeLayer(int index)
{
    if (index < 0 || index >= layers.size())
        return 0;
    return layers.takeAt(index);
}

Page *Page::clone() const
{
    Page *p = new Page(QSizeF(width, height));
    p->background = background;
    qDeleteAll(p->layers);
    p->layers.clear();
    for (int i = 0; i < layers.size(); ++i)
        p->layers.append(layers.at(i)->clone());
    return p;
}
}
