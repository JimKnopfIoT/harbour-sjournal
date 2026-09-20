#include "model/layer.h"

namespace xn {
Layer::Layer()
    : visible(true)
    , locked(false)
    , opacity(1.0)
{
}

Layer::~Layer()
{
    clear();
}

void Layer::append(Element *e)
{
    if (e)
        elements.append(e);
}

void Layer::insert(int index, Element *e)
{
    if (!e)
        return;
    if (index < 0 || index > elements.size())
        index = elements.size();
    elements.insert(index, e);
}

Element *Layer::take(int index)
{
    if (index < 0 || index >= elements.size())
        return 0;
    return elements.takeAt(index);
}

int Layer::indexOf(Element *e) const
{
    return elements.indexOf(e);
}

void Layer::clear()
{
    qDeleteAll(elements);
    elements.clear();
}

Layer *Layer::clone() const
{
    Layer *l = new Layer;
    l->name = name;
    l->visible = visible;
    l->locked = locked;
    l->opacity = opacity;
    for (int i = 0; i < elements.size(); ++i)
        l->elements.append(elements.at(i)->clone());
    return l;
}
}
