#ifndef TPAGINATOR_H
#define TPAGINATOR_H

#include <QString>
#include <QList>
#include <TGlobal>


class T_CORE_EXPORT TPaginator
{
public:
    TPaginator(int itemsCount = 0, int limit = 1, int midRange = 1);
    virtual ~TPaginator() { }

    // Setter
    void setItemsCount(int count);
    void setLimit(int limit);
    void setMidRange(int range);
    void setCurrentPage(int page);

    // Getter
    int itemsCount() const { return itemsCount_; }
    int numPages() const { return numPages_; }
    int limit() const { return limit_; }
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

    int itemsCount_;
    int limit_;
    int midRange_;
    int numPages_;
    int currentPage_;
};

Q_DECLARE_METATYPE(TPaginator)

#endif // TPAGINATOR_H
