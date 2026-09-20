#ifndef XN_DOCUMENTCONTROLLER_H
#define XN_DOCUMENTCONTROLLER_H

#include <QObject>
#include <QPointF>
#include <QString>
#include <QStringList>
#include <QVector>

namespace xn {
class Document;
class Page;
class Layer;
class Element;
class Stroke;
class TextItem;
class UndoStack;
class LayerModel;
class ToolSettings;
class Selection;
class ImageTools;
class NodeEditor;
class SelectionTransform;
class PathTools;
class StyleTools;

class DocumentController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString filePath READ filePath NOTIFY filePathChanged)
    Q_PROPERTY(bool modified READ isModified NOTIFY modifiedChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)

    Q_PROPERTY(int pageCount READ pageCount NOTIFY pageCountChanged)
    Q_PROPERTY(int currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged)

    Q_PROPERTY(int layerCount READ layerCount NOTIFY layersChanged)
    Q_PROPERTY(int currentLayer READ currentLayer WRITE setCurrentLayer NOTIFY currentLayerChanged)

    Q_PROPERTY(xn::ToolSettings *tools READ tools CONSTANT)
    Q_PROPERTY(xn::Selection *selection READ selection CONSTANT)
    Q_PROPERTY(xn::LayerModel *layers READ layers CONSTANT)
    Q_PROPERTY(xn::NodeEditor *nodes READ nodeEditor CONSTANT)
    Q_PROPERTY(xn::SelectionTransform *transform READ transform CONSTANT)
    Q_PROPERTY(xn::PathTools *paths READ pathTools CONSTANT)
    Q_PROPERTY(xn::StyleTools *style READ styleTools CONSTANT)

    Q_PROPERTY(QString pendingText READ pendingText NOTIFY textEditRequested)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged)

public:
    explicit DocumentController(QObject *parent = 0);
    ~DocumentController();

    Q_INVOKABLE static QString notesDirectory();
    Q_INVOKABLE static QString homeDirectory();
    Q_INVOKABLE static QString picturesDirectory();
    Q_INVOKABLE static QString documentsDirectory();
    Q_INVOKABLE static QString downloadsDirectory();
    Q_INVOKABLE static QString screenshotsDirectory();
    Q_INVOKABLE static QString latestScreenshotName();

    Q_INVOKABLE bool createNote(const QString &title, qreal pageWidth = 0, qreal pageHeight = 0);
    Q_INVOKABLE bool open(const QString &path);
    Q_INVOKABLE bool save();
    Q_INVOKABLE bool saveAs(const QString &path);
    Q_INVOKABLE void close();

    Q_INVOKABLE void addPage();
    Q_INVOKABLE void duplicatePage(int index);
    Q_INVOKABLE void removePage(int index);
    Q_INVOKABLE qreal pageWidth(int index) const;
    Q_INVOKABLE qreal pageHeight(int index) const;
    Q_INVOKABLE void setPageSize(int index, qreal width, qreal height);
    Q_INVOKABLE QString pageBackgroundStyle(int index) const;
    Q_INVOKABLE void setPageBackgroundStyle(int index, const QString &style);

    Q_INVOKABLE QString layerName(int index) const;
    Q_INVOKABLE void setLayerName(int index, const QString &name);
    Q_INVOKABLE bool layerVisible(int index) const;
    Q_INVOKABLE void setLayerVisible(int index, bool visible);
    Q_INVOKABLE bool layerLocked(int index) const;
    Q_INVOKABLE void setLayerLocked(int index, bool locked);
    Q_INVOKABLE int layerElementCount(int index) const;
    Q_INVOKABLE void addLayer();
    Q_INVOKABLE void removeLayer(int index);
    Q_INVOKABLE void moveLayer(int from, int to);
    Q_INVOKABLE void clearLayer(int index);

    Q_INVOKABLE bool insertImage(const QString &fileUrl, bool ownLayer = true);
    Q_INVOKABLE bool insertLatestScreenshot();
    Q_INVOKABLE bool traceSelection(int threshold, bool invert, bool filled, int speckSize);

    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();

    Q_INVOKABLE QString exportSvg(int pageIndex, const QString &directory = QString(),
                                  bool withBackground = true);
    Q_INVOKABLE QStringList exportSvgAllPages(const QString &directory = QString(),
                                              bool withBackground = true);
    Q_INVOKABLE QStringList exportImageFormats() const;
    Q_INVOKABLE QString exportImage(int pageIndex, const QString &format, int maxPixels = 2048,
                                    const QString &directory = QString(),
                                    bool withBackground = true, int quality = 92);
    Q_INVOKABLE QString exportPdf(const QString &directory = QString(),
                                  bool withBackground = true);

    Q_INVOKABLE QString exportPng(int pageIndex, int maxPixels = 2048,
                                  const QString &directory = QString(),
                                  bool withBackground = true);

    QString title() const;
    void setTitle(const QString &t);
    QString filePath() const;
    bool isModified() const { return m_modified; }
    void setModified(bool m);
    QString errorString() const { return m_error; }
    void setError(const QString &e);

    int pageCount() const;
    int currentPage() const { return m_currentPage; }
    void setCurrentPage(int index);
    int layerCount() const;
    int currentLayer() const { return m_currentLayer; }
    void setCurrentLayer(int index);

    ToolSettings *tools() const { return m_tools; }
    Selection *selection() const { return m_selection; }
    LayerModel *layers() const { return m_layers; }
    NodeEditor *nodeEditor() const { return m_nodeEditor; }
    SelectionTransform *transform() const { return m_transform; }
    PathTools *pathTools() const { return m_pathTools; }
    StyleTools *styleTools() const { return m_styleTools; }

    bool canUndo() const;
    bool canRedo() const;

    Document *document() const { return m_document; }
    Page *page(int index) const;
    Layer *drawingLayer(int pageIndex) const;

    void commitStroke(int pageIndex, Stroke *stroke);
    void commitElement(int pageIndex, Element *element);

    void editTextAt(int pageIndex, const QPointF &pos);
    Q_INVOKABLE void commitText(const QString &text);
    Q_INVOKABLE void cancelTextEdit();
    QString pendingText() const { return m_pendingText; }
    void beginErase(int pageIndex);
    bool eraseAt(int pageIndex, const QPointF &pos, qreal radius);
    void endErase();

    QPointF snapPoint(int pageIndex, const QPointF &pos, qreal radius) const;

Q_SIGNALS:
    void titleChanged();
    void textEditRequested();
    void filePathChanged();
    void modifiedChanged();
    void errorStringChanged();
    void pageCountChanged();
    void currentPageChanged();
    void layersChanged();
    void currentLayerChanged();
    void historyChanged();
    void pageContentChanged(int pageIndex);
    void documentReplaced();
    void saved(const QString &path);

private Q_SLOTS:
    void markModified();
    void onToolChanged();
    void applyTextFormatToSelection();
    void syncTextFormatFromSelection();

private:
    Layer *layerAt(int pageIndex, int layerIndex) const;
    void connectParts();
    void resetAfterStructuralChange();

    Document *m_document;
    UndoStack *m_undo;
    LayerModel *m_layers;
    ToolSettings *m_tools;
    Selection *m_selection;
    ImageTools *m_images;
    NodeEditor *m_nodeEditor;
    SelectionTransform *m_transform;
    PathTools *m_pathTools;
    StyleTools *m_styleTools;

    int m_currentPage;
    int m_currentLayer;
    bool m_modified;
    QString m_error;

    QVector<Element *> m_erased;
    QVector<int> m_erasedIndexes;
    int m_erasePage;

    TextItem *m_editingText;
    int m_textPage;
    QPointF m_textPos;
    QString m_pendingText;
    bool m_syncingTextFormat;
};
}

#endif
