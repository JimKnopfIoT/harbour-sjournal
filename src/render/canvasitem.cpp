#include "render/canvasitem.h"

#include "app/nodeeditor.h"
#include "app/selectiontransform.h"
#include "render/overlaypainter.h"

#include <QPolygonF>

#include "model/page.h"
#include "app/selection.h"
#include "app/toolsettings.h"
#include "render/elementpainter.h"
#include "tools/curvefitter.h"
#include "tools/shapefactory.h"

#include <QDateTime>
#include <QMouseEvent>
#include <QLineF>
#include <QPainter>
#include <QTouchEvent>

namespace xn {
static const int kLongPressMs = 500;

static const qreal kLongPressSlackPx = 12.0;

static const qreal kPenSpread = 0.55;

static const qreal kBrushSpread = 0.85;

static const qreal kBrushWidthFactor = 2.2;

CanvasItem::CanvasItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , m_controller(0)
    , m_pageIndex(0)
    , m_zoom(1.0)
    , m_drawingEnabled(true)
    , m_eraserRadius(6.0)
    , m_palmThreshold(0)
    , m_pressureAvailable(false)
    , m_erasing(false)
    , m_shapeStroke(0)
    , m_curveCursorValid(false)
    , m_marqueeActive(false)
    , m_lassoActive(false)
    , m_movingSelection(false)
    , m_longPressHandled(false)
    , m_draggingNode(false)
    , m_scalingSelection(false)
    , m_cacheValid(false)
{
    setFlag(ItemHasContents, true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setOpaquePainting(true);
    setAntialiasing(true);
}

CanvasItem::~CanvasItem()
{
    delete m_shapeStroke;
}

void CanvasItem::applyStyle(Stroke *s) const
{
    if (!s || !m_controller)
        return;

    s->color = m_controller->tools()->color();
    s->width = m_controller->tools()->penWidth();
    s->cap = Stroke::RoundCap;
    s->lineStyle = m_controller->tools()->lineStyle();
    s->tool = m_controller->tools()->tool() == ToolSettings::HighlighterTool
            ? Stroke::Highlighter : Stroke::Pen;

    if (m_controller->tools()->fillEnabled()) {
        s->fill = m_controller->tools()->fillAlpha();
        s->fillColor = m_controller->tools()->fillColor();
    } else {
        s->fill = -1;
        s->fillColor = QColor();
    }
}

void CanvasItem::applyStyle(Path *p) const
{
    if (!p || !m_controller)
        return;

    p->color = m_controller->tools()->color();
    p->width = m_controller->tools()->penWidth();
    p->lineStyle = m_controller->tools()->lineStyle();

    if (m_controller->tools()->fillEnabled()) {
        p->fill = m_controller->tools()->fillAlpha();
        p->fillColor = m_controller->tools()->fillColor();
    } else {
        p->fill = -1;
        p->fillColor = QColor();
    }
}

void CanvasItem::setController(DocumentController *c)
{
    if (m_controller == c)
        return;

    if (m_controller)
        m_controller->disconnect(this);

    m_controller = c;

    if (m_controller) {
        connect(m_controller, SIGNAL(pageContentChanged(int)), this, SLOT(onPageContentChanged(int)));
        connect(m_controller, SIGNAL(documentReplaced()), this, SLOT(onDocumentReplaced()));
        connect(m_controller, SIGNAL(layersChanged()), this, SLOT(onDocumentReplaced()));
        connect(m_controller->selection(), SIGNAL(changed()), this, SLOT(update()));
        connect(m_controller->tools(), SIGNAL(outlineViewChanged()),
                this, SLOT(onDocumentReplaced()));
        connect(m_controller->tools(), SIGNAL(toolChanged()), this, SLOT(onToolChanged()));
        connect(m_controller->tools(), SIGNAL(shapeKindChanged()), this, SLOT(onToolChanged()));
    }

    invalidateCache();
    Q_EMIT controllerChanged();
}

void CanvasItem::setPageIndex(int index)
{
    if (m_pageIndex == index)
        return;
    discardCurve();
    m_pageIndex = index;
    invalidateCache();
    Q_EMIT pageIndexChanged();
}

void CanvasItem::setZoom(qreal z)
{
    const qreal clamped = qBound(qreal(0.05), z, qreal(20.0));
    if (qFuzzyCompare(m_zoom, clamped))
        return;
    m_zoom = clamped;
    invalidateCache();
    Q_EMIT zoomChanged();
}

void CanvasItem::setDrawingEnabled(bool on)
{
    if (m_drawingEnabled == on)
        return;
    if (!on)
        inputCancel();
    m_drawingEnabled = on;
    Q_EMIT drawingEnabledChanged();
}

void CanvasItem::setEraserRadius(qreal r)
{
    if (qFuzzyCompare(m_eraserRadius, r))
        return;
    m_eraserRadius = r;
    Q_EMIT eraserRadiusChanged();
}

void CanvasItem::setPalmThreshold(qreal t)
{
    if (qFuzzyCompare(m_palmThreshold, t))
        return;
    m_palmThreshold = t;
    Q_EMIT palmThresholdChanged();
}

void CanvasItem::refresh()
{
    invalidateCache();
}

void CanvasItem::invalidateCache()
{
    m_cacheValid = false;
    update();
}

void CanvasItem::onPageContentChanged(int pageIndex)
{
    if (pageIndex == m_pageIndex)
        invalidateCache();
}

void CanvasItem::onDocumentReplaced()
{
    discardCurve();
    invalidateCache();
}

void CanvasItem::onToolChanged()
{
    commitCurve(false);
}

void CanvasItem::timerEvent(QTimerEvent *event)
{
    if (event->timerId() != m_longPressTimer.timerId()) {
        QQuickPaintedItem::timerEvent(event);
        return;
    }

    m_longPressTimer.stop();
    if (!m_controller)
        return;

    if (m_movingSelection) {
        m_controller->selection()->endMove();
        m_movingSelection = false;
    }

    m_longPressHandled = true;
    m_controller->selection()->toggleAt(m_pageIndex, m_pressPagePos, snapRadius() * 0.5);
    update();
}

void CanvasItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChanged(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size())
        invalidateCache();
}

void CanvasItem::rebuildCache()
{
    const int w = qMax(1, qRound(width()));
    const int h = qMax(1, qRound(height()));

    if (m_cache.width() != w || m_cache.height() != h)
        m_cache = QImage(w, h, QImage::Format_ARGB32_Premultiplied);

    m_cache.fill(Qt::white);
    m_cacheValid = true;

    if (!m_controller)
        return;

    const Page *page = m_controller->page(m_pageIndex);
    if (!page)
        return;

    QPainter p(&m_cache);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.scale(m_zoom, m_zoom);

    ElementPainter::drawBackground(&p, page);

    for (int i = 0; i < page->layers.size(); ++i) {
        const Layer *layer = page->layers.at(i);
        if (!layer->visible)
            continue;
        p.save();
        if (layer->opacity < 1.0)
            p.setOpacity(layer->opacity);
        for (int j = 0; j < layer->elements.size(); ++j)
            ElementPainter::drawElement(&p, layer->elements.at(j), m_controller->tools()->outlineView());
        p.restore();
    }
}

void CanvasItem::paint(QPainter *painter)
{
    if (!m_cacheValid)
        rebuildCache();

    painter->drawImage(0, 0, m_cache);

    if (m_builder.isActive() || m_shapeStroke) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->scale(m_zoom, m_zoom);
        ElementPainter::drawStroke(painter, m_shapeStroke ? m_shapeStroke : m_builder.peek(),
                                   m_controller && m_controller->tools()->outlineView());
        painter->restore();
    }

    QVector<QPointF> previewPoints = m_curveAnchors;
    if (m_curveCursorValid)
        previewPoints << m_curveCursor;
    if (Element *preview = buildCurve(previewPoints, curveClosesByDefault())) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->scale(m_zoom, m_zoom);
        ElementPainter::drawElement(painter, preview,
                                    m_controller && m_controller->tools()->outlineView());
        painter->restore();
        delete preview;
    }

    OverlayPainter::drawCurveAnchors(painter, m_curveAnchors, m_zoom);

    const QRectF marquee = m_controller && !m_marqueeActive
            ? (m_controller->currentPage() == m_pageIndex ? m_controller->selection()->rect()
                                                          : QRectF())
            : m_marquee;
    OverlayPainter::drawMarquee(painter, marquee, m_zoom);
    OverlayPainter::drawLasso(painter, m_lasso, m_zoom);

    if (m_controller && m_controller->tools()->tool() == ToolSettings::NodeTool
            && m_controller->nodeEditor()->isActive()
            && m_controller->currentPage() == m_pageIndex) {
        OverlayPainter::drawNodes(painter, m_controller->nodeEditor(), m_zoom);
    }

    if (m_controller && m_controller->selection()->page() == m_pageIndex
            && m_controller->selection()->count() > 0) {
        OverlayPainter::drawSelection(painter, m_controller->selection(), m_zoom);
    }
}

qreal CanvasItem::snapRadius() const
{
    return m_zoom > 0 ? 24.0 / m_zoom : 24.0;
}

QPointF CanvasItem::toPage(const QPointF &itemPos) const
{
    if (m_zoom <= 0)
        return itemPos;
    return QPointF(itemPos.x() / m_zoom, itemPos.y() / m_zoom);
}

QRect CanvasItem::pageRectToItem(const QRectF &pageRect, qreal padPoints) const
{
    const QRectF padded = pageRect.adjusted(-padPoints, -padPoints, padPoints, padPoints);
    return QRectF(padded.x() * m_zoom, padded.y() * m_zoom,
                  padded.width() * m_zoom, padded.height() * m_zoom).toAlignedRect();
}

void CanvasItem::configureBuilder()
{
    if (!m_controller)
        return;

    const ToolSettings::Tool tool = m_controller->tools()->tool();
    const bool brush = tool == ToolSettings::BrushTool;

    switch (tool) {
    case ToolSettings::HighlighterTool:
        m_builder.setTool(Stroke::Highlighter);
        break;
    case ToolSettings::EraserTool:
        m_builder.setTool(Stroke::Eraser);
        break;
    case ToolSettings::PenTool:
    case ToolSettings::BrushTool:
    case ToolSettings::BezierTool:
    case ToolSettings::TextTool:
    case ToolSettings::MoveTool:
    case ToolSettings::ShapeTool:
    case ToolSettings::SelectTool:
        m_builder.setTool(Stroke::Pen);
        break;
    }

    m_builder.setColor(m_controller->tools()->color());
    m_builder.setBaseWidth(m_controller->tools()->penWidth()
                           * (brush ? kBrushWidthFactor : 1.0));
    m_builder.setWidthSpread(brush ? kBrushSpread : kPenSpread);
    m_builder.setDynamicWidth((m_controller->tools()->dynamicWidth() || brush)
                              && tool != ToolSettings::HighlighterTool);
    m_builder.setMinDistance(m_zoom > 0 ? 0.5 / m_zoom : 0.5);
}

static bool wantsStraightSegments(const ToolSettings *t)
{
    return t->tool() == ToolSettings::ShapeTool
            && t->shapeKind() == ToolSettings::PolylineKind;
}

bool CanvasItem::curveClosesByDefault() const
{
    return m_controller
            && m_controller->tools()->tool() == ToolSettings::ShapeTool
            && m_controller->tools()->shapeKind() == ToolSettings::ClosedSplineKind;
}

Element *CanvasItem::buildCurve(const QVector<QPointF> &pts, bool closed) const
{
    if (!m_controller || pts.size() < 2)
        return 0;

    if (wantsStraightSegments(m_controller->tools())) {
        Stroke *s = new Stroke;
        applyStyle(s);
        s->shape = Stroke::PolylineShape;
        QVector<StrokePoint> points;
        points.reserve(pts.size() + 1);
        for (int i = 0; i < pts.size(); ++i)
            points << StrokePoint(pts.at(i).x(), pts.at(i).y());
        if (closed)
            points << StrokePoint(pts.first().x(), pts.first().y());
        s->setPoints(points);
        return s;
    }

    Path *p = new Path;
    applyStyle(p);
    p->closed = closed;
    p->setSegments(pts.first(), CurveFitter::through(pts, closed));
    return p;
}

void CanvasItem::curveTap(const QPointF &snappedPos)
{
    const qreal closeRadius = snapRadius();

    if (!m_curveAnchors.isEmpty()
            && QLineF(snappedPos, m_curveAnchors.last()).length() < closeRadius) {
        commitCurve(false);
        return;
    }

    if (m_curveAnchors.size() > 2
            && QLineF(snappedPos, m_curveAnchors.first()).length() < closeRadius) {
        commitCurve(true);
        return;
    }

    m_curveAnchors << snappedPos;
    m_controller->setCurrentPage(m_pageIndex);
    m_controller->tools()->reportHint(
                m_curveAnchors.size() == 1 ? tr("Tap for the next point")
              : m_curveAnchors.size() == 2 ? tr("Tap the last point to finish")
                                           : tr("Last point finishes, first closes"));
    Q_EMIT drawingChanged();
    update();
}

void CanvasItem::commitCurve(bool closed)
{
    if (!m_controller || m_curveAnchors.size() < 2) {
        discardCurve();
        return;
    }

    Element *e = buildCurve(m_curveAnchors, closed || curveClosesByDefault());
    if (!e) {
        discardCurve();
        return;
    }

    m_curveAnchors.clear();
    m_curveCursorValid = false;
    m_controller->tools()->reportHint(QString());

    m_controller->commitElement(m_pageIndex, e);
    Q_EMIT drawingChanged();
    Q_EMIT strokeFinished();
    update();
}

void CanvasItem::discardCurve()
{
    if (m_curveAnchors.isEmpty() && !m_curveCursorValid)
        return;
    m_curveAnchors.clear();
    m_curveCursorValid = false;
    if (m_controller)
        m_controller->tools()->reportHint(QString());
    Q_EMIT drawingChanged();
    update();
}

void CanvasItem::notePressure(qreal pressure)
{
    if (pressure <= 0.001 || pressure >= 0.999)
        return;

    if (!m_pressureAvailable) {
        m_pressureAvailable = true;
        Q_EMIT pressureAvailableChanged();
    }
    if (m_controller && m_builder.pressureIsUsable()) {
        m_controller->tools()->reportPressure(m_builder.observedPressureMin(),
                                     m_builder.observedPressureMax());
    }
}

void CanvasItem::inputBegin(const QPointF &itemPos, qreal pressure, qint64 timeMs)
{
    if (!m_controller || !m_drawingEnabled)
        return;

    notePressure(pressure);
    const QPointF pagePos = toPage(itemPos);

    if (m_controller->tools()->tool() == ToolSettings::MoveTool
            && m_controller->transform()->mode() == SelectionTransform::Resize
            && m_controller->selection()->page() == m_pageIndex
            && m_controller->selection()->count() > 0) {
        const int handle = m_controller->transform()->grabHandle(pagePos, snapRadius() * 0.7);
        if (handle >= 0) {
            m_scalingSelection = true;
            m_controller->transform()->beginScale(handle);
            Q_EMIT drawingChanged();
            return;
        }
    }

    if (m_controller->tools()->tool() == ToolSettings::MoveTool) {
        m_pressPagePos = pagePos;
        m_longPressHandled = false;
        m_longPressTimer.start(kLongPressMs, this);

        if (!m_controller->selection()->isOn(m_pageIndex, pagePos, snapRadius() * 0.5))
            m_controller->selection()->selectAt(m_pageIndex, pagePos, snapRadius() * 0.5);

        if (m_controller->selection()->count() > 0) {
            m_movingSelection = true;
            m_lastMovePos = pagePos;
            m_controller->selection()->beginMove();
        }
        Q_EMIT drawingChanged();
        update();
        return;
    }

    if (m_controller->tools()->tool() == ToolSettings::NodeTool) {
        NodeEditor *nodes = m_controller->nodeEditor();
        const int node = nodes->hit(pagePos, snapRadius() * 0.6);
        if (node >= 0) {
            m_draggingNode = true;
            nodes->beginDrag(node);
        } else {
            Element *e = m_controller->selection()->elementAt(m_pageIndex, pagePos,
                                                             snapRadius() * 0.5);
            m_controller->setCurrentPage(m_pageIndex);
            nodes->setTarget(e);
            if (!e)
                m_controller->tools()->reportHint(QString());
            else if (nodes->isBusy())
                m_controller->tools()->reportHint(tr("Too many points \u2014 long press to thin out"));
            else
                m_controller->tools()->reportHint(tr("%n node(s)", "", nodes->count()));
        }
        Q_EMIT drawingChanged();
        update();
        return;
    }

    if (m_controller->tools()->tool() == ToolSettings::SelectTool) {
        m_controller->setCurrentPage(m_pageIndex);

        if (m_controller->tools()->selectShape() == ToolSettings::PointSelect) {
            const QPointF at = m_controller->snapPoint(m_pageIndex, pagePos, snapRadius());
            m_controller->selection()->toggleAt(m_pageIndex, at, snapRadius() * 0.5);
            Q_EMIT drawingChanged();
            update();
            return;
        }

        if (m_controller->tools()->selectShape() == ToolSettings::LassoSelect) {
            m_lassoActive = true;
            m_lasso.clear();
            m_lasso << pagePos;
            Q_EMIT drawingChanged();
            update();
            return;
        }

        m_marqueeActive = true;
        m_shapeOrigin = pagePos;
        m_marquee = QRectF(pagePos, pagePos);
        Q_EMIT drawingChanged();
        update();
        return;
    }

    if (m_controller->tools()->tool() == ToolSettings::TextTool) {
        m_controller->editTextAt(m_pageIndex, pagePos);
        return;
    }

    if (m_controller->tools()->placesPoints()) {
        m_curveCursor = m_controller->snapPoint(m_pageIndex, pagePos, snapRadius());
        m_curveCursorValid = true;
        Q_EMIT drawingChanged();
        update();
        return;
    }

    if (m_controller->tools()->tool() == ToolSettings::ShapeTool) {
        delete m_shapeStroke;
        m_shapeStroke = new Stroke;
        applyStyle(m_shapeStroke);
        m_shapeStroke->shape = Stroke::Shape(m_controller->tools()->currentShape());
        m_shapeOrigin = m_controller->snapPoint(m_pageIndex, pagePos, snapRadius());
        m_shapeStroke->setPoints(ShapeFactory::build(
                Stroke::Shape(m_controller->tools()->currentShape()), m_shapeOrigin, pagePos,
                m_controller->tools()->polygonCorners(), m_controller->tools()->equalSides(),
                m_controller->tools()->snapStep()));
        m_liveBounds = m_shapeStroke->bounds();
        Q_EMIT drawingChanged();
        update();
        return;
    }

    if (m_controller->tools()->tool() == ToolSettings::EraserTool) {
        m_erasing = true;
        m_controller->beginErase(m_pageIndex);
        m_controller->eraseAt(m_pageIndex, pagePos, m_eraserRadius);
        Q_EMIT drawingChanged();
        return;
    }

    configureBuilder();
    m_builder.begin(pagePos, pressure, timeMs);
    m_liveBounds = QRectF(pagePos, QSizeF(0, 0));
    Q_EMIT drawingChanged();
    update();
}

void CanvasItem::inputMove(const QPointF &itemPos, qreal pressure, qint64 timeMs)
{
    if (!m_controller)
        return;

    notePressure(pressure);
    const QPointF pagePos = toPage(itemPos);

    if (m_longPressTimer.isActive()
            && QLineF(m_pressPagePos, pagePos).length() * m_zoom > kLongPressSlackPx) {
        m_longPressTimer.stop();
    }

    if (m_longPressHandled)
        return;

    if (m_scalingSelection) {
        m_controller->transform()->scaleTo(pagePos, m_controller->tools()->equalSides());
        return;
    }

    if (m_draggingNode) {
        m_controller->nodeEditor()->dragTo(pagePos);
        return;
    }

    if (m_movingSelection) {
        m_controller->selection()->moveBy(pagePos.x() - m_lastMovePos.x(),
                                      pagePos.y() - m_lastMovePos.y());
        m_lastMovePos = pagePos;
        return;
    }

    if (m_curveCursorValid) {
        m_curveCursor = m_controller->snapPoint(m_pageIndex, pagePos, snapRadius());
        update();
        return;
    }

    if (m_lassoActive) {
        if (m_lasso.isEmpty() || QLineF(m_lasso.last(), pagePos).length() * m_zoom > 3) {
            m_lasso << pagePos;
            update();
        }
        return;
    }

    if (m_marqueeActive) {
        const QRectF before = m_marquee;
        m_marquee = QRectF(m_shapeOrigin, pagePos).normalized();
        update(pageRectToItem(before.united(m_marquee), 4));
        return;
    }

    if (m_shapeStroke) {
        const QRectF before = m_shapeStroke->bounds();
        const QPointF snappedPos = m_controller->snapPoint(m_pageIndex, pagePos, snapRadius());
        m_shapeStroke->setPoints(ShapeFactory::build(
                Stroke::Shape(m_controller->tools()->currentShape()), m_shapeOrigin, snappedPos,
                m_controller->tools()->polygonCorners(), m_controller->tools()->equalSides(),
                m_controller->tools()->snapStep()));
        m_controller->tools()->reportHint(ShapeFactory::describeSize(
                ShapeFactory::dragRect(m_shapeOrigin, snappedPos,
                                       m_controller->tools()->equalSides(),
                                       m_controller->tools()->snapStep())));
        update(pageRectToItem(before.united(m_shapeStroke->bounds()),
                              m_controller->tools()->penWidth() + 2));
        return;
    }

    if (m_erasing) {
        m_controller->eraseAt(m_pageIndex, pagePos, m_eraserRadius);
        return;
    }

    if (!m_builder.isActive())
        return;

    if (!m_builder.extend(pagePos, pressure, timeMs))
        return;

    const QRectF grown = QRectF(m_liveBounds).united(QRectF(pagePos, QSizeF(0, 0)));
    const qreal pad = m_controller->tools()->penWidth() + 2;
    update(pageRectToItem(grown, pad));
    m_liveBounds = grown;
}

void CanvasItem::inputEnd()
{
    if (!m_controller)
        return;

    m_longPressTimer.stop();

    if (m_longPressHandled) {
        m_longPressHandled = false;
        m_movingSelection = false;
        Q_EMIT drawingChanged();
        return;
    }

    if (m_scalingSelection) {
        m_scalingSelection = false;
        m_controller->transform()->endScale();
        Q_EMIT drawingChanged();
        Q_EMIT strokeFinished();
        update();
        return;
    }

    if (m_draggingNode) {
        m_draggingNode = false;
        m_controller->nodeEditor()->endDrag();
        Q_EMIT drawingChanged();
        Q_EMIT strokeFinished();
        update();
        return;
    }

    if (m_movingSelection) {
        m_movingSelection = false;
        m_controller->selection()->endMove();
        Q_EMIT drawingChanged();
        Q_EMIT strokeFinished();
        return;
    }

    if (m_curveCursorValid) {
        const QPointF tap = m_curveCursor;
        m_curveCursorValid = false;
        curveTap(tap);
        return;
    }

    if (m_lassoActive) {
        m_lassoActive = false;
        if (m_lasso.size() < 3)
            m_controller->selection()->clear();
        else
            m_controller->selection()->setLasso(m_pageIndex, QPolygonF(m_lasso));
        m_lasso.clear();
        Q_EMIT drawingChanged();
        Q_EMIT strokeFinished();
        update();
        return;
    }

    if (m_marqueeActive) {
        m_marqueeActive = false;
        if (m_marquee.width() < 4 || m_marquee.height() < 4)
            m_controller->selection()->clear();
        else
            m_controller->selection()->setRect(m_pageIndex, m_marquee);
        m_marquee = QRectF();
        Q_EMIT drawingChanged();
        Q_EMIT strokeFinished();
        update();
        return;
    }

    if (m_shapeStroke) {
        Stroke *s = m_shapeStroke;
        m_shapeStroke = 0;
        m_controller->tools()->reportHint(QString());
        if (s->points.size() < 2 || s->bounds().width() + s->bounds().height() < 4) {
            delete s;
            update();
        } else {
            m_controller->commitStroke(m_pageIndex, s);
        }
        Q_EMIT drawingChanged();
        Q_EMIT strokeFinished();
        return;
    }

    if (m_erasing) {
        m_erasing = false;
        m_controller->endErase();
        Q_EMIT drawingChanged();
        Q_EMIT strokeFinished();
        return;
    }

    if (!m_builder.isActive())
        return;

    Stroke *s = m_builder.take();
    m_controller->commitStroke(m_pageIndex, s);
    Q_EMIT drawingChanged();
    Q_EMIT strokeFinished();
}

void CanvasItem::inputCancel()
{
    m_draggingNode = false;
    m_scalingSelection = false;
    m_lassoActive = false;
    m_lasso.clear();
    m_longPressTimer.stop();

    if (m_longPressHandled) {
        m_longPressHandled = false;
        m_movingSelection = false;
        Q_EMIT drawingChanged();
        return;
    }

    if (m_movingSelection) {
        m_movingSelection = false;
        m_controller->selection()->endMove();
        Q_EMIT drawingChanged();
        Q_EMIT strokeFinished();
        return;
    }

    if (m_curveCursorValid) {
        m_curveCursorValid = false;
        update();
    }
    if (m_marqueeActive) {
        m_marqueeActive = false;
        m_marquee = QRectF();
        Q_EMIT drawingChanged();
        update();
    }
    if (m_shapeStroke) {
        delete m_shapeStroke;
        m_shapeStroke = 0;
        if (m_controller)
            m_controller->tools()->reportHint(QString());
        Q_EMIT drawingChanged();
        update();
    }
    if (m_builder.isActive()) {
        m_builder.cancel();
        Q_EMIT drawingChanged();
        update();
    }
    if (m_erasing) {
        m_erasing = false;
        if (m_controller)
            m_controller->endErase();
        Q_EMIT drawingChanged();
    }
}

void CanvasItem::touchEvent(QTouchEvent *event)
{
    if (!m_drawingEnabled || !m_controller) {
        inputCancel();
        event->ignore();
        return;
    }

    const QList<QTouchEvent::TouchPoint> &points = event->touchPoints();

    if (points.count() > 1) {
        inputCancel();
        setKeepTouchGrab(false);
        event->ignore();
        return;
    }

    if (points.isEmpty()) {
        event->ignore();
        return;
    }

    const QTouchEvent::TouchPoint &tp = points.first();

    if (m_palmThreshold > 0) {
        const QRectF area = tp.rect();
        const qreal widest = qMax(area.width(), area.height()) / qMax(m_zoom, qreal(0.001));
        if (widest > m_palmThreshold) {
            inputCancel();
            event->ignore();
            return;
        }
    }

    const qint64 timeMs = event->timestamp() > 0
            ? qint64(event->timestamp())
            : QDateTime::currentMSecsSinceEpoch();

    switch (event->type()) {
    case QEvent::TouchBegin:
        inputBegin(tp.pos(), tp.pressure(), timeMs);
        setKeepTouchGrab(true);
        event->accept();
        break;
    case QEvent::TouchUpdate:
        inputMove(tp.pos(), tp.pressure(), timeMs);
        event->accept();
        break;
    case QEvent::TouchEnd:
        inputEnd();
        setKeepTouchGrab(false);
        event->accept();
        break;
    case QEvent::TouchCancel:
        inputCancel();
        setKeepTouchGrab(false);
        event->accept();
        break;
    default:
        event->ignore();
        break;
    }
}

void CanvasItem::mousePressEvent(QMouseEvent *event)
{
    if (!m_drawingEnabled || !m_controller) {
        event->ignore();
        return;
    }
    inputBegin(event->localPos(), 1.0, QDateTime::currentMSecsSinceEpoch());
    event->accept();
}

void CanvasItem::mouseMoveEvent(QMouseEvent *event)
{
    inputMove(event->localPos(), 1.0, QDateTime::currentMSecsSinceEpoch());
    event->accept();
}

void CanvasItem::mouseReleaseEvent(QMouseEvent *event)
{
    inputEnd();
    event->accept();
}
}
