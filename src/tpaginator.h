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
    int offset() const { return offset_; }
    int midRange() const { return midRange_; }
    const QList<int> &range() const { return range_; }
    int currentPage() const { return currentPage_; }
    int firstPage() const { return 1; }
    int previousPage() const { return hasPreviousPage() ? currentPage_ - 1 : 1; }
    int nextPage() const { return hasNextPage() ? currentPage_ + 1 : numPages_; }
    int lastPage() const { return numPages_; }

    bool haveToPaginate() const { return (numPages_ > 1); }
    bool isFirstPage() const { return (currentPage_ != 1); }
    bool hasPreviousPage() const { return (currentPage_ >= 2); }
    bool hasNextPage() const { return (currentPage_ + 1 <= numPages_); }
    bool isLastPage() const { return (currentPage_ != numPages_); }
    bool isValidPage(int page) const { return (1 <= page && page <= numPages_); }

private:
    // Internal process
    void calculateNumPages();
    void calculateOffset();
    void calculateRange();

    int itemsCount_;
    int numPages_;
    int limit_;
    int offset_;
    int midRange_;
    QList<int> range_;
    int currentPage_;
};

Q_DECLARE_METATYPE(TPaginator)

#endif // TPAGINATOR_H
