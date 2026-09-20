#ifndef XN_TOOLSETTINGS_H
#define XN_TOOLSETTINGS_H

#include <QColor>
#include <QObject>
#include <QString>

namespace xn {
class ToolSettings : public QObject
{
    Q_OBJECT

    Q_PROPERTY(Tool tool READ tool WRITE setTool NOTIFY toolChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(qreal penWidth READ penWidth WRITE setPenWidth NOTIFY penWidthChanged)
    Q_PROPERTY(bool dynamicWidth READ dynamicWidth WRITE setDynamicWidth NOTIFY dynamicWidthChanged)
    Q_PROPERTY(qreal fontSize READ fontSize WRITE setFontSize NOTIFY fontSizeChanged)
    Q_PROPERTY(bool fontBold READ fontBold WRITE setFontBold NOTIFY fontFormChanged)
    Q_PROPERTY(bool fontItalic READ fontItalic WRITE setFontItalic NOTIFY fontFormChanged)
    Q_PROPERTY(bool fontUnderline READ fontUnderline WRITE setFontUnderline NOTIFY fontFormChanged)
    Q_PROPERTY(int textStyle READ textStyle WRITE setTextStyle NOTIFY textStyleChanged)
    Q_PROPERTY(bool shapeRecognition READ shapeRecognition WRITE setShapeRecognition NOTIFY shapeRecognitionChanged)
    Q_PROPERTY(bool outlineView READ outlineView WRITE setOutlineView NOTIFY outlineViewChanged)

    Q_PROPERTY(ShapeKind shapeKind READ shapeKind WRITE setShapeKind NOTIFY shapeKindChanged)
    Q_PROPERTY(int polygonCorners READ polygonCorners WRITE setPolygonCorners NOTIFY polygonCornersChanged)
    Q_PROPERTY(bool equalSides READ equalSides WRITE setEqualSides NOTIFY equalSidesChanged)

    Q_PROPERTY(bool fillEnabled READ fillEnabled WRITE setFillEnabled NOTIFY fillChanged)
    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor NOTIFY fillChanged)
    Q_PROPERTY(int fillAlpha READ fillAlpha WRITE setFillAlpha NOTIFY fillChanged)
    Q_PROPERTY(QString lineStyle READ lineStyle WRITE setLineStyle NOTIFY lineStyleChanged)

    Q_PROPERTY(bool snapSizes READ snapSizes WRITE setSnapSizes NOTIFY snapSizesChanged)
    Q_PROPERTY(bool snapToPoints READ snapToPoints WRITE setSnapToPoints NOTIFY snapToPointsChanged)

    Q_PROPERTY(SelectShape selectShape READ selectShape WRITE setSelectShape NOTIFY selectShapeChanged)

    Q_PROPERTY(QString hintText READ hintText NOTIFY hintTextChanged)
    Q_PROPERTY(bool pressureSeen READ pressureSeen NOTIFY pressureSeenChanged)
    Q_PROPERTY(QString pressureRange READ pressureRange NOTIFY pressureSeenChanged)

public:
    enum Tool { PenTool, HighlighterTool, EraserTool, MoveTool, ShapeTool, SelectTool,
                BezierTool, BrushTool, TextTool, NodeTool };
    Q_ENUM(Tool)

    enum SelectShape { RectSelect, LassoSelect, PointSelect };
    Q_ENUM(SelectShape)

    enum ShapeKind { EllipseKind, RectKind, PolygonKind, LineKind, ArrowKind, StarKind,
                     PolylineKind, SplineKind, ClosedSplineKind };
    Q_ENUM(ShapeKind)

    explicit ToolSettings(QObject *parent = 0);

    Tool tool() const { return m_tool; }
    void setTool(Tool t);
    QColor color() const { return m_color; }
    void setColor(const QColor &c);
    qreal penWidth() const { return m_penWidth; }
    void setPenWidth(qreal w);
    bool dynamicWidth() const { return m_dynamicWidth; }
    void setDynamicWidth(bool on);
    qreal fontSize() const { return m_fontSize; }
    void setFontSize(qreal size);

    bool fontBold() const { return m_fontBold; }
    void setFontBold(bool on);
    bool fontItalic() const { return m_fontItalic; }
    void setFontItalic(bool on);
    bool fontUnderline() const { return m_fontUnderline; }
    void setFontUnderline(bool on);
    int textStyle() const { return m_textStyle; }
    void setTextStyle(int style);
    bool shapeRecognition() const { return m_shapeRecognition; }
    void setShapeRecognition(bool on);
    bool outlineView() const { return m_outlineView; }
    void setOutlineView(bool on);

    ShapeKind shapeKind() const { return m_shapeKind; }
    void setShapeKind(ShapeKind kind);
    int polygonCorners() const { return m_polygonCorners; }
    void setPolygonCorners(int corners);
    bool equalSides() const { return m_equalSides; }
    void setEqualSides(bool on);
    int currentShape() const;
    bool placesPoints() const;

    bool fillEnabled() const { return m_fillEnabled; }
    void setFillEnabled(bool on);
    QColor fillColor() const { return m_fillColor; }
    void setFillColor(const QColor &c);
    int fillAlpha() const { return m_fillAlpha; }
    void setFillAlpha(int alpha);
    QString lineStyle() const { return m_lineStyle; }
    void setLineStyle(const QString &style);

    bool snapSizes() const { return m_snapSizes; }
    void setSnapSizes(bool on);
    bool snapToPoints() const { return m_snapToPoints; }
    void setSnapToPoints(bool on);
    qreal snapStep() const;

    SelectShape selectShape() const { return m_selectShape; }
    void setSelectShape(SelectShape s);

    QString hintText() const { return m_hintText; }
    Q_INVOKABLE void reportHint(const QString &text);
    bool pressureSeen() const { return m_pressureSeen; }
    QString pressureRange() const { return m_pressureRange; }
    void reportPressure(qreal minSeen, qreal maxSeen);

Q_SIGNALS:
    void toolChanged();
    void colorChanged();
    void penWidthChanged();
    void dynamicWidthChanged();
    void fontSizeChanged();
    void fontFormChanged();
    void textStyleChanged();
    void shapeRecognitionChanged();
    void outlineViewChanged();
    void shapeKindChanged();
    void polygonCornersChanged();
    void equalSidesChanged();
    void fillChanged();
    void lineStyleChanged();
    void snapSizesChanged();
    void snapToPointsChanged();
    void selectShapeChanged();
    void hintTextChanged();
    void pressureSeenChanged();

private:
    void resetCorners();

    Tool m_tool;
    QColor m_color;
    qreal m_penWidth;
    bool m_dynamicWidth;
    qreal m_fontSize;
    bool m_fontBold;
    bool m_fontItalic;
    bool m_fontUnderline;
    int m_textStyle;
    bool m_shapeRecognition;
    bool m_outlineView;

    ShapeKind m_shapeKind;
    int m_polygonCorners;
    bool m_equalSides;

    bool m_fillEnabled;
    QColor m_fillColor;
    int m_fillAlpha;
    QString m_lineStyle;

    bool m_snapSizes;
    bool m_snapToPoints;

    SelectShape m_selectShape;
    QString m_hintText;
    bool m_pressureSeen;
    QString m_pressureRange;
};
}

#endif
