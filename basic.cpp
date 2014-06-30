#include "DBReflectionHelper.h"
#define BOOST_TEST_MODULE DatabaseBasic
#include <boost/test/unit_test.hpp>
#include <boost/range/irange.hpp>

BOOST_AUTO_TEST_SUITE(DbSelectSuite)

BOOST_AUTO_TEST_CASE(testInitNull)
{
    DB::Select::factory();
}

BOOST_AUTO_TEST_CASE(testInitNotExist)
{
    DB::Select::factory("", "");
}

BOOST_AUTO_TEST_CASE(testInitTableNotExist)
{
    DB::Select::factory("asdf");
}

BOOST_AUTO_TEST_CASE(testInitMultiple)
{
    for(auto i : boost::irange(0, 100))
        DB::Select::factory();
}

BOOST_AUTO_TEST_CASE(innerJoinInitStrings)
{
    auto v = DB::Join(DB::JOIN::INNER, "Accounts", "Konta", "id", "account_id");
    BOOST_CHECK_EQUAL(v.get_string(), "JOIN Accounts AS `Konta` ON id = account_id");
}

BOOST_AUTO_TEST_CASE(innerJoinInitStringsNonAlias)
{
    auto v = DB::Join(DB::JOIN::INNER, "Accounts", "id", "account_id");
    BOOST_CHECK_EQUAL(v.get_string(), "JOIN Accounts ON id = account_id");
}

BOOST_AUTO_TEST_CASE(crossJoinInit)
{
    auto v = DB::Join("Accounts", "Konta");
    BOOST_CHECK_EQUAL(v.get_string(), "CROSS JOIN Accounts AS `Konta`");
}

BOOST_AUTO_TEST_CASE(outerJoinInit)
{
    auto v = DB::Join(DB::JOIN::LEFT_OUTER, "Accounts", "id", "account_id");
    BOOST_CHECK_EQUAL(v.get_string(), "LEFT OUTER JOIN Accounts ON id = account_id");
}

BOOST_AUTO_TEST_SUITE_END()



