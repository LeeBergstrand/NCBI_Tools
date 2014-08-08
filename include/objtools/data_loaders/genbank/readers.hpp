#ifndef GENBANK___READERS__HPP
#define GENBANK___READERS__HPP

/* $Id: readers.hpp 182287 2010-01-28 16:15:58Z vasilche $
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
 * Author:  Aleksey Grichenko
 *   
 * File Description:  Data readers' registration
 *
 */


BEGIN_NCBI_SCOPE

extern "C" {

extern void NCBI_XREADER_ID1_EXPORT      GenBankReaders_Register_Id1   (void);
extern void NCBI_XREADER_ID2_EXPORT      GenBankReaders_Register_Id2   (void);
extern void NCBI_XREADER_PUBSEQOS_EXPORT GenBankReaders_Register_Pubseq(void);
extern void NCBI_XREADER_PUBSEQOS2_EXPORT GenBankReaders_Register_Pubseq2(void);
extern void NCBI_XREADER_CACHE_EXPORT    GenBankReaders_Register_Cache (void);
extern void NCBI_XREADER_CACHE_EXPORT    GenBankWriters_Register_Cache (void);
extern void NCBI_XREADER_GICACHE_EXPORT  GenBankReaders_Register_GICache(void);

}

END_NCBI_SCOPE

#endif  /* GENBANK___READERS__HPP */
