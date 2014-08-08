/*  $Id: sdbapi_unit_test_object.cpp 341842 2011-10-24 16:51:11Z ivanovp $
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
BOOST_AUTO_TEST_CASE(Test_DateTime)
{
    string sql;
    CQuery query = GetDatabase().NewQuery();
    CTime t;
    CTime dt_value;

    try {
        if (true) {
            // Initialization ...
            {
                sql =
                    "CREATE TABLE #test_datetime ( \n"
                    "   id INT, \n"
                    "   dt_field DATETIME NULL \n"
                    ") \n";

                query.SetSql( sql );
                query.Execute();
            }

            {
                // Initialization ...
                {
                    sql = "INSERT INTO #test_datetime(id, dt_field) "
                          "VALUES(1, GETDATE() )";

                    query.SetSql( sql );
                    query.Execute();
                }

                // Retrieve data ...
                {
                    sql = "SELECT * FROM #test_datetime";

                    query.SetSql( sql );
                    query.Execute();
                    BOOST_CHECK( query.HasMoreResultSets() );
                    CQuery::iterator it = query.begin();
                    BOOST_CHECK( it != query.end() );
                    BOOST_CHECK( !it[2].IsNull());
                    dt_value = it[2].AsDateTime();
                    BOOST_CHECK( !dt_value.IsEmpty() );
                }

                // Insert data using parameters ...
                {
                    query.SetSql( "DELETE FROM #test_datetime" );
                    query.Execute();

                    query.SetParameter( "@dt_val", dt_value );

                    sql = "INSERT INTO #test_datetime(id, dt_field) "
                          "VALUES(1, @dt_val)";

                    query.SetSql( sql );
                    query.Execute();
                }

                // Retrieve data again ...
                {
                    sql = "SELECT * FROM #test_datetime";

                    // ClearParamList is necessary here ...
                    query.ClearParameters();
                    query.SetSql( sql );
                    query.Execute();
                    BOOST_CHECK( query.HasMoreResultSets() );
                    CQuery::iterator it = query.begin();
                    BOOST_CHECK( it != query.end() );
                    BOOST_CHECK( !it[2].IsNull());
                    CTime dt_value2 = it[2].AsDateTime();
                    BOOST_CHECK_EQUAL( dt_value.AsString(), dt_value2.AsString() );
                }

                // Insert NULL data using parameters ...
                {
                    query.SetSql( "DELETE FROM #test_datetime" );
                    query.Execute();

                    query.SetNullParameter( "@dt_val", eSDB_DateTime );

                    sql = "INSERT INTO #test_datetime(id, dt_field) "
                          "VALUES(1, @dt_val)";

                    query.SetSql( sql );
                    query.Execute();
                }


                // Retrieve data again ...
                {
                    sql = "SELECT * FROM #test_datetime";

                    // ClearParamList is necessary here ...
                    query.ClearParameters();
                    query.SetSql( sql );
                    query.Execute();
                    BOOST_CHECK( query.HasMoreResultSets() );
                    CQuery::iterator it = query.begin();
                    BOOST_CHECK( it != query.end() );
                    BOOST_CHECK( it[2].IsNull());
                }
            }
        }
    }
    catch(const CException& ex) {
        DBAPI_BOOST_FAIL(ex);
    }
}

///////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(Test_Numeric)
{
    const string table_name("#test_numeric");
    const string str_value("2843113322.00");
    const string str_value_short("2843113322");
    const Uint8 value = 2843113322U;
    string sql;

    try {
        CQuery query = GetDatabase().NewQuery();

        // Initialization ...
        {
            sql =
                "CREATE TABLE " + table_name + " ( \n"
                "   id NUMERIC(18, 0) IDENTITY NOT NULL, \n"
                "   num_field1 NUMERIC(18, 2) NULL, \n"
                "   num_field2 NUMERIC(35, 2) NULL \n"
                ") \n";

            query.SetSql( sql );
            query.Execute();
        }

    // First test ...
        {
            // Initialization ...
            {
                sql = "INSERT INTO " + table_name + "(num_field1, num_field2) "
                    "VALUES(" + str_value + ", " + str_value + " )";

                query.SetSql( sql );
                query.Execute();
            }

            // Retrieve data ...
            {
                sql = "SELECT num_field1, num_field2 FROM " + table_name;

                query.SetSql( sql );
                query.Execute();
                BOOST_CHECK( query.HasMoreResultSets() );
                CQuery::iterator it = query.begin();
                BOOST_CHECK( it != query.end() );

                BOOST_CHECK(!it[1].IsNull());
                BOOST_CHECK(!it[2].IsNull());
                BOOST_CHECK_EQUAL(it[1].AsString(), str_value);
                BOOST_CHECK_EQUAL(it[2].AsString(), str_value);
            }

            // Insert data using parameters ...
            {
                query.SetSql( "DELETE FROM " + table_name );
                query.Execute();

                sql = "INSERT INTO " + table_name + "(num_field1, num_field2) "
                    "VALUES(@value1, @value2)";

                //
                {
                    //
                    query.SetParameter( "@value1", static_cast<double>(value) );
                    query.SetParameter( "@value2", static_cast<double>(value) );

                    query.SetSql( sql );
                    query.Execute();
                }

                // ClearParamList is necessary here ...
                query.ClearParameters();
            }

            // Retrieve data again ...
            {
                sql = "SELECT num_field1, num_field2 FROM " + table_name +
                    " ORDER BY id";

                query.SetSql( sql );
                query.Execute();
                BOOST_CHECK( query.HasMoreResultSets() );
                CQuery::iterator it = query.begin();
                BOOST_CHECK( it != query.end() );

                BOOST_CHECK(!it[1].IsNull());
                BOOST_CHECK(!it[2].IsNull());
                BOOST_CHECK_EQUAL(it[1].AsString(), str_value);
                BOOST_CHECK_EQUAL(it[2].AsString(), str_value);
            }
        }
    }
    catch(const CException& ex) {
        DBAPI_BOOST_FAIL(ex);
    }
}

///////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(Test_VARCHAR_MAX)
{
    string sql;
    const string table_name = "#test_varchar_max_table";
    // const string table_name = "DBAPI_Sample..test_varchar_max_table";

    try {
        CQuery query = GetDatabase().NewQuery();

        // Create table ...
        if (table_name[0] =='#') {
            sql =
                "CREATE TABLE " + table_name + " ( \n"
                "   id NUMERIC IDENTITY NOT NULL, \n"
                "   vc_max VARCHAR(MAX) NULL"
                ") \n";

            query.SetSql( sql );
            query.Execute();
        }

        // SQL value injection technique ...
        {
            // Clean table ...
            {
                sql = "DELETE FROM " + table_name;

                query.SetSql( sql );
                query.Execute();
            }

            const string msg(8001, 'Z');

            // Insert data into the table ...
            {
                sql =
                    "INSERT INTO " + table_name + "(vc_max) VALUES(\'" + msg + "\')";

                query.SetSql( sql );
                query.Execute();
            }

            // Actual check ...
            {
                sql = "SELECT vc_max FROM " + table_name + " ORDER BY id";

                query.SetSql( sql );
                query.Execute();
                ITERATE(CQuery, it, query.SingleSet()) {
                    const string value = it[1].AsString();
                    BOOST_CHECK_EQUAL(value.size(), msg.size());
                    BOOST_CHECK_EQUAL(value, msg);
                }
            }
        }

        // Parameters ...
        {
            const string msg(4000, 'Z');

            // Clean table ...
            {
                sql = "DELETE FROM " + table_name;

                query.SetSql( sql );
                query.Execute();
            }

            // Insert data into the table ...
            {
                sql =
                    "INSERT INTO " + table_name + "(vc_max) VALUES(@vc_max)";

                query.SetParameter( "@vc_max", msg );
                query.SetSql( sql );
                query.Execute();
            }

            // Actual check ...
            {
                sql = "SELECT vc_max FROM " + table_name + " ORDER BY id";

                query.SetSql( sql );
                query.Execute();
                ITERATE(CQuery, it, query.SingleSet()) {
                    const string value = it[1].AsString();
                    BOOST_CHECK_EQUAL(value.size(), msg.size());
                    // BOOST_CHECK_EQUAL(value, msg);
                }
            }
        }

    }
    catch(const CSDB_Exception& ex) {
        DBAPI_BOOST_FAIL(ex);
    }
    catch(const CException& ex) {
        DBAPI_BOOST_FAIL(ex);
    }
}

///////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(Test_VARCHAR_MAX_BCP)
{
    string sql;
    const string table_name = "#test_varchar_max_bcp_table";
    // const string table_name = "DBAPI_Sample..test_varchar_max_bcp_table";
    // const string msg(32000, 'Z');
    const string msg(8001, 'Z');

    try {
        CQuery query = GetDatabase().NewQuery();

        // Create table ...
        if (table_name[0] =='#') {
            sql =
                "CREATE TABLE " + table_name + " ( \n"
                "   id NUMERIC IDENTITY NOT NULL, \n"
                "   vc_max VARCHAR(MAX) NULL"
                ") \n";

            query.SetSql( sql );
            query.Execute();
        }

        {
            // Clean table ...
            {
                sql = "DELETE FROM " + table_name;

                query.SetSql( sql );
                query.Execute();
            }

            // Insert data into the table ...
            {
                CBulkInsert bi = GetDatabase().NewBulkInsert(table_name, 1);

                bi.Bind(1, eSDB_Int4);
                bi.Bind(2, eSDB_Text);

                bi << 1 << msg << EndRow;
                bi.Complete();
            }

            // Actual check ...
            {
                sql = "SELECT vc_max FROM " + table_name + " ORDER BY id";

                query.SetSql( sql );
                query.Execute();
                ITERATE(CQuery, it, query.SingleSet()) {
                    const string value = it[1].AsString();
                    BOOST_CHECK_EQUAL(value.size(), msg.size());
                    BOOST_CHECK_EQUAL(value, msg);
                }
            }
        }
    }
    catch(const CSDB_Exception& ex) {
        DBAPI_BOOST_FAIL(ex);
    }
    catch(const CException& ex) {
        DBAPI_BOOST_FAIL(ex);
    }
}

///////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(Test_CHAR)
{
    string sql;
    const string table_name = "#test_char_table";
    // const string table_name = "DBAPI_Sample..test_char_table";

    try {
        CQuery query = GetDatabase().NewQuery();

        // Create table ...
        if (table_name[0] =='#') {
            sql =
                "CREATE TABLE " + table_name + " ( \n"
                "   id NUMERIC IDENTITY NOT NULL, \n"
                "   char1_field CHAR(1) NULL"
                ") \n";

            query.SetSql( sql );
            query.Execute();
        }

        // Parameters ...
        {
            // const CVariant char_value;

            // Clean table ...
            {
                sql = "DELETE FROM " + table_name;

                query.SetSql( sql );
                query.Execute();
            }

            // Insert data into the table ...
            {
                sql =
                    "INSERT INTO " + table_name + "(char1_field) VALUES(@char1)";

                query.SetParameter( "@char1", "" );
                query.SetSql( sql );
                query.Execute();
            }

            // Actual check ...
            {
                // ClearParamList is necessary here ...
                query.ClearParameters();

                sql = "SELECT char1_field FROM " + table_name + " ORDER BY id";

                query.SetSql( sql );
                query.Execute();
                ITERATE(CQuery, it, query.SingleSet()) {
                    const string value = it[1].AsString();
                    BOOST_CHECK_EQUAL(value.size(), 1U);
                    BOOST_CHECK_EQUAL(value, string(" "));
                }
            }
        }

    }
    catch(const CSDB_Exception& ex) {
        DBAPI_BOOST_FAIL(ex);
    }
    catch(const CException& ex) {
        DBAPI_BOOST_FAIL(ex);
    }
}

///////////////////////////////////////////////////////////////////////////////
BOOST_AUTO_TEST_CASE(Test_NTEXT)
{
    string sql;

    try {
        string ins_value = "asdfghjkl";

        CQuery query = GetDatabase().NewQuery();

        sql = "SET TEXTSIZE 2147483647";
        query.SetSql( sql );
        query.Execute();

        sql = "create table #test_ntext (txt_fld ntext)";
        query.SetSql( sql );
        query.Execute();

        sql = "insert into #test_ntext values ('" + ins_value + "')";
        query.SetSql( sql );
        query.Execute();

        sql = "SELECT txt_fld from #test_ntext";
        query.SetSql( sql );
        query.Execute();
        ITERATE(CQuery, it, query.SingleSet()) {
            string var = it["txt_fld"].AsString();
            BOOST_CHECK_EQUAL(var, ins_value);
        }
    }
    catch(const CSDB_Exception& ex) {
        DBAPI_BOOST_FAIL(ex);
    }
    catch(const CException& ex) {
        DBAPI_BOOST_FAIL(ex);
    }
}

END_NCBI_SCOPE
