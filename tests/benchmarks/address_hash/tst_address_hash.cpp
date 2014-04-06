#include <QString>
#include <QtTest>

#include <random>
#include "../../../src/address.h"

class tst_address_hash : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testCase();
};


void tst_address_hash::testCase()
{
    std::uniform_int_distribution<int> distr;
    std::mt19937_64 generator;

    std::vector<ultra::address> vec(1000000);
    std::generate(vec.begin(), vec.end(), [&]() {
            return ultra::address {
                distr(generator), distr(generator), distr(generator)
            };
        });

    ultra::address_hash hash;

    QBENCHMARK {
        for(const ultra::address &addr : vec)
            hash(addr);
    }
}

QTEST_APPLESS_MAIN(tst_address_hash)

#include "tst_address_hash.moc"
