#include "app/documentstore.h"

#include "io/xoppreader.h"
#include "io/xoppwriter.h"
#include "model/document.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <QStandardPaths>
#include <QUrl>

namespace xn {
QString DocumentStore::notesDirectory()
{
    const QString dir = documentsDirectory() + QStringLiteral("/SJournal");
    QDir().mkpath(dir);
    return dir;
}

QString DocumentStore::homeDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
}

QString DocumentStore::picturesDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
}

QString DocumentStore::documentsDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
}

QString DocumentStore::downloadsDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
}

QString DocumentStore::screenshotsDirectory()
{
    return picturesDirectory() + QStringLiteral("/Screenshots");
}

QString DocumentStore::latestScreenshot()
{
    QStringList filters;
    filters << QStringLiteral("*.png") << QStringLiteral("*.jpg") << QStringLiteral("*.jpeg");
    const QFileInfoList files =
            QDir(screenshotsDirectory()).entryInfoList(filters, QDir::Files, QDir::Time);
    return files.isEmpty() ? QString() : files.first().absoluteFilePath();
}

QString DocumentStore::localPath(const QString &pathOrUrl)
{
    return pathOrUrl.startsWith(QLatin1String("file://"))
            ? QUrl(pathOrUrl).toLocalFile() : pathOrUrl;
}

QString DocumentStore::uniqueNotePath(const QString &title)
{
    QString base = title;
    base.replace(QRegExp(QStringLiteral("[^\\w\\-. ]")), QStringLiteral("_"));
    base = base.trimmed();
    if (base.isEmpty())
        base = QStringLiteral("note");

    QString path = notesDirectory() + QLatin1Char('/') + base + QStringLiteral(".xopp");
    int n = 2;
    while (QFile::exists(path)) {
        path = notesDirectory() + QLatin1Char('/') + base
                + QStringLiteral(" %1").arg(n++) + QStringLiteral(".xopp");
    }
    return path;
}

bool DocumentStore::load(const QString &path, Document *document)
{
    XoppReader reader;
    if (!reader.read(localPath(path), document)) {
        m_error = reader.errorString();
        return false;
    }
    m_error.clear();
    return true;
}

bool DocumentStore::save(const QString &path, const Document *document, const QImage &preview)
{
    XoppWriter writer;
    if (!writer.write(localPath(path), document, preview)) {
        m_error = writer.errorString();
        return false;
    }
    m_error.clear();
    return true;
}
}
