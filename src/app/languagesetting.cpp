#include "languagesetting.h"

#include <QSettings>

namespace xn {
namespace {
const char kKey[] = "language";
const char kName[] = "harbour-sjournal";

QString savedLanguage()
{
    return QSettings(QString::fromLatin1(kName), QString::fromLatin1(kName)).value(QLatin1String(kKey)).toString();
}
}

LanguageSetting::LanguageSetting(QObject *parent)
    : QObject(parent)
    , m_language(savedLanguage())
    , m_started(m_language)
{
}

void LanguageSetting::applySaved()
{
    const QString language = savedLanguage();
    if (language == QLatin1String("en")) {
        qputenv("LANGUAGE", "en_GB");
        qputenv("LC_ALL", "en_GB.UTF-8");
    } else if (language == QLatin1String("de")) {
        qputenv("LANGUAGE", "de_DE");
        qputenv("LC_ALL", "de_DE.UTF-8");
    }
}

void LanguageSetting::setLanguage(const QString &language)
{
    if (language == m_language)
        return;
    m_language = language;
    QSettings s(QString::fromLatin1(kName), QString::fromLatin1(kName));
    if (language.isEmpty())
        s.remove(QLatin1String(kKey));
    else
        s.setValue(QLatin1String(kKey), language);
    Q_EMIT languageChanged();
}
}
