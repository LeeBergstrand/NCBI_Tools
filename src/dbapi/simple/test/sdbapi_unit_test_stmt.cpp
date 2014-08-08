/*  $Id: sdbapi_unit_test_stmt.cpp 348256 2011-12-27 16:21:37Z ivanovp $
 * ===========================================================================
 *
 *                            PUBLIC DOMAIN NOTICE
 *               National Center for Biotechnology Information
 *
 *  This software/database is a "United States Government Work" under the
 *  terms of the United States Copyright Act.  It was written as part of
 *  the author's official duties as a United States Government employee and
 *  thus cannot be copyrighted.  This software/database is freely available
 *  to the public for use. The National Library of Medicine and the U.S.
 *  Government have not placed any restriction on its use or reproduction.
 *
 *  Although all reasonable efforts have been taken to ensure the accuracy
 *  and reliability of the software and data, the NLM and the U.S.
 *  Government do not and cannot warrant the performance or results that
 *  may be obtained by using this software or data. The NLM and the U.S.
 *  Government disclaim all warranties, express or implied, including
 *  warranties of performance, merchantability or fitness for any particular
 *  purpose.
 *
 *  Please cite the author in any work or product based on this material.
 *
 * ===========================================================================
 *
 * Author: Sergey Sikorskiy
 *
 * File Description: DBAPI unit-test
 *
 * ===========================================================================
 */

#include "sdbapi_unit_test_pch.hpp"


BEGIN_NCBI_SCOPE

///////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(Test_SelectStmt)
{
    try {
        // Scenario:
        // 1) Select recordset with just one record
        // 2) Retrive only one record.
        // 3) Select another recordset with just one record
        {
            CQuery query = GetDatabase().NewQuery();
            CQuery::iterator it;

            // 1) Select recordset with just one record
            query.SetSql( "select qq = 57 + 33" );
            query.Execute();

            // 2) Retrive a record.
            it = query.begin();
            BOOST_CHECK( it != query.end() );
            ++it;
            BOOST_CHECK( it == query.end() );

            // 3) Select another recordset with just one record
            query.SetSql( "select qq = 57.55 + 0.0033" );
            query.Execute();
            it = query.begin();
            BOOST_CHECK( it != query.end() );
            ++it;
            BOOST_CHECK( it == query.end() );
        }

        // Check column name ...
        {
            CQuery query = GetDatabase().NewQuery();

            query.SetSql( "select @@version as oops" );
            query.Execute();
            CQuery::iterator it = query.begin();
            BOOST_CHECK( it != query.end() );
            BOOST_CHECK_EQUAL( string("oops"), it.GetColumnName(1) );
        }

        // Check resultset ...
        {
            int num = 0;
            string sql = "select user_id(), convert(varchar(64), user_name()), "
                "convert(nvarchar(64), user_name())";

            CQuery query = GetDatabase().NewQuery(sql);

            // 1) Select recordset with just one record
            query.Execute();

            ITERATE(CQuery, it, query.MultiSet()) {
                BOOST_CHECK(it[1].AsInt4() > 0);
                BOOST_CHECK(it[2].AsString().size() > 0);
                BOOST_CHECK(it[3].AsString().size() > 0);
                ++num;
            }

            BOOST_CHECK_EQUAL(num, 1);
        }

        // Check sequent call of ExecuteQuery ...
        {
            CQuery query = GetDatabase().NewQuery();

            // Run first time ...
            query.SetSql( "select @@version as oops" );
            query.Execute();
            BOOST_CHECK( query.begin() != query.end() );

            // Run second time ...
            query.SetSql( "select @@version as oops" );
            query.Execute();
            BOOST_CHECK( query.begin() != query.end() );

            // Run third time ...
            query.SetSql( "select @@version as oops" );
            query.Execute();
            BOOST_CHECK( query.begin() != query.end() );
        }

        // Select NULL values and empty strings ...
        {
            CQuery query = GetDatabase().NewQuery();

            query.SetSql( "SELECT '', NULL, NULL, NULL" );
            query.Execute();
            BOOST_CHECK( query.begin() != query.end() );

            query.SetSql( "SELECT NULL, '', NULL, NULL" );
            query.Execute();
            BOOST_CHECK( query.begin() != query.end() );

            query.SetSql( "SELECT NULL, NULL, '', NULL" );
            query.Execute();
            BOOST_CHECK( query.begin() != query.end() );

            query.SetSql( "SELECT NULL, NULL, NULL, ''" );
            query.Execute();
            BOOST_CHECK( query.begin() != query.end() );

            query.SetSql( "SELECT '', '', NULL, NULL" );
            query.Execute();
            BOOST_CHECK( query.begin() != query.end() );

            query.SetSql( "SELECT NULL, '', '', NULL" );
            query.Execute();
            BOOST_CHECK( query.begin() != query.end() );

            query.SetSql( "SELECT NULL, NULL, '', ''" );
            query.Execute();
            BOOST_CHECK( query.begin() != query.end() );
        }


    }
    catch(const CException& ex) {
        DBAPI_BOOST_FAIL(ex);
    }
}


///////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(Test_SelectStmt2)
{
    string sql;
    string table_name("#select_stmt2");

    try {
        CQuery query = GetDatabase().NewQuery();

        // Running parametrized statement ...
        {
            int result_num = 0;

            query.SetParameter( "@dbname", "master" );

            query.SetSql( "exec sp_helpdb @dbname" );
            query.Execute();

            while(query.HasMoreResultSets()) {
                query.begin();
                ++result_num;
            }

            query.ClearParameters();
        }

        // Create table ...
        {
            sql =
                "CREATE TABLE " + table_name + " ( \n"
                "id INT NOT NULL PRIMARY KEY, \n"
                "name VARCHAR(255) NOT NULL \n"
                ")"
                ;

            query.SetSql(sql);
            query.Execute();
        }

        // Insert data ...
        {
            sql  = "insert into ";
            sql += table_name + "( id, name ) values ( 1, 'one' )";

            query.SetSql(sql);
            query.Execute();
        }

        // Query data ...
        {
            sql  = "select id, name from ";
            sql += table_name;

            query.SetSql(sql);
            query.Execute();
            query.PurgeResults();
        }
    }
    catch(const CException& ex) {
        DBAPI_BOOST_FAIL(ex);
    }
}

///////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(Test_SelectStmtXML)
{
    try {
        // SQL + XML
        {
            string sql;
            CQuery query = GetDatabase().NewQuery();

            sql = "select 1 as Tag, null as Parent, 1 as [x!1!id] for xml explicit";
            query.SetSql(sql);
            query.Execute();
            BOOST_CHECK( query.HasMoreResultSets() );
            CQuery::iterator it = query.begin();
            if ( it == query.end() ) {
                BOOST_FAIL( msg_record_expected );
            }
        }
    }
    catch(const CException& ex) {
        DBAPI_BOOST_FAIL(ex);
    }
}

///////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(Test_Recordset)
{
    try {
        CQuery query = GetDatabase().NewQuery();
        CQuery::iterator it;

        // First test ...
        {
            // bit
            {
                query.SetSql("select convert(bit, 1)");
                query.Execute();
                BOOST_CHECK(query.HasMoreResultSets());
                it = query.begin();
                BOOST_CHECK(it != query.end());
                unsigned char value = it[1].AsByte();
                BOOST_CHECK_EQUAL(value, 1);

                query.PurgeResults();
            }

            // tinyint
            {
                query.SetSql("select convert(tinyint, 1)");
                query.Execute();
                BOOST_CHECK(query.HasMoreResultSets());
                it = query.begin();
                BOOST_CHECK(it != query.end());
                unsigned char value = it[1].AsByte();
                BOOST_CHECK_EQUAL(value, 1);

                query.PurgeResults();
            }

            // smallint
            {
                query.SetSql("select convert(smallint, 1)");
                query.Execute();
                BOOST_CHECK(query.HasMoreResultSets());
                it = query.begin();
                BOOST_CHECK(it != query.end());
                short value = it[1].AsShort();
                BOOST_CHECK_EQUAL(value, 1);

                query.PurgeResults();
            }

            // int
            {
                query.SetSql("select convert(int, 1)");
                query.Execute();
                BOOST_CHECK(query.HasMoreResultSets());
                it = query.begin();
                BOOST_CHECK(it != query.end());
                int value = it[1].AsInt4();
                BOOST_CHECK_EQUAL(value, 1);

                query.PurgeResults();
            }

            // numeric
            {
                //
                {
                    query.SetSql("select convert(numeric(38, 0), 1)");
                    query.Execute();
                    BOOST_CHECK(query.HasMoreResultSets());
                    it = query.begin();
                    BOOST_CHECK(it != query.end());
                    string value = it[1].AsString();
                    BOOST_CHECK_EQUAL(value, string("1"));

                    query.PurgeResults();
                }

                //
                {
                    query.SetSql("select convert(numeric(18, 2), 2843113322)");
                    query.Execute();
                    BOOST_CHECK(query.HasMoreResultSets());
                    it = query.begin();
                    BOOST_CHECK(it != query.end());
                    string value = it[1].AsString();
                    BOOST_CHECK_EQUAL(value, string("2843113322.00"));

                    query.PurgeResults();
                }
            }

            // decimal
            {
                query.SetSql("select convert(decimal(38, 0), 1)");
                query.Execute();
                BOOST_CHECK(query.HasMoreResultSets());
                it = query.begin();
                BOOST_CHECK(it != query.end());
                string value = it[1].AsString();
                BOOST_CHECK_EQUAL(value, string("1"));

                query.PurgeResults();
            }

            // float
            {
                query.SetSql("select convert(float(4), 1)");
                query.Execute();
                BOOST_CHECK(query.HasMoreResultSets());
                it = query.begin();
                BOOST_CHECK(it != query.end());
                float value = it[1].AsFloat();
                BOOST_CHECK_EQUAL(value, 1);

                query.PurgeResults();
            }

            // double
            {
                //
                {
                    query.SetSql("select convert(double precision, 1)");
                    query.Execute();
                    BOOST_CHECK(query.HasMoreResultSets());
                    it = query.begin();
                    BOOST_CHECK(it != query.end());
                    double value = it[1].AsDouble();
                    BOOST_CHECK_EQUAL(value, 1);

                    query.PurgeResults();
                }

                //
                {
                    query.SetSql("select convert(double precision, 2843113322)");
                    query.Execute();
                    BOOST_CHECK(query.HasMoreResultSets());
                    it = query.begin();
                    BOOST_CHECK(it != query.end());
                    double value = it[1].AsDouble();
                    BOOST_CHECK_EQUAL(value, 2843113322U);

                    query.PurgeResults();
                }
            }

            // real
            {
                query.SetSql("select convert(real, 1)");
                query.Execute();
                BOOST_CHECK(query.HasMoreResultSets());
                it = query.begin();
                BOOST_CHECK(it != query.end());
                float value = it[1].AsFloat();
                BOOST_CHECK_EQUAL(value, 1);

                query.PurgeResults();
            }

            // smalldatetime
            {
                query.SetSql("select convert(smalldatetime, 'January 1, 1900')");
                query.Execute();
                BOOST_CHECK(query.HasMoreResultSets());
                it = query.begin();
                BOOST_CHECK(it != query.end());
                /*CTime value =*/ it[1].AsDateTime();
                //BOOST_CHECK_EQUAL(value, 1);

                query.PurgeResults();
            }

            // datetime
            {
                query.SetSql("select convert(datetime, 'January 1, 1753')");
                query.Execute();
                BOOST_CHECK(query.HasMoreResultSets());
                it = query.begin();
                BOOST_CHECK(it != query.end());
                /*CTime value =*/ it[1].AsDateTime();
                //BOOST_CHECK_EQUAL(value, 1);

                query.PurgeResults();
            }

            // char
            {
                query.SetSql("select convert(char(10), '12345')");
                query.Execute();
                BOOST_CHECK(query.HasMoreResultSets());
                it = query.begin();
                BOOST_CHECK(it != query.end());
                string value = it[1].AsString();
                BOOST_CHECK_EQUAL(value, string("12345     "));

                query.PurgeResults();
            }

            // varchar
            {
                query.SetSql("select convert(varchar(10), '12345')");
                query.Execute();
                BOOST_CHECK(query.HasMoreResultSets());
                it = query.begin();
                BOOST_CHECK(it != query.end());
                string value = it[1].AsString();
                BOOST_CHECK_EQUAL(value, string("12345"));

                query.PurgeResults();
            }

            // nchar
            {
                query.SetSql("select convert(nchar(10), '12345')");
                query.Execute();
                BOOST_CHECK(query.HasMoreResultSets());
                it = query.begin();
                BOOST_CHECK(it != query.end());
                string value = it[1].AsString();
                BOOST_CHECK_EQUAL(value, string("12345     "));

                query.PurgeResults();
            }

            // nvarchar
            {
                query.SetSql("select convert(nvarchar(10), '12345')");
                query.Execute();
                BOOST_CHECK(query.HasMoreResultSets());
                it = query.begin();
                BOOST_CHECK(it != query.end());
                string value = it[1].AsString();
                BOOST_CHECK_EQUAL(value, string("12345"));

                query.PurgeResults();
            }

            // binary
            {
                query.SetSql("select convert(binary(10), '12345')");
                query.Execute();
                BOOST_CHECK(query.HasMoreResultSets());
                it = query.begin();
                BOOST_CHECK(it != query.end());
                string value = it[1].AsString();
                BOOST_CHECK_EQUAL(value, string("12345\0\0\0\0\0", 10));

                query.PurgeResults();
            }

            // long binary
            {
                query.SetSql("select convert(binary(1000), '12345')");
                query.Execute();
                BOOST_CHECK(query.HasMoreResultSets());
                it = query.begin();
                BOOST_CHECK(it != query.end());
                string value = it[1].AsString();
                BOOST_CHECK_EQUAL(value, "12345" + string(995, '\0'));

                query.PurgeResults();
            }

            // varbinary
            {
                query.SetSql("select convert(varbinary(10), '12345')");
                query.Execute();
                BOOST_CHECK(query.HasMoreResultSets());
                it = query.begin();
                BOOST_CHECK(it != query.end());
                string value = it[1].AsString();
                BOOST_CHECK_EQUAL(value, string("12345"));

                query.PurgeResults();
            }

            // text
            {
                query.SetSql("select convert(text, '12345')");
                query.Execute();
                BOOST_CHECK(query.HasMoreResultSets());
                it = query.begin();
                BOOST_CHECK(it != query.end());
                string value = it[1].AsString();
                BOOST_CHECK_EQUAL(value, string("12345"));

                query.PurgeResults();
            }

            // image
            {
                query.SetSql("select convert(image, '12345')");
                query.Execute();
                BOOST_CHECK(query.HasMoreResultSets());
                it = query.begin();
                BOOST_CHECK(it != query.end());
                string value = it[1].AsString();
                BOOST_CHECK_EQUAL(value, string("12345"));

                query.PurgeResults();
            }
        }

    }
    catch(const CException& ex) {
        DBAPI_BOOST_FAIL(ex);
    }
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(Test_NULL)
{
    const string table_name = GetTableName();
    // const string table_name("DBAPI_Sample..dbapi_unit_test");

    enum {rec_num = 10};
    string sql;

    try {
        // Initialize data (strings are NOT empty) ...
        {
            CQuery query = GetDatabase().NewQuery();

            {
                // Drop all records ...
                sql  = " DELETE FROM " + table_name;
                query.SetSql(sql);
                query.Execute();

                sql  = " INSERT INTO " + table_name +
                    "(int_field, vc1000_field) "
                    "VALUES(@int_field, @vc1000_field) \n";
                query.SetSql(sql);

                // Insert data ...
                for (long ind = 0; ind < rec_num; ++ind) {
                    if (ind % 2 == 0) {
                        query.SetParameter( "@int_field", Int4(ind) );
                        query.SetNullParameter( "@vc1000_field", eSDB_String );
                    } else {
                        query.SetNullParameter( "@int_field", eSDB_Int4 );
                        query.SetParameter( "@vc1000_field", NStr::NumericToString(ind) );
                    }

                    // Execute a statement with parameters ...
                    query.Execute();
                }

                // Check record number ...
                BOOST_CHECK_EQUAL(size_t(rec_num),
                                  GetNumOfRecords(query, table_name)
                                  );
            }

            {
                // !!! Do not forget to clear a parameter list ....
                query.ClearParameters();

                // Drop all records ...
                sql  = " DELETE FROM #test_unicode_table";
                query.SetSql(sql);
                query.Execute();

                sql  = " INSERT INTO #test_unicode_table"
                    "(nvc255_field) VALUES(@nvc255_field)";
                query.SetSql(sql);

                // Insert data ...
                for (long ind = 0; ind < rec_num; ++ind) {
                    if (ind % 2 == 0) {
                        query.SetNullParameter( "@nvc255_field", eSDB_String );
                    } else {
                        query.SetParameter( "@nvc255_field", NStr::NumericToString(ind) );
                    }

                    // Execute a statement with parameters ...
                    query.Execute();
                }

                // Check record number ...
                BOOST_CHECK_EQUAL(size_t(rec_num),
                                  GetNumOfRecords(query, "#test_unicode_table"));
            }
        }

        // Check ...
        {
            CQuery query = GetDatabase().NewQuery();

            {
                sql = "SELECT int_field, vc1000_field FROM " + table_name +
                    " ORDER BY id";

                query.SetSql(sql);
                query.Execute();
                BOOST_CHECK(query.HasMoreResultSets());

                CQuery::iterator it = query.begin();
                for (long ind = 0; ind < rec_num; ++ind, ++it) {
                    BOOST_CHECK(it != query.end());

                    int int_field = it[1].AsInt4();
                    string vc1000_field = it[2].AsString();

                    if (ind % 2 == 0) {
                        BOOST_CHECK(!it[1].IsNull());
                        BOOST_CHECK_EQUAL( int_field, ind );

                        BOOST_CHECK(it[2].IsNull());
                    } else {
                        BOOST_CHECK(it[1].IsNull());

                        BOOST_CHECK(!it[2].IsNull());
                        BOOST_CHECK_EQUAL( vc1000_field,
                                           NStr::NumericToString(ind) );
                    }
                }

                query.PurgeResults();
            }

            {
                sql = "SELECT nvc255_field FROM #test_unicode_table ORDER BY id";

                query.SetSql(sql);
                query.Execute();
                BOOST_CHECK(query.HasMoreResultSets());

                CQuery::iterator it = query.begin();
                for (long ind = 0; ind < rec_num; ++ind, ++it) {
                    BOOST_CHECK(it != query.end());

                    string nvc255_field = it[1].AsString();

                    if (ind % 2 == 0) {
                        BOOST_CHECK(it[1].IsNull());
                    } else {
                        BOOST_CHECK(!it[1].IsNull());
                        BOOST_CHECK_EQUAL( NStr::TruncateSpaces(
                                                nvc255_field
                                                ),
                                           NStr::NumericToString(ind) );
                    }
                }

                query.PurgeResults();
            }
        }

        // Check NULL with stored procedures ...
        {
            {
                CQuery query = GetDatabase().NewQuery();

                // Set parameter to NULL ...
                query.SetNullParameter( "@attribute_id", eSDB_Int4 );
                query.ExecuteSP("sp_server_info");
                query.PurgeResults();
                if (GetArgs().GetServerType() == eSqlSrvSybase) {
                    BOOST_CHECK_EQUAL( 30, query.GetRowCount() );
                } else {
                    BOOST_CHECK_EQUAL( 29, query.GetRowCount() );
                }

                // Set parameter to 1 ...
                query.SetParameter( "@attribute_id", 1 );
                query.ExecuteSP("sp_server_info");
                query.PurgeResults();
                BOOST_CHECK_EQUAL( 1, query.GetRowCount() );
            }
        }


        // Special case: empty strings and strings with spaces.
        {
            CQuery query = GetDatabase().NewQuery();

            // Initialize data (strings are EMPTY) ...
            {
                // Drop all records ...
                sql  = " DELETE FROM " + table_name;
                query.SetSql(sql);
                query.Execute();

                sql  = " INSERT INTO " + table_name +
                    "(int_field, vc1000_field) "
                    "VALUES(@int_field, @vc1000_field) \n";
                query.SetSql(sql);

                // Insert data ...
                for (long ind = 0; ind < rec_num; ++ind) {
                    if (ind % 2 == 0) {
                        query.SetParameter( "@int_field", Int4(ind) );
                        query.SetNullParameter("@vc1000_field", eSDB_String);
                    } else {
                        query.SetNullParameter( "@int_field", eSDB_Int4 );
                        query.SetParameter( "@vc1000_field", string() );
                    }

                    // Execute a statement with parameters ...
                    query.Execute();
                }

                // Check record number ...
                BOOST_CHECK_EQUAL(size_t(rec_num), GetNumOfRecords(query, table_name));
            }

            // Check ...
            {
                sql = "SELECT int_field, vc1000_field FROM " + table_name +
                    " ORDER BY id";

                query.ClearParameters();
                query.SetSql(sql);
                query.Execute();
                BOOST_CHECK(query.HasMoreResultSets());

                CQuery::iterator it = query.begin();
                for (long ind = 0; ind < rec_num; ++ind, ++it) {
                    BOOST_CHECK(it != query.end());

                    int int_field = it[1].AsInt4();
                    string vc1000_field = it[2].AsString();

                    if (ind % 2 == 0) {
                        BOOST_CHECK(!it[1].IsNull());
                        BOOST_CHECK_EQUAL( int_field, ind );

                        BOOST_CHECK(it[2].IsNull());
                    } else {
                        BOOST_CHECK(it[1].IsNull());
                        BOOST_CHECK(!it[2].IsNull());
                        // Old protocol version has this strange feature
                        if (GetArgs().GetServerType() == eSqlSrvSybase) {
                            BOOST_CHECK_EQUAL( vc1000_field, string(" ") );
                        }
                        else {
                            BOOST_CHECK_EQUAL( vc1000_field, string() );
                        }
                    }
                }

                query.PurgeResults();
            }

            // Initialize data (strings are full of spaces) ...
            {
                // Drop all records ...
                sql  = " DELETE FROM " + GetTableName();
                query.SetSql(sql);
                query.Execute();

                sql  = " INSERT INTO " + GetTableName() +
                    "(int_field, vc1000_field) "
                    "VALUES(@int_field, @vc1000_field) \n";
                query.SetSql(sql);

                // !!! Do not forget to clear a parameter list ....
                query.ClearParameters();

                // Insert data ...
                for (long ind = 0; ind < rec_num; ++ind) {
                    if (ind % 2 == 0) {
                        query.SetParameter( "@int_field", Int4(ind) );
                        query.SetNullParameter("@vc1000_field", eSDB_String);
                    } else {
                        query.SetNullParameter( "@int_field", eSDB_Int4 );
                        query.SetParameter( "@vc1000_field", string("    ") );
                    }

                    // Execute a statement with parameters ...
                    query.Execute();
                }

                // Check record number ...
                BOOST_CHECK_EQUAL(size_t(rec_num), GetNumOfRecords(query, GetTableName()));
            }

            // Check ...
            {
                sql = "SELECT int_field, vc1000_field FROM " + GetTableName() +
                    " ORDER BY id";

                query.ClearParameters();
                query.SetSql(sql);
                query.Execute();
                BOOST_CHECK(query.HasMoreResultSets());

                CQuery::iterator it = query.begin();
                for (long ind = 0; ind < rec_num; ++ind, ++it) {
                    BOOST_CHECK(it != query.end());

                    int int_field = it[1].AsInt4();
                    string vc1000_field = it[2].AsString();

                    if (ind % 2 == 0) {
                        BOOST_CHECK(!it[1].IsNull());
                        BOOST_CHECK_EQUAL( int_field, ind );

                        BOOST_CHECK(it[2].IsNull());
                    } else {
                        BOOST_CHECK(it[1].IsNull());

                        BOOST_CHECK(!it[2].IsNull());
                        // Old protocol version has this strange feature
                        if (GetArgs().GetServerType() == eSqlSrvSybase) {
                            BOOST_CHECK_EQUAL( vc1000_field, string(" ") );
                        }
                        else {
                            BOOST_CHECK_EQUAL( vc1000_field, string("    ") );
                        }
                    }
                }

                query.PurgeResults();
            }
        }
    }
    catch(const CException& ex) {
        DBAPI_BOOST_FAIL(ex);
    }
}

////////////////////////////////////////////////////////////////////////////////
static void
s_CheckGetRowCount(
    int row_count,
    ETransBehavior tb = eNoTrans,
    CQuery* query = NULL
    )
{
    // Transaction ...
    CTestTransaction transaction(GetDatabase(), tb);
    string sql;
    sql  = " INSERT INTO " + GetTableName() + "(int_field) VALUES( 1 ) \n";

    // Insert row_count records into the table ...
    for ( int i = 0; i < row_count; ++i ) {
        CQuery this_query;
        CQuery* curr_query = NULL;
        if ( !query ) {
            this_query = GetDatabase().NewQuery();
            curr_query = &this_query;
        } else {
            curr_query = query;
        }

        curr_query->SetSql(sql);
        curr_query->Execute();
        int nRows = curr_query->GetRowCount();
        BOOST_CHECK_EQUAL( nRows, 1 );
    }

    // Check a SELECT statement
    {
        CQuery this_query;
        CQuery* curr_query = NULL;
        if ( !query ) {
            this_query = GetDatabase().NewQuery();
            curr_query = &this_query;
        } else {
            curr_query = query;
        }

        sql  = " SELECT * FROM " + GetTableName();
        curr_query->SetSql(sql);
        curr_query->Execute();
        curr_query->PurgeResults();

        int nRows = curr_query->GetRowCount();

        BOOST_CHECK_EQUAL( row_count, nRows );
    }

    // Check an UPDATE statement
    {
        CQuery this_query;
        CQuery* curr_query = NULL;
        if ( !query ) {
            this_query = GetDatabase().NewQuery();
            curr_query = &this_query;
        } else {
            curr_query = query;
        }

        sql  = " UPDATE " + GetTableName() + " SET int_field = 0 ";
        curr_query->SetSql(sql);
        curr_query->Execute();

        int nRows = curr_query->GetRowCount();

        BOOST_CHECK_EQUAL( row_count, nRows );
    }

    // Check a SELECT statement again
    {
        CQuery this_query;
        CQuery* curr_query = NULL;
        if ( !query ) {
            this_query = GetDatabase().NewQuery();
            curr_query = &this_query;
        } else {
            curr_query = query;
        }

        sql  = " SELECT * FROM " + GetTableName() + " WHERE int_field = 0";
        curr_query->SetSql(sql);
        curr_query->Execute();
        curr_query->PurgeResults();

        int nRows = curr_query->GetRowCount();

        BOOST_CHECK_EQUAL( row_count, nRows );
    }

    // Check a DELETE statement
    {
        CQuery this_query;
        CQuery* curr_query = NULL;
        if ( !query ) {
            this_query = GetDatabase().NewQuery();
            curr_query = &this_query;
        } else {
            curr_query = query;
        }

        sql  = " DELETE FROM " + GetTableName();
        curr_query->SetSql(sql);
        curr_query->Execute();

        int nRows = curr_query->GetRowCount();

        BOOST_CHECK_EQUAL( row_count, nRows );
    }

    // Check a SELECT statement again and again ...
    {
        CQuery this_query;
        CQuery* curr_query = NULL;
        if ( !query ) {
            this_query = GetDatabase().NewQuery();
            curr_query = &this_query;
        } else {
            curr_query = query;
        }

        sql  = " SELECT * FROM " + GetTableName();
        curr_query->SetSql(sql);
        curr_query->Execute();
        curr_query->PurgeResults();

        int nRows = curr_query->GetRowCount();

        BOOST_CHECK_EQUAL( 0, nRows );
    }

}

////////////////////////////////////////////////////////////////////////////////
static void
s_CheckGetRowCount2(
    int row_count,
    ETransBehavior tb = eNoTrans,
    CQuery* query = NULL
    )
{
    // Transaction ...
    CTestTransaction transaction(GetDatabase(), tb);
    string sql;
    sql  = " INSERT INTO " + GetTableName() + "(int_field) VALUES( @value ) \n";

    // Insert row_count records into the table ...
    for ( Int4 i = 0; i < row_count; ++i ) {
        CQuery this_query;
        CQuery* curr_query = NULL;
        if ( !query ) {
            this_query = GetDatabase().NewQuery();
            curr_query = &this_query;
        } else {
            curr_query = query;
        }

        curr_query->SetParameter("@value", i);
        curr_query->SetSql(sql);
        curr_query->Execute();

        int nRows = curr_query->GetRowCount();
        BOOST_CHECK_EQUAL( nRows, 1 );

        int nRows2 = curr_query->GetRowCount();
        BOOST_CHECK_EQUAL( nRows2, 1 );
    }

    if ( query ) {
        query->ClearParameters();
    }

    // Check a SELECT statement
    {
        CQuery this_query;
        CQuery* curr_query = NULL;
        if ( !query ) {
            this_query = GetDatabase().NewQuery();
            curr_query = &this_query;
        } else {
            curr_query = query;
        }

        sql  = " SELECT int_field FROM " + GetTableName() +
            " ORDER BY int_field";
        curr_query->SetSql(sql);
        curr_query->Execute();
        curr_query->PurgeResults();

        int nRows = curr_query->GetRowCount();
        BOOST_CHECK_EQUAL( row_count, nRows );

        int nRows2 = curr_query->GetRowCount();
        BOOST_CHECK_EQUAL( row_count, nRows2 );
    }

    // Check an UPDATE statement
    {
        CQuery this_query;
        CQuery* curr_query = NULL;
        if ( !query ) {
            this_query = GetDatabase().NewQuery();
            curr_query = &this_query;
        } else {
            curr_query = query;
        }

        sql  = " UPDATE " + GetTableName() + " SET int_field = 0 ";
        curr_query->SetSql(sql);
        curr_query->Execute();

        int nRows = curr_query->GetRowCount();
        BOOST_CHECK_EQUAL( row_count, nRows );

        int nRows2 = curr_query->GetRowCount();
        BOOST_CHECK_EQUAL( row_count, nRows2 );
    }

    // Check a SELECT statement again
    {
        CQuery this_query;
        CQuery* curr_query = NULL;
        if ( !query ) {
            this_query = GetDatabase().NewQuery();
            curr_query = &this_query;
        } else {
            curr_query = query;
        }

        sql  = " SELECT int_field FROM " + GetTableName() +
            " WHERE int_field = 0";
        curr_query->SetSql(sql);
        curr_query->Execute();
        curr_query->PurgeResults();

        int nRows = curr_query->GetRowCount();
        BOOST_CHECK_EQUAL( row_count, nRows );

        int nRows2 = curr_query->GetRowCount();
        BOOST_CHECK_EQUAL( row_count, nRows2 );
    }

    // Check a DELETE statement
    {
        CQuery this_query;
        CQuery* curr_query = NULL;
        if ( !query ) {
            this_query = GetDatabase().NewQuery();
            curr_query = &this_query;
        } else {
            curr_query = query;
        }

        sql  = " DELETE FROM " + GetTableName();
        curr_query->SetSql(sql);
        curr_query->Execute();

        int nRows = curr_query->GetRowCount();
        BOOST_CHECK_EQUAL( row_count, nRows );

        int nRows2 = curr_query->GetRowCount();
        BOOST_CHECK_EQUAL( row_count, nRows2 );
    }

    // Check a DELETE statement again
    {
        CQuery this_query;
        CQuery* curr_query = NULL;
        if ( !query ) {
            this_query = GetDatabase().NewQuery();
            curr_query = &this_query;
        } else {
            curr_query = query;
        }

        sql  = " DELETE FROM " + GetTableName();
        curr_query->SetSql(sql);
        curr_query->Execute();

        int nRows = curr_query->GetRowCount();
        BOOST_CHECK_EQUAL( 0, nRows );

        int nRows2 = curr_query->GetRowCount();
        BOOST_CHECK_EQUAL( 0, nRows2 );
    }

    // Check a SELECT statement again and again ...
    {
        CQuery this_query;
        CQuery* curr_query = NULL;
        if ( !query ) {
            this_query = GetDatabase().NewQuery();
            curr_query = &this_query;
        } else {
            curr_query = query;
        }

        sql  = " SELECT int_field FROM " + GetTableName() + " ORDER BY int_field";
        curr_query->SetSql(sql);
        curr_query->Execute();
        curr_query->PurgeResults();

        int nRows = curr_query->GetRowCount();
        BOOST_CHECK_EQUAL( 0, nRows );

        int nRows2 = curr_query->GetRowCount();
        BOOST_CHECK_EQUAL( 0, nRows2 );
    }

}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(Test_GetRowCount)
{
    try {
        {
            CQuery query = GetDatabase().NewQuery("DELETE FROM " + GetTableName());

            query.Execute();
        }

        enum {repeat_num = 5};
        for ( int i = 0; i < repeat_num; ++i ) {
            // Shared/Reusable statement
            {
                CQuery query = GetDatabase().NewQuery();

                s_CheckGetRowCount( i, eNoTrans, &query );
                s_CheckGetRowCount( i, eTransCommit, &query );
                s_CheckGetRowCount( i, eTransRollback, &query );
            }
            // Dedicated statement
            {
                s_CheckGetRowCount( i, eNoTrans );
                s_CheckGetRowCount( i, eTransCommit );
                s_CheckGetRowCount( i, eTransRollback );
            }
        }

        for ( int i = 0; i < repeat_num; ++i ) {
            // Shared/Reusable statement
            {
                CQuery query = GetDatabase().NewQuery();

                s_CheckGetRowCount2( i, eNoTrans, &query );
                s_CheckGetRowCount2( i, eTransCommit, &query );
                s_CheckGetRowCount2( i, eTransRollback, &query );
            }
            // Dedicated statement
            {
                s_CheckGetRowCount2( i, eNoTrans );
                s_CheckGetRowCount2( i, eTransCommit );
                s_CheckGetRowCount2( i, eTransRollback );
            }
        }
    }
    catch(const CException& ex) {
        DBAPI_BOOST_FAIL(ex);
    }
}

///////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(Test_GetTotalColumns)
{
    string sql;

    try {
        CQuery query = GetDatabase().NewQuery();

        // Create table ...
        {
            sql =
                "CREATE TABLE #Overlaps ( \n"
                "   pairId int NOT NULL , \n"
                "   overlapNum smallint NOT NULL , \n"
                "   start1 int NOT NULL , \n"
                "   start2 int NOT NULL , \n"
                "   stop1 int NOT NULL , \n"
                "   stop2 int NOT NULL , \n"
                "   orient char (2) NOT NULL , \n"
                "   gaps int NOT NULL , \n"
                "   mismatches int NOT NULL , \n"
                "   adjustedLen int NOT NULL , \n"
                "   length int NOT NULL , \n"
                "   contained tinyint NOT NULL , \n"
                "   seq_align text  NULL , \n"
                "   merged_sa char (1) NOT NULL , \n"
                "   PRIMARY KEY  \n"
                "   ( \n"
                "       pairId, \n"
                "       overlapNum \n"
                "   ) \n"
                ") \n";

            query.SetSql(sql);
            query.Execute();
        }

        // Insert data into the table ...
        {
            sql =
                "INSERT INTO #Overlaps VALUES( \n"
                "1, 1, 0, 25794, 7126, 32916, '--', 1, 21, 7124, 7127, 0, \n"
                "'Seq-align ::= { }', 'n')";

            query.SetSql(sql);
            query.Execute();
        }

        // Actual check ...
        {
            sql = "SELECT * FROM #Overlaps";

            query.SetSql(sql);
            query.Execute();
            while( query.HasMoreResultSets() ) {
                query.begin();
                int col_num = query.GetTotalColumns();
                BOOST_CHECK_EQUAL( 14, col_num );
            }
        }
    }
    catch(const CException& ex) {
        DBAPI_BOOST_FAIL(ex);
    }
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(Test_Identity)
{
    string sql;
    Int8 table_id = 0;
    Int8 identity_id = 0;
    ESDB_Type curr_type;

    try {
        CQuery query = GetDatabase().NewQuery();

        // Clean table ...
        query.SetSql("DELETE FROM " + GetTableName());
        query.Execute();

        // Insert data ...
        {
            // Pure SQL ...
            query.SetSql("INSERT INTO " + GetTableName() + "(int_field) VALUES(1)");
            query.Execute();
        }

        // Retrieve data ...
        {
            sql  = " SELECT id, int_field FROM " + GetTableName();

            query.SetSql(sql);
            query.Execute();

            BOOST_CHECK(query.HasMoreResultSets());
            CQuery::iterator it = query.begin();
            BOOST_CHECK(it != query.end());

            curr_type = it.GetColumnType(1);
            BOOST_CHECK_EQUAL(curr_type, eSDB_Int8);

            BOOST_CHECK(!it[1].IsNull());
            table_id = it[1].AsInt8();
            ++it;
            BOOST_CHECK(it == query.end());
        }

        // Retrieve identity ...
        {
            sql  = " SELECT CONVERT(NUMERIC(18, 0), @@identity) ";

            query.SetSql(sql);
            query.Execute();

            BOOST_CHECK(query.HasMoreResultSets());
            CQuery::iterator it = query.begin();
            BOOST_CHECK(it != query.end());
            BOOST_CHECK(!it[1].IsNull());
            identity_id = it[1].AsInt8();
            ++it;
            BOOST_CHECK(it == query.end());
        }

        BOOST_CHECK_EQUAL(table_id, identity_id);

        // Check identity type ...
        {
            sql  = " SELECT @@identity ";

            query.SetSql(sql);
            query.Execute();

            BOOST_CHECK(query.HasMoreResultSets());
            CQuery::iterator it = query.begin();
            BOOST_CHECK(it != query.end());

            curr_type = it.GetColumnType(1);
            BOOST_CHECK_EQUAL(curr_type, eSDB_String);
            BOOST_CHECK(!it[1].IsNull());

            ++it;
            BOOST_CHECK(it == query.end());
        }

    }
    catch(const CException& ex) {
        DBAPI_BOOST_FAIL(ex);
    }
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(Test_StatementParameters)
{
    try {
        // Very first test ...
        {
            const string table_name(GetTableName());
//             const string table_name("DBAPI_Sample..tmp_table_01");
            string sql;
            CQuery query = GetDatabase().NewQuery();

            if (false) {
                sql  = " CREATE TABLE " + table_name + "( \n";
                sql += "    id NUMERIC(18, 0) IDENTITY NOT NULL, \n";
                sql += "    int_field INT NULL \n";
                sql += " )";

                query.SetSql(sql);
                query.Execute();
            }

            // Clean previously inserted data ...
            {
                query.SetSql("DELETE FROM " + table_name);
                query.Execute();
            }

            {
                sql  = " INSERT INTO " + table_name +
                    "(int_field) VALUES( @value ) \n";

                query.SetParameter( "@value", 0 );
                // Execute a statement with parameters ...
                query.SetSql(sql);
                query.Execute();

                query.SetParameter( "@value", 1 );
                // Execute a statement with parameters ...
                query.SetSql(sql);
                query.Execute();

                // !!! Do not forget to clear a parameter list ....
                // ClearParamList is necessary here ...
                query.ClearParameters();
            }


            {
                sql  = " SELECT int_field FROM " + table_name +
                    " ORDER BY int_field";
                // Execute a statement without parameters ...
                query.SetSql(sql);
                query.Execute();
            }

            // Get number of records ...
            {
                query.SetSql("SELECT COUNT(*) FROM " + table_name);
                query.Execute();
                BOOST_CHECK( query.HasMoreResultSets() );
                CQuery::iterator it = query.begin();
                BOOST_CHECK( it != query.end() );
                BOOST_CHECK_EQUAL( it[1].AsInt4(), 2 );
                query.PurgeResults();
            }

            // Read first inserted value back ...
            {
                // !!! Do not forget to clear a parameter list ....
                // ClearParamList is necessary here ...
                query.ClearParameters();

                query.SetParameter( "@value", 0 );
                query.SetSql( " SELECT int_field FROM " + table_name +
                              " WHERE int_field = @value");
                query.Execute();
                BOOST_CHECK( query.HasMoreResultSets() );
                CQuery::iterator it = query.begin();
                BOOST_CHECK( it != query.end() );
                BOOST_CHECK_EQUAL( it[1].AsInt4(), 0 );
                query.PurgeResults();
            }

            // Read second inserted value back ...
            {
                // !!! Do not forget to clear a parameter list ....
                // ClearParamList is necessary here ...
                query.ClearParameters();

                query.SetParameter( "@value", 1 );
                query.SetSql( " SELECT int_field FROM " + table_name +
                              " WHERE int_field = @value");
                query.Execute();
                BOOST_CHECK( query.HasMoreResultSets() );
                CQuery::iterator it = query.begin();
                BOOST_CHECK( it != query.end() );
                BOOST_CHECK_EQUAL( it[1].AsInt4(), 1 );
                query.PurgeResults();
            }

            // Clean previously inserted data ...
            {
                // !!! Do not forget to clear a parameter list ....
                // ClearParamList is necessary here ...
                query.ClearParameters();

                query.SetSql("DELETE FROM " + table_name);
                query.Execute();
            }
        }
    }
    catch(const CException& ex) {
        DBAPI_BOOST_FAIL(ex);
    }
}

////////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(Test_ClearParamList)
{
    const long rec_num  = 10;
    const string table_name = GetTableName();
    const string str_value = "asdfghjkl";
    string sql;

    // Initialize data (strings are full of spaces) ...
    try {
        CQuery query = GetDatabase().NewQuery();

        // First test ...
        {
            {
                // Drop all records ...
                sql  = " DELETE FROM " + GetTableName();
                query.SetSql(sql);
                query.Execute();
            }

            {
                sql  = " INSERT INTO " + GetTableName() +
                    "(int_field, vc1000_field) "
                    "VALUES(@int_field, @vc1000_field) \n";

                // !!! Do not forget to clear a parameter list ....
                query.ClearParameters();

                query.SetParameter( "@vc1000_field", string(str_value) );

                // Insert data ...
                for (long ind = 0; ind < rec_num; ++ind) {
                    if (ind % 2 == 0) {
                        query.SetParameter( "@int_field", Int4(ind) );
                    } else {
                        query.SetNullParameter( "@int_field", eSDB_Int4 );
                    }

                    // Execute a statement with parameters ...
                    query.SetSql(sql);
                    query.Execute();
                }

                // Check record number ...
                BOOST_CHECK_EQUAL(size_t(rec_num), GetNumOfRecords(query, GetTableName()));
            }

            // Check ...
            {
                sql = "SELECT int_field, vc1000_field FROM " + table_name +
                    " ORDER BY id";

                query.SetSql(sql);
                query.Execute();
                BOOST_CHECK(query.HasMoreResultSets());

                CQuery::iterator it = query.begin();
                for (long ind = 0; ind < rec_num; ++ind, ++it) {
                    BOOST_CHECK(it != query.end());

                    BOOST_CHECK(!it[2].IsNull());
                    BOOST_CHECK_EQUAL(it[2].AsString(), str_value);
                }

                query.PurgeResults();
            }
        }
    }
    catch(const CSDB_Exception& ex) {
        DBAPI_BOOST_FAIL(ex);
    }
}

END_NCBI_SCOPE
