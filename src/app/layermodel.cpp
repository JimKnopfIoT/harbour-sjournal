#include "app/layermodel.h"

#include "app/documentcontroller.h"
#include "app/selection.h"

namespace xn {
LayerModel::LayerModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_controller(0)
{
}

void LayerModel::setController(DocumentController *controller)
{
    if (m_controller == controller)
        return;

    if (m_controller)
        m_controller->disconnect(this);

    m_controller = controller;

    if (m_controller) {
        connect(m_controller, SIGNAL(layersChanged()), this, SLOT(reload()));
        connect(m_controller, SIGNAL(currentPageChanged()), this, SLOT(reload()));
        connect(m_controller, SIGNAL(documentReplaced()), this, SLOT(reload()));
        connect(m_controller, SIGNAL(currentLayerChanged()), this, SLOT(reload()));
    }
    reload();
}

void LayerModel::reload()
{
    beginResetModel();
    endResetModel();
    Q_EMIT countChanged();
    Q_EMIT currentRowChanged();
}

int LayerModel::toLayerIndex(int row) const
{
    return rowCount() - 1 - row;
}

int LayerModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || !m_controller)
        return 0;
    return m_controller->layerCount();
}

QVariant LayerModel::data(const QModelIndex &index, int role) const
{
    if (!m_controller || !index.isValid() || index.row() < 0 || index.row() >= rowCount())
        return QVariant();

    const int layer = toLayerIndex(index.row());

    switch (role) {
    case NameRole:
        return m_controller->layerName(layer);
    case VisibleRole:
        return m_controller->layerVisible(layer);
    case LockedRole:
        return m_controller->layerLocked(layer);
    case ElementCountRole:
        return m_controller->layerElementCount(layer);
    case CurrentRole:
        return m_controller->currentLayer() == layer;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> LayerModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "layerName";
    roles[VisibleRole] = "layerVisible";
    roles[LockedRole] = "layerLocked";
    roles[ElementCountRole] = "elementCount";
    roles[CurrentRole] = "isCurrent";
    return roles;
}

int LayerModel::visibleCount() const
{
    if (!m_controller)
        return 0;
    int n = 0;
    for (int i = 0; i < m_controller->layerCount(); ++i) {
        if (m_controller->layerVisible(i))
            ++n;
    }
    return n;
}

int LayerModel::currentRow() const
{
    if (!m_controller || rowCount() == 0)
        return 0;
    return rowCount() - 1 - m_controller->currentLayer();
}

void LayerModel::setCurrentRow(int row)
{
    select(row);
}

void LayerModel::toggle(int row)
{
    if (!m_controller || row < 0 || row >= rowCount())
        return;
    const int layer = toLayerIndex(row);
    m_controller->setLayerVisible(layer, !m_controller->layerVisible(layer));
}

void LayerModel::toggleLock(int row)
{
    if (!m_controller || row < 0 || row >= rowCount())
        return;
    const int layer = toLayerIndex(row);
    m_controller->setLayerLocked(layer, !m_controller->layerLocked(layer));
}

int LayerModel::moveSelectionHere(int row)
{
    if (!m_controller || row < 0 || row >= rowCount())
        return 0;
    return m_controller->selection()->moveToLayer(toLayerIndex(row));
}

void LayerModel::select(int row)
{
    if (!m_controller || row < 0 || row >= rowCount())
        return;
    m_controller->setCurrentLayer(toLayerIndex(row));
}

void LayerModel::rename(int row, const QString &name)
{
    if (!m_controller || row < 0 || row >= rowCount())
        return;
    m_controller->setLayerName(toLayerIndex(row), name);
}

void LayerModel::addLayer()
{
    if (m_controller)
        m_controller->addLayer();
}

void LayerModel::remove(int row)
{
    if (!m_controller || row < 0 || row >= rowCount())
        return;
    m_controller->removeLayer(toLayerIndex(row));
}

void LayerModel::clear(int row)
{
    if (!m_controller || row < 0 || row >= rowCount())
        return;
    m_controller->clearLayer(toLayerIndex(row));
}

void LayerModel::raise(int row)
{
    if (!m_controller || row <= 0 || row >= rowCount())
        return;
    const int from = toLayerIndex(row);
    m_controller->moveLayer(from, from + 1);
}

void LayerModel::lower(int row)
{
    if (!m_controller || row < 0 || row >= rowCount() - 1)
        return;
    const int from = toLayerIndex(row);
    m_controller->moveLayer(from, from - 1);
}

void LayerModel::showAll()
{
    if (!m_controller)
        return;
    for (int i = 0; i < m_controller->layerCount(); ++i)
        m_controller->setLayerVisible(i, true);
}

void LayerModel::hideAll()
{
    if (!m_controller)
        return;
    for (int i = 0; i < m_controller->layerCount(); ++i)
        m_controller->setLayerVisible(i, i == m_controller->currentLayer());
}

void LayerModel::isolate(int row)
{
    if (!m_controller || row < 0 || row >= rowCount())
        return;
    const int keep = toLayerIndex(row);
    for (int i = 0; i < m_controller->layerCount(); ++i)
        m_controller->setLayerVisible(i, i == keep);
    m_controller->setCurrentLayer(keep);
}
}
