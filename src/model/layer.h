#ifndef XN_LAYER_H
#define XN_LAYER_H

#include "model/element.h"

#include <QList>
#include <QString>

namespace xn {
class Layer
{
public:
    Layer();
    ~Layer();

    void append(Element *e);
    void insert(int index, Element *e);

    Element *take(int index);
    int indexOf(Element *e) const;

    void clear();
    bool isEmpty() const { return elements.isEmpty(); }
    int count() const { return elements.size(); }

    Layer *clone() const;

    QString name;
    bool visible;
    bool locked;
    qreal opacity;
    QList<Element *> elements;

private:
    Layer(const Layer &);
    Layer &operator=(const Layer &);
};
}

#endif
