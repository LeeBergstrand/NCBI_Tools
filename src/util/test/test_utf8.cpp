/*  $Id: test_utf8.cpp 210746 2010-11-05 20:03:04Z gouriano $
 * ===========================================================================
 *
 *                            PUBLIC DOMAIN NOTICE
 *               National Center for Biotechnology Information
 *
 *   This software/database is a "United States Government Work" under the
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
 * Author: Vladimir Ivanov
 *
 * File Description:
 *   Test program for test UTF-8 conversion functions
 *
 */

#include <ncbi_pch.hpp>
#include <util/utf8.hpp>
#include <util/sgml_entity.hpp>
#include <corelib/ncbiapp.hpp>
#include <util/unicode.hpp>

#define BOOST_AUTO_TEST_MAIN
#include <corelib/test_boost.hpp>

#include <common/test_assert.h>  // This header must go last


USING_NCBI_SCOPE;

BOOST_AUTO_TEST_CASE(TestUtf8)
{
    const size_t MAX_TEST_NUM = 6;
    const char* sTest[MAX_TEST_NUM]={
          "Archiv für Gynäkologie",
          "Phillip Journal für restaurative Zahnmedizin.",
          "Revista odontológica.",
          "Veterinární medicína.",
          "Zhōnghuá zhŏngliú zázhì",
          "Biokhimii\357\270\240a\357\270\241"
    };
    const char* sResult[MAX_TEST_NUM]={
        "Archiv fur Gynakologie",
        "Phillip Journal fur restaurative Zahnmedizin.",
        "Revista odontologica.",
        "Veterinarni medicina.",
        "Zhonghua zhongliu zazhi",
        "Biokhimiia"
    };
    string sRes;
    string s, res;
    vector<long> v;
    utf8::EConversionStatus stat;
    char ch;
    size_t len;
    long vres[] = {
        90, 104, 333, 110, 103, 104, 117, 225, 32, 122, 104, 335, 110, 103,
        108, 105, 250, 32, 122, 225, 122, 104, 236};
    

    // Start passes

    //-----------------------------------
    NcbiCout << "\nUTF -> Ascii\n" << NcbiEndl;

    for (size_t i=0; i<MAX_TEST_NUM; i++)
    {
        sRes=utf8::StringToAscii(sTest[i]);
        BOOST_CHECK_EQUAL(sRes, sResult[i]);
//        NcbiCout << sTest[i] << "\t -> " << sRes << NcbiEndl;
    }

    //-----------------------------------
    NcbiCout << "\nUTF -> Chars\n" << NcbiEndl;

    for (size_t i=0; i<MAX_TEST_NUM; i++)
    {
        s=sTest[i]; 
        res = sResult[i];
        NcbiCout << s << "\n" << NcbiEndl;

        size_t j=0, r=0;
        for (; j<s.size(); )
        {
            ch=utf8::StringToChar(s.data()+j, &len, false, &stat);
//            NcbiCout << s[j] << " (len = "<<len<<")\t - result "
//                     << ch << ", status " << stat << NcbiEndl;
            BOOST_REQUIRE(len > 0);
            if (len == 1) {
                BOOST_REQUIRE(r < res.size());

                BOOST_CHECK_EQUAL(ch,res[r]);
                BOOST_CHECK_EQUAL(stat,utf8::eSuccess);
            } else if (len >= 2) {
                BOOST_CHECK_EQUAL(ch,utf8::kOutrangeChar);
                BOOST_CHECK_EQUAL(stat,utf8::eOutrangeChar);
            }
            j+=len;
            ++r;

// Only for this input data, when len = 3, we should not increment r
            if (len > 2) {
                --r;
            }
        }
        BOOST_CHECK_EQUAL(j, s.size());
        BOOST_CHECK_EQUAL(r, res.size());
        NcbiCout << "----" << NcbiEndl;
    }

    //-----------------------------------
    NcbiCout << "\nUTF -> Vector (last string)\n" << NcbiEndl;

    v=utf8::StringToVector(sTest[4]);    
    for (size_t i=0; i<v.size(); i++)
    {
//        NcbiCout << v[i] << ' ';
        BOOST_CHECK_EQUAL(v[i],vres[i]);
    }

    //-----------------------------------
}


BOOST_AUTO_TEST_CASE(TestSgml)
{
    const size_t MAX_TEST_NUM = 4;
    const char* sTest[MAX_TEST_NUM]={
        "&Agr;&Bgr;&Dgr;&EEgr;&Egr;&Ggr;&Igr;&KHgr;&Kgr;&zgr;",
        ";&Agr;;&Bgr;;&Dgr;;&EEgr;;&Egr;;&Ggr;;&Igr;;&KHgr;;&Kgr;;&zgr;;",
        ";&Agr;&&Bgr;&&Dgr;&&EEgr;&&Egr;&&Ggr;&&Igr;&&KHgr;&&Kgr;&&zgr;&",
        ";&Agr;&;&Bgr;&;&Dgr;&;&EEgr;&;&Egr;&;&Ggr;&;&Igr;&;&KHgr;&;&Kgr;&;&zgr;&;"
    };
    const char* sResult[MAX_TEST_NUM]={
        "<Alpha><Beta><Delta><Eta><Epsilon><Gamma><Iota><Chi><Kappa><zeta>",
        ";<Alpha>;<Beta>;<Delta>;<Eta>;<Epsilon>;<Gamma>;<Iota>;<Chi>;<Kappa>;<zeta>;",
        ";<Alpha>&<Beta>&<Delta>&<Eta>&<Epsilon>&<Gamma>&<Iota>&<Chi>&<Kappa>&<zeta>&",
        ";<Alpha>&;<Beta>&;<Delta>&;<Eta>&;<Epsilon>&;<Gamma>&;<Iota>&;<Chi>&;<Kappa>&;<zeta>&;"
    };

    //-----------------------------------
    NcbiCout << "\nSGML -> Ascii\n" << NcbiEndl;

    for (size_t i=0; i<MAX_TEST_NUM; i++)
    {
        string sRes = Sgml2Ascii(sTest[i]);
//        NcbiCout << sTest[i] << " -> " << sRes << NcbiEndl;
        BOOST_CHECK_EQUAL(sRes, sResult[i]);
    }
}

BOOST_AUTO_TEST_CASE(TestUnicodeToAscii)
{
    NcbiCout << "\nUnicode -> Ascii\n" << NcbiEndl;
    utf8::TUnicode unidata[256];
    const utf8::SUnicodeTranslation* u2a_translation;
    string ascdata;
    int i;

// --------------------------------------
    for (i=0; i< 126; ++i) {
        unidata[i] = i+1;
    }
    unidata[i] = 0;
    for (i=0; unidata[i] != 0; ++i) {
        u2a_translation = utf8::UnicodeToAscii(unidata[i]);
        NcbiCout << unidata[i] << " -> ";
        if (u2a_translation) {
            NcbiCout << u2a_translation->Subst;
        }
        NcbiCout << NcbiEndl;
        BOOST_CHECK (u2a_translation);
        BOOST_CHECK (u2a_translation->Subst[0] == (char)unidata[i]);
    }

// --------------------------------------
    ascdata.clear();
    for (i=0; i< 5; ++i) {
        unidata[i] = i+934;
    }
    unidata[i] = 0;
    for (i=0; unidata[i] != 0; ++i) {
        u2a_translation = utf8::UnicodeToAscii(unidata[i]);
        NcbiCout << unidata[i] << " -> ";
        if (u2a_translation) {
            NcbiCout << u2a_translation->Subst;
            ascdata += u2a_translation->Subst;
        }
        NcbiCout << NcbiEndl;
    }
    BOOST_CHECK_EQUAL(ascdata, "PhiChiPsiOmegaIota");

// --------------------------------------
    ascdata.clear();
    for (i=0; i< 3; ++i) {
        unidata[i] = i+128640;
    }
    unidata[i] = 0;
    for (i=0; unidata[i] != 0; ++i) {
        u2a_translation = utf8::UnicodeToAscii(unidata[i]);
        NcbiCout << unidata[i] << " -> ";
        if (u2a_translation) {
            NcbiCout << u2a_translation->Subst;
            ascdata += u2a_translation->Subst;
        }
        NcbiCout << NcbiEndl;
    }
    BOOST_CHECK_EQUAL(ascdata, "RocketHelicopterSteam locomotive");
}
