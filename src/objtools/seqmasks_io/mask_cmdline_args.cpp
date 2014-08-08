/*  $Id: mask_cmdline_args.cpp 123978 2008-04-08 16:50:21Z camacho $
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
 * Author: 
 *
 */

/** @file mask_cmdline_args.cpp
 *  
 */

#ifndef SKIP_DOXYGEN_PROCESSING
static char const rcsid[] = 
    "$Id: mask_cmdline_args.cpp 123978 2008-04-08 16:50:21Z camacho $";
#endif /* SKIP_DOXYGEN_PROCESSING */

#include <ncbi_pch.hpp>
#include <objtools/seqmasks_io/mask_cmdline_args.hpp>

BEGIN_NCBI_SCOPE

/* Constants from mask_cmdline_args.hpp */
const std::string kInput("in");
const std::string kInputFormat("infmt");
const std::string kOutput("out");
const std::string kOutputFormat("outfmt");
const size_t kNumInputFormats = 2;
const size_t kNumOutputFormats = 8;
const char* kInputFormats[] =  { "fasta", "blastdb" };
const char* kOutputFormats[] = 
    { "interval", "fasta", 
      "seqloc_asn1_bin", "seqloc_asn1_text", "seqloc_xml",
      "maskinfo_asn1_bin", "maskinfo_asn1_text", "maskinfo_xml",
    };

END_NCBI_SCOPE
