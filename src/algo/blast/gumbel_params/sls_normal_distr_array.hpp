#ifndef ALGO_BLAST_GUMBEL_PARAMS__INCLUDED_NORMAL_DISTR_ARRAY
#define ALGO_BLAST_GUMBEL_PARAMS__INCLUDED_NORMAL_DISTR_ARRAY

/* $Id: sls_normal_distr_array.hpp 328112 2011-08-01 13:59:27Z boratyng $
* ===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's offical duties as a United States Government employee and
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
* ===========================================================================*/

/*****************************************************************************

File name: sls_normal_distr_array.hpp

Author: Sergey Sheetlin

Contents: Normal distribution array for P-values calculation

******************************************************************************/

#include <corelib/ncbistl.hpp>

BEGIN_NCBI_SCOPE
BEGIN_SCOPE(blast)

BEGIN_SCOPE(Sls)

#define NORMAL_DISTR_ARRAY_DIM 1000

double* GetNormalDistrArrayForPvaluesCalculation(void);

END_SCOPE(Sls)

END_SCOPE(blast)
END_NCBI_SCOPE

#endif //! ALGO_BLAST_GUMBEL_PARAMS__INCLUDED_NORMAL_DISTR_ARRAY

