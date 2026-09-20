#ifndef XN_NOTEBOOKMODEL_H
#define XN_NOTEBOOKMODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QQuickImageProvider>
#include <QString>
#include <QVector>

namespace xn {
class NotebookModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(QString directory READ directory NOTIFY directoryChanged)

public:
    enum Roles {
        TitleRole = Qt::UserRole + 1,
        FilePathRole,
        FileNameRole,
        ModifiedRole,
        ModifiedTextRole,
        PageCountRole,
        PreviewRole,
        SizeTextRole
    };

    explicit NotebookModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;

    QString filter() const { return m_filter; }
    void setFilter(const QString &f);

    QString directory() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE QString filePathAt(int row) const;
    Q_INVOKABLE bool remove(int row);
    Q_INVOKABLE bool rename(int row, const QString &title);

Q_SIGNALS:
    void filterChanged();
    void countChanged();
    void directoryChanged();

private:
    struct Entry
    {
        Entry(): pageCount(0), size(0) {}

        QString path;
        QString title;
        QString text;
        QDateTime modified;
        int pageCount;
        qint64 size;
    };

    void loadCache();
    void saveCache() const;
    void rebuild();
    bool matches(const Entry &e) const;

    QVector<Entry> m_all;
    QVector<int> m_visible;
    QString m_filter;
    QString m_cachePath;
};

class PreviewImageProvider : public QQuickImageProvider
{
public:
    PreviewImageProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);
};
}

#endif
