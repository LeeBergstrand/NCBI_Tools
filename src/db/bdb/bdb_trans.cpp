/*  $Id: bdb_trans.cpp 163327 2009-06-15 15:40:12Z ivanovp $
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
 * Author: Anatoliy Kuznetsov
 *
 * File Description:  BDB libarary transaction class implementation.
 *
 */

#include <ncbi_pch.hpp>
#include <db/bdb/bdb_trans.hpp>
#include <db/bdb/bdb_env.hpp>
#include <db/bdb/bdb_file.hpp>
#include <db.h>

BEGIN_NCBI_SCOPE

CBDB_Transaction::CBDB_Transaction(CBDB_Env&            env,
                                   ETransSync           tsync,
                                   EKeepFileAssociation assoc)
 : m_Env(env),
   m_TSync(tsync),
   m_Assoc(assoc),
   m_Txn(0)
{
    if (m_TSync == eEnvDefault) {
        m_TSync = m_Env.GetTransactionSync();
    }
    // Env default should not come from the env (env must be specific)
    _ASSERT(m_TSync != eEnvDefault);
}


CBDB_Transaction::~CBDB_Transaction()
{
    if (m_Txn) {       // Active transaction is in progress
        x_Abort(true); // Abort - ignore errors (no except. from destructor)
    }
    x_DetachFromFiles();
}

DB_TXN* CBDB_Transaction::GetTxn()
{
    if (m_Env.IsTransactional()) {
        if (m_Txn == 0) {
            m_Txn = m_Env.CreateTxn(0, 0);
        }
    }
    return m_Txn;
}

void CBDB_Transaction::Commit()
{
    if (m_Txn) {
        u_int32_t flags =
            m_TSync == eTransSync ? DB_TXN_SYNC : DB_TXN_NOSYNC;
        int ret = m_Txn->commit(m_Txn, flags);
        m_Txn = 0;
        BDB_CHECK(ret, "DB_TXN::commit");
    }
    x_DetachFromFiles();
}

void CBDB_Transaction::Abort()
{
    x_Abort(false); // abort with full error processing
    x_DetachFromFiles();
}

void CBDB_Transaction::x_Abort(bool ignore_errors)
{
    if (m_Txn) {
        int ret = m_Txn->abort(m_Txn);
        m_Txn = 0;
        if (!ignore_errors) {
            BDB_CHECK(ret, "DB_TXN::abort");
        }
    }
}

void CBDB_Transaction::x_DetachFromFiles()
{
    if (m_Assoc == eFullAssociation) {
        NON_CONST_ITERATE(TTransVector, it, m_TransFiles) {
            ITransactional* dbfile = *it;
            dbfile->RemoveTransaction(this);
        }
    }
    m_TransFiles.resize(0);
}


void CBDB_Transaction::Add(ITransactional* dbfile)
{
    if (m_Assoc == eFullAssociation) {
        m_TransFiles.push_back(dbfile);
    }
}


void CBDB_Transaction::Remove(ITransactional* dbfile)
{
    if (m_Assoc == eFullAssociation) {
        NON_CONST_ITERATE(TTransVector, it, m_TransFiles) {
            if (dbfile == *it) {
                m_TransFiles.erase(it);
                break;
            }
        }
    }
}

CBDB_Transaction* CBDB_Transaction::CastTransaction(ITransaction* trans)
{
    if (trans == 0) {
        return 0;
    }
    CBDB_Transaction* db_trans = dynamic_cast<CBDB_Transaction*>(trans);
    if (db_trans == 0) {
        BDB_THROW(eForeignTransaction,
                  "Incorrect transaction type (non-BerkeleyDB)");
    }
    return db_trans;
}



END_NCBI_SCOPE
