#include <QStringList>
#include <QListIterator>
#include <QTest>
#include <random>
#include <iostream>

class ForList : public QObject
{
    Q_OBJECT
public:
    enum IterationType {
        IndexAt,
        Java,
        Stl,
        StlConst,
        Foreach,
        ForeachConstRef,
        ForC11Ref,
        ForC11ConstRef,
        ForC11ConstRefConst,
    };

private slots:
    void qStringList_data()
    {
        QTest::addColumn<QStringList>("list1");
        QTest::addColumn<QStringList>("list2");
        QTest::addColumn<int>("type");


        std::random_device rd;
        std::mt19937 mt(rd());

        QStringList list1;
        for (int i = 0; i < 1000; i++) {
            list1 << QString::number(mt());
        }

        QStringList list2;
        for (int i = 0; i < 1000000; i++) {
            list2 << QString::number(mt());
        }

        QTest::newRow("index based") << list1 << list2 << (int)IndexAt;
        QTest::newRow("Java") << list1 << list2 << (int)Java;
        QTest::newRow("STL") << list1 << list2 << (int)Stl;
        QTest::newRow("STL (const iterators)") << list1 << list2 << (int)StlConst;
        QTest::newRow("foreach (const references)") << list1 << list2 << (int)ForeachConstRef;
        QTest::newRow("for (references)") << list1 << list2 << (int)ForC11Ref;
        QTest::newRow("for (const references)") << list1 << list2 << (int)ForC11ConstRef;
        QTest::newRow("for (const references const 3)") << list1 << list2 << (int)ForC11ConstRefConst;
    }

    void qStringList() const
    {
        QFETCH(QStringList, list1);
        QFETCH(QStringList, list2);
        QFETCH(int, type);

        uint dummy = 0;

        switch (type) {
        case IndexAt:
            QBENCHMARK {
                // 1000 times
                for (int j = 0; j < 1000; j++) {
                    const int listSize = list1.size();
                    for (int i = 0; i < listSize; ++i)
                        dummy += list1.at(i).size();
                }

                const int listSize = list2.size();
                for (int i = 0; i < listSize; ++i)
                    dummy += list2.at(i).size();
            }
            break;

        case Java: {
            QBENCHMARK {
                // 1000 times
                for (int j = 0; j < 1000; j++) {
                    QStringListIterator iter(list1);
                    while (iter.hasNext())
                        dummy += (iter.next()).size();
                }

                QStringListIterator iter(list2);
                while (iter.hasNext())
                    dummy += (iter.next()).size();
            }
            break; }

        case Stl: {
            QBENCHMARK {
                // 1000 times
                for (int j = 0; j < 1000; j++) {
                    QStringList::iterator iter = list1.begin();
                    const QStringList::iterator end = list1.end();
                    for (; iter != end; ++iter)
                        dummy += (*iter).size();
                }

                QStringList::iterator iter = list2.begin();
                const QStringList::iterator end = list2.end();
                for (; iter != end; ++iter)
                    dummy += (*iter).size();
            }
            break; }

        case StlConst:
            QBENCHMARK {
                // 1000 times
                for (int j = 0; j < 1000; j++) {
                    QStringList::const_iterator iter = list1.constBegin();
                    const QStringList::const_iterator end = list1.constEnd();
                    for (; iter != end; ++iter) {
                        dummy += (*iter).size();
                    }
                }

                QStringList::const_iterator iter = list2.constBegin();
                const QStringList::const_iterator end = list2.constEnd();
                for (; iter != end; ++iter) {
                    dummy += (*iter).size();
                }
            }
            break;

        case Foreach:
            QBENCHMARK {
                // 1000 times
                for (int j = 0; j < 1000; j++) {
                    foreach (QString s, list1) {
                        dummy += s.size();
                    }
                }

                foreach (QString s, list2) {
                    dummy += s.size();
                }
            }
            break;

        case ForeachConstRef:
            QBENCHMARK {
                // 1000 times
                for (int j = 0; j < 1000; j++) {
                    foreach (const auto &s, list1)
                        dummy += s.size();
                }

                foreach (const auto &s, list2)
                    dummy += s.size();
            }
            break;

        case ForC11Ref:
            QBENCHMARK {
                // 1000 times
                for (int j = 0; j < 1000; j++) {
                    for (QString &s : list1)
                        dummy += s.size();
                }

                for (QString &s : list2)
                    dummy += s.size();
            }
            break;

        case ForC11ConstRef:
            QBENCHMARK {
                // 1000 times
                for (int j = 0; j < 1000; j++) {
                    for (const QString &s : list1)
                        dummy += s.size();
                }

                for (const QString &s : list2)
                    dummy += s.size();
            }
            break;

        case ForC11ConstRefConst:
            QBENCHMARK {
                // 1000 times
                for (int j = 0; j < 1000; j++) {
                    for (auto &s : (const QStringList&)list1)
                        dummy += s.size();
                }

                for (auto &s : (const QStringList&)list2)
                    dummy += s.size();
            }
            break;

        default:
            break;
        }
    }

};


QTEST_MAIN(ForList)
#include "main.moc"
