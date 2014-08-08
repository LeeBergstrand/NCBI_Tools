#ifndef LOG___BDB_SPLIT_CURSOR__HPP
#define LOG___BDB_SPLIT_CURSOR__HPP

/*  $Id: bdb_split_cursor.hpp 163413 2009-06-15 19:59:58Z ivanovp $
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
 * Authors:  Mike DiCuccio
 *
 * File Description:
 *
 */

#include <corelib/ncbistd.hpp>
#include <corelib/ncbifile.hpp>
#include <db/bdb/bdb_file.hpp>
#include <db/bdb/bdb_cursor.hpp>
#include <db/bdb/bdb_env.hpp>
#include <db/error_codes.hpp>


BEGIN_NCBI_SCOPE



template <typename BDB_SplitStore, typename BDB_Vol = typename BDB_SplitStore::TBlobFile>
class CBDB_SplitCursor
{
public:
    struct SVolumeLess : public binary_function<string, string, bool>
    {
        bool operator() (const string& s1, const string& s2) const
        {
            string::size_type pos1 = s1.find_last_of("_");
            if (pos1 == string::npos) {
                pos1 = 0;
            }
            string::size_type pos2 = s2.find_last_of("_");
            if (pos2 == string::npos) {
                pos2 = 0;
            }

            CTempString ts1(s1, pos1, s1.size());
            CTempString ts2(s2, pos2, s2.size());
            if (ts1 < ts2) {
                return true;
            }
            if (ts2 < ts1) {
                return false;
            }

            return s1 < s2;
        }
    };
    typedef BDB_SplitStore TSplitStore;
    typedef BDB_Vol        TVolume;

    CBDB_SplitCursor(TSplitStore& store);
    void InitMultiFetch(size_t buffer_size);
    EBDB_ErrCode Fetch();
    const void* GetLastMultiFetchData() const;
    size_t      GetLastMultiFetchDataLen() const;
    TVolume&    GetSourceVolume();
    NCBI_DEPRECATED Uint4       GetCurrentBlobId() const;
    Uint4       GetLastBlobId() const;

private:
    CBDB_Env* m_Env;
    string m_Path;
    string m_StoreName;
    size_t m_BufferSize;

    vector<string>            m_Files;
    auto_ptr<TVolume>         m_Volume;
    auto_ptr<CBDB_FileCursor> m_Cursor;

    CStopWatch m_SW;

    void x_NextVolume();
};


template <typename BDB_SplitStore, typename BDB_Vol>
inline
CBDB_SplitCursor<BDB_SplitStore, BDB_Vol>::CBDB_SplitCursor(TSplitStore& store)
    : m_Env(NULL)
    , m_BufferSize(40 * 1024 * 1024)
{
    m_SW.Start();
    m_Env = store.GetEnv();

    ///
    /// find our relevant files
    ///
    {{
         vector<string> paths;
         vector<string> masks;

         string path = store.GetFileName();
         string path_dir;
         string path_base;
         string path_ext;
         CDirEntry::SplitPath(path, &path_dir, &path_base, &path_ext);
         path_base += path_ext;

         if (CDirEntry::IsAbsolutePath(path_dir)) {
             path = path_dir;
         } else {
             path.erase();
             if (m_Env) {
                 path = m_Env->GetPath();
                 path += "/";
             }
             path += path_dir;
             path = CDirEntry::CreateAbsolutePath(path);
         }

         paths.push_back(path);
         masks.push_back(path_base + "_*");
         FindFiles(m_Files,
                   paths.begin(), paths.end(), masks.begin(), masks.end(),
                   fFF_File);

         std::sort(m_Files.begin(), m_Files.end(), SVolumeLess());

         LOG_POST_XX(Db_Bdb_Cursor, 2, Info <<
                     "found " << m_Files.size() << " candidate files");
     }}
}


template <typename BDB_SplitStore, typename BDB_Vol>
inline EBDB_ErrCode
CBDB_SplitCursor<BDB_SplitStore, BDB_Vol>::Fetch()
{
    for (;;) {
        if ( !m_Cursor.get()  ||  m_Cursor->Fetch() != eBDB_Ok) {
            x_NextVolume();
            if ( !m_Cursor.get() ) {
                return eBDB_NotFound;
            }
        } else {
            break;
        }
    }

    return eBDB_Ok;
}


template <typename BDB_SplitStore, typename BDB_Vol>
inline void
CBDB_SplitCursor<BDB_SplitStore, BDB_Vol>::InitMultiFetch(size_t size)
{
    m_BufferSize = size;
}


template <typename BDB_SplitStore, typename BDB_Vol>
inline const void*
CBDB_SplitCursor<BDB_SplitStore, BDB_Vol>::GetLastMultiFetchData() const
{
    if (m_Cursor.get()) {
        return m_Cursor->GetLastMultiFetchData();
    }
    return NULL;
}


template <typename BDB_SplitStore, typename BDB_Vol>
inline size_t
CBDB_SplitCursor<BDB_SplitStore, BDB_Vol>::GetLastMultiFetchDataLen() const
{
    if (m_Cursor.get()) {
        return m_Cursor->GetLastMultiFetchDataLen();
    }
    return 0;
}


template <typename BDB_SplitStore, typename BDB_Vol>
inline typename CBDB_SplitCursor<BDB_SplitStore, BDB_Vol>::TVolume&
CBDB_SplitCursor<BDB_SplitStore, BDB_Vol>::GetSourceVolume()
{
    if (m_Volume.get()) {
        return *m_Volume;
    }
    NCBI_THROW(CException, eUnknown, "no open volume");
}


template <typename BDB_SplitStore, typename BDB_Vol>
inline Uint4
CBDB_SplitCursor<BDB_SplitStore, BDB_Vol>::GetCurrentBlobId() const
{
    return GetLastBlobId();
}


template <typename BDB_SplitStore, typename BDB_Vol>
inline Uint4
CBDB_SplitCursor<BDB_SplitStore, BDB_Vol>::GetLastBlobId() const
{
    if (m_Volume.get()) {
        return (Uint4)m_Volume->GetUid();
    }
    NCBI_THROW(CException, eUnknown, "no open volume");
}


template <typename BDB_SplitStore, typename BDB_Vol>
inline void
CBDB_SplitCursor<BDB_SplitStore, BDB_Vol>::x_NextVolume()
{
    /// get rid of our existing cursor + volume
    m_Cursor.reset();
    m_Volume.reset();
    if ( !m_Files.size() ) {
        return;
    }

    /// open the next file
    string path = m_Files.back();
    m_Files.pop_back();

    m_Volume.reset(new TVolume);
    m_Volume->SetCacheSize(10 * 1024 * 1024);
    if (m_Env) {
        m_Volume->SetEnv(*m_Env);
    }

    LOG_POST_XX(Db_Bdb_Cursor, 1, Info
                << "CBDB_SplitCursor::x_NextVolume(): opening: " << path);
    m_Volume->Open(path, CBDB_RawFile::eReadOnly);

    m_Cursor.reset(new CBDB_FileCursor(*m_Volume));
    m_Cursor->InitMultiFetch(m_BufferSize);
}


END_NCBI_SCOPE


#endif  // LOG___BDB_SPLIT_CURSOR__HPP
