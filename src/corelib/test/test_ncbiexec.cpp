/*  $Id: test_ncbiexec.cpp 366759 2012-06-18 17:17:54Z ivanov $
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
 * Author: Vladimir Ivanov
 *
 * File Description:   Test program for portable exec functions
 *
 * Note:  On mS-Windows Cygwin should be installed and added to PATH before
 *        run this test program, because we use 'ls' command.
 *
 */

#include <ncbi_pch.hpp>
#include <corelib/ncbiapp.hpp>
#include <corelib/ncbiargs.hpp>
#include <corelib/ncbienv.hpp>
#include <corelib/ncbiexec.hpp>

#if defined(HAVE_UNISTD_H)
#  include <unistd.h>         // for _exit()
#endif

#include <common/test_assert.h> // This header must go last


USING_NCBI_SCOPE;


#define TEST_RESULT_C    99   // Test exit code for current test
#define TEST_RESULT_P     0   // Test exit code for test from PATH


////////////////////////////////
// Test application
//

class CTest : public CNcbiApplication
{
public:
    void Init(void);
    int Run(void);
};


void CTest::Init(void)
{
    SetDiagPostLevel(eDiag_Warning);
    auto_ptr<CArgDescriptions> d(new CArgDescriptions);
    d->SetUsageContext("test_files",
                       "test file's accessory functions");
    SetupArgDescriptions(d.release());
}


int CTest::Run(void)
{
    string app = GetArguments().GetProgramName();
    LOG_POST("Application path: " << app);

    // Initialization of variables and structures

    char* app_c = strdup(app.c_str());
    assert( app_c != 0 );

    const char* app_p  = "ls";
    const char* app_pp = "..";

    const char* my_env[] =   // Environment for Spawn*E
    {
        "THIS=environment will be",
        "PASSED=to new process by the",
        "EXEC=functions",
        "TEST_NCBI_EXEC=yes",
        NULL
    };

    {{
#if defined(NCBI_OS_CYGWIN)
        string path_setting("PATH=");
        const string& path = GetEnvironment().Get("PATH");
#else
        string path_setting("LD_LIBRARY_PATH=");
        const string& path = GetEnvironment().Get("LD_LIBRARY_PATH");
#endif
        if (path.size()) {
            path_setting += path;
            my_env[0] = strdup(path_setting.c_str());
        }
    }}

    const char* args_c[3];   // Arguments for SpawnV[E]
    args_c[1] = "SpawnV[E]";
    args_c[2] = NULL;

    const char* args_p[3];   // Arguments for SpawnVP[E]
    args_p[1] = app_pp;
    args_p[2] = NULL;


    // ResolvePath() test

    assert( CExec::IsExecutable(app_c) );
    string res_path;
    res_path = CExec::ResolvePath(app_c);
    LOG_POST("Resolve path: " << app_c << " -> " << res_path);
    assert( !res_path.empty() );
    res_path = CExec::ResolvePath(app_p);
    LOG_POST("Resolve path: " << app_p << " -> " << res_path);
    assert( !res_path.empty() );
#if defined(NCBI_OS_MSWIN)
    res_path = CExec::ResolvePath("winver.exe");
    LOG_POST("Resolve path: " << "winver.exe" << " -> " << res_path);
    assert( !res_path.empty() );
#endif


    // System

#if !defined(NCBI_OS_CYGWIN)
    // This test doesn't work on GCC/Cygwin
    assert( CExec::System(0) > 0 );
#endif
    string cmd = app + " System";
    assert( CExec::System(cmd.c_str()) == TEST_RESULT_C );
    cmd = string(app_p) + " " + app_pp;
    assert( CExec::System(cmd.c_str()) == TEST_RESULT_P );
    
    // Spawn with eWait

    TExitCode code;

    code = CExec::SpawnL  (CExec::eWait, app_c, "SpawnL_eWait",
                           NULL).GetExitCode(); 
    LOG_POST("Exit code: " << code);
    assert( code == TEST_RESULT_C );

    code = CExec::SpawnLP (CExec::eWait, app_p, app_pp,
                           NULL).GetExitCode();
    LOG_POST("Exit code: " << code);
    assert( code == TEST_RESULT_P );

    code = CExec::SpawnLE (CExec::eWait, app_c, "SpawnLE_eWait",
                           NULL, my_env).GetExitCode(); 
    LOG_POST("Exit code: " << code);
    assert( code == TEST_RESULT_C );

    code = CExec::SpawnLPE(CExec::eWait, app_c, "SpawnLPE_eWait",
                           NULL, my_env).GetExitCode();
    LOG_POST("Exit code: " << code);
    assert( code == TEST_RESULT_C );

    code = CExec::SpawnV  (CExec::eWait, app_c, args_c).GetExitCode();
    LOG_POST("Exit code: " << code);
    assert( code == TEST_RESULT_C );

    code = CExec::SpawnVP (CExec::eWait, app_p, args_p).GetExitCode();
    LOG_POST("Exit code: " << code);
    assert( code == TEST_RESULT_P );

    code = CExec::SpawnVE (CExec::eWait, app_c, args_c, my_env).GetExitCode();
    LOG_POST("Exit code: " << code);
    assert( code == TEST_RESULT_C );

    code = CExec::SpawnVPE(CExec::eWait, app_c, args_c, my_env).GetExitCode();
    LOG_POST("Exit code: " << code);
    assert( code == TEST_RESULT_C );


    // Spawn with eNoWait, waiting self

    TProcessHandle handle;

    handle = CExec::SpawnL  (CExec::eNoWait, app_c, "SpawnL_eNoWait",
                             NULL).GetProcessHandle();
    assert(CExec::Wait(handle) == TEST_RESULT_C );
    handle = CExec::SpawnLP (CExec::eNoWait, app_p, app_pp,
                             NULL).GetProcessHandle();
    assert(CExec::Wait(handle) == TEST_RESULT_P);
    handle = CExec::SpawnLE (CExec::eNoWait, app_c, "SpawnLE_eNoWait",
                             NULL, my_env).GetProcessHandle();
    assert(CExec::Wait(handle) == TEST_RESULT_C);
    handle = CExec::SpawnLPE(CExec::eNoWait, app_c, "SpawnLPE_eNoWait",
                             NULL, my_env).GetProcessHandle();
    assert(CExec::Wait(handle) == TEST_RESULT_C);


    // Spawn with eDetach

    CExec::SpawnL  (CExec::eDetach, app_c, "SpawnL_eDetach", NULL);
    CExec::SpawnLP (CExec::eDetach, app_p, app_pp, NULL);
    CExec::SpawnLE (CExec::eDetach, app_c, "SpawnLE_eDetach", NULL, my_env);
    CExec::SpawnLPE(CExec::eDetach, app_c, "SpawnLPE_eDetach",NULL, my_env);
    CExec::SpawnV  (CExec::eDetach, app_c, args_c);
    CExec::SpawnVP (CExec::eDetach, app_p, args_p);
    CExec::SpawnVE (CExec::eDetach, app_c, args_c, my_env);
    CExec::SpawnVPE(CExec::eDetach, app_c, args_c, my_env);


    // Spawn with eWaitGroup

    code = CExec::SpawnL(CExec::eWaitGroup, app_c, "SpawnL_eWaitGroup",
                         NULL).GetExitCode(); 
    LOG_POST("Exit code: " << code);
    assert( code == TEST_RESULT_C );


    // Spawn with eNoWaitGroup, waiting self

    handle = CExec::SpawnL(CExec::eNoWaitGroup, app_c, "SpawnL_eNoWaitGroup",
                           NULL).GetProcessHandle();
    assert(CExec::Wait(handle) == TEST_RESULT_C );


    // Spawn with eOverlay

    code = CExec::SpawnL(CExec::eOverlay, app_c, "SpawnL_eOverlay",
                         NULL).GetExitCode();
    assert( code == TEST_RESULT_C );

    // At success code below never been executed
    LOG_POST("\nTEST execution fails!\n");

    return 77;
}


///////////////////////////////////
// MAIN
//

int main(int argc, const char* argv[], const char* envp[])
{
    // Exec from test?
    if ( argc > 1) {
        assert(argv[1] && *argv[1]);
        cout << endl << "Exec: " << argv[1] << endl;
        // View environment
        const char** env_var = &envp[0];
        while (*env_var) {
            cout << *env_var << endl;
            env_var++;
        }
        // Check environment
        if ( strstr(argv[1],"E_e")) {
            char* ptr = getenv("TEST_NCBI_EXEC");
            if (!ptr || !*ptr) {
                cout << "Environment variable TEST_NCBI_EXEC not found" <<endl;
                cout.flush();
                _exit(88);
            } else {
                cout << "TEST_NCBI_EXEC=" << ptr << endl;
            }
        }
        _exit(TEST_RESULT_C);
    }
    LOG_POST("Start tests:\n");

    // Execute main application function
    return CTest().AppMain(argc, argv, 0, eDS_Default, 0);
}
