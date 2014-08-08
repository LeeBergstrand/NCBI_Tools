/*  $Id: ncbi_param.cpp 222093 2011-01-25 15:32:16Z gouriano $
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
 * Authors:  Eugene Vasilchenko, Aleksey Grichenko
 *
 * File Description:
 *   Parameters storage implementation
 *
 */


#include <ncbi_pch.hpp>
#include <corelib/ncbi_param.hpp>
#include <corelib/error_codes.hpp>
#include "ncbisys.hpp"


#define NCBI_USE_ERRCODE_X   Corelib_Config


BEGIN_NCBI_SCOPE

/////////////////////////////////////////////////////////////////////////////
///
/// Helper functions for getting values from registry/environment
///

SSystemMutex& CParamBase::s_GetLock(void)
{
    DEFINE_STATIC_MUTEX(s_ParamValueLock);
    return s_ParamValueLock;
}


const char* kNcbiConfigPrefix = "NCBI_CONFIG__";

// g_GetConfigXxx() functions

namespace {
    bool s_StringToBool(const string& value)
    {
        if ( !value.empty() && isdigit((unsigned char)value[0]) ) {
            return NStr::StringToInt(value) != 0;
        }
        else {
            return NStr::StringToBool(value);
        }
    }

    string s_GetEnvVarName(const char* section,
                         const char* variable,
                         const char* env_var_name)
    {
        string env_var;
        if ( env_var_name  &&  *env_var_name ) {
            env_var = env_var_name;
        }
        else {
            env_var = kNcbiConfigPrefix;
            if ( section  &&  *section ) {
                env_var += section;
                env_var += "__";
            }
            if (variable) {
                env_var += variable;
            }
        }
        NStr::ToUpper(env_var);
        return env_var;
    }

    const TXChar* s_GetEnv(const char* section,
                         const char* variable,
                         const char* env_var_name)
    {
        return NcbiSys_getenv( _T_XCSTRING(
            s_GetEnvVarName(section, variable, env_var_name).c_str() ));
    }

#ifdef _DEBUG
    static const char* const CONFIG_DUMP_SECTION = "NCBI";
    static const char* const CONFIG_DUMP_VARIABLE = "CONFIG_DUMP_VARIABLES";
    static bool config_dump = g_GetConfigFlag(CONFIG_DUMP_SECTION,
                                              CONFIG_DUMP_VARIABLE,
                                              0,
                                              false);
#endif
}


bool NCBI_XNCBI_EXPORT g_GetConfigFlag(const char* section,
                                       const char* variable,
                                       const char* env_var_name,
                                       bool default_value)
{
#ifdef _DEBUG
    bool dump = variable != CONFIG_DUMP_VARIABLE && config_dump;
#endif
    if ( section  &&  *section ) {
        CNcbiApplication* app = CNcbiApplication::Instance();
        if ( app  &&  app->HasLoadedConfig() ) {
            const string& str = app->GetConfig().Get(section, variable);
            if ( !str.empty() ) {
                try {
                    bool value = s_StringToBool(str);
#ifdef _DEBUG
                    if ( dump ) {
                        LOG_POST_X(5, "NCBI_CONFIG: bool variable"
                                      " [" << section << "]"
                                      " " << variable <<
                                      " = " << value <<
                                      " from registry");
                    }
#endif
                    return value;
                }
                catch ( ... ) {
                    // ignored
                }
            }
        }
    }
    const TXChar* str = s_GetEnv(section, variable, env_var_name);
    if ( str && *str ) {
        try {
            bool value = s_StringToBool(_T_CSTRING(str));
#ifdef _DEBUG
            if ( dump ) {
                if ( section  &&  *section ) {
                    LOG_POST_X(6, "NCBI_CONFIG: bool variable"
                                  " [" << section << "]"
                                  " " << variable <<
                                  " = " << value <<
                                  " from env var " <<
                                  s_GetEnvVarName(section, variable, env_var_name));
                }
                else {
                    LOG_POST_X(7, "NCBI_CONFIG: bool variable "
                                  " " << variable <<
                                  " = " << value <<
                                  " from env var");
                }
            }
#endif
            return value;
        }
        catch ( ... ) {
            // ignored
        }
    }
    bool value = default_value;
#ifdef _DEBUG
    if ( dump ) {
        if ( section  &&  *section ) {
            LOG_POST_X(8, "NCBI_CONFIG: bool variable"
                          " [" << section << "]"
                          " " << variable <<
                          " = " << value <<
                          " by default");
        }
        else {
            LOG_POST_X(9, "NCBI_CONFIG: bool variable"
                          " " << variable <<
                          " = " << value <<
                          " by default");
        }
    }
#endif
    return value;
}


int NCBI_XNCBI_EXPORT g_GetConfigInt(const char* section,
                                     const char* variable,
                                     const char* env_var_name,
                                     int default_value)
{
    if ( section  &&  *section ) {
        CNcbiApplication* app = CNcbiApplication::Instance();
        if ( app  &&  app->HasLoadedConfig() ) {
            const string& str = app->GetConfig().Get(section, variable);
            if ( !str.empty() ) {
                try {
                    int value = NStr::StringToInt(str);
#ifdef _DEBUG
                    if ( config_dump ) {
                        LOG_POST_X(10, "NCBI_CONFIG: int variable"
                                       " [" << section << "]"
                                       " " << variable <<
                                       " = " << value <<
                                       " from registry");
                    }
#endif
                    return value;
                }
                catch ( ... ) {
                    // ignored
                }
            }
        }
    }
    const TXChar* str = s_GetEnv(section, variable, env_var_name);
    if ( str && *str ) {
        try {
            int value = NStr::StringToInt(_T_CSTRING(str));
#ifdef _DEBUG
            if ( config_dump ) {
                if ( section  &&  *section ) {
                    LOG_POST_X(11, "NCBI_CONFIG: int variable"
                                   " [" << section << "]"
                                   " " << variable <<
                                   " = " << value <<
                                   " from env var " <<
                                   s_GetEnvVarName(section, variable, env_var_name));
                }
                else {
                    LOG_POST_X(12, "NCBI_CONFIG: int variable "
                                   " " << variable <<
                                   " = " << value <<
                                   " from env var");
                }
            }
#endif
            return value;
        }
        catch ( ... ) {
            // ignored
        }
    }
    int value = default_value;
#ifdef _DEBUG
    if ( config_dump ) {
        if ( section  &&  *section ) {
            LOG_POST_X(13, "NCBI_CONFIG: int variable"
                           " [" << section << "]"
                           " " << variable <<
                           " = " << value <<
                           " by default");
        }
        else {
            LOG_POST_X(14, "NCBI_CONFIG: int variable"
                           " " << variable <<
                           " = " << value <<
                           " by default");
        }
    }
#endif
    return value;
}


double NCBI_XNCBI_EXPORT g_GetConfigDouble(const char* section,
                                           const char* variable,
                                           const char* env_var_name,
                                           double default_value)
{
    if ( section  &&  *section ) {
        CNcbiApplication* app = CNcbiApplication::Instance();
        if ( app  &&  app->HasLoadedConfig() ) {
            const string& str = app->GetConfig().Get(section, variable);
            if ( !str.empty() ) {
                try {
                    double value = NStr::StringToDouble(str,
                        NStr::fDecimalPosixOrLocal |
                        NStr::fAllowLeadingSpaces | NStr::fAllowTrailingSpaces);
#ifdef _DEBUG
                    if ( config_dump ) {
                        LOG_POST_X(10, "NCBI_CONFIG: double variable"
                                       " [" << section << "]"
                                       " " << variable <<
                                       " = " << value <<
                                       " from registry");
                    }
#endif
                    return value;
                }
                catch ( ... ) {
                    // ignored
                }
            }
        }
    }
    const TXChar* str = s_GetEnv(section, variable, env_var_name);
    if ( str && *str ) {
        try {
            double value = NStr::StringToDouble(_T_CSTRING(str),
                NStr::fDecimalPosixOrLocal |
                NStr::fAllowLeadingSpaces | NStr::fAllowTrailingSpaces);
#ifdef _DEBUG
            if ( config_dump ) {
                if ( section  &&  *section ) {
                    LOG_POST_X(11, "NCBI_CONFIG: double variable"
                                   " [" << section << "]"
                                   " " << variable <<
                                   " = " << value <<
                                   " from env var " <<
                                   s_GetEnvVarName(section, variable, env_var_name));
                }
                else {
                    LOG_POST_X(12, "NCBI_CONFIG: double variable "
                                   " " << variable <<
                                   " = " << value <<
                                   " from env var");
                }
            }
#endif
            return value;
        }
        catch ( ... ) {
            // ignored
        }
    }
    double value = default_value;
#ifdef _DEBUG
    if ( config_dump ) {
        if ( section  &&  *section ) {
            LOG_POST_X(13, "NCBI_CONFIG: double variable"
                           " [" << section << "]"
                           " " << variable <<
                           " = " << value <<
                           " by default");
        }
        else {
            LOG_POST_X(14, "NCBI_CONFIG: int variable"
                           " " << variable <<
                           " = " << value <<
                           " by default");
        }
    }
#endif
    return value;
}


string NCBI_XNCBI_EXPORT g_GetConfigString(const char* section,
                                           const char* variable,
                                           const char* env_var_name,
                                           const char* default_value)
{
    if ( section  &&  *section ) {
        CNcbiApplication* app = CNcbiApplication::Instance();
        if ( app  &&  app->HasLoadedConfig() ) {
            const string& value = app->GetConfig().Get(section, variable);
            if ( !value.empty() ) {
#ifdef _DEBUG
                if ( config_dump ) {
                    LOG_POST_X(15, "NCBI_CONFIG: str variable"
                                   " [" << section << "]"
                                   " " << variable <<
                                   " = \"" << value << "\""
                                   " from registry");
                }
#endif
                return value;
            }
        }
    }
    const TXChar* value = s_GetEnv(section, variable, env_var_name);
    if ( value ) {
#ifdef _DEBUG
        if ( config_dump ) {
            if ( section  &&  *section ) {
                LOG_POST_X(16, "NCBI_CONFIG: str variable"
                               " [" << section << "]"
                               " " << variable <<
                               " = \"" << value << "\""
                               " from env var " <<
                               s_GetEnvVarName(section, variable, env_var_name));
            }
            else {
                LOG_POST_X(17, "NCBI_CONFIG: str variable"
                               " " << variable <<
                               " = \"" << value << "\""
                               " from env var");
            }
        }
#endif
        return _T_STDSTRING(value);
    }
    const char* dvalue = default_value? default_value: "";
#ifdef _DEBUG
    if ( config_dump ) {
        if ( section  &&  *section ) {
            LOG_POST_X(18, "NCBI_CONFIG: str variable"
                           " [" << section << "]"
                           " " << variable <<
                           " = \"" << dvalue << "\""
                           " by default");
        }
        else {
            LOG_POST_X(19, "NCBI_CONFIG: str variable"
                           " " << variable <<
                           " = \"" << dvalue << "\""
                           " by default");
        }
    }
#endif
    return dvalue;
}

const char* CParamException::GetErrCodeString(void) const
{
    switch (GetErrCode()) {
    case eParserError:   return "eParserError";
    case eBadValue:      return "eBadValue";
    case eNoThreadValue: return "eNoThreadValue";
    case eRecursion:     return "eRecursion";
    default:             return CException::GetErrCodeString();
    }
}


END_NCBI_SCOPE
