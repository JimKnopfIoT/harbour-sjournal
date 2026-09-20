#include "model/textitem.h"

#include <qmath.h>

#include <QFont>
#include <QFontMetricsF>
#include <QtGlobal>
#include <QStringList>

namespace xn {
TextItem::TextItem()
    : fontName(QStringLiteral("Sans"))
    , fontSize(12)
    , bold(false)
    , italic(false)
    , underline(false)
    , style(Filled)
    , color(Qt::black)
{
}

QFont TextItem::font() const
{
    QFont f(fontName);
    // fontSize is a PDF point length, not a typographic point size.
    f.setPixelSize(qMax(1, qRound(fontSize)));
    f.setBold(bold);
    f.setItalic(italic);
    f.setUnderline(underline);
    return f;
}

void TextItem::scale(const QPointF &origin, qreal sx, qreal sy)
{
    const QRectF box = bounds();
    const qreal left = origin.x() + (box.left() - origin.x()) * sx;
    const qreal top = origin.y() + (box.top() - origin.y()) * sy;
    pos = QPointF(sx < 0 ? left - box.width() * qAbs(sx) : left,
                  sy < 0 ? top - box.height() * qAbs(sy) : top);
    fontSize *= qSqrt(qAbs(sx * sy));
}

QRectF TextItem::bounds() const
{
    const QFontMetricsF fm(font());

    const QStringList lines = text.split(QLatin1Char('\n'));
    qreal w = 0;
    for (int i = 0; i < lines.size(); ++i)
        w = qMax(w, fm.width(lines.at(i)));

    return QRectF(pos.x(), pos.y(), w, fm.height() * lines.size());
}

Element *TextItem::clone() const
{
    return new TextItem(*this);
}
}
