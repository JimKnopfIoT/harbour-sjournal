#include "app/documentcontroller.h"

#include "app/documentstore.h"
#include "app/exporter.h"
#include "app/imagetools.h"
#include "app/layermodel.h"
#include "app/nodeeditor.h"
#include "app/pathtools.h"
#include "app/selectiontransform.h"
#include "app/styletools.h"
#include "app/selection.h"
#include "app/toolsettings.h"
#include "app/undostack.h"
#include "model/document.h"
#include "model/path.h"
#include "model/stroke.h"
#include "model/textitem.h"
#include "tools/shaperecognizer.h"

#include <QDateTime>
#include <QFileInfo>
#include <QLineF>
#include <QtMath>

namespace xn {
static const int kPreviewWidth = 240;

DocumentController::DocumentController(QObject *parent)
    : QObject(parent)
    , m_document(new Document)
    , m_undo(new UndoStack)
    , m_currentPage(0)
    , m_currentLayer(0)
    , m_modified(false)
    , m_erasePage(-1)
    , m_editingText(0)
    , m_textPage(0)
    , m_syncingTextFormat(false)
{
    m_tools = new ToolSettings(this);
    m_selection = new Selection(m_document, m_undo, this);
    m_images = new ImageTools(m_document, m_undo, this);
    m_nodeEditor = new NodeEditor(m_undo, this);
    m_transform = new SelectionTransform(m_selection, m_undo, this);
    m_pathTools = new PathTools(m_document, m_selection, m_undo, this);
    m_styleTools = new StyleTools(m_selection, m_undo, this);
    m_layers = new LayerModel(this);
    m_layers->setController(this);
    connectParts();
}

DocumentController::~DocumentController()
{
    qDeleteAll(m_erased);
    delete m_undo;
    delete m_document;
}

void DocumentController::connectParts()
{
    connect(m_selection, &Selection::contentChanged, this, &DocumentController::pageContentChanged);
    connect(m_selection, &Selection::historyChanged, this, &DocumentController::historyChanged);
    connect(m_selection, &Selection::changed, this, &DocumentController::markModified);
    connect(m_selection, &Selection::layerRequested, this, &DocumentController::setCurrentLayer);
    connect(m_selection, &Selection::error, this, &DocumentController::setError);
    connect(m_selection, &Selection::changed, this,
            &DocumentController::syncTextFormatFromSelection);

    connect(m_tools, &ToolSettings::fontSizeChanged, this,
            &DocumentController::applyTextFormatToSelection);
    connect(m_tools, &ToolSettings::fontFormChanged, this,
            &DocumentController::applyTextFormatToSelection);
    connect(m_tools, &ToolSettings::textStyleChanged, this,
            &DocumentController::applyTextFormatToSelection);

    connect(m_images, &ImageTools::contentChanged, this, &DocumentController::pageContentChanged);
    connect(m_images, &ImageTools::layersChanged, this, &DocumentController::layersChanged);
    connect(m_images, &ImageTools::historyCleared, this, &DocumentController::historyChanged);
    connect(m_images, &ImageTools::layerRequested, this, &DocumentController::setCurrentLayer);
    connect(m_images, &ImageTools::error, this, &DocumentController::setError);

    connect(m_nodeEditor, &NodeEditor::contentChanged, this,
            &DocumentController::pageContentChanged);
    connect(m_nodeEditor, &NodeEditor::historyChanged, this,
            &DocumentController::historyChanged);
    connect(m_nodeEditor, &NodeEditor::historyChanged, this,
            &DocumentController::markModified);

    connect(m_selection, &Selection::contentChanged, m_nodeEditor, &NodeEditor::clear);
    connect(m_selection, &Selection::changed, m_transform, &SelectionTransform::boxChanged);

    connect(m_transform, &SelectionTransform::contentChanged, this,
            &DocumentController::pageContentChanged);
    connect(m_transform, &SelectionTransform::historyChanged, this,
            &DocumentController::historyChanged);
    connect(m_transform, &SelectionTransform::historyChanged, this,
            &DocumentController::markModified);

    connect(m_pathTools, &PathTools::contentChanged, this,
            &DocumentController::pageContentChanged);
    connect(m_pathTools, &PathTools::historyChanged, this,
            &DocumentController::historyChanged);
    connect(m_pathTools, &PathTools::historyChanged, this,
            &DocumentController::markModified);
    connect(m_pathTools, &PathTools::historyChanged, this,
            &DocumentController::layersChanged);
    connect(m_pathTools, &PathTools::error, this, &DocumentController::setError);

    connect(m_styleTools, &StyleTools::contentChanged, this,
            &DocumentController::pageContentChanged);
    connect(m_styleTools, &StyleTools::historyChanged, this,
            &DocumentController::historyChanged);
    connect(m_styleTools, &StyleTools::historyChanged, this,
            &DocumentController::markModified);
    connect(m_tools, &ToolSettings::toolChanged, this, &DocumentController::onToolChanged);
}

void DocumentController::markModified()
{
    setModified(true);
}

void DocumentController::onToolChanged()
{
    if (m_tools->tool() == ToolSettings::NodeTool)
        return;
    if (m_nodeEditor->isActive())
        m_tools->reportHint(QString());
    m_nodeEditor->clear();
}

QString DocumentController::notesDirectory() { return DocumentStore::notesDirectory(); }
QString DocumentController::homeDirectory() { return DocumentStore::homeDirectory(); }
QString DocumentController::picturesDirectory() { return DocumentStore::picturesDirectory(); }
QString DocumentController::documentsDirectory() { return DocumentStore::documentsDirectory(); }
QString DocumentController::downloadsDirectory() { return DocumentStore::downloadsDirectory(); }
QString DocumentController::screenshotsDirectory() { return DocumentStore::screenshotsDirectory(); }

QString DocumentController::latestScreenshotName()
{
    const QString path = DocumentStore::latestScreenshot();
    return path.isEmpty() ? QString() : QFileInfo(path).fileName();
}

bool DocumentController::createNote(const QString &title, qreal pageWidth, qreal pageHeight)
{
    m_selection->forget();
    m_document->clear();
    m_document->title = title.trimmed().isEmpty()
            ? QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm"))
            : title.trimmed();
    m_document->addPage(pageWidth > 1 && pageHeight > 1 ? QSizeF(pageWidth, pageHeight)
                                                        : Page::a4());
    m_document->filePath = DocumentStore::uniqueNotePath(m_document->title);

    m_undo->clear();
    m_currentPage = 0;
    m_currentLayer = 0;
    setModified(true);
    resetAfterStructuralChange();
    return true;
}

bool DocumentController::open(const QString &path)
{
    Document *loaded = new Document;
    DocumentStore store;
    if (!store.load(path, loaded)) {
        delete loaded;
        setError(store.errorString());
        return false;
    }

    m_selection->forget();
    qDeleteAll(m_erased);
    m_erased.clear();
    m_erasedIndexes.clear();
    m_erasePage = -1;

    delete m_document;
    m_document = loaded;
    m_selection->setDocument(m_document);
    m_images->setDocument(m_document);

    m_undo->clear();
    m_currentPage = 0;
    m_currentLayer = qMax(0, layerCount() - 1);
    setModified(false);
    setError(QString());
    resetAfterStructuralChange();
    return true;
}

bool DocumentController::save()
{
    if (m_document->filePath.isEmpty()) {
        setError(tr("Note has no file name"));
        return false;
    }
    return saveAs(m_document->filePath);
}

bool DocumentController::saveAs(const QString &path)
{
    Exporter exporter(m_document);
    const QImage preview = exporter.renderPage(0, QSize(kPreviewWidth, kPreviewWidth * 2), true);

    DocumentStore store;
    if (!store.save(path, m_document, preview)) {
        setError(store.errorString());
        return false;
    }

    const QString local = DocumentStore::localPath(path);
    if (m_document->filePath != local) {
        m_document->filePath = local;
        Q_EMIT filePathChanged();
    }
    setModified(false);
    setError(QString());
    Q_EMIT saved(local);
    return true;
}

void DocumentController::close()
{
    m_selection->forget();
    m_document->clear();
    m_undo->clear();
    qDeleteAll(m_erased);
    m_erased.clear();
    m_erasedIndexes.clear();
    m_currentPage = 0;
    m_currentLayer = 0;
    setModified(false);
    resetAfterStructuralChange();
}

void DocumentController::resetAfterStructuralChange()
{
    m_nodeEditor->forget();
    m_nodeEditor->setPage(m_currentPage);
    m_pathTools->setDocument(m_document);
    m_selection->setPage(m_currentPage);
    m_selection->setLayer(m_currentLayer);
    m_images->setPage(m_currentPage);
    m_images->setLayer(m_currentLayer);

    Q_EMIT documentReplaced();
    Q_EMIT titleChanged();
    Q_EMIT filePathChanged();
    Q_EMIT pageCountChanged();
    Q_EMIT currentPageChanged();
    Q_EMIT layersChanged();
    Q_EMIT currentLayerChanged();
    Q_EMIT historyChanged();
}

void DocumentController::addPage()
{
    const Page *last = m_document->pageAt(m_document->pageCount() - 1);
    Page *added = m_document->addPage(last ? QSizeF(last->width, last->height) : Page::a4());
    if (last)
        added->background = last->background;

    m_undo->clear();
    setModified(true);
    Q_EMIT pageCountChanged();
    Q_EMIT historyChanged();
    setCurrentPage(m_document->pageCount() - 1);
}

void DocumentController::duplicatePage(int index)
{
    const Page *source = m_document->pageAt(index);
    if (!source)
        return;
    m_document->insertPage(index + 1, source->clone());
    m_undo->clear();
    setModified(true);
    Q_EMIT pageCountChanged();
    Q_EMIT historyChanged();
    setCurrentPage(index + 1);
}

void DocumentController::removePage(int index)
{
    if (m_document->pageCount() <= 1)
        return;
    m_selection->clear();
    delete m_document->takePage(index);

    m_undo->clear();
    setModified(true);
    Q_EMIT pageCountChanged();
    Q_EMIT historyChanged();
    setCurrentPage(qMin(m_currentPage, m_document->pageCount() - 1));
}

qreal DocumentController::pageWidth(int index) const
{
    const Page *p = m_document->pageAt(index);
    return p ? p->width : Page::a4().width();
}

qreal DocumentController::pageHeight(int index) const
{
    const Page *p = m_document->pageAt(index);
    return p ? p->height : Page::a4().height();
}

void DocumentController::setPageSize(int index, qreal width, qreal height)
{
    Page *p = m_document->pageAt(index);
    if (!p || width < 1 || height < 1)
        return;
    if (qFuzzyCompare(p->width, width) && qFuzzyCompare(p->height, height))
        return;

    p->width = width;
    p->height = height;
    setModified(true);
    Q_EMIT pageContentChanged(index);
}

QString DocumentController::pageBackgroundStyle(int index) const
{
    const Page *p = m_document->pageAt(index);
    return p ? p->background.style : QString();
}

void DocumentController::setPageBackgroundStyle(int index, const QString &style)
{
    Page *p = m_document->pageAt(index);
    if (!p || p->background.style == style)
        return;
    p->background.style = style;
    setModified(true);
    Q_EMIT pageContentChanged(index);
}

Layer *DocumentController::layerAt(int pageIndex, int layerIndex) const
{
    Page *p = m_document->pageAt(pageIndex);
    return p ? p->layerAt(layerIndex) : 0;
}

int DocumentController::layerCount() const
{
    const Page *p = m_document->pageAt(m_currentPage);
    return p ? p->layerCount() : 0;
}

QString DocumentController::layerName(int index) const
{
    const Layer *l = layerAt(m_currentPage, index);
    return l ? l->name : QString();
}

void DocumentController::setLayerName(int index, const QString &name)
{
    Layer *l = layerAt(m_currentPage, index);
    if (!l || l->name == name)
        return;
    l->name = name;
    setModified(true);
    Q_EMIT layersChanged();
}

bool DocumentController::layerVisible(int index) const
{
    const Layer *l = layerAt(m_currentPage, index);
    return l ? l->visible : false;
}

void DocumentController::setLayerVisible(int index, bool visible)
{
    Layer *l = layerAt(m_currentPage, index);
    if (!l || l->visible == visible)
        return;
    l->visible = visible;
    setModified(true);
    Q_EMIT layersChanged();
    Q_EMIT pageContentChanged(m_currentPage);
}

bool DocumentController::layerLocked(int index) const
{
    const Layer *l = layerAt(m_currentPage, index);
    return l ? l->locked : false;
}

void DocumentController::setLayerLocked(int index, bool locked)
{
    Layer *l = layerAt(m_currentPage, index);
    if (!l || l->locked == locked)
        return;
    l->locked = locked;

    if (locked) {
        m_selection->clear();
        if (m_currentLayer == index) {
            Page *p = m_document->pageAt(m_currentPage);
            for (int i = p ? p->layerCount() - 1 : -1; i >= 0; --i) {
                if (!p->layers.at(i)->locked) {
                    setCurrentLayer(i);
                    break;
                }
            }
        }
    }

    setModified(true);
    Q_EMIT layersChanged();
}

int DocumentController::layerElementCount(int index) const
{
    const Layer *l = layerAt(m_currentPage, index);
    return l ? l->count() : 0;
}

void DocumentController::addLayer()
{
    Page *p = m_document->pageAt(m_currentPage);
    if (!p)
        return;
    p->addLayer();
    m_undo->clear();
    setModified(true);
    Q_EMIT layersChanged();
    Q_EMIT historyChanged();
    setCurrentLayer(p->layerCount() - 1);
}

void DocumentController::removeLayer(int index)
{
    Page *p = m_document->pageAt(m_currentPage);
    if (!p || p->layerCount() <= 1)
        return;
    m_selection->clear();
    delete p->takeLayer(index);

    m_undo->clear();
    setModified(true);
    Q_EMIT layersChanged();
    Q_EMIT historyChanged();
    Q_EMIT pageContentChanged(m_currentPage);
    setCurrentLayer(qMin(m_currentLayer, p->layerCount() - 1));
}

void DocumentController::moveLayer(int from, int to)
{
    Page *p = m_document->pageAt(m_currentPage);
    if (!p || from == to)
        return;
    if (from < 0 || from >= p->layerCount() || to < 0 || to >= p->layerCount())
        return;

    p->layers.move(from, to);
    m_undo->clear();
    setModified(true);
    Q_EMIT layersChanged();
    Q_EMIT historyChanged();
    Q_EMIT pageContentChanged(m_currentPage);
    setCurrentLayer(to);
}

void DocumentController::clearLayer(int index)
{
    Layer *l = layerAt(m_currentPage, index);
    if (!l || l->isEmpty())
        return;

    m_selection->clear();
    QVector<Element *> all;
    for (int i = 0; i < l->count(); ++i)
        all.append(l->elements.at(i));
    m_undo->push(new RemoveElementsCommand(l, all));

    setModified(true);
    Q_EMIT historyChanged();
    Q_EMIT layersChanged();
    Q_EMIT pageContentChanged(m_currentPage);
}

Layer *DocumentController::drawingLayer(int pageIndex) const
{
    Page *p = m_document->pageAt(pageIndex);
    if (!p)
        return 0;
    Layer *l = p->ensureLayer(m_currentLayer);
    return (l && l->locked) ? 0 : l;
}

void DocumentController::commitStroke(int pageIndex, Stroke *stroke)
{
    if (!stroke)
        return;

    Layer *layer = drawingLayer(pageIndex);
    if (!layer) {
        delete stroke;
        setError(tr("This layer is locked"));
        return;
    }

    if (m_tools->shapeRecognition() && stroke->shape == Stroke::FreeShape
            && stroke->tool != Stroke::Eraser) {
        ShapeRecognizer recognizer;
        if (recognizer.apply(stroke)) {
            for (int i = 0; i < stroke->points.size(); ++i)
                stroke->points[i].width = -1;
            stroke->invalidate();
        }
    }

    m_undo->push(new AddElementCommand(layer, stroke));
    setModified(true);
    Q_EMIT historyChanged();
    Q_EMIT layersChanged();
    Q_EMIT pageContentChanged(pageIndex);
}

void DocumentController::commitElement(int pageIndex, Element *element)
{
    if (!element)
        return;

    Layer *layer = drawingLayer(pageIndex);
    if (!layer) {
        delete element;
        setError(tr("This layer is locked"));
        return;
    }

    m_undo->push(new AddElementCommand(layer, element));
    setModified(true);
    Q_EMIT historyChanged();
    Q_EMIT layersChanged();
    Q_EMIT pageContentChanged(pageIndex);
}

void DocumentController::editTextAt(int pageIndex, const QPointF &pos)
{
    Layer *layer = drawingLayer(pageIndex);
    if (!layer) {
        setError(tr("This layer is locked"));
        return;
    }

    m_editingText = 0;
    for (int i = layer->count() - 1; i >= 0; --i) {
        Element *e = layer->elements.at(i);
        if (e->type() != Element::TextType)
            continue;
        if (e->bounds().adjusted(-4, -4, 4, 4).contains(pos)) {
            m_editingText = static_cast<TextItem *>(e);
            break;
        }
    }

    m_textPage = pageIndex;
    m_textPos = pos;
    m_pendingText = m_editingText ? m_editingText->text : QString();
    setCurrentPage(pageIndex);
    Q_EMIT textEditRequested();
}

void DocumentController::commitText(const QString &text)
{
    Layer *layer = drawingLayer(m_textPage);
    if (!layer) {
        cancelTextEdit();
        return;
    }

    if (m_editingText) {
        if (text == m_editingText->text) {
            cancelTextEdit();
            return;
        }
        if (text.isEmpty()) {
            const int index = layer->indexOf(m_editingText);
            QVector<Element *> gone;
            QVector<int> at;
            gone << m_editingText;
            at << index;
            layer->take(index);
            m_selection->clear();
            m_undo->pushWithoutRedo(new RemoveElementsCommand(layer, gone, at));
        } else {
            m_undo->push(new ChangeTextCommand(m_editingText, m_editingText->text, text));
        }
    } else if (!text.isEmpty()) {
        TextItem *t = new TextItem;
        t->text = text;
        t->pos = m_textPos;
        t->fontSize = m_tools->fontSize();
        t->color = m_tools->color();
        t->bold = m_tools->fontBold();
        t->italic = m_tools->fontItalic();
        t->underline = m_tools->fontUnderline();
        t->style = TextItem::Style(m_tools->textStyle());
        m_undo->push(new AddElementCommand(layer, t));
    } else {
        cancelTextEdit();
        return;
    }

    m_editingText = 0;
    m_pendingText.clear();
    setModified(true);
    Q_EMIT historyChanged();
    Q_EMIT layersChanged();
    Q_EMIT pageContentChanged(m_textPage);
}

static TextFormat formatOf(const TextItem *t)
{
    TextFormat f;
    f.size = t->fontSize;
    f.bold = t->bold;
    f.italic = t->italic;
    f.underline = t->underline;
    f.style = int(t->style);
    return f;
}

static QVector<TextItem *> selectedText(const QVector<Element *> &elements)
{
    QVector<TextItem *> items;
    for (int i = 0; i < elements.size(); ++i) {
        Element *e = elements.at(i);
        if (e->type() == Element::TextType)
            items << static_cast<TextItem *>(e);
    }
    return items;
}

void DocumentController::applyTextFormatToSelection()
{
    if (m_syncingTextFormat)
        return;

    const QVector<TextItem *> items = selectedText(m_selection->elements());
    if (items.isEmpty())
        return;

    TextFormat after;
    after.size = m_tools->fontSize();
    after.bold = m_tools->fontBold();
    after.italic = m_tools->fontItalic();
    after.underline = m_tools->fontUnderline();
    after.style = m_tools->textStyle();

    QVector<TextFormat> before;
    bool differs = false;
    for (int i = 0; i < items.size(); ++i) {
        const TextFormat f = formatOf(items.at(i));
        before << f;
        if (f != after)
            differs = true;
    }
    if (!differs)
        return;

    m_undo->push(new ChangeTextFormatCommand(items, before, after));
    setModified(true);
    Q_EMIT historyChanged();
    Q_EMIT pageContentChanged(m_selection->page());
}

void DocumentController::syncTextFormatFromSelection()
{
    const QVector<TextItem *> items = selectedText(m_selection->elements());
    if (items.isEmpty())
        return;

    const TextFormat f = formatOf(items.first());

    m_syncingTextFormat = true;
    m_tools->setFontSize(f.size);
    m_tools->setFontBold(f.bold);
    m_tools->setFontItalic(f.italic);
    m_tools->setFontUnderline(f.underline);
    m_tools->setTextStyle(f.style);
    m_syncingTextFormat = false;
}

void DocumentController::cancelTextEdit()
{
    m_editingText = 0;
    m_pendingText.clear();
}

void DocumentController::beginErase(int pageIndex)
{
    m_nodeEditor->clear();
    endErase();
    m_erasePage = pageIndex;
}

static qreal distanceToStroke(const Stroke *s, const QPointF &pos, qreal limit)
{
    qreal best = -1;
    for (int i = 0; i + 1 < s->points.size(); ++i) {
        const StrokePoint &a = s->points.at(i);
        const StrokePoint &b = s->points.at(i + 1);
        const qreal dx = b.x - a.x;
        const qreal dy = b.y - a.y;
        const qreal lenSq = dx * dx + dy * dy;

        qreal t = 0;
        if (lenSq > 1e-9)
            t = qBound(qreal(0), ((pos.x() - a.x) * dx + (pos.y() - a.y) * dy) / lenSq, qreal(1));

        const qreal d = QLineF(pos, QPointF(a.x + t * dx, a.y + t * dy)).length()
                - s->widthAt(i) / 2;
        if (d < limit && (best < 0 || d < best))
            best = qMax(qreal(0), d);
    }
    return best;
}

bool DocumentController::eraseAt(int pageIndex, const QPointF &pos, qreal radius)
{
    Layer *layer = drawingLayer(pageIndex);
    if (!layer)
        return false;

    if (m_erasePage != pageIndex)
        beginErase(pageIndex);

    const QRectF hitRect(pos.x() - radius, pos.y() - radius, radius * 2, radius * 2);
    bool erased = false;

    for (int i = layer->count() - 1; i >= 0; --i) {
        Element *e = layer->elements.at(i);
        if (!e->bounds().intersects(hitRect))
            continue;

        bool hit;
        if (e->type() == Element::StrokeType)
            hit = distanceToStroke(static_cast<const Stroke *>(e), pos, radius) >= 0;
        else if (e->type() == Element::PathType)
            hit = static_cast<const Path *>(e)->hits(pos, radius);
        else
            hit = e->bounds().contains(pos);

        if (hit) {
            m_erased.append(e);
            m_erasedIndexes.append(i);
            layer->take(i);
            erased = true;
        }
    }

    if (erased) {
        m_selection->clear();
        setModified(true);
        Q_EMIT pageContentChanged(pageIndex);
    }
    return erased;
}

void DocumentController::endErase()
{
    if (m_erased.isEmpty()) {
        m_erasePage = -1;
        return;
    }

    Layer *layer = drawingLayer(m_erasePage);
    if (!layer) {
        qDeleteAll(m_erased);
    } else {
        QVector<Element *> elements;
        QVector<int> indexes;
        for (int i = m_erased.size() - 1; i >= 0; --i) {
            elements.append(m_erased.at(i));
            indexes.append(m_erasedIndexes.at(i));
        }
        m_undo->pushWithoutRedo(new RemoveElementsCommand(layer, elements, indexes));
        Q_EMIT historyChanged();
        Q_EMIT layersChanged();
    }

    m_erased.clear();
    m_erasedIndexes.clear();
    m_erasePage = -1;
}

QPointF DocumentController::snapPoint(int pageIndex, const QPointF &pos, qreal radius) const
{
    if (!m_tools->snapToPoints())
        return pos;

    Page *page = m_document->pageAt(pageIndex);
    if (!page)
        return pos;

    QPointF best = pos;
    qreal bestDistance = radius;

    for (int l = 0; l < page->layerCount(); ++l) {
        const Layer *layer = page->layers.at(l);
        if (!layer->visible)
            continue;

        for (int i = 0; i < layer->count(); ++i) {
            const Element *e = layer->elements.at(i);

            QVector<QPointF> candidates;
            if (e->type() == Element::PathType) {
                candidates = static_cast<const Path *>(e)->anchors();
            } else if (e->type() == Element::StrokeType) {
                const Stroke *s = static_cast<const Stroke *>(e);
                if (s->points.isEmpty())
                    continue;
                const bool everyPoint = s->shape != Stroke::FreeShape && s->points.size() <= 32;
                for (int p = 0; p < s->points.size(); ++p) {
                    if (!everyPoint && p != 0 && p != s->points.size() - 1)
                        continue;
                    candidates << QPointF(s->points.at(p).x, s->points.at(p).y);
                }
            } else {
                continue;
            }

            for (int c = 0; c < candidates.size(); ++c) {
                const qreal distance = QLineF(pos, candidates.at(c)).length();
                if (distance < bestDistance) {
                    bestDistance = distance;
                    best = candidates.at(c);
                }
            }
        }
    }
    return best;
}

bool DocumentController::insertImage(const QString &fileUrl, bool ownLayer)
{
    const bool ok = m_images->insertImage(fileUrl, ownLayer);
    if (ok)
        setModified(true);
    return ok;
}

bool DocumentController::insertLatestScreenshot()
{
    const bool ok = m_images->insertLatestScreenshot();
    if (ok)
        setModified(true);
    return ok;
}

bool DocumentController::traceSelection(int threshold, bool invert, bool filled, int speckSize)
{
    const bool ok = m_images->traceRegion(m_selection->rect(), threshold, invert, filled,
                                          speckSize, m_tools->color(), m_tools->penWidth());
    if (ok)
        setModified(true);
    return ok;
}

void DocumentController::undo()
{
    endErase();
    if (!m_undo->canUndo())
        return;
    m_selection->clear();
    m_undo->undo();
    setModified(true);
    Q_EMIT historyChanged();
    Q_EMIT layersChanged();
    Q_EMIT pageContentChanged(m_currentPage);
}

void DocumentController::redo()
{
    endErase();
    if (!m_undo->canRedo())
        return;
    m_selection->clear();
    m_undo->redo();
    setModified(true);
    Q_EMIT historyChanged();
    Q_EMIT layersChanged();
    Q_EMIT pageContentChanged(m_currentPage);
}

bool DocumentController::canUndo() const { return m_undo->canUndo(); }
bool DocumentController::canRedo() const { return m_undo->canRedo(); }

QString DocumentController::exportSvg(int pageIndex, const QString &directory, bool withBackground)
{
    Exporter exporter(m_document);
    const QString path = exporter.exportSvg(pageIndex, directory, withBackground);
    setError(path.isEmpty() ? exporter.errorString() : QString());
    return path;
}

QStringList DocumentController::exportSvgAllPages(const QString &directory, bool withBackground)
{
    Exporter exporter(m_document);
    const QStringList paths = exporter.exportSvgAllPages(directory, withBackground);
    setError(paths.isEmpty() ? exporter.errorString() : QString());
    return paths;
}

QStringList DocumentController::exportImageFormats() const
{
    return Exporter::imageFormats();
}

QString DocumentController::exportImage(int pageIndex, const QString &format, int maxPixels,
                                        const QString &directory, bool withBackground,
                                        int quality)
{
    Exporter exporter(m_document);
    const QString path = exporter.exportImage(pageIndex, format, maxPixels, directory,
                                              withBackground, quality);
    setError(path.isEmpty() ? exporter.errorString() : QString());
    return path;
}

QString DocumentController::exportPdf(const QString &directory, bool withBackground)
{
    Exporter exporter(m_document);
    const QString path = exporter.exportPdf(directory, withBackground);
    setError(path.isEmpty() ? exporter.errorString() : QString());
    return path;
}

QString DocumentController::exportPng(int pageIndex, int maxPixels, const QString &directory,
                                      bool withBackground)
{
    Exporter exporter(m_document);
    const QString path = exporter.exportPng(pageIndex, maxPixels, directory, withBackground);
    setError(path.isEmpty() ? exporter.errorString() : QString());
    return path;
}

Page *DocumentController::page(int index) const { return m_document->pageAt(index); }
QString DocumentController::title() const { return m_document->title; }
QString DocumentController::filePath() const { return m_document->filePath; }
int DocumentController::pageCount() const { return m_document->pageCount(); }

void DocumentController::setTitle(const QString &t)
{
    if (m_document->title == t)
        return;
    m_document->title = t;
    setModified(true);
    Q_EMIT titleChanged();
}

void DocumentController::setCurrentPage(int index)
{
    const int clamped = qBound(0, index, qMax(0, m_document->pageCount() - 1));
    if (m_currentPage == clamped)
        return;
    endErase();
    m_currentPage = clamped;
    m_nodeEditor->clear();
    m_nodeEditor->setPage(clamped);
    m_selection->setPage(clamped);
    m_images->setPage(clamped);
    Q_EMIT currentPageChanged();
    Q_EMIT layersChanged();
    setCurrentLayer(qMin(m_currentLayer, qMax(0, layerCount() - 1)));
}

void DocumentController::setCurrentLayer(int index)
{
    const int clamped = qBound(0, index, qMax(0, layerCount() - 1));
    if (m_currentLayer == clamped)
        return;
    endErase();
    m_currentLayer = clamped;
    m_selection->setLayer(clamped);
    m_images->setLayer(clamped);
    Q_EMIT currentLayerChanged();
}

void DocumentController::setModified(bool m)
{
    if (m_modified == m)
        return;
    m_modified = m;
    Q_EMIT modifiedChanged();
}

void DocumentController::setError(const QString &e)
{
    if (m_error == e)
        return;
    m_error = e;
    Q_EMIT errorStringChanged();
}
}
