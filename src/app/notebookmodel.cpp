#include "app/notebookmodel.h"

#include "app/documentcontroller.h"
#include "io/gzfile.h"
#include "io/xoppreader.h"
#include "io/xoppwriter.h"
#include "model/document.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QStandardPaths>
#include <QUrl>
#include <QXmlStreamReader>

namespace xn {
NotebookModel::NotebookModel(QObject *parent)
    : QAbstractListModel(parent)
{
    const QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(cacheDir);
    m_cachePath = cacheDir + QStringLiteral("/notebook-index.json");

    loadCache();
    refresh();
}

QString NotebookModel::directory() const
{
    return DocumentController::notesDirectory();
}

int NotebookModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_visible.size();
}

QVariant NotebookModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visible.size())
        return QVariant();

    const Entry &e = m_all.at(m_visible.at(index.row()));

    switch (role) {
    case TitleRole:
        return e.title;
    case FilePathRole:
        return e.path;
    case FileNameRole:
        return QFileInfo(e.path).fileName();
    case ModifiedRole:
        return e.modified;
    case ModifiedTextRole: {
        const QDateTime now = QDateTime::currentDateTime();
        if (e.modified.date() == now.date())
            return e.modified.toString(QStringLiteral("hh:mm"));
        if (e.modified.daysTo(now) < 7)
            return QLocale().toString(e.modified, QStringLiteral("dddd hh:mm"));
        return QLocale().toString(e.modified, QLocale::ShortFormat);
    }
    case PageCountRole:
        return e.pageCount;
    case PreviewRole:
        return QStringLiteral("image://notepreview/%1?v=%2")
                .arg(e.path).arg(e.modified.toMSecsSinceEpoch());
    case SizeTextRole:
        return e.size < 1024 * 1024
                ? QStringLiteral("%1 kB").arg(qMax<qint64>(1, e.size / 1024))
                : QStringLiteral("%1 MB").arg(QString::number(e.size / 1048576.0, 'f', 1));
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> NotebookModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TitleRole] = "title";
    roles[FilePathRole] = "filePath";
    roles[FileNameRole] = "fileName";
    roles[ModifiedRole] = "modified";
    roles[ModifiedTextRole] = "modifiedText";
    roles[PageCountRole] = "pageCount";
    roles[PreviewRole] = "preview";
    roles[SizeTextRole] = "sizeText";
    return roles;
}

void NotebookModel::setFilter(const QString &f)
{
    if (m_filter == f)
        return;
    m_filter = f;
    Q_EMIT filterChanged();
    rebuild();
}

bool NotebookModel::matches(const Entry &e) const
{
    if (m_filter.trimmed().isEmpty())
        return true;
    const QString needle = m_filter.trimmed();
    return e.title.contains(needle, Qt::CaseInsensitive)
            || e.text.contains(needle, Qt::CaseInsensitive)
            || QFileInfo(e.path).fileName().contains(needle, Qt::CaseInsensitive);
}

void NotebookModel::rebuild()
{
    beginResetModel();
    m_visible.clear();
    for (int i = 0; i < m_all.size(); ++i) {
        if (matches(m_all.at(i)))
            m_visible.append(i);
    }
    endResetModel();
    Q_EMIT countChanged();
}

void NotebookModel::refresh()
{
    QHash<QString, Entry> cached;
    for (int i = 0; i < m_all.size(); ++i)
        cached.insert(m_all.at(i).path, m_all.at(i));

    QDir dir(directory());
    QStringList filters;
    filters << QStringLiteral("*.xopp") << QStringLiteral("*.xoj");
    const QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Time);

    QVector<Entry> entries;
    entries.reserve(files.size());

    for (int i = 0; i < files.size(); ++i) {
        const QFileInfo &fi = files.at(i);

        Entry e;
        e.path = fi.absoluteFilePath();
        e.modified = fi.lastModified();
        e.size = fi.size();

        const QHash<QString, Entry>::const_iterator it = cached.constFind(e.path);
        if (it != cached.constEnd() && it->modified == e.modified) {
            e.title = it->title;
            e.text = it->text;
            e.pageCount = it->pageCount;
        } else {
            QString title;
            QString text;
            int pages = 0;
            if (XoppReader::readSummary(e.path, &title, &pages, &text)) {
                e.title = title;
                e.text = text;
                e.pageCount = pages;
            } else {
                e.title = fi.completeBaseName();
                e.pageCount = 0;
            }
        }
        entries.append(e);
    }

    m_all = entries;
    saveCache();
    rebuild();
}

QString NotebookModel::filePathAt(int row) const
{
    if (row < 0 || row >= m_visible.size())
        return QString();
    return m_all.at(m_visible.at(row)).path;
}

bool NotebookModel::remove(int row)
{
    const QString path = filePathAt(row);
    if (path.isEmpty())
        return false;
    if (!QFile::remove(path))
        return false;
    refresh();
    return true;
}

bool NotebookModel::rename(int row, const QString &title)
{
    const QString path = filePathAt(row);
    if (path.isEmpty() || title.trimmed().isEmpty())
        return false;

    Document doc;
    XoppReader reader;
    if (!reader.read(path, &doc))
        return false;
    doc.title = title.trimmed();

    XoppWriter writer;
    if (!writer.write(path, &doc))
        return false;

    refresh();
    return true;
}

void NotebookModel::loadCache()
{
    QFile f(m_cachePath);
    if (!f.open(QIODevice::ReadOnly))
        return;

    const QJsonDocument json = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!json.isArray())
        return;

    const QJsonArray array = json.array();
    m_all.clear();
    m_all.reserve(array.size());
    for (int i = 0; i < array.size(); ++i) {
        const QJsonObject o = array.at(i).toObject();
        Entry e;
        e.path = o.value(QStringLiteral("path")).toString();
        e.title = o.value(QStringLiteral("title")).toString();
        e.text = o.value(QStringLiteral("text")).toString();
        e.pageCount = o.value(QStringLiteral("pages")).toInt();
        e.size = qint64(o.value(QStringLiteral("size")).toDouble());
        e.modified = QDateTime::fromMSecsSinceEpoch(
                    qint64(o.value(QStringLiteral("mtime")).toDouble()));
        if (!e.path.isEmpty())
            m_all.append(e);
    }
}

void NotebookModel::saveCache() const
{
    QJsonArray array;
    for (int i = 0; i < m_all.size(); ++i) {
        const Entry &e = m_all.at(i);
        QJsonObject o;
        o.insert(QStringLiteral("path"), e.path);
        o.insert(QStringLiteral("title"), e.title);
        o.insert(QStringLiteral("text"), e.text.left(4000));
        o.insert(QStringLiteral("pages"), e.pageCount);
        o.insert(QStringLiteral("size"), double(e.size));
        o.insert(QStringLiteral("mtime"), double(e.modified.toMSecsSinceEpoch()));
        array.append(o);
    }

    QFile f(m_cachePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    f.write(QJsonDocument(array).toJson(QJsonDocument::Compact));
    f.close();
}

PreviewImageProvider::PreviewImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage PreviewImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    QString path = id;
    const int q = path.indexOf(QLatin1Char('?'));
    if (q >= 0)
        path = path.left(q);
    path = QUrl::fromPercentEncoding(path.toUtf8());

    const QByteArray data = gz::readAll(path);
    if (data.isEmpty())
        return QImage();

    QXmlStreamReader xml(data);
    QImage image;
    while (!xml.atEnd()) {
        if (xml.readNext() != QXmlStreamReader::StartElement)
            continue;
        if (xml.name() == QLatin1String("preview")) {
            const QByteArray png = QByteArray::fromBase64(xml.readElementText().toLatin1());
            image.loadFromData(png);
            break;
        }
        if (xml.name() == QLatin1String("page"))
            break;
    }

    if (image.isNull())
        return QImage();

    if (requestedSize.isValid() && !requestedSize.isEmpty()) {
        image = image.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    if (size)
        *size = image.size();
    return image;
}
}
