/*===========================================================================
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
*/

#include <ktst/unit_test.hpp>

using namespace ncbi::NK;

TestRunner::TestRunner() : argc(0), argv(NULL) {}

void TestRunner::ReportTestNumber(void) 
{
    T::size_type sz = _cases.size();
    if (sz == 1) {
       LOG(LogLevel::e_fatal_error, "Running " << sz << " test case...\n");
    } else if (sz > 1) {
       LOG(LogLevel::e_fatal_error, "Running " << sz << " test cases...\n");
    }
}

void TestRunner::SetArgs(int argc, char* argv[]) 
{
    this->argc = argc;
    this->argv = argv;
}

void TestRunner::Add(ncbi::NK::TestInvoker* t) 
{
    if (t) 
    {
        _cases.push_back(t);
    }
}

counter_t TestRunner::Run(void* globalFixture) const 
{
    counter_t ec = 0;
    for (TCI it = _cases.begin(); it != _cases.end(); ++it) 
    {
       ncbi::NK::TestInvoker* c = *it;
       try {
           LOG(LogLevel::e_test_suite,
               "Entering test case \"" << c->GetName() << "\"\n");
           c->Run(globalFixture);
           LOG(LogLevel::e_test_suite,
               "Leaving test case \"" << c->GetName() << "\"\n");
       } catch (const ncbi::NK::execution_aborted&) {
           LOG(LogLevel::e_test_suite,
               "Leaving test case \"" << c->GetName() << "\"\n");
       }
       ec += c->GetErrorCounter();
    }
    return ec;
}

