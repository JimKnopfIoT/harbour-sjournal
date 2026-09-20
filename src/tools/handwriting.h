#ifndef XN_HANDWRITING_H
#define XN_HANDWRITING_H

#include <QString>
#include <QStringList>
#include <QVector>

namespace zinnia {
class Recognizer;
class Trainer;
}

namespace xn {
class Stroke;

struct Guess
{
    Guess(): score(0) {}
    Guess(const QString &t, qreal s): text(t), score(s) {}

    QString text;
    qreal score;
};

// Zinnia wrapper; the normalisation lives here so training and recognition agree.
class Handwriting
{
public:
    Handwriting();
    ~Handwriting();

    bool loadModel(const QString &path);
    bool hasModel() const { return m_recognizer != 0; }
    QString errorString() const { return m_error; }

    QVector<Guess> classify(const QVector<const Stroke *> &strokes, int nbest = 3) const;

    void beginTraining();
    bool addSample(const QString &label, const QVector<const Stroke *> &strokes);
    int sampleCount() const { return m_samples; }
    bool train(const QString &path);
    void discardTraining();

private:
    Handwriting(const Handwriting &);
    Handwriting &operator=(const Handwriting &);

    zinnia::Recognizer *m_recognizer;
    zinnia::Trainer *m_trainer;
    int m_samples;
    mutable QString m_error;
};
}

#endif
