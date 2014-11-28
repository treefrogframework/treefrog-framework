#ifndef TPAGINATOR_H
#define TPAGINATOR_H

#include <QList>
#include <TGlobal>


class T_CORE_EXPORT TPaginator
{
public:
    TPaginator(int itemsTotal = 0, int itemsPerPage = 10, int midRange = 5);
    TPaginator(const TPaginator &other);
    virtual ~TPaginator() { }

    TPaginator &operator=(const TPaginator &other);

    // Setter
    void setItemTotalCount(int total);
    void setItemCountPerPage(int count);
    void setMidRange(int range);
    void setCurrentPage(int page);

    // Getter
    int itemTotalCount() const { return itemsTotal_; }
    int numPages() const { return numPages_; }
    int itemCountPerPage() { return itemsPerPage_; }
    int offset() const;
    int midRange() const { return midRange_; }
    virtual QList<int> range() const;
    int currentPage() const { return currentPage_; }
    int firstPage() const { return 1; }
    int previousPage() const { return qMax(currentPage_ - 1, 1); }
    int nextPage() const { return qMin(currentPage_ + 1, numPages_); }
    int lastPage() const { return numPages_; }
    bool hasPrevious() const { return (currentPage_ >= 2); }
    bool hasNext() const { return (currentPage_ < numPages_); }
    bool hasPage(int page) const { return (page > 0 && page <= numPages_); }

protected:
    void calculateNumPages();  // Internal use

private:
    int itemsTotal_;
    int itemsPerPage_;
    int midRange_;
    int numPages_;
    int currentPage_;
};

Q_DECLARE_METATYPE(TPaginator)

#endif // TPAGINATOR_H
