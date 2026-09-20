#include "app/exporter.h"

#include "io/svgexporter.h"
#include "model/document.h"
#include "model/layer.h"
#include "model/page.h"
#include "render/elementpainter.h"

#include <QDir>
#include <QFileInfo>
#include <QImageWriter>
#include <QMarginsF>
#include <QPainter>
#include <QPdfWriter>
#include <QRegExp>
#include <QStandardPaths>
#include <QUrl>

namespace xn {
Exporter::Exporter(const Document *document)
    : m_document(document)
{
}

QString Exporter::directoryFor(const QString &directory) const
{
    QString dir = directory;
    if (dir.startsWith(QLatin1String("file://")))
        dir = QUrl(dir).toLocalFile();
    if (dir.isEmpty()) {
        dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                + QStringLiteral("/SJournal/Export");
    }
    QDir().mkpath(dir);
    return dir;
}

QString Exporter::baseName() const
{
    QString base = QFileInfo(m_document->filePath).completeBaseName();
    if (base.isEmpty())
        base = m_document->title;
    if (base.isEmpty())
        base = QStringLiteral("note");
    base.replace(QRegExp(QStringLiteral("[^\\w\\-. ]")), QStringLiteral("_"));
    return base;
}

QString Exporter::pathFor(const QString &directory, int pageIndex, const QString &suffix) const
{
    if (m_document->pageCount() > 1) {
        return QStringLiteral("%1/%2-%3.%4").arg(directory, baseName())
                .arg(pageIndex + 1, 2, 10, QLatin1Char('0')).arg(suffix);
    }
    return QStringLiteral("%1/%2.%3").arg(directory, baseName(), suffix);
}

QString Exporter::exportSvg(int pageIndex, const QString &directory, bool withBackground)
{
    const Page *page = m_document->pageAt(pageIndex);
    if (!page) {
        m_error = QStringLiteral("No such page");
        return QString();
    }

    const QString path = pathFor(directoryFor(directory), pageIndex, QStringLiteral("svg"));

    SvgExporter exporter;
    if (!exporter.exportPage(path, page, withBackground)) {
        m_error = exporter.errorString();
        return QString();
    }
    m_error.clear();
    return path;
}

QStringList Exporter::exportSvgAllPages(const QString &directory, bool withBackground)
{
    QStringList written;
    const QString dir = directoryFor(directory);
    for (int i = 0; i < m_document->pageCount(); ++i) {
        const QString path = exportSvg(i, dir, withBackground);
        if (path.isEmpty())
            return written;
        written.append(path);
    }
    return written;
}

// Asked at runtime: an image plugin may simply not be on the device.
QStringList Exporter::imageFormats()
{
    QStringList wanted;
    wanted << QStringLiteral("png") << QStringLiteral("jpg") << QStringLiteral("webp");

    QStringList out;
    const QList<QByteArray> writable = QImageWriter::supportedImageFormats();
    for (int i = 0; i < wanted.size(); ++i) {
        const QByteArray name = wanted.at(i).toLatin1();
        if (writable.contains(name) || (name == "jpg" && writable.contains("jpeg")))
            out << wanted.at(i);
    }
    return out;
}

QString Exporter::exportImage(int pageIndex, const QString &format, int maxPixels,
                              const QString &directory, bool withBackground, int quality)
{
    const QString suffix = format.toLower();
    const QImage image = renderPage(pageIndex, QSize(maxPixels, maxPixels), withBackground);
    if (image.isNull()) {
        m_error = QStringLiteral("Nothing to render");
        return QString();
    }

    QImage toWrite = image;
    if (suffix == QStringLiteral("jpg") || suffix == QStringLiteral("jpeg")) {
        // JPEG has no alpha; without this a transparent export comes out black.
        QImage flat(image.size(), QImage::Format_RGB32);
        flat.fill(Qt::white);
        QPainter p(&flat);
        p.drawImage(0, 0, image);
        p.end();
        toWrite = flat;
    }

    const QString path = pathFor(directoryFor(directory), pageIndex, suffix);
    QImageWriter writer(path, suffix.toLatin1());
    if (quality > 0)
        writer.setQuality(quality);
    if (!writer.write(toWrite)) {
        m_error = QStringLiteral("Cannot write %1: %2").arg(path, writer.errorString());
        return QString();
    }
    m_error.clear();
    return path;
}

QString Exporter::exportPdf(const QString &directory, bool withBackground)
{
    if (m_document->pageCount() == 0) {
        m_error = QStringLiteral("Nothing to render");
        return QString();
    }

    const QString path = QStringLiteral("%1/%2.pdf")
            .arg(directoryFor(directory), baseName());

    QPdfWriter writer(path);
    writer.setCreator(QStringLiteral("SJournal"));
    writer.setTitle(m_document->title);
    // The document is already in PDF points, so one unit is one point.
    writer.setResolution(72);

    QPainter painter;
    for (int i = 0; i < m_document->pageCount(); ++i) {
        const Page *page = m_document->pageAt(i);
        if (!page)
            continue;

        writer.setPageSizeMM(QSizeF(page->width * 25.4 / 72.0, page->height * 25.4 / 72.0));
        writer.setPageMargins(QMarginsF(0, 0, 0, 0));

        if (i == 0) {
            if (!painter.begin(&writer)) {
                m_error = QStringLiteral("Cannot write %1").arg(path);
                return QString();
            }
        } else if (!writer.newPage()) {
            break;
        }

        painter.setRenderHint(QPainter::Antialiasing, true);
        if (withBackground)
            ElementPainter::drawBackground(&painter, page);

        for (int l = 0; l < page->layers.size(); ++l) {
            const Layer *layer = page->layers.at(l);
            if (!layer->visible)
                continue;
            painter.save();
            if (layer->opacity < 1.0)
                painter.setOpacity(layer->opacity);
            for (int e = 0; e < layer->elements.size(); ++e)
                ElementPainter::drawElement(&painter, layer->elements.at(e), false);
            painter.restore();
        }
    }
    painter.end();

    m_error.clear();
    return path;
}

QString Exporter::exportPng(int pageIndex, int maxPixels, const QString &directory,
                            bool withBackground)
{
    const QImage image = renderPage(pageIndex, QSize(maxPixels, maxPixels), withBackground);
    if (image.isNull()) {
        m_error = QStringLiteral("Nothing to render");
        return QString();
    }

    const QString path = pathFor(directoryFor(directory), pageIndex, QStringLiteral("png"));
    if (!image.save(path, "PNG")) {
        m_error = QStringLiteral("Cannot write %1").arg(path);
        return QString();
    }
    m_error.clear();
    return path;
}

QImage Exporter::renderPage(int index, const QSize &maxSize, bool withBackground) const
{
    const Page *page = m_document->pageAt(index);
    if (!page || page->width <= 0 || page->height <= 0)
        return QImage();

    qreal scale = qMin(maxSize.width() / page->width, maxSize.height() / page->height);
    if (scale <= 0)
        scale = 1;

    QImage image(qMax(1, qRound(page->width * scale)), qMax(1, qRound(page->height * scale)),
                 QImage::Format_ARGB32_Premultiplied);
    image.fill(withBackground ? Qt::white : Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.scale(scale, scale);

    if (withBackground)
        ElementPainter::drawBackground(&painter, page);

    for (int i = 0; i < page->layers.size(); ++i) {
        const Layer *layer = page->layers.at(i);
        if (!layer->visible)
            continue;
        painter.save();
        if (layer->opacity < 1.0)
            painter.setOpacity(layer->opacity);
        for (int j = 0; j < layer->elements.size(); ++j)
            ElementPainter::drawElement(&painter, layer->elements.at(j));
        painter.restore();
    }
    painter.end();
    return image;
}
}
