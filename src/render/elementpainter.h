#ifndef XN_ELEMENTPAINTER_H
#define XN_ELEMENTPAINTER_H

class QPainter;

namespace xn {
class Element;
class Page;
class Path;
class Stroke;

namespace ElementPainter {
void drawBackground(QPainter *p, const Page *page);

void drawElement(QPainter *p, const Element *e, bool outlineOnly = false);
void drawStroke(QPainter *p, const Stroke *s, bool outlineOnly = false);
void drawPath(QPainter *p, const Path *path, bool outlineOnly = false);
}
}

#endif
