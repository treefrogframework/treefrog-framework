#ifndef TPAGINATOR_H
#define TPAGINATOR_H

#include <QString>
#include <QList>
#include <TGlobal>

class TPaginator
{
public:
    TPaginator(int itemsCount = 0, int limit = 10, int midRange = 3);
    virtual ~TPaginator() { }

    // Setter
    void setItemsCount(int itemsCount = 0);
    void setLimit(int limit = 10);
    void setMidRange(int midRange = 3);
    void setCurrentPage(int page = 1);

    // Getter
    int getItemsCount() const;
    int getNumPages() const;
    int getLimit() const;
    int getOffset() const;
    int getMidRange() const;
    QList<int> getRange() const;
    int getCurrentPage() const;
    int getFirstPage() const;
    int getPreviousPage() const;
    int getNextPage() const;
    int getLastPage() const;

    bool haveToPaginate() const;
    bool isFirstPageEnabled() const;
    bool hasPreviousPage() const;
    bool hasNextPage() const;
    bool isLastPageEnabled() const;
    bool isValidPage(int page) const;

private:
    void setDefaults();
    
    // Internal process
    void calculateNumPages();
    void calculateOffset();
    void calculateRange();

    int itemsCount;
    int numPages;
    int currentPage;
    int limit;
    int offset;
    int midRange;
    QList<int> range;
};

Q_DECLARE_METATYPE(TPaginator)

inline int TPaginator::getItemsCount() const
{
    return itemsCount;
}

inline int TPaginator::getNumPages() const
{
    return numPages;
}

inline int TPaginator::getLimit() const
{
    return limit;
}

inline int TPaginator::getOffset() const
{
    return offset;
}

inline int TPaginator::getMidRange() const
{
    return midRange;
}

inline QList<int> TPaginator::getRange() const
{
    return range;
}

inline int TPaginator::getCurrentPage() const
{
    return currentPage;
}

inline int TPaginator::getFirstPage() const
{
    return 1;
}

inline int TPaginator::getPreviousPage() const
{
    if (hasPreviousPage())
        return currentPage - 1;
    else
        return 1;
}

inline int TPaginator::getNextPage() const
{
    if (hasNextPage())
        return currentPage + 1;
    else
        return numPages;
}

inline int TPaginator::getLastPage() const
{
    return numPages;
}

inline bool TPaginator::haveToPaginate() const
{
    return (numPages > 1);
}

inline bool TPaginator::isFirstPageEnabled() const
{
    return (currentPage != 1);
}

inline bool TPaginator::hasPreviousPage() const
{
    return (currentPage - 1 >= 1);
}

inline bool TPaginator::hasNextPage() const
{
    return (currentPage + 1 <= numPages);
}

inline bool TPaginator::isLastPageEnabled() const
{
    return (currentPage != numPages);
}

inline bool TPaginator::isValidPage(int page) const
{
    if (1 <= page && page <= numPages)
    {
        return true;
    }
    else
    {
        return false;
    }
}

#endif // TPAGINATOR_H
