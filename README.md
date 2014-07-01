cpp-sql-query-builder
=====================

Simple SQL query builder built on top of PoCo database abstraction layer  
  
Planned functionality:
* full query builder implementing as much as possible of the SQL standard for at least SELECT, INSERT, UPDATE and DELETE
* data sanitizing before query execution and putting that into the query
* as much simplicity as possible with maximum test coverage
* more support for STL/boost/PoCo containers and types
* big amount of filter functions for easier data printing (some implemented as of now)

This project isn't by any means intended to be ORM replacement*, but should make some things easier, faster and little bit more safe to do. As of now it has partially implemented SELECT clause support with some filter functions included (for example timestamp to date conversion).  
As of now, it have some dependencies on the boost C++ library and in the future it will use it's features even more.  
  
* But with usage of few containers and some type conversion could make sth like "ORM for the poorest" ;)
