/* $Id: prosplign_demo.cpp 386465 2013-01-18 14:49:28Z chetvern $
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
 * Author:  Vyacheslav Chetvernin
 *
 * File Description: ProSplign sample application
 *                   
*/

#include <ncbi_pch.hpp>
#include <corelib/ncbiapp.hpp>
#include <objmgr/object_manager.hpp>
#include <objmgr/scope.hpp>
#include <objects/seqloc/Seq_loc.hpp>
#include <serial/objostrasn.hpp>
#include <serial/serial.hpp>
#include <objects/seqalign/Seq_align.hpp>
#include <objtools/data_loaders/genbank/gbloader.hpp>

#include <algo/align/prosplign/prosplign.hpp>


BEGIN_NCBI_SCOPE

class CProSplignApp: public CNcbiApplication
{
public:
    virtual void Init(void);
    virtual int  Run();
};

void CProSplignApp::Init(void)
{
    auto_ptr<CArgDescriptions> arg_desc(new CArgDescriptions);
    arg_desc->AddKey("protein", "proteinSeqEntryID",
                             "Seq-id of the protein",
                             CArgDescriptions::eString);
    arg_desc->AddKey("genomic", "genomicSeqEntryID",
                             "Seq-id of the genomic sequence",
                             CArgDescriptions::eString);
    arg_desc->AddKey("start", "start",
                     "genomic region start",
                     CArgDescriptions::eInteger);
    arg_desc->AddKey("stop", "stop",
                     "genomic region stop",
                     CArgDescriptions::eInteger);
    
    arg_desc->AddFlag("t", "produce alignment text");

    // Program description
    string prog_description = "Example of the ProSplign usage\n";
    arg_desc->SetUsageContext(GetArguments().GetProgramBasename(),
                              prog_description, false);
 
    arg_desc->AddFlag("no_introns", "alignment without introns");
    CProSplignScoring::SetupArgDescriptions(arg_desc.get());
    CProSplignOutputOptions::SetupArgDescriptions(arg_desc.get());

    // Pass argument descriptions to the application
    //
    
    SetupArgDescriptions(arg_desc.release());
}

int CProSplignApp::Run()
{ 
    USING_SCOPE(objects);

    const CArgs& args = GetArgs();

    CRef<CObjectManager> obj_mgr = CObjectManager::GetInstance();

    CGBDataLoader::RegisterInObjectManager(*obj_mgr);

    CScope scope(*obj_mgr);
    scope.AddDefaults();
    /* examples
plus strand:
prosplign_demo -protein EAT35703.1 -genomic NW_001262488.1 -start 10353 -stop 12115
minus strand:
prosplign_demo -protein NP_002346.1 -genomic NT_009714.16 -start 1851937 -stop 1861227

    */

    CSeq_id protein(args["protein"].AsString());
    CSeq_id genomic(args["genomic"].AsString());
    CSeq_loc seqloc(genomic, args["start"].AsInteger(), args["stop"].AsInteger(),eNa_strand_unknown);

    CProSplign prosplign(CProSplignScoring(args),args["no_introns"]);
    CProSplignOutputOptions output_options(args);
    CRef<CSeq_align> alignment = prosplign.FindAlignment(scope, protein, seqloc,
                                                         output_options
                                                         );

    if (args["t"]) {
        CProSplignText::Output(*alignment, scope, cout, 80);
    } else {
        cout << MSerial_AsnText << *alignment << endl;
    }

    return 0;
}

END_NCBI_SCOPE
USING_NCBI_SCOPE;

int main(int argc, const char* argv[]) 
{
    return CProSplignApp().AppMain(argc, argv);
}
