#! /usr/bin/env python
 
# $Id: sample10.py 171052 2009-09-21 12:53:12Z ivanovp $
# ===========================================================================
#
#                            PUBLIC DOMAIN NOTICE
#               National Center for Biotechnology Information
#
#  This software/database is a "United States Government Work" under the
#  terms of the United States Copyright Act.  It was written as part of
#  the author's official duties as a United States Government employee and
#  thus cannot be copyrighted.  This software/database is freely available
#  to the public for use. The National Library of Medicine and the U.S.
#  Government have not placed any restriction on its use or reproduction.
#
#  Although all reasonable efforts have been taken to ensure the accuracy
#  and reliability of the software and data, the NLM and the U.S.
#  Government do not and cannot warrant the performance or results that
#  may be obtained by using this software or data. The NLM and the U.S.
#  Government disclaim all warranties, express or implied, including
#  warranties of performance, merchantability or fitness for any particular
#  purpose.
#
#  Please cite the author in any work or product based on this material.
#
# ===========================================================================
#
# File Name: sample10.py
#
# Author: Sergey Sikorskiy
#
# Description: Stored Procedures.
#
# ===========================================================================

# 1) Import NCBI DBAPI Python extension module
import python_ncbi_dbapi

# 2) Connect to a database
# Parameters: connect(driver_name, db_type, server_name, db_name, user_name, user_pswd, use_std_interface)
# driver_name: ctlib, dblib, ftds, odbc, mysql
# db_type (case insensitive): SYBASE, MSSQL, MYSQL
# server_name: database server name
# db_name: default database name
# use_std_interface: an optional parameter (default value is "False")
conn = python_ncbi_dbapi.connect('ftds', 'MSSQL', 'MS_DEV1', 'DBAPI_Sample', 'anyone', 'allowed')

# 3) Allocate a cursor
cursor = conn.cursor()

# 4) Call Stored Procedure using a "callproc" method.

cursor.callproc('sp_databases')
result = cursor.fetchall()
# Retrieve return status
rc = cursor.get_proc_return_status()

# Call a stored procedure with a parameter.
cursor.callproc('sp_server_info', {'@attribute_id':1} )
result = cursor.fetchall()
# Retrieve return status
rc = cursor.get_proc_return_status()

# 5) Call Stored Procedure using an "execute" method.

cursor.execute('execute sp_databases')
result = cursor.fetchall()
# Retrieve return status
rc = cursor.get_proc_return_status()

# Call a stored procedure with a parameter.
cursor.execute('execute sp_server_info 1')
result = cursor.fetchall()
# Retrieve return status
rc = cursor.get_proc_return_status()
