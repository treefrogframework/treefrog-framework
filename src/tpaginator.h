#ifndef TPAGINATOR_H
#define TPAGINATOR_H

#include <QString>
#include <QList>
#include <TGlobal>

class TPaginator
{
public:
    TPaginator(int itemsCount = 0, int limit = 1, int midRange = 1);
    virtual ~TPaginator() { }

    // Setter
    void setItemsCount(int itemsCount);
    void setLimit(int limit);
    void setMidRange(int midRange);
    void setCurrentPage(int page);

    // Getter
    const int &itemsCount() const { return itemsCount_; }
    const int &numPages() const { return numPages_; }
    const int &limit() const { return limit_; }
    const int &offset() const { return offset_; }
    const int &midRange() const { return midRange_; }
    const QList<int> &range() const { return range_; }
    const int &currentPage() const { return currentPage_; }
    int firstPage() const { return 1; }
    int previousPage() const { return hasPreviousPage() ? currentPage_ - 1 : 1; }
    int nextPage() const { return hasNextPage() ? currentPage_ + 1 : numPages_; }
    int lastPage() const { return numPages_; }

    bool haveToPaginate() const { return (numPages_ > 1); }
    bool isFirstPageEnabled() const { return (currentPage_ != 1); }
    bool hasPreviousPage() const { return (currentPage_ - 1 >= 1); }
    bool hasNextPage() const { return (currentPage_ + 1 <= numPages_); }
    bool isLastPageEnabled() const { return (currentPage_ != numPages_); }
    bool isValidPage(int page) const { return (1 <= page && page <= numPages_) ? true : false; }

private:
    void setDefaults();
    
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
