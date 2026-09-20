#ifndef XN_DOCUMENT_H
#define XN_DOCUMENT_H

#include "model/page.h"

#include <QList>
#include <QString>

namespace xn {
class Document
{
public:
    Document();
    ~Document();

    Page *pageAt(int index) const;
    int pageCount() const { return pages.size(); }

    Page *addPage(const QSizeF &size = Page::a4());
    void insertPage(int index, Page *p);
    Page *takePage(int index);

    void clear();

    QString title;
    QString filePath;
    QList<Page *> pages;

private:
    Document(const Document &);
    Document &operator=(const Document &);
};
}

#endif
