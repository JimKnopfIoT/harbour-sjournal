#ifndef XN_EXPORTER_H
#define XN_EXPORTER_H

#include <QImage>
#include <QSize>
#include <QString>
#include <QStringList>

namespace xn {
class Document;

class Exporter
{
public:
    explicit Exporter(const Document *document);

    QString exportSvg(int pageIndex, const QString &directory, bool withBackground);
    QStringList exportSvgAllPages(const QString &directory, bool withBackground);
    QString exportPng(int pageIndex, int maxPixels, const QString &directory, bool withBackground);
    QString exportImage(int pageIndex, const QString &format, int maxPixels,
                        const QString &directory, bool withBackground, int quality);
    QString exportPdf(const QString &directory, bool withBackground);

    static QStringList imageFormats();

    QImage renderPage(int index, const QSize &maxSize, bool withBackground) const;

    QString errorString() const { return m_error; }

private:
    QString directoryFor(const QString &directory) const;
    QString baseName() const;
    QString pathFor(const QString &directory, int pageIndex, const QString &suffix) const;

    const Document *m_document;
    QString m_error;
};
}

#endif
