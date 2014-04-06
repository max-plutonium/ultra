#include <QString>
#include <QtTest>

#include <random>
#include "../../../src/address.h"

class tst_address_hash : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testCase_data();
    void testCase();
};


class Generator
{
    std::uniform_int_distribution<int> distr;
    std::mt19937_64 generator;

public:
    int operator ()()
    {
        return distr(generator);
    }
};


void tst_address_hash::testCase_data()
{
    QTest::addColumn<int>("x");
    QTest::addColumn<int>("y");
    QTest::addColumn<int>("z");

    constexpr int numTests = 10;
    Generator generator;

    for(int i = 0; i < numTests; ++i)
        QTest::newRow(qPrintable(QString::number(i)))
                << generator() << generator() << generator();
}

void tst_address_hash::testCase()
{
    QFETCH(int, x);
    QFETCH(int, y);
    QFETCH(int, z);

    ultra::address addr { x, y, z };
    ultra::address_hash hash;

    QBENCHMARK {
        hash(addr);
    }
}

QTEST_APPLESS_MAIN(tst_address_hash)

#include "tst_address_hash.moc"
