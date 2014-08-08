/*  $Id: test_reader_pubseq.cpp 141052 2008-09-23 18:45:55Z vasilche $
* ===========================================================================
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
* ===========================================================================
*/

#include <ncbi_pch.hpp>
#include <iostream>
#include <serial/serial.hpp>
#include <objects/seqset/Seq_entry.hpp>
#include <objects/seqloc/Seq_id.hpp>
#include <objects/seqloc/Textseq_id.hpp>
#include <serial/objistrasnb.hpp>
#include <serial/objostrasn.hpp>
#include <objtools/data_loaders/genbank/pubseq/reader_pubseq.hpp>
#include <objtools/data_loaders/genbank/request_result.hpp>
#include <objtools/data_loaders/genbank/dispatcher.hpp>
#include <objmgr/impl/tse_info.hpp>

#include <connect/ncbi_util.h>
#include <connect/ncbi_core_cxx.hpp>

#include <common/test_assert.h>  /* This header must go last */


USING_NCBI_SCOPE;
USING_SCOPE(objects);
using namespace std;

int main()
{
    for(int k = 0; k < 10; ++k) {
        cout << "K: " << k << endl;

        CRef<CReadDispatcher> dispatcher(new CReadDispatcher);
        CRef<CReader> reader(new CPubseqReader);
        dispatcher->InsertReader(0, reader);

        int gi = 156895+k-1;

        CSeq_id_Handle seq_id = CSeq_id_Handle::GetGiHandle(gi);
        CStandaloneRequestResult request(seq_id);
        CLoadLockBlob_ids ids(request, seq_id, 0);
        dispatcher->LoadSeq_idBlob_ids(request, seq_id, 0);
        ITERATE ( CLoadInfoBlob_ids, i, *ids ) {
            CConstRef<CBlob_id> blob_id = i->first;
            cout << "gi: " << gi <<
                " Sat=" << blob_id->GetSat() <<
                " SatKey=" << blob_id->GetSatKey() << endl;
      
            CLoadLockBlob blob(request, *blob_id);
            dispatcher->LoadBlob(request, *blob_id);
            if ( !blob.IsLoaded() ) {
                cout << "blob is not available\n";
                continue;
            }
            cout <<"gi: " << gi << " " << 1 << " blobs" << endl;
        }
    }
    return 0;
}
