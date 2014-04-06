#include <QString>
#include <QtTest>
#include <QCoreApplication>

#include <random>
#include "../../../src/address.h"

class tst_address : public QObject
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

void tst_address::testCase_data()
{
    QTest::addColumn<int>("x");
    QTest::addColumn<int>("y");
    QTest::addColumn<int>("z");
    QTest::addColumn<int>("x2");
    QTest::addColumn<int>("y2");
    QTest::addColumn<int>("z2");

    constexpr int numTests = 10;
    Generator generator;

    for(int i = 0; i < numTests; ++i)
        QTest::newRow(qPrintable(QString::number(i)))
                << generator() << generator() << generator()
                << generator() << generator() << generator();
}

void tst_address::testCase()
{
    QFETCH(int, x);
    QFETCH(int, y);
    QFETCH(int, z);
    QFETCH(int, x2);
    QFETCH(int, y2);
    QFETCH(int, z2);

    ultra::address addr1;
    QCOMPARE(addr1.x(), 0);
    QCOMPARE(addr1.y(), 0);
    QCOMPARE(addr1.z(), 0);

    ultra::address addr2(x, y, z);
    QCOMPARE(addr2.x(), x);
    QCOMPARE(addr2.y(), y);
    QCOMPARE(addr2.z(), z);

    ultra::address addr3 = { x2, y2, z2 };
    QCOMPARE(addr3.x(), x2);
    QCOMPARE(addr3.y(), y2);
    QCOMPARE(addr3.z(), z2);

    ultra::address addr4(addr2); // copy ctor
    addr1 = addr3;               // copy assign
    QVERIFY(addr1 == addr1);
    QVERIFY(addr1 == addr3);
    QVERIFY(addr2 == addr4);
    QVERIFY(addr2 != addr3); // Крайне врядли совпадут три координаты
    QVERIFY(addr1 != addr4);
    QVERIFY(addr1 != addr2);
    QVERIFY(addr4 != addr3);

    addr1.set_x(x);
    addr1.set_y(y);
    addr1.set_z(z);
    QCOMPARE(addr1.x(), x);
    QCOMPARE(addr1.y(), y);
    QCOMPARE(addr1.z(), z);
    QVERIFY(addr1 != addr3);
    QVERIFY(addr1 == addr2);
    QVERIFY(addr1 == addr4);
}


QTEST_APPLESS_MAIN(tst_address)

#include "tst_address.moc"
