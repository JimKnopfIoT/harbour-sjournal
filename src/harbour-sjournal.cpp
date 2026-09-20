
#include "app/documentcontroller.h"
#include "app/layermodel.h"
#include "app/nodeeditor.h"
#include "app/pathtools.h"
#include "app/styletools.h"
#include "app/selection.h"
#include "app/selectiontransform.h"
#include "app/toolsettings.h"
#include "app/notebookmodel.h"
#include "render/canvasitem.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>
#include <QScopedPointer>

#include <sailfishapp.h>

int main(int argc, char *argv[])
{
    QScopedPointer<QGuiApplication> app(SailfishApp::application(argc, argv));
    app->setApplicationName(QStringLiteral("harbour-sjournal"));
    app->setOrganizationName(QStringLiteral("harbour-sjournal"));

    qmlRegisterType<xn::DocumentController>("harbour.sjournal", 1, 0, "DocumentController");
    qmlRegisterType<xn::CanvasItem>("harbour.sjournal", 1, 0, "NoteCanvas");
    qmlRegisterType<xn::NotebookModel>("harbour.sjournal", 1, 0, "NotebookModel");
    qmlRegisterUncreatableType<xn::LayerModel>("harbour.sjournal", 1, 0, "LayerModel",
                                               QStringLiteral("Use DocumentController.layers"));
    qmlRegisterUncreatableType<xn::ToolSettings>("harbour.sjournal", 1, 0, "ToolSettings",
                                                 QStringLiteral("Use DocumentController.tools"));
    qmlRegisterUncreatableType<xn::Selection>("harbour.sjournal", 1, 0, "Selection",
                                              QStringLiteral("Use DocumentController.selection"));
    qmlRegisterUncreatableType<xn::NodeEditor>("harbour.sjournal", 1, 0, "NodeEditor",
                                               QStringLiteral("Use DocumentController.nodes"));
    qmlRegisterUncreatableType<xn::PathTools>("harbour.sjournal", 1, 0, "PathTools",
                                              QStringLiteral("Use DocumentController.paths"));
    qmlRegisterUncreatableType<xn::StyleTools>("harbour.sjournal", 1, 0, "StyleTools",
                                               QStringLiteral("Use DocumentController.style"));
    qmlRegisterUncreatableType<xn::SelectionTransform>("harbour.sjournal", 1, 0,
                                                       "SelectionTransform",
                                                       QStringLiteral("Use DocumentController.transform"));

    QScopedPointer<QQuickView> view(SailfishApp::createView());
    view->engine()->addImageProvider(QStringLiteral("notepreview"), new xn::PreviewImageProvider);
    view->setSource(SailfishApp::pathToMainQml());
    view->show();

    return app->exec();
}
