#include "io/gzfile.h"

#include <QCoreApplication>
#include <QFile>

#include <zlib.h>

namespace xn {
namespace gz {
QByteArray readAll(const QString &path, QString *error)
{
    const QByteArray localPath = QFile::encodeName(path);
    gzFile f = gzopen(localPath.constData(), "rb");
    if (!f) {
        if (error)
            *error = QCoreApplication::translate("gz", "Cannot open %1").arg(path);
        return QByteArray();
    }

    QByteArray out;
    char buf[64 * 1024];
    for (;;) {
        const int n = gzread(f, buf, sizeof(buf));
        if (n < 0) {
            int err = 0;
            const char *msg = gzerror(f, &err);
            if (error)
                *error = QCoreApplication::translate("gz", "Cannot read %1: %2").arg(path, QString::fromLocal8Bit(msg));
            gzclose(f);
            return QByteArray();
        }
        if (n == 0)
            break;
        out.append(buf, n);
    }
    gzclose(f);
    return out;
}

bool writeAll(const QString &path, const QByteArray &data, QString *error)
{
    const QString tmpPath = path + QStringLiteral(".part");
    const QByteArray localPath = QFile::encodeName(tmpPath);

    gzFile f = gzopen(localPath.constData(), "wb6");
    if (!f) {
        if (error)
            *error = QCoreApplication::translate("gz", "Cannot write %1").arg(tmpPath);
        return false;
    }

    qint64 written = 0;
    while (written < data.size()) {
        const int chunk = int(qMin<qint64>(data.size() - written, 256 * 1024));
        const int n = gzwrite(f, data.constData() + written, unsigned(chunk));
        if (n <= 0) {
            int err = 0;
            const char *msg = gzerror(f, &err);
            if (error)
                *error = QCoreApplication::translate("gz", "Cannot write %1: %2").arg(tmpPath, QString::fromLocal8Bit(msg));
            gzclose(f);
            QFile::remove(tmpPath);
            return false;
        }
        written += n;
    }

    if (gzclose(f) != Z_OK) {
        if (error)
            *error = QCoreApplication::translate("gz", "Cannot flush %1").arg(tmpPath);
        QFile::remove(tmpPath);
        return false;
    }

    QFile::remove(path);
    if (!QFile::rename(tmpPath, path)) {
        if (error)
            *error = QCoreApplication::translate("gz", "Cannot replace %1").arg(path);
        QFile::remove(tmpPath);
        return false;
    }
    return true;
}
}
}
