#ifndef XN_DOCUMENTSTORE_H
#define XN_DOCUMENTSTORE_H

#include <QImage>
#include <QString>

namespace xn {
class Document;

class DocumentStore
{
public:
    static QString notesDirectory();
    static QString homeDirectory();
    static QString picturesDirectory();
    static QString documentsDirectory();
    static QString downloadsDirectory();
    static QString screenshotsDirectory();
    static QString latestScreenshot();

    static QString localPath(const QString &pathOrUrl);
    static QString uniqueNotePath(const QString &title);

    bool load(const QString &path, Document *document);
    bool save(const QString &path, const Document *document, const QImage &preview);

    QString errorString() const { return m_error; }

private:
    QString m_error;
};
}

#endif
