/*  $Id: ns_util.cpp 364029 2012-05-22 16:00:04Z satskyse $
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
 * Authors:  Victor Joukov
 *
 * File Description: Utility functions for NetSchedule
 *
 */

#include <ncbi_pch.hpp>

#include "ns_util.hpp"
#include "ns_queue.hpp"
#include <util/bitset/bmalgo.h>
#include <connect/ncbi_socket.hpp>


BEGIN_NCBI_SCOPE


string NS_FormatIPAddress(unsigned int ipaddr)
{
    unsigned int    hostaddr = CSocketAPI::HostToNetLong(ipaddr);
    char            buf[32];

    sprintf(buf, "%u.%u.%u.%u",
                (hostaddr >> 24) & 0xff,
                (hostaddr >> 16) & 0xff,
                (hostaddr >> 8)  & 0xff,
                 hostaddr        & 0xff);
    return buf;
}


END_NCBI_SCOPE

