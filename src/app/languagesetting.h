#ifndef XN_LANGUAGESETTING_H
#define XN_LANGUAGESETTING_H

#include <QObject>
#include <QString>

namespace xn {
class LanguageSetting : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(bool restartNeeded READ restartNeeded NOTIFY languageChanged)

public:
    explicit LanguageSetting(QObject *parent = 0);

    static void applySaved();

    QString language() const { return m_language; }
    void setLanguage(const QString &language);
    bool restartNeeded() const { return m_language != m_started; }

Q_SIGNALS:
    void languageChanged();

private:
    QString m_language;
    QString m_started;
};
}

#endif
