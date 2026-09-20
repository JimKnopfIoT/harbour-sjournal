#include "tools/handwriting.h"

#include "model/stroke.h"

#include "zinnia.h"

#include <QByteArray>
#include <QRectF>

namespace xn {
static const int kCanvas = 256;

// By hand: a zero sized QRectF reports isNull(), so united() never takes hold.
static bool strokesBounds(const QVector<const Stroke *> &strokes, QRectF *box)
{
    bool have = false;
    qreal x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    for (int i = 0; i < strokes.size(); ++i) {
        const Stroke *s = strokes.at(i);
        for (int j = 0; j < s->points.size(); ++j) {
            const qreal x = s->points.at(j).x;
            const qreal y = s->points.at(j).y;
            if (!have) {
                x1 = x2 = x;
                y1 = y2 = y;
                have = true;
            } else {
                x1 = qMin(x1, x);
                y1 = qMin(y1, y);
                x2 = qMax(x2, x);
                y2 = qMax(y2, y);
            }
        }
    }
    if (have)
        *box = QRectF(QPointF(x1, y1), QPointF(x2, y2));
    return have;
}

// Centred in zinnia's square canvas, so position on the page does not matter.
static zinnia::Character *toCharacter(const QVector<const Stroke *> &strokes,
                                      const QByteArray &label = QByteArray())
{
    QRectF box;
    if (!strokesBounds(strokes, &box))
        return 0;

    const qreal span = qMax(qMax(box.width(), box.height()), qreal(1));
    const qreal scale = (kCanvas - 1) / span;
    const qreal offsetX = (kCanvas - 1 - box.width() * scale) / 2;
    const qreal offsetY = (kCanvas - 1 - box.height() * scale) / 2;

    zinnia::Character *c = zinnia::Character::create();
    c->set_value(label.constData());
    c->set_width(kCanvas);
    c->set_height(kCanvas);

    size_t id = 0;
    for (int i = 0; i < strokes.size(); ++i) {
        const Stroke *s = strokes.at(i);
        if (s->points.isEmpty())
            continue;
        for (int j = 0; j < s->points.size(); ++j) {
            const int x = qRound((s->points.at(j).x - box.left()) * scale + offsetX);
            const int y = qRound((s->points.at(j).y - box.top()) * scale + offsetY);
            c->add(id, x, y);
        }
        ++id;
    }

    if (id == 0) {
        delete c;
        return 0;
    }
    return c;
}

Handwriting::Handwriting()
    : m_recognizer(0)
    , m_trainer(0)
    , m_samples(0)
{
}

Handwriting::~Handwriting()
{
    delete m_recognizer;
    delete m_trainer;
}

bool Handwriting::loadModel(const QString &path)
{
    delete m_recognizer;
    m_recognizer = zinnia::Recognizer::create();
    if (!m_recognizer->open(path.toUtf8().constData())) {
        m_error = QString::fromUtf8(m_recognizer->what());
        delete m_recognizer;
        m_recognizer = 0;
        return false;
    }
    m_error.clear();
    return true;
}

QVector<Guess> Handwriting::classify(const QVector<const Stroke *> &strokes, int nbest) const
{
    QVector<Guess> out;
    if (!m_recognizer || strokes.isEmpty())
        return out;

    zinnia::Character *c = toCharacter(strokes);
    if (!c)
        return out;

    zinnia::Result *result = m_recognizer->classify(*c, nbest);
    if (!result) {
        m_error = QString::fromUtf8(m_recognizer->what());
        delete c;
        return out;
    }

    for (size_t i = 0; i < result->size(); ++i)
        out << Guess(QString::fromUtf8(result->value(i)), result->score(i));

    delete result;
    delete c;
    return out;
}

void Handwriting::beginTraining()
{
    delete m_trainer;
    m_trainer = zinnia::Trainer::create();
    m_samples = 0;
}

bool Handwriting::addSample(const QString &label, const QVector<const Stroke *> &strokes)
{
    if (!m_trainer || label.isEmpty())
        return false;

    zinnia::Character *c = toCharacter(strokes, label.toUtf8());
    if (!c)
        return false;

    const bool ok = m_trainer->add(*c);
    delete c;
    if (ok)
        ++m_samples;
    else
        m_error = QString::fromUtf8(m_trainer->what());
    return ok;
}

bool Handwriting::train(const QString &path)
{
    if (!m_trainer || m_samples == 0)
        return false;

    // train() already writes binary; Trainer::convert() is for the text format.
    if (!m_trainer->train(path.toUtf8().constData())) {
        m_error = QString::fromUtf8(m_trainer->what());
        return false;
    }

    discardTraining();
    return loadModel(path);
}

void Handwriting::discardTraining()
{
    delete m_trainer;
    m_trainer = 0;
    m_samples = 0;
}
}
