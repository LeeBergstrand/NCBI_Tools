#ifndef UTIL_IMAGE___IMAGE_UTIL__HPP
#define UTIL_IMAGE___IMAGE_UTIL__HPP

/*  $Id: image_util.hpp 103491 2007-05-04 17:18:18Z kazimird $
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

#include <util/image/image.hpp>

BEGIN_NCBI_SCOPE


class NCBI_XIMAGE_EXPORT CImageUtil
{
public:

    enum EScale {
        eScale_Average,
        eScale_Max,
        eScale_Min
    };

    static CImage* Scale(const CImage& image,
                         size_t width, size_t height,
                         EScale scale = eScale_Average);

    // flip an image along the X axis (its width)
    static void FlipX(CImage& image);

    // flip an axis along the Y axis (its height)
    static void FlipY(CImage& image);
};


END_NCBI_SCOPE

#endif  // UTIL_IMAGE___IMAGE_UTIL__HPP
