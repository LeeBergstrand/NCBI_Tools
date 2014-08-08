/*  $Id: test_relloc.cpp 103491 2007-05-04 17:18:18Z kazimird $
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
* Author:  Aaron Ucko
*
* File Description:
*   test code for SRelLoc
*/

#include <ncbi_pch.hpp>
#include <corelib/ncbistd.hpp>

#include <corelib/ncbiapp.hpp>
#include <corelib/ncbiargs.hpp>
#include <corelib/ncbienv.hpp>

#include <objects/seq/Bioseq.hpp>
#include <objects/seqfeat/Seq_feat.hpp>

#include <objmgr/bioseq_handle.hpp>
#include <objmgr/feat_ci.hpp>
#include <objtools/data_loaders/genbank/gbloader.hpp>
#include <objmgr/object_manager.hpp>
#include <objmgr/scope.hpp>

#include <objmgr/util/sequence.hpp>

BEGIN_NCBI_SCOPE
USING_SCOPE(objects);
USING_SCOPE(sequence);

class CRelLocTester : public CNcbiApplication
{
    virtual void Init(void);
    virtual int  Run(void);
};


void CRelLocTester::Init(void)
{
    auto_ptr<CArgDescriptions> arg_desc(new CArgDescriptions);
    arg_desc->SetUsageContext(GetArguments().GetProgramBasename(),
                              "Test the use of SRelLoc", false);

    arg_desc->AddKey("gi", "SeqEntryID", "GI id of the Seq-Entry to examine",
                     CArgDescriptions::eInteger);

    SetupArgDescriptions(arg_desc.release());
}


int CRelLocTester::Run(void)
{
    const CArgs&   args = GetArgs();
    CRef<CObjectManager> objmgr = CObjectManager::GetInstance();
    CScope         scope(*objmgr);
    CSeq_id        id;
    
    id.SetGi(args["gi"].AsInteger());

    CGBDataLoader::RegisterInObjectManager(*objmgr);
    scope.AddDefaults();

    CBioseq_Handle handle = scope.GetBioseqHandle(id); 
    CConstRef<CSeq_loc> parent;
    for (CFeat_CI it(handle, CSeqFeatData::e_Cdregion);  it;  ++it) {
        parent = &it->GetLocation();
        BREAK(it);
    }
    if ( !parent ) {
        // use the middle third
        TSeqPos length = handle.GetBioseqCore()->GetInst().GetLength();
        CRef<CSeq_interval> ival(new CSeq_interval);
        ival->SetFrom(  length/3);
        ival->SetTo  (2*length/3);
        CRef<CSeq_loc> p(new CSeq_loc);
        p->SetInt(*ival);
        parent = p;
    }
    {{
        string label;
        parent->GetLabel(&label);
        cout << "Using parent location " << label << endl;
    }}

    for (CFeat_CI it(handle);  it;  ++it) {
        const CSeq_loc& child = it->GetLocation();
        string label;
        child.GetLabel(&label);
        cout << "Child location " << label << " maps to " << flush;
        SRelLoc rl(*parent, child, &scope);
        if (rl.m_Ranges.empty()) {
            cout << "nothing" << endl;
            _ASSERT(sequence::Compare(*parent, child, &scope) ==
                    sequence::eNoOverlap);
        } else {
            string sep;
            ITERATE (SRelLoc::TRanges, r, rl.m_Ranges) {
                cout << sep << (*r)->GetFrom() << "-" << (*r)->GetTo();
                sep = ", ";
            }
            label.erase();
            rl.Resolve(&scope)->GetLabel(&label);
            cout << endl << "... and back to " << label << endl;
        }
    }
    return 0;
}


END_NCBI_SCOPE

USING_NCBI_SCOPE;

int main(int argc, const char** argv)
{
    return CRelLocTester().AppMain(argc, argv);
}
