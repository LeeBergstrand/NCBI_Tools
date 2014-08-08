/*  $Id: test_semaphore_mt.cpp 366759 2012-06-18 17:17:54Z ivanov $
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
 * Author:  Andrei Gourianov, gouriano@ncbi.nlm.nih.gov
 *
 * File Description:
 *   Test CSemaphore class in multithreaded environment
 *   NOTE: in order to run correctly the number of threads MUST be even!
 *
 *   the test is a very simple producer/consumer model
 *   one thread produces "items" (increments integer counter)
 *	 next thread consumes the same amount of items (decrements integer counter)
 *	 "Content" semaphore is used to notify consumers of how many items are
 *   available.
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbithr.hpp>
#include <corelib/ncbimtx.hpp>
#include <corelib/ncbi_system.hpp>
#include <corelib/test_mt.hpp>

#include <common/test_assert.h>  /* This header must go last */

USING_NCBI_SCOPE;


/////////////////////////////////////////////////////////////////////////////
//  Test application

class CTestSemaphoreApp : public CThreadedApp
{
public:
    virtual bool Thread_Init(int idx);
    virtual bool Thread_Run(int idx);
protected:
    virtual bool TestApp_Init(void);
    virtual bool TestApp_Exit(void);

private:
    // produce Num items
    void Produce(int Num);
    // consume Num items
    void Consume(int Num);

    static CSemaphore s_semContent, s_semStorage;
    static int s_Counter, s_Id;
};


/////////////////////////////////////////////////////////////////////////////

CSemaphore CTestSemaphoreApp::s_semContent(0,10);
CSemaphore CTestSemaphoreApp::s_semStorage(1,1);

int CTestSemaphoreApp::s_Counter=0;
int CTestSemaphoreApp::s_Id=0;


/////////////////////////////////////////////////////////////////////////////
//  IMPLEMENTATION

void CTestSemaphoreApp::Produce(int Num)
{
    // Storage semaphore acts as a kind of mutex - its only purpose
    // is to protect Counter
    s_semStorage.Wait();
    s_Counter += Num;
    NcbiCout << "+" << Num << "=" << s_Counter << NcbiEndl;
    s_semStorage.Post();

    // Content semaphore notifies consumer threads of how many items can be
    // consumed. Slow consumption with fast production causes Content semaphore
    // to overflow from time to time. We catch exception and wait for consumers
    // to consume something
    for (bool Posted=false; !Posted;) {
        try {
            s_semContent.Post(Num);
            Posted = true;
        }
        catch (exception& e) {
            NcbiCout << e.what() << NcbiEndl;
            SleepMilliSec(500);
        }
    }
}


void CTestSemaphoreApp::Consume(int Num)
{
    for (int i = Num; i > 0; --i ) {
        // we can only consume one by one
        s_semContent.Wait();
        s_semStorage.Wait();
        --s_Counter;
        NcbiCout << "-1=" << s_Counter << NcbiEndl;
        s_semStorage.Post();
        SleepMilliSec(500);
    }
}


bool CTestSemaphoreApp::Thread_Init(int /*idx*/)
{
    return true;
}


bool CTestSemaphoreApp::Thread_Run(int idx)
{
    //  One thread produces, next - consumes;
    //  production is fast, consumption is slow (because of Sleep).
    //  NOTE:  In order to run correctly the number of threads MUST be even!

    xncbi_SetValidateAction(eValidate_Throw);

    if ( idx % 2 != 1) {
        Produce((idx/2)%3 + 1);
    } 
    else {
        Consume((idx/2)%3 + 1);
    }
    return true;
}


bool CTestSemaphoreApp::TestApp_Init(void)
{
    NcbiCout
        << NcbiEndl
        << "Testing semaphores with "
        << NStr::UIntToString(s_NumThreads)
        << " threads"
        << NcbiEndl;
    if ( s_NumThreads%2 != 0 ) {
#ifdef NCBI_THREADS
        throw runtime_error("The number of threads MUST be even");
#else
        ++s_NumThreads;
#endif
    }
    return true;
}


bool CTestSemaphoreApp::TestApp_Exit(void)
{
    NcbiCout
        << "Test completed"
        << NcbiEndl
        << " counter = " << s_Counter
        << NcbiEndl;
    // storage must be available
    assert( s_semStorage.TryWait() );
    // content must be empty
    assert( !s_semContent.TryWait() );
	assert( s_Counter == 0 );
    return true;
}



/////////////////////////////////////////////////////////////////////////////
//  MAIN

int main(int argc, const char* argv[]) 
{
    return CTestSemaphoreApp().AppMain(argc, argv, 0, eDS_Default, 0);
}
