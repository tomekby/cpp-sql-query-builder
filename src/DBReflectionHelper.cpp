#include "DBReflectionHelper.h"

namespace DB
{
    unsigned QueryCounter::_count;
    bool Select::_is_registred = false;

    /**
     * Database connector shutdown
     */
    void shutdown()
    {
        Poco::Data::SQLite::Connector::unregisterConnector();
    }

    /**
     * Get string which can be used in the statement
     *
     * @return StringType JOIN part of statement
     */
    StringType Join::get_string() const
    {
        const auto TABLE = _alias != StringType() ? (_table + " AS `" + _alias + "`") : _table;
        if(_type == JOIN::CROSS)
            return "CROSS JOIN " + TABLE;
        if(_type == JOIN::INNER)
        {
            assert(_col1 != StringType());
            assert(_col2 != StringType());
            return "JOIN " + TABLE + " ON " + _col1 + " = " + _col2;
        }
        else
        {
            assert(_col1 != StringType());
            assert(_col2 != StringType());
            return "LEFT OUTER JOIN " + TABLE + " ON " + _col1 + " = " + _col2;
        }
    }

    /**
     * Factory pattern
     * This let's us create query with one-liner, such as: Select::factory("table").where("id", "10").get();
     *
     * @param StringType table_name name of table used in queries
     * @param StringType db_name name of database used in queries
     */
    Select Select::factory(const StringType &table_name, const StringType &db_name)
    {
        return Select(table_name, db_name);
    }

    /**
     * Reading list of columns in table
     *
     * @param  StringType table_name name of table
     * @return info about columns
     */
    ColsInfo Select::get_cols(StringType table_name)
    {
        using namespace Poco::Data;

        if( ! _columns.empty())
            return _get_cols_for_userset();

        assert((table_name != StringType() and _table_name != StringType()));
        table_name = table_name != StringType() ? table_name : _table_name;
        _cols_list.clear();
        _cols_types.clear();
        // Reading columns list
        Statement cols(*_ses);
        cols << StringType("PRAGMA TABLE_INFO(`") + table_name + StringType("`);");
        cols.execute();
        RecordSet rs(cols);
        bool more = rs.moveFirst();
        while(more)
        {
            _cols_list.push_back(rs[1].convert<StringType>());
            _cols_types.push_back(rs[2].convert<StringType>());
            more = rs.moveNext();
        }

        return _cols_list;
    }

    /**
     * Get ColsInfo container with column names (or aliases if are present)
     *
     * @return info about columns
     */
    ColsInfo Select::_get_cols_for_userset()
    {
        _cols_list.clear();
        if(_columns.empty())
            return _cols_list;
        auto columns = _columns;
        while( ! columns.empty())
        {
            const auto tmp = columns.front();
            if(tmp.second != StringType())
                _cols_list.push_back(tmp.second);
            else
                _cols_list.push_back(tmp.first);
            columns.pop();
        }

        return _cols_list;
    }

    /**
     * Getting data from table
     *
     * @param  StringType table_name name of table
     * @return void
     */
    Data Select::get(StringType table_name)
    {
        using namespace Poco::Data;

        table_name = table_name != StringType() ? table_name : _table_name;
        assert(table_name != StringType());
        get_cols(table_name);
        Statement select(*_ses);
        // Executing query
        select << _construct_query(table_name);
        select.execute();
        RecordSet rs(select);
        bool more = rs.moveFirst();
        _table_data.clear();
        while(more)
        {
            Row tmp;
            for(auto col_id = 0; col_id < _cols_list.size(); ++col_id)
                tmp[_cols_list[col_id]] = rs[col_id].convert<StringType>();
            _table_data.push_back(tmp);
            more = rs.moveNext();
        }
        QueryCounter::inc();

        return _table_data;
    }

    /**
     * Clearing data before next request
     *
     * @return Select
     */
    Select& Select::clear()
    {
        _cols_list.clear();
        _cols_types.clear();
        _where = WhereClauses();
        _distinct = false;
        _group_by = StringType();
        _order_by = StringType();
        _limit = StringType();
        _offset = StringType();
        _columns = Columns();

        return (*this);
    }

    /**
     * Constructing SQL query
     *
     * @return query string
     */
    StringType Select::_construct_query(const StringType &table_name)
    {
        StringType query = StringType("SELECT ");
        if(_distinct)
            query += StringType("DISTINCT ");
        query += _get_columns() + StringType(" FROM `") + table_name + StringType("` ");
        query += _get_joins();
        query += _get_where();
        if(_group_by != StringType())
            query += StringType(" GROUP BY ") + _group_by;
        if(_order_by != StringType())
            query += StringType(" ORDER BY ") + _order_by;
        if(_limit != StringType())
            query += StringType(" LIMIT ") + _limit;
        if(_offset != StringType())
            query += StringType(" OFFSET ") + _offset;
        query += StringType(";");
        #ifdef _DEBUG
        std::cout << query << std::endl << std::endl;
        #endif

        return query;
    }

    /**
     * Constructing part of where clause
     *
     * @param  WHERE type type of clause (AND/OR)
     * @param  StringType lvalue left operand or whole part of clause
     * @param  StringType op operator (= > < etc.) or right operand
     * @param  StringType rvalue right operand
     * @return Select
     */
    Select& Select::where(const WHERE type, const StringType lvalue, const StringType op, const StringType rvalue)
    {
        StringType expr = StringType();
        StringType val = StringType();
        if(op == StringType())
            expr = lvalue;
        else if(rvalue == StringType())
            expr = lvalue + StringType("=") + op;
        else
            expr = lvalue + op + rvalue;
        // Push expression into the queue
        _where.push(std::make_tuple(type, expr));

        return (*this);
    }

    /**
     * Alias for Select::where(WHERE.OR, lvalue, op, rvalue);
     * @see Select::where()
     *
     * @return Select
     */
    Select& Select::where(const StringType lvalue, const StringType op, const StringType rvalue)
    {
        return where(WHERE::OR, lvalue, op, rvalue);
    }

    /**
     * Alias for Select::where(WHERE.OR, lvalue, op, rvalue);
     * @see Select::where()
     *
     * @return Select
     */
    Select& Select::or_where(const StringType lvalue, const StringType op, const StringType rvalue)
    {
        return where(WHERE::OR, lvalue, op, rvalue);
    }

    /**
     * Alias for Select::where(WHERE.AND, lvalue, op, rvalue);
     * @see Select::where()
     *
     * @return Select
     */
    Select& Select::and_where(const StringType lvalue, const StringType op, const StringType rvalue)
    {
        return where(WHERE::AND, lvalue, op, rvalue);
    }

    /**
     * Set SELECT type (all/distinct)
     *
     * @param bool distinct distinct?
     */
    Select& Select::distinct(const bool distinct)
    {
        _distinct = distinct;
        return (*this);
    }

    /**
     * Set grouping using certain column and direction
     *
     * @param  StringType column name of the column used for grouping
     * @return Select
     */
    Select& Select::group_by(const StringType column)
    {
        _group_by = column;
        return (*this);
    }

    /**
     * Set sorting using certain column and direction
     *
     * @param  StringType column name of the column used for sorting
     * @param  StringType type direction of sorting
     * @return Select
     */
    Select& Select::order_by(const StringType column, const ORDER type)
    {
        _order_by = column + StringType(type == ORDER::ASC ? " ASC" : " DESC");
        return (*this);
    }

    /**
     * Set limit and, optionally offset for the query
     *
     * @param  StringType limit setted limi for query
     * @param  StringType offset number of ommitted rows
     * @return Select
     */
    Select& Select::limit(const StringType limit, const StringType offset)
    {
        assert(limit != StringType("0"));
        _limit = limit;
        if(offset != StringType())
            this->offset(offset);
        return (*this);
    }

    /**
     * Set offset for the query
     *
     * @param  StringType offset number of ommitted rows
     * @return Select
     */
    Select& Select::offset(const StringType offset)
    {
        _offset = offset;
        return (*this);
    }

    /**
     * Set column name with optional alias
     *
     * @param  StringType column name of the added column
     * @param  StringType (optional) alias  for the column
     * @return Select
     */
    Select& Select::columns(const StringType column, const StringType alias)
    {
        _columns.emplace(column, alias);
        return (*this);
    }

    /**
     * Set column name with alias
     *
     * @param  Column col name and alias for column
     * @return Select
     */
    Select& Select::columns(const Column col)
    {
        _columns.push(col);
        return (*this);
    }

    /**
     * Set column names
     *
     * @param  Column cols names of the columns
     * @return Select
     */
    Select& Select::columns(const std::vector<StringType> cols)
    {
        std::for_each(cols.begin(), cols.end(), [&](const StringType &x) {_columns.emplace(x, StringType()); });
        return (*this);
    }

    /**
     * Set column names with aliases
     *
     * @param  Column cols names and aliases for columns
     * @return Select
     */
    Select& Select::columns(const std::vector<Column> cols)
    {
        std::for_each(cols.begin(), cols.end(), [&](const Column &x) { _columns.push(x); });
        return (*this);
    }

    /**
     * Full list of columns suited for using int the SELECT statement
     *
     * @return StringType list of columns
     */
    StringType Select::_get_columns()
    {
        StringType res = StringType("");
        if(_columns.empty())
            return StringType("*");
        while( ! _columns.empty())
        {
            const auto tmp = _columns.front();
            res += tmp.first;
            if(tmp.second != StringType())
                res += StringType(" AS `") + tmp.second + StringType("`");
            _columns.pop();
            if( ! _columns.empty())
                res += StringType(", ");
        }

        return res;
    }

    /**
     * Constructing WHERE part of the SQL query
     *
     * @return full WHERE clause
     */
    StringType Select::_get_where()
    {
        StringType res;
        if(_where.empty())
            return res;
        while( ! _where.empty())
        {
            if(res != StringType())
                res += std::get<0>(_where.front()) == WHERE::OR ? StringType(" OR ") : StringType(" AND ");
            res += std::get<1>(_where.front());
            _where.pop();
        }

        return StringType(" WHERE ") + res;
    }

    /**
     * Add JOIN statement to commands queue
     *
     * @param  Join class describing JOIN statement
     * @return Select
     */
    Select& Select::join(const Join &join)
    {
        _joins.push(join);
        return (*this);
    }

    /**
     * Get all join clauses
     *
     * @return StringType string usable in Select Statement
     */
    StringType Select::_get_joins()
    {
        StringType res;
        if(_joins.empty())
            return res;
        while( ! _joins.empty())
        {
            res += _joins.front().get_string() + " ";
            _joins.pop();
        }

        return res;
    }
} // End namespace DB





