#ifndef TPAGINATOR_H
#define TPAGINATOR_H

#include <QList>
#include <TGlobal>


class T_CORE_EXPORT TPaginator
{
public:
    TPaginator(int itemsTotal = 0, int itemsPerPage = 1, int midRange = 1);
    virtual ~TPaginator() { }

    // Setter
    void setItemsCount(int count);  // obsolete function
    void setItemTotalCount(int total);
    void setLimit(int limit);   // obsolete function
    void setItemCountPerPage(int count);
    void setMidRange(int range);
    void setCurrentPage(int page);

    // Getter
    int itemsCount() const { return itemsTotal_; } // obsolete function
    int itemTotalCount() const { return itemsTotal_; }
    int numPages() const { return numPages_; }
    int limit() const { return itemsPerPage_; }  // obsolete function
    int itemCountPerPage() { return itemsPerPage_; }
    int offset() const;
    int midRange() const { return midRange_; }
    QList<int> range() const;
    int currentPage() const { return currentPage_; }
    int firstPage() const { return 1; }
    int previousPage() const { return qMax(currentPage_ - 1, 1); }
    int nextPage() const { return qMin(currentPage_ + 1, numPages_); }
    int lastPage() const { return numPages_; }
    bool hasPreviousPage() const { return (currentPage_ >= 2); }
    bool hasNextPage() const { return (currentPage_ < numPages_); }
    bool isValidPage(int page) const { return (page >= 1 && page <= numPages_); }

private:
    // Internal use
    void calculateNumPages();

    int itemsTotal_;
    int itemsPerPage_;
    int midRange_;
    int numPages_;
    int currentPage_;
};

Q_DECLARE_METATYPE(TPaginator)

#endif // TPAGINATOR_H
