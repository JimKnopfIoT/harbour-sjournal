#ifndef XN_GZFILE_H
#define XN_GZFILE_H

#include <QByteArray>
#include <QString>

namespace xn {
namespace gz {
QByteArray readAll(const QString &path, QString *error = 0);
bool writeAll(const QString &path, const QByteArray &data, QString *error = 0);
}
}

#endif
