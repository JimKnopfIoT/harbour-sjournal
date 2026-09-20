#include "render/overlaypainter.h"

#include "app/nodeeditor.h"
#include "app/selection.h"
#include "model/element.h"

#include <QPainter>

namespace xn {
static const QColor kAccent(0xff, 0x8a, 0x17);
static const QColor kMarquee(0x1a, 0x72, 0xd0);

void OverlayPainter::drawCurveAnchors(QPainter *p, const QVector<QPointF> &anchors, qreal zoom)
{
    if (anchors.isEmpty())
        return;

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setPen(Qt::NoPen);
    p->setBrush(kAccent);
    for (int i = 0; i < anchors.size(); ++i) {
        const QPointF a = anchors.at(i);
        p->drawEllipse(QPointF(a.x() * zoom, a.y() * zoom), 4, 4);
    }
    p->restore();
}

void OverlayPainter::drawMarquee(QPainter *p, const QRectF &rect, qreal zoom)
{
    if (rect.isEmpty())
        return;

    p->save();
    p->setRenderHint(QPainter::Antialiasing, false);
    const QRectF r(rect.x() * zoom, rect.y() * zoom,
                   rect.width() * zoom, rect.height() * zoom);
    p->fillRect(r, QColor(kMarquee.red(), kMarquee.green(), kMarquee.blue(), 40));
    QPen pen(kMarquee);
    pen.setWidth(2);
    pen.setStyle(Qt::DashLine);
    p->setPen(pen);
    p->setBrush(Qt::NoBrush);
    p->drawRect(r);
    p->restore();
}

void OverlayPainter::drawLasso(QPainter *p, const QVector<QPointF> &path, qreal zoom)
{
    if (path.size() < 2)
        return;

    QPolygonF scaled;
    scaled.reserve(path.size());
    for (int i = 0; i < path.size(); ++i)
        scaled << QPointF(path.at(i).x() * zoom, path.at(i).y() * zoom);

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->setBrush(QColor(kMarquee.red(), kMarquee.green(), kMarquee.blue(), 30));
    QPen pen(kMarquee);
    pen.setWidth(2);
    pen.setStyle(Qt::DashLine);
    p->setPen(pen);
    p->drawPolygon(scaled);
    p->restore();
}

void OverlayPainter::drawSelection(QPainter *p, const Selection *selection, qreal zoom)
{
    const QRectF b = selection->bounds();
    if (b.isNull())
        return;

    p->save();
    p->setRenderHint(QPainter::Antialiasing, false);
    const QRectF r = QRectF(b.x() * zoom, b.y() * zoom, b.width() * zoom, b.height() * zoom)
            .adjusted(-6, -6, 6, 6);

    const QVector<Element *> &items = selection->elements();
    if (items.size() > 1) {
        QPen thin(QColor(kAccent.red(), kAccent.green(), kAccent.blue(), 140));
        thin.setWidth(1);
        p->setPen(thin);
        p->setBrush(Qt::NoBrush);
        for (int i = 0; i < items.size(); ++i) {
            const QRectF eb = items.at(i)->bounds();
            p->drawRect(QRectF(eb.x() * zoom, eb.y() * zoom,
                               eb.width() * zoom, eb.height() * zoom).adjusted(-2, -2, 2, 2));
        }
    }

    QPen pen(kAccent);
    pen.setWidth(2);
    pen.setStyle(Qt::DashLine);
    p->setPen(pen);
    p->setBrush(Qt::NoBrush);
    p->drawRect(r);

    p->setPen(Qt::NoPen);
    p->setBrush(kAccent);
    const qreal h = 5;
    p->drawRect(QRectF(r.left() - h, r.top() - h, 2 * h, 2 * h));
    p->drawRect(QRectF(r.right() - h, r.top() - h, 2 * h, 2 * h));
    p->drawRect(QRectF(r.left() - h, r.bottom() - h, 2 * h, 2 * h));
    p->drawRect(QRectF(r.right() - h, r.bottom() - h, 2 * h, 2 * h));
    p->restore();
}

void OverlayPainter::drawNodes(QPainter *p, const NodeEditor *nodes, qreal zoom)
{
    const QVector<NodeEditor::Node> &list = nodes->nodes();
    if (list.isEmpty())
        return;

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);

    QPointF anchorPos;
    bool haveAnchor = false;
    for (int i = 0; i < list.size() && !haveAnchor; ++i) {
        if (list.at(i).kind == NodeEditor::Anchor && list.at(i).anchor == nodes->activeAnchor()) {
            anchorPos = list.at(i).pos * zoom;
            haveAnchor = true;
        }
    }

    if (haveAnchor) {
        QPen tangent(QColor(kMarquee.red(), kMarquee.green(), kMarquee.blue(), 160));
        tangent.setWidth(1);
        p->setPen(tangent);
        for (int i = 0; i < list.size(); ++i) {
            if (list.at(i).kind != NodeEditor::Anchor)
                p->drawLine(anchorPos, list.at(i).pos * zoom);
        }
    }

    QPen border(kMarquee);
    border.setWidth(2);
    p->setPen(border);
    for (int i = 0; i < list.size(); ++i) {
        const NodeEditor::Node &n = list.at(i);
        const QPointF at = n.pos * zoom;
        if (n.kind == NodeEditor::Anchor) {
            p->setBrush(n.anchor == nodes->activeAnchor() ? QBrush(kMarquee) : QBrush(Qt::white));
            p->drawRect(QRectF(at.x() - 4, at.y() - 4, 8, 8));
        } else {
            p->setBrush(Qt::white);
            p->drawEllipse(at, 4, 4);
        }
    }

    p->restore();
}
}
