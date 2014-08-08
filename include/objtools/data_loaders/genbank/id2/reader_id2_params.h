#ifndef GBLOADER_ID2_PARAMS__H_INCLUDED
#define GBLOADER_ID2_PARAMS__H_INCLUDED

/*  $Id: reader_id2_params.h 178597 2009-12-14 20:14:16Z vasilche $
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
*  ===========================================================================
*
*  Author: Eugene Vasilchenko
*
*  File Description:
*    GenBank reader id2 configuration parameters
*
* ===========================================================================
*/

#include <objtools/data_loaders/genbank/reader_service_params.h>

/* Name of ID2 reader driver */
#define NCBI_GBLOADER_READER_ID2_DRIVER_NAME "id2"

/* NCBI service name to connect to */
#define NCBI_GBLOADER_READER_ID2_PARAM_SERVICE_NAME "service"

#endif
