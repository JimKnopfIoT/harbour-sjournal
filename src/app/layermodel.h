#ifndef XN_LAYERMODEL_H
#define XN_LAYERMODEL_H

#include <QAbstractListModel>

namespace xn {
class DocumentController;

class LayerModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int visibleCount READ visibleCount NOTIFY countChanged)
    Q_PROPERTY(int currentRow READ currentRow WRITE setCurrentRow NOTIFY currentRowChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        VisibleRole,
        LockedRole,
        ElementCountRole,
        CurrentRole
    };

    explicit LayerModel(QObject *parent = 0);

    void setController(DocumentController *controller);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;

    int visibleCount() const;

    int currentRow() const;
    void setCurrentRow(int row);

    Q_INVOKABLE void toggle(int row);
    Q_INVOKABLE void toggleLock(int row);
    Q_INVOKABLE int moveSelectionHere(int row);
    Q_INVOKABLE void select(int row);
    Q_INVOKABLE void rename(int row, const QString &name);
    Q_INVOKABLE void addLayer();
    Q_INVOKABLE void remove(int row);
    Q_INVOKABLE void clear(int row);

    Q_INVOKABLE void raise(int row);
    Q_INVOKABLE void lower(int row);

    Q_INVOKABLE void showAll();
    Q_INVOKABLE void hideAll();

    Q_INVOKABLE void isolate(int row);

Q_SIGNALS:
    void countChanged();
    void currentRowChanged();

private Q_SLOTS:
    void reload();

private:
    int toLayerIndex(int row) const;

    DocumentController *m_controller;
};
}

#endif
