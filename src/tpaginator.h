#pragma once
#include <QList>
#include <TGlobal>


class T_CORE_EXPORT TPaginator {
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
    int itemTotalCount() const { return _itemsTotal; }
    int numPages() const { return _numPages; }
    int itemCountPerPage() const { return _itemsPerPage; }
    int itemCountOfCurrentPage() const;
    int offset() const;
    int midRange() const { return _midRange; }
    virtual QList<int> range() const;
    int currentPage() const;
    int firstPage() const { return 1; }
    int previousPage() const { return qMax(currentPage() - 1, 1); }
    int nextPage() const { return qMin(currentPage() + 1, _numPages); }
    int lastPage() const { return _numPages; }
    bool hasPrevious() const { return (currentPage() >= 2); }
    bool hasNext() const { return (currentPage() < _numPages); }
    bool hasPage(int page) const { return (page > 0 && page <= _numPages); }

protected:
    void calculateNumPages();  // Internal use

private:
    int _itemsTotal {0};
    int _itemsPerPage {10};
    int _midRange {5};
    int _numPages {1};
    int _currentPage {1};
};

Q_DECLARE_METATYPE(TPaginator)

