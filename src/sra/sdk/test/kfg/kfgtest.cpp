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

/**
* Unit tests for Kfg interface
*/

#include <ktst/unit_test.hpp>

#include <kfg/extern.h>
#include <os-native.h>

#if !WINDOWS
    #include <sys/utsname.h>
#endif

#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include <klib/writer.h>
#include <klib/log.h>
#include <klib/printf.h>
#include <kfg/config.h>
#include <kfs/directory.h>
#include <kfs/impl.h>
#include <kfs/dyload.h>
#include <vfs/manager.h>
#include <vfs/path.h>

#include <klib/text.h>

#include <common/test_assert.h>

using namespace std;

TEST_SUITE(KfgTestSuite);

// test fixture for creation and lookup of kfg files
class KfgFixture
{
public:
    KfgFixture() : wd(0), kfg(0), file(0), node(0)
    {
        if ( KDirectoryNativeDir ( & wd ) != 0 )
            throw logic_error("KfgFixture: KDirectoryNativeDir failed");
            
        if (KConfigMake ( & kfg, wd ) != 0)
            throw logic_error("KfgFixture: KConfigMake failed");
            
        if (apppath.length() == 0) // first call
        {
            if (!GetValue("APPPATH", apppath))
                throw logic_error("KfgFixture: GetValue failed");
        }
    }
    ~KfgFixture() 
    {
        if ( node != 0 && KConfigNodeRelease(node) )
            throw logic_error("~KfgFixture: KConfigNodeRelease failed");
        
        if ( KConfigRelease ( kfg ) != 0 )
            throw logic_error("~KfgFixture: KConfigRelease failed");
            
        if ( KDirectoryRelease ( wd ) != 0 )
            throw logic_error("~KfgFixture: KDirectoryRelease failed");
            
        if ( KFileRelease( file ) != 0 )
            throw logic_error("~KfgFixture: KFileRelease failed");
    }
    void CreateFile(const char* name, const char* contents)
    {
        if (KDirectoryCreateFile(wd, &file, true, 0664, kcmInit, name) != 0)
            throw logic_error("CreateFile: KDirectoryCreateFile failed");

        size_t num_writ=0;
        if (KFileWrite(file, 0, contents, strlen(contents), &num_writ) != 0)
            throw logic_error("CreateFile: KFileWrite failed");

        if (KFileRelease(file) != 0)
            throw logic_error("CreateFile: KFileRelease failed");
        
        file=0;
    }
    
    void LoadFile(const char* name)
    {
        if (KDirectoryOpenFileRead(wd, (const KFile**)&file, name) != 0)
            throw logic_error("LoadFile: KDirectoryOpenFileRead failed");
            
        if (KConfigLoadFile ( kfg, name, file) != 0)
            throw logic_error("LoadFile: KConfigLoadFile failed");

        if (KFileRelease(file) != 0)
            throw logic_error("LoadFile: KFileRelease failed");
            
        file=0;
    }
    
    void CreateAndLoad(const char* name, const char* contents)
    {
        CreateFile(name, contents);
        LoadFile(name);
        // the .kfg is not needed anymore
        if (KDirectoryRemove(wd, true, name) != 0)
            throw logic_error("CreateAndLoad: KDirectoryRemove failed");
    }

    bool GetValue(const char* path, string& value)
    {
        const KConfigNode *node;
        rc_t rc=KConfigOpenNodeRead(kfg, &node, "%.*s", strlen(path), path);
        if (rc == 0) 
        {
            size_t num_read = 0;
            size_t remaining = 0;
            char tmp[1];
            rc = KConfigNodeRead(node, 0, tmp, 1, &num_read, &remaining);

            char* buf=new char[num_read+remaining+1];
            rc = KConfigNodeRead(node, 0, buf, num_read+remaining, &num_read, &remaining);
            buf[num_read+remaining]=0;
            value=buf;
            delete [] buf;
            return KConfigNodeRelease(node) == 0;
        }
        return false;
    }
    bool ValueMatches(const char* path, const char* value, bool nullAsEmpty=false)
    {
        if (nullAsEmpty && value == 0)
        {
            value="";
        }
        string v;
        if (GetValue(path, v))
        {
            bool ret=true;
            if (v != string(value, strlen(value)))
            {
                pLogMsg(klogErr, "ValueMatches mismatch: expected='$(EXP)', actual='$(ACT)'", 
                                 "EXP=%s,ACT=%s", 
                                 value, v.c_str());
                ret=false;
            }
            return ret;
        }
        return false;
    }
    void UpdateNode(const char* key, const char* value)
    {
        KConfigNode *node;
        if (KConfigOpenNodeUpdate(kfg, &node, key) != 0)
            throw logic_error("UpdateNode: KConfigOpenNodeUpdate failed");
        if (KConfigNodeWrite(node, value, strlen(value)) != 0)
            throw logic_error("UpdateNode: KConfigNodeWrite failed");
        if (KConfigNodeRelease(node) != 0)
            throw logic_error("UpdateNode: KConfigNodeRelease failed");
    }

    string DirPath(const KDirectory* dir)
    {
        char resolved[4097];
        if (KDirectoryResolvePath(dir, true, resolved, sizeof resolved, ".") == 0)
        {
            return string(resolved);
        }
        else
        {
            return "??";
        }
    }
    string GetHomeDirectory()
    {   
        string ret;
        if (getenv("HOME") != NULL)
            ret = getenv("HOME");
        else if (getenv("USERPROFILE") != NULL) // on Windows the value is taken from USERPROFILE
            ret = getenv("USERPROFILE");
        
        char buf[4096];
        if (KDirectoryResolvePath(wd, true, buf, sizeof(buf), ret.c_str()) != 0)
            throw logic_error("GetHomeDirectory: KDirectoryResolvePath failed");
        
        return buf;
    }
    const KConfigNode* GetNode(const char* path) 
    {
        if ( node != 0 && KConfigNodeRelease(node) )
            throw logic_error("GetNode: KConfigNodeRelease failed");
        if (KConfigOpenNodeRead(kfg, &node, path) != 0)
            throw logic_error("GetNode: KConfigOpenNodeRead failed");
        return node;
    }
    
    KDirectory* wd;
    KConfig* kfg;
    KFile* file;
    const KConfigNode* node;

    static string apppath; // only gets set for the 1st instance of KConfig; save it here for the corresponding test case
};
string KfgFixture::apppath; 

///////////////////////////////////////////////// KFG parser test cases

FIXTURE_TEST_CASE(KConfigLoadFile_should_report_null_inputs, KfgFixture)
{
    KFile file;
    REQUIRE_RC_FAIL(KConfigLoadFile ( 0, "qweert", &file));
    REQUIRE_RC_FAIL(KConfigLoadFile ( kfg, "qweert", 0));
}

FIXTURE_TEST_CASE(one_name_value_double_quotes, KfgFixture)
{
    CreateAndLoad(GetName().c_str(), "name=\"value\"");
    REQUIRE(ValueMatches("name", "value"));
}

FIXTURE_TEST_CASE(one_pathname_value_single_quotes, KfgFixture)
{
    CreateAndLoad(GetName().c_str(), "root/subname/name='val'");
    REQUIRE(ValueMatches("root/subname/name", "val"));
}

FIXTURE_TEST_CASE(numeric_pathnames, KfgFixture)
{   
    CreateAndLoad(GetName().c_str(), " root1/1 = 'val1'\n");
    REQUIRE(ValueMatches("root1/1", "val1"));
}

FIXTURE_TEST_CASE(multiple_pathnames, KfgFixture)
{   // sprinkle some spaces and tabs (only allowed outside pathnames)
    const char* contents=" root1/subname1/name1 =\t \"val1\"\n"
                         "root1/subname1/name2 =\t \"val2\"\n"
                         "\troot1/subname2/name3\t = \"val3\"\n"
                         "root1/subname2/name4\t = \"val4\"\n"
                         "root2/subname1/name5 =\t \"val5\"\n"
                         "root2/subname2/name6 = \"val6\"\n";
    CreateAndLoad(GetName().c_str(), contents);

    REQUIRE(ValueMatches("root1/subname1/name1", "val1"));
    REQUIRE(ValueMatches("root1/subname1/name2", "val2"));
    REQUIRE(ValueMatches("root1/subname2/name3", "val3"));
    REQUIRE(ValueMatches("root1/subname2/name4", "val4"));
    REQUIRE(ValueMatches("root2/subname1/name5", "val5"));
    REQUIRE(ValueMatches("root2/subname2/name6", "val6"));
}

FIXTURE_TEST_CASE(comments, KfgFixture)
{
    const char* contents="root1/subname1/name1 ='val1'\n"
                         "root1/subname1/name2 ='val2'\n"
                         "#root1/subname1/name1='val11'\n"       // the commented out lines do not override the lines with the same paths above
                         "/*root1/subname1/name2=\"val12\"*/\n";
    CreateAndLoad(GetName().c_str(), contents);

    REQUIRE(ValueMatches("root1/subname1/name1", "val1"));
    REQUIRE(ValueMatches("root1/subname1/name2", "val2"));
}

FIXTURE_TEST_CASE(unescaping_escapes, KfgFixture)
{   
    CreateAndLoad(GetName().c_str(), "name='\\a'\n");
    REQUIRE(ValueMatches("name", "\a"));
}

FIXTURE_TEST_CASE(dots_in_pathnames, KfgFixture)
{   
    CreateAndLoad(GetName().c_str(), "root.1./subname1.ext='val100'\n");
    REQUIRE(ValueMatches("root.1./subname1.ext", "val100"));
}

FIXTURE_TEST_CASE(variable_expansion_simple, KfgFixture)
{
    const char* contents="var='value'\n"
                         "ref=$(var)\n";
    CreateAndLoad(GetName().c_str(), contents);

    REQUIRE(ValueMatches("var", "value"));
    REQUIRE(ValueMatches("ref", "value"));
}

FIXTURE_TEST_CASE(variable_expansion_concat, KfgFixture)
{
    const char* contents="var1='value1'\n"
                         "var2='value2'\n"
                         "ref=\"$(var1)'$(var2)\"\n";
    CreateAndLoad(GetName().c_str(), contents);

    REQUIRE(ValueMatches("ref", "value1'value2"));
}

FIXTURE_TEST_CASE(variable_expansion_path, KfgFixture)
{
    const char* contents="root/var='value'\n"
                         "ref=$(root/var)\n";
    CreateAndLoad(GetName().c_str(), contents);

    REQUIRE(ValueMatches("ref", "value"));
}

FIXTURE_TEST_CASE(in_string_variable_expansion_path, KfgFixture)
{
    const char* contents="root/var='value'\n"
                         "ref=\"+$(root/var)+\"\n";
    CreateAndLoad(GetName().c_str(), contents);

    REQUIRE(ValueMatches("ref", "+value+"));
}

FIXTURE_TEST_CASE(can_reference_keys_across_files, KfgFixture)
{  
    const char* contents1="root/var='Value'\n";
    CreateAndLoad((GetName()+"1").c_str(), contents1);
    const char* contents2="ref=$(root/var)\n";
    CreateAndLoad((GetName()+"2").c_str(), contents2);

    REQUIRE(ValueMatches("ref", "Value"));
}

FIXTURE_TEST_CASE(long_key, KfgFixture)
{
    string key(1025, 'k');
    CreateAndLoad(GetName().c_str(), (key+"='value'").c_str());
    REQUIRE(ValueMatches(key.c_str(), "value"));
}

FIXTURE_TEST_CASE(long_path, KfgFixture)
{
    string path=string(4097, 'v');
    string line("k='");
    line+=path;
    line+="'";
    CreateAndLoad(GetName().c_str(), line.c_str());
    REQUIRE(ValueMatches("k", path.c_str()));
}

///////////////////////////////////////////////// predefined variables
FIXTURE_TEST_CASE(predef_LIBPATH, KfgFixture)
{
#if defined(WINDOWS) ? !defined(_STATIC) \
    : (!defined(NCBI_DLL_SUPPORT) \
       ||  (!defined(NCBI_DLL_BUILD)  ||  defined(NCBI_OS_SOLARIS) \
            ||  defined(NCBI_XCODE_BUILD)))
    // since this program and libkfg.dll live in different directories, they contain separate copies of KConfigMake under Windows, 
    // so we cannot compare them

    // Meanwhile, when building as part of the C++ Toolkit and mostly
    // using static libraries, the SRA tree finds its configuration
    // via a new kfg-beacon library, which also leads to mismatches.
#else
    const char* contents="var=$(vdb/lib/paths/kfg)\n";
    CreateAndLoad(GetName().c_str(), contents);

    KDyld *dyld;
    REQUIRE_RC(KDyldMake ( & dyld ));
    const KDirectory *dir;
    REQUIRE_RC(KDyldHomeDirectory ( dyld, & dir, ( fptr_t ) KConfigMake ));
    REQUIRE(ValueMatches("var", DirPath(dir).c_str()));
    KDirectoryRelease ( dir );
    KDyldRelease ( dyld );
#endif
}

FIXTURE_TEST_CASE(predef_KFGDIR, KfgFixture)
{
    CreateAndLoad(GetName().c_str(), "var=$(kfg/dir)\n");
    REQUIRE(ValueMatches("var", DirPath(wd).c_str()));
}
FIXTURE_TEST_CASE(predef_KFGNAME, KfgFixture)
{
    CreateAndLoad(GetName().c_str(), "var=$(kfg/name)\n");
    REQUIRE(ValueMatches("var", GetName().c_str()));
}


FIXTURE_TEST_CASE(predef_ARCHNAME, KfgFixture)
{
    CreateAndLoad(GetName().c_str(), "var=$(kfg/arch/name)\n");
    #if WINDOWS
        REQUIRE(ValueMatches("var", ""));
    #else
        struct utsname name;
        REQUIRE_NE(uname(&name), -1);
        REQUIRE(ValueMatches("var", name.nodename));
    #endif
}
FIXTURE_TEST_CASE(predef_ARCHBITS, KfgFixture)
{
    CreateAndLoad(GetName().c_str(), "var=$(kfg/arch/bits)\n");
    char buf[10];
    size_t num_writ;
    string_printf(buf, sizeof(buf), &num_writ, "%d", _ARCH_BITS);
    REQUIRE(ValueMatches("var", buf));
}

FIXTURE_TEST_CASE(predef_OS, KfgFixture)
{
    CreateAndLoad(GetName().c_str(), "var=$(OS)\n");
    #if LINUX
        #define OS "linux"
    #elif SUN
        #define OS "sun"    
    #elif MAC 
        #define OS "mac"
    #elif WINDOWS
        #define OS "win"
    #endif
    REQUIRE(ValueMatches("var", OS));
    #undef OS
 }

FIXTURE_TEST_CASE(predef_BUILD_LINKAGE, KfgFixture)
{
    CreateAndLoad(GetName().c_str(), "var=$(BUILD_LINKAGE)\n");
    #if _STATIC
        #define BUILD_LINKAGE "STATIC"
    #else
        #define BUILD_LINKAGE "DYNAMIC"
    #endif
    REQUIRE(ValueMatches("var", BUILD_LINKAGE));
    #undef BUILD_LINKAGE
}

FIXTURE_TEST_CASE(predef_BUILD, KfgFixture)
{
    CreateAndLoad(GetName().c_str(), "var=$(BUILD)\n");
    #if _PROFILING
        #define BUILD "PROFILE"
    #else
        #if _DEBUGGING
            #define BUILD "DEBUG"
        #else 
            #define BUILD "RELEASE"
        #endif
    #endif
    REQUIRE(ValueMatches("var", BUILD));
    #undef BUILD
}

#if 0 // only appropriate when invoked by a canonical path ?
FIXTURE_TEST_CASE(predef_APPPATH, KfgFixture)
{
    // REQUIRE_RC(CreateAndLoad(GetName().c_str(), "var=$(APPPATH)\n")); 
    // APPPATH is only set correctly for the 1st instance of KConfig, so we saved it off in the first call to fixture's 
    // constructor, test here
    string path(ncbi::NK::GetTestSuite()->argv[0]);
    string::size_type lastSlash=path.find_last_of("/");
    if (lastSlash == string::npos)
    {
        lastSlash=path.find_last_of("\\");
    }
    if (lastSlash != string::npos)
    {
        path.erase(lastSlash);
    }
    REQUIRE_EQ(strcase_cmp(apppath.c_str(), apppath.length(), 
                           path.c_str(), path.length(), 
                           max(apppath.length(), path.length())), 
               0);
}
#endif

FIXTURE_TEST_CASE(predef_APPNAME, KfgFixture)
{
    CreateAndLoad(GetName().c_str(), "var=$(APPNAME)\n");
    char buf[4096];
    size_t num_writ;
    REQUIRE_RC(LogAppName(buf, sizeof(buf), &num_writ));
    buf[num_writ]=0;
    REQUIRE(ValueMatches("var", buf));
}

FIXTURE_TEST_CASE(predef_PWD, KfgFixture)
{
    CreateAndLoad(GetName().c_str(), "var=$(PWD)\n");
    KDirectory* dir;
    REQUIRE_RC(KDirectoryNativeDir(&dir));
    REQUIRE(ValueMatches("var", DirPath(dir).c_str()));
    KDirectoryRelease(dir);
}

FIXTURE_TEST_CASE(predef_ENV, KfgFixture)
{
    const char* contents=
        "host=$(HOST)\n"
        "user=$(USER)\n"
        "vdb_root=$(VDB_ROOT)\n"
        "vdb_config=$(VDB_CONFIG)\n"
        "home=$(HOME)\n"
        "ncbi_home=$(NCBI_HOME)\n"
        "ncbi_settings=$(NCBI_SETTINGS)\n"
        ;
    CreateAndLoad(GetName().c_str(), contents);
    REQUIRE(ValueMatches("host",            getenv("HOST"), true));
    REQUIRE(ValueMatches("user",            getenv("USER"), true));
    REQUIRE(ValueMatches("vdb_root",        getenv("VDB_ROOT"), true));
    REQUIRE(ValueMatches("vdb_config",      getenv("VDB_CONFIG"), true));
    REQUIRE(ValueMatches("home",            GetHomeDirectory().c_str(), true));
    REQUIRE(ValueMatches("ncbi_home",       (GetHomeDirectory()+"/.ncbi").c_str(), true));
    REQUIRE(ValueMatches("ncbi_settings",   (GetHomeDirectory()+"/.ncbi/user-settings.mkfg").c_str(), true));
}

#if 0
FIXTURE_TEST_CASE(include_files, KfgFixture)
{  
#define includeName "include_file"
    const char* contents1="root/var='Value'\n";
    CreateFile((GetName()+"1").c_str(), contents1);
    const char* contents2="include ./" includeName "\n"
                          "ref=$(root/var)\n";
    CreateAndLoad((GetName()+"2").c_str(), contents2);

    REQUIRE(ValueMatches("ref", "Value"));
    REQUIRE_RC(KDirectoryRemove(wd, true, includeName)); 
}
#endif

///////////////////////////////////////////////// modification and commit

FIXTURE_TEST_CASE(ChangeCommit, KfgFixture)
{
    const char* contents=
        "one=\"1\"\n"
        "one/two=\"2\"\n"
        "one/two/three=\"3\"\n"
        ;
    // override NCBI_SETTINGS 
    const char* LocalSettingsFile = "settings.mkfg";
    string FullMagicPath = DirPath(wd) + "/" + LocalSettingsFile;
    CreateAndLoad( GetName().c_str(), (string(contents) + "NCBI_SETTINGS=\"" + FullMagicPath + "\"\n").c_str() );
    
    // make, commit changes
    UpdateNode("one", "1+0");
    UpdateNode("one/two", "0+2");
    REQUIRE_RC(KConfigCommit(kfg));
    REQUIRE_RC(KConfigRelease(kfg));
    
    // load the changes from the new location
    REQUIRE_RC(KConfigMake(&kfg,wd));
    LoadFile(FullMagicPath.c_str());
    
    // verify changes
    REQUIRE(ValueMatches("one", "1+0"));
    REQUIRE(ValueMatches("one/two", "0+2"));
    string s;
    REQUIRE(! GetValue("one/two/three", s)); // unchanged values are not saved
    
    REQUIRE_RC(KDirectoryRemove(wd, true, LocalSettingsFile));     
}

FIXTURE_TEST_CASE(ChangeCommitEscapes, KfgFixture)
{
    const char* LocalSettingsFile = "settings.mkfg";
    string FullMagicPath = DirPath(wd) + "/" + LocalSettingsFile;
    CreateAndLoad( GetName().c_str(), (string() + "NCBI_SETTINGS=\"" + FullMagicPath + "\"\n").c_str() );
    
    // make, commit changes
    UpdateNode("double/quote", "\"");
    UpdateNode("escaped/hex", "\x0a");
    REQUIRE_RC(KConfigCommit(kfg));
    REQUIRE_RC(KConfigRelease(kfg));
    
    // load the changes from the new location
    REQUIRE_RC(KConfigMake(&kfg,wd));
    LoadFile(FullMagicPath.c_str());
    
    // verify changes
    REQUIRE(ValueMatches("double/quote", "\""));
    REQUIRE(ValueMatches("escaped/hex", "\x0a"));
    
    REQUIRE_RC(KDirectoryRemove(wd, true, LocalSettingsFile));     
}

//////////////////////////////////////////// KConfig Accessors

FIXTURE_TEST_CASE(ConfigAccessBool, KfgFixture)
{
    const char* contents=
        "bool/f=\"FALSE\"\n"
        "bool/t=\"true\"\n"
        "bool=\"dunno\"\n"
        ;
    CreateAndLoad(GetName().c_str(), contents);
    bool b = true;
    REQUIRE_RC(KConfigReadBool(kfg, "bool/f", &b));
    REQUIRE(! b);
    REQUIRE_RC(KConfigReadBool(kfg, "bool/t", &b));
    REQUIRE(b);
    REQUIRE_RC_FAIL(KConfigReadBool(kfg, "bool", &b));
}

FIXTURE_TEST_CASE(ConfigAccessInt, KfgFixture)
{
    const char* contents=
        "int/i1=\"100\"\n"
        "int/i2=\"-100000000000\"\n"
        "int=\"0dunno\"\n"
        ;
    CreateAndLoad(GetName().c_str(), contents);
    int64_t i = 0;
    REQUIRE_RC(KConfigReadI64(kfg, "int/i1", &i));
    REQUIRE_EQ(i, INT64_C(100));
    REQUIRE_RC(KConfigReadI64(kfg, "int/i2", &i));
    REQUIRE_EQ(i, INT64_C(-100000000000));
    REQUIRE_RC_FAIL(KConfigReadI64(kfg, "int", &i));
}

FIXTURE_TEST_CASE(ConfigAccessUnsigned, KfgFixture)
{
    const char* contents=
        "uint/i1=\"100000000000\"\n"
        "uint=\"1dunno\"\n"
        ;
    CreateAndLoad(GetName().c_str(), contents);
    uint64_t i = 0;
    REQUIRE_RC(KConfigReadU64(kfg, "uint/i1", &i));
    REQUIRE_EQ(i, UINT64_C(100000000000));
    REQUIRE_RC_FAIL(KConfigReadU64(kfg, "uint", &i));
}

FIXTURE_TEST_CASE(ConfigAccessF64, KfgFixture)
{
    const char* contents=
        "f64/i1=\"3.14\"\n"
        "f64=\"2.3dunno\"\n"
        ;
    CreateAndLoad(GetName().c_str(), contents);
    double f = 0.0;
    REQUIRE_RC(KConfigReadF64(kfg, "f64/i1", &f));
    REQUIRE_CLOSE(f, 3.14, 0.001);
    REQUIRE_RC_FAIL(KConfigReadF64(kfg, "v64", &f));
}

FIXTURE_TEST_CASE(ConfigAccessVPath, KfgFixture)
{
    // example from vfs/path.h
    #define VPATH "ncbi-file:///c:/scanned-data/0001/file.sra?enc?pwd-file=c:/Users/JamesMcCoy/ncbi.pwd"
    
    const char* contents=
        "vpath/i1=\"" VPATH "\"\n" 
        ;
        
    CreateAndLoad(GetName().c_str(), contents);
    VPath* p;
    REQUIRE_RC(KConfigReadVPath(kfg, "vpath/i1", &p));
    char buf[200];
    size_t num_read;
    VPathReadPath(p, buf, sizeof(buf), &num_read);
    REQUIRE_EQ(string(buf), string(VPATH));
    VPathRelease(p);
}

// FIXTURE_TEST_CASE(ConfigAccessString, KfgFixture)
// {
    // #define STRING "ncbi-file:///c:/scanned-data/0001/file.sra?enc?pwd-file=c:/Users/JamesMcCoy/ncbi.pwd"
    // const char* contents=
        // "string/i1=\"" STRING "\"\n" 
        // ;
        
    // CreateAndLoad(GetName().c_str(), contents);
    // String* str;
    // REQUIRE_RC(KConfigReadString(kfg, "string/i1", &str));
    // REQUIRE_NOT_NULL(str);
    // REQUIRE_EQ(string(str->addr), string(STRING));
    // free(str);
// }

//////////////////////////////////////////// KConfigNode Accessors

FIXTURE_TEST_CASE(ConfigNodeAccessBool, KfgFixture)
{
    const char* contents=
        "bool/f=\"FALSE\"\n"
        ;
    CreateAndLoad(GetName().c_str(), contents);
    bool b = true;
    REQUIRE_RC(KConfigNodeReadBool(GetNode("bool/f"), &b));
    REQUIRE(! b);
}

FIXTURE_TEST_CASE(ConfigNodeAccessInt, KfgFixture)
{
    const char* contents=
        "int/i1=\"100\"\n"
        ;
    CreateAndLoad(GetName().c_str(), contents);
    int64_t i = 0;
    REQUIRE_RC(KConfigNodeReadI64(GetNode("int/i1"), &i));
    REQUIRE_EQ(i, (int64_t)100);
}

FIXTURE_TEST_CASE(ConfigNodeAccessUnsigned, KfgFixture)
{
    const char* contents=
        "uint/i1=\"100000000000\"\n"
        ;
    CreateAndLoad(GetName().c_str(), contents);
    uint64_t i = 0;
    REQUIRE_RC(KConfigNodeReadU64(GetNode("uint/i1"), &i));
    REQUIRE_EQ(i, UINT64_C(100000000000));
}

FIXTURE_TEST_CASE(ConfigNodeAccessF64, KfgFixture)
{
    const char* contents=
        "f64/i1=\"3.14\"\n"
        ;
    CreateAndLoad(GetName().c_str(), contents);
    double f = 0.0;
    REQUIRE_RC(KConfigNodeReadF64(GetNode("f64/i1"), &f));
    REQUIRE_CLOSE(f, 3.14, 0.001);
}

FIXTURE_TEST_CASE(ConfigNodeAccessVPath, KfgFixture)
{
    // example from vfs/path.h
    #define VPATH "ncbi-file:///c:/scanned-data/0001/file.sra?enc?pwd-file=c:/Users/JamesMcCoy/ncbi.pwd"
    
    const char* contents=
        "vpath/i1=\"" VPATH "\"\n" 
        ;
        
    CreateAndLoad(GetName().c_str(), contents);
    VPath* p;
    REQUIRE_RC(KConfigNodeReadVPath(GetNode("vpath/i1"), &p));
    char buf[200];
    size_t num_read;
    VPathReadPath(p, buf, sizeof(buf), &num_read);
    REQUIRE_EQ(string(buf), string(VPATH));
    VPathRelease(p);
}

// FIXTURE_TEST_CASE(ConfigNodeAccessString, KfgFixture)
// {
    // #define STRING "ncbi-file:///c:/scanned-data/0001/file.sra?enc?pwd-file=c:/Users/JamesMcCoy/ncbi.pwd"
    // const char* contents=
        // "string/i1=\"" STRING "\"\n" 
        // ;
        
    // CreateAndLoad(GetName().c_str(), contents);
    // String* str;
    // REQUIRE_RC(KConfigNodeReadString(GetNode("string/i1"), &str));
    // REQUIRE_NOT_NULL(str);
    // REQUIRE_EQ(string(str->addr), string(STRING));
    // free(str);
// }

//////////////////////////////////////////// Main
extern "C"
{

#include <kapp/args.h>

ver_t CC KAppVersion ( void )
{
    return 0x1000000;
}
rc_t CC UsageSummary (const char * progname)
{
    return 0;
}

rc_t CC Usage ( const Args * args )
{
    return 0;
}

const char UsageDefaultName[] = "test-kfg";

rc_t CC KMain ( int argc, char *argv [] )
{
    KConfigDisableUserSettings();
    rc_t rc=KfgTestSuite(argc, argv);
    return rc;
}

}
