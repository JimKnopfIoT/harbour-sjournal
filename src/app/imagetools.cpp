#include "app/imagetools.h"

#include "app/documentstore.h"
#include "app/undostack.h"
#include "model/document.h"
#include "model/imageitem.h"
#include "model/stroke.h"
#include "tools/bitmaptracer.h"

#include <QFileInfo>

namespace xn {
static const int kMaxImagePixels = 1600;

static ImageItem *imageUnder(Page *page, const QRectF &rect)
{
    if (!page || rect.isEmpty())
        return 0;
    for (int l = page->layerCount() - 1; l >= 0; --l) {
        const Layer *layer = page->layers.at(l);
        if (!layer->visible)
            continue;
        for (int i = layer->count() - 1; i >= 0; --i) {
            Element *e = layer->elements.at(i);
            if (e->type() != Element::ImageType)
                continue;
            ImageItem *image = static_cast<ImageItem *>(e);
            if (image->rect.intersects(rect))
                return image;
        }
    }
    return 0;
}

ImageTools::ImageTools(Document *document, UndoStack *undo, QObject *parent)
    : QObject(parent)
    , m_document(document)
    , m_undo(undo)
    , m_page(0)
    , m_layer(0)
{
}

bool ImageTools::insertImage(const QString &fileUrl, bool ownLayer)
{
    const QString local = DocumentStore::localPath(fileUrl);

    QImage image(local);
    if (image.isNull()) {
        Q_EMIT error(tr("Cannot read image %1").arg(QFileInfo(local).fileName()));
        return false;
    }

    if (image.width() > kMaxImagePixels || image.height() > kMaxImagePixels) {
        image = image.scaled(kMaxImagePixels, kMaxImagePixels,
                             Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    Page *page = m_document->pageAt(m_page);
    if (!page)
        return false;

    ImageItem *item = new ImageItem;
    item->image = image;
    item->fitInto(page->rect().adjusted(page->width * 0.05, page->height * 0.05,
                                        -page->width * 0.05, -page->height * 0.05));

    if (ownLayer) {
        Layer *photoLayer = page->addLayer(tr("Photo"));
        photoLayer->append(item);
        page->addLayer();
        m_undo->clear();
        Q_EMIT historyCleared();
        Q_EMIT layersChanged();
        Q_EMIT layerRequested(page->layerCount() - 1);
    } else {
        Layer *layer = page->ensureLayer(m_layer);
        if (layer->locked) {
            delete item;
            Q_EMIT error(tr("This layer is locked"));
            return false;
        }
        m_undo->push(new AddElementCommand(layer, item));
        Q_EMIT layersChanged();
    }

    Q_EMIT contentChanged(m_page);
    return true;
}

bool ImageTools::insertLatestScreenshot()
{
    const QString path = DocumentStore::latestScreenshot();
    if (path.isEmpty()) {
        Q_EMIT error(tr("No screenshot found"));
        return false;
    }
    return insertImage(path, true);
}

bool ImageTools::traceRegion(const QRectF &region, int threshold, bool invert, bool filled,
                             int speckSize, const QColor &color, qreal strokeWidth)
{
    Page *page = m_document->pageAt(m_page);
    ImageItem *image = imageUnder(page, region);
    if (!image) {
        Q_EMIT error(tr("No image under the selection"));
        return false;
    }

    const QRectF overlap = image->rect.intersected(region);
    if (overlap.isEmpty() || image->image.isNull()) {
        Q_EMIT error(tr("Nothing to trace"));
        return false;
    }

    const qreal sx = image->image.width() / image->rect.width();
    const qreal sy = image->image.height() / image->rect.height();
    const QRect source(qRound((overlap.left() - image->rect.left()) * sx),
                       qRound((overlap.top() - image->rect.top()) * sy),
                       qRound(overlap.width() * sx),
                       qRound(overlap.height() * sy));

    BitmapTracer::Options options;
    options.threshold = qBound(1, threshold, 254);
    options.invert = invert;
    options.filled = filled;
    options.speckSize = qMax(0, speckSize);

    BitmapTracer tracer;
    const QList<Stroke *> traced =
            tracer.trace(image->image, source, overlap, options, color, strokeWidth);
    if (traced.isEmpty()) {
        Q_EMIT error(tracer.errorString());
        return false;
    }

    Layer *layer = page->addLayer(tr("Trace"));
    for (int i = 0; i < traced.size(); ++i)
        layer->append(traced.at(i));

    m_undo->clear();
    Q_EMIT historyCleared();
    Q_EMIT layersChanged();
    Q_EMIT contentChanged(m_page);
    Q_EMIT layerRequested(page->layerCount() - 1);
    return true;
}
}
