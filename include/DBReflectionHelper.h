#ifndef DBREFLECTIONHELPER_H
#define DBREFLECTIONHELPER_H

#include <string>
#include <algorithm>
#include <utility>
#include <vector>
#include <tuple>
#include <queue>
#include <map>
#include <cassert>
#include <memory>
#include "Poco/Data/Common.h"
#include "Poco/Data/SQLite/Connector.h"
#include "Poco/Data/RecordSet.h"
#ifdef _DEBUG
#include <iostream>
#endif

namespace DB
{
    void shutdown();

    /// String type used for storing database records
    typedef std::string StringType;
    /// Columns info (names/types)
    typedef std::vector<StringType> ColsInfo;
    /// Single table row
    typedef std::map<StringType, StringType> Row;
    /// Data read from the table
    typedef std::vector<Row> Data;
    /// Name of column (or expression) and, optionally alias for it
    typedef std::pair<StringType, StringType> Column;
    /// Names of columns or expressions used in statement
    typedef std::queue<Column> Columns;

    /// Types of where clauses
    enum class WHERE { OR, AND, };
    /// Types of grouping
    enum class ORDER { ASC, DESC, };
    /// Types of table joining
    enum class JOIN { CROSS, INNER, LEFT_OUTER };

    /// Wrapper around join instead of std::tuple for easier use
    class Join {
        public:
            /// Constructors without column aliases
            Join(const JOIN &type, const StringType table, const StringType alias, const StringType col1, StringType col2) :
                _table(table), _col1(col1), _col2(col2), _type(type), _alias(alias)
                { assert(table != StringType()); }
            Join(const JOIN &type, const StringType table, const StringType col1, const StringType col2)
                : Join(type, table, StringType(), col1, col2) {}
            Join(const StringType table, const StringType alias)
                : Join(JOIN::CROSS, table, alias, StringType(), StringType()) {}
            Join(const StringType table)
                : Join(JOIN::CROSS, table, StringType(), StringType()) {}
            StringType get_string() const;
        private:
            StringType _table;
            StringType _alias;
            StringType _col1;
            StringType _col2;
            JOIN _type = JOIN::CROSS;
    };

    /**
     * Helper for simplified database reflection
     * In this case, we don't need Create/Update/Delete because we want to leave base untouched
     */
    class Select
    {
        public:
            static Select factory(const StringType &table_name = StringType(), const StringType &db_name = StringType("main.db"));
            ColsInfo get_cols(StringType table_name = StringType());
            Data get(StringType table_name = StringType());
            Select& distinct(const bool distinct);
            // Where clauses
            Select& where(const WHERE type, const StringType lvalue, const StringType op = StringType(), const StringType rvalue = StringType());
            Select& where(const StringType lvalue, const StringType op = StringType(), const StringType rvalue = StringType());
            Select& or_where(const StringType lvalue, const StringType op = StringType(), const StringType rvalue = StringType());
            Select& and_where(const StringType lvalue, const StringType op = StringType(), const StringType rvalue = StringType());
            Select& group_by(const StringType column);
            Select& order_by(const StringType column, ORDER direction = ORDER::ASC);
            Select& limit(const StringType limit, const StringType offset = StringType());
            Select& offset(const StringType offset);
            Select& columns(const StringType column, const StringType alias = StringType());
            Select& columns(const Column col);
            Select& columns(const std::vector<StringType> cols);
            Select& columns(const std::vector<Column> cols);
            Select& clear();
            // Having ?
            // Joins
            Select& join(const Join& join);
        protected:
        private:
            // We allow only initialization using Factory pattern
            Select(const StringType &table_name = StringType(), const StringType &db_name = StringType("main.db"))
                : _table_name(table_name)
            {
                if( ! Select::_is_registred) // If connector is not registred, do it
                {
                    Select::_is_registred = true;
                    Poco::Data::SQLite::Connector::registerConnector();
                }
                _ses = std::shared_ptr<Poco::Data::Session>(new Poco::Data::Session("SQLite", db_name));
            }

            /// Data for SQL where clause
            typedef std::queue<std::tuple<WHERE, StringType>> WhereClauses;

            StringType _get_where();
            StringType _get_columns();
            ColsInfo _get_cols_for_userset();
            StringType _get_joins();
            StringType _construct_query(const StringType &table_name);

            StringType _table_name;
            std::shared_ptr<Poco::Data::Session> _ses; /// Database session
            ColsInfo _cols_list;  /// List of column names
            ColsInfo _cols_types; /// List of column types
            Data _table_data; /// Data read from table with the last request
            WhereClauses _where; /// Data for SQL where clauses
            bool _distinct = false; /// Select type (ALL/DISTINCT)
            StringType _group_by = StringType();
            StringType _order_by = StringType();
            StringType _limit = StringType();
            StringType _offset = StringType();
            Columns _columns;
            std::queue<Join> _joins; /// Parts of join clause

            static bool _is_registred; /// Is database Connector registred?
    };

    /**
     * Number of queries executed
     */
    class QueryCounter {
        friend class Select;
        public:
            static unsigned get() { return _count; };
        private:
            static void inc() { _count++; };
            static unsigned _count;
    };
}
#endif // DBREFLECTIONHELPER_H
