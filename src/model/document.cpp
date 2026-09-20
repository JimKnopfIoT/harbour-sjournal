#include "model/document.h"

namespace xn {
Document::Document()
{
}

Document::~Document()
{
    clear();
}

Page *Document::pageAt(int index) const
{
    if (index < 0 || index >= pages.size())
        return 0;
    return pages.at(index);
}

Page *Document::addPage(const QSizeF &size)
{
    Page *p = new Page(size);
    pages.append(p);
    return p;
}

void Document::insertPage(int index, Page *p)
{
    if (!p)
        return;
    if (index < 0 || index > pages.size())
        index = pages.size();
    pages.insert(index, p);
}

Page *Document::takePage(int index)
{
    if (index < 0 || index >= pages.size())
        return 0;
    return pages.takeAt(index);
}

void Document::clear()
{
    qDeleteAll(pages);
    pages.clear();
    title.clear();
}
}
