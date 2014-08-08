#ifndef CORELIB___EXPR__HPP
#define CORELIB___EXPR__HPP


/*  $Id: expr.hpp 390714 2013-03-01 15:58:48Z vakatov $
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
 * Author: Sergey Sikorskiy, Mikhail Zakharov
 *
 * File Description: 
 *      Expression parsing and evaluation.
 *
 * ===========================================================================
 */

#include <corelib/ncbistd.hpp>

BEGIN_NCBI_SCOPE



////////////////////////////////////////////////////////////////////////////////
class CExprSymbol;

class NCBI_XNCBI_EXPORT CExprValue
{ 
public:
    CExprValue(void);
    template <typename VT> CExprValue(VT* value)
    {
        // If you got here, you are using wrong data type.
        value->please_use_Int8_double_bool_instead();
    }
    CExprValue(Uint4 value);
    CExprValue(Int4 value);
    CExprValue(Uint8 value);
    CExprValue(Int8 value);
    CExprValue(double value);
    CExprValue(bool value);
    CExprValue(string value);
    CExprValue(const CExprValue& value);

public:
    /// Value type.
    enum EValue {
        eINT,
        eFLOAT,
        eBOOL,
        eSTRING
    };

public:
    EValue GetType(void) const
    {
        return m_Tag;
    }
    void SetType(EValue type)
    {
        m_Tag = type;
    }

    string GetString(void) const
    {
        string str;

        switch (m_Tag) {
            case eINT:
                NStr::NumericToString(str, ival);
                return str;
            case eBOOL:
                return bval ? "true" : "false";
            case eSTRING:
                return m_sval;
            default:
                break;
        }

        return NStr::DoubleToString(fval); 
    }

    double GetDouble(void) const
    { 
        switch (m_Tag) {
            case eINT:
                return static_cast<double>(ival);
            case eBOOL:
                return bval ? 1.0 : 0.0;
            case eSTRING:
                return 0;
            default:
                break;
        }

        return fval; 
    }

    Int8 GetInt(void) const
    { 
        switch (m_Tag) {
            case eFLOAT:
                return static_cast<Int8>(fval);
            case eBOOL:
                return bval ? 1 : 0;
            case eSTRING:
                return 0;
            default:
                break;
        }

        return ival; 
    }

    bool GetBool(void) const
    { 
        switch (m_Tag) {
            case eINT:
                return ival != 0;
            case eFLOAT:
                return fval != 0.0;
            case eSTRING:
                return false;
            default:
                break;
        }

        return bval; 
    }

public:
    union { 
        Int8    ival;
        double  fval;
        bool    bval;
    };
    string m_sval;

    CExprSymbol*    m_Var;
    int             m_Pos;

private:
    EValue          m_Tag;
};

////////////////////////////////////////////////////////////////////////////////
class NCBI_XNCBI_EXPORT CExprSymbol 
{ 
public:
    typedef Int8    (*FIntFunc1)    (Int8);
    typedef Int8    (*FIntFunc2)    (Int8,Int8);
    typedef double  (*FFloatFunc1)  (double);
    typedef double  (*FFloatFunc2)  (double, double);
    typedef bool    (*FBoolFunc1)   (bool);
    typedef bool    (*FBoolFunc2)   (bool, bool);
    typedef bool    (*FStringFunc1)  (const string&);

    CExprSymbol(void);
    template <typename VT> CExprSymbol(const char* name, VT* value)
    {
        // If you got here, you are using wrong data type.
        value->please_use_Int8_double_bool_instead();
    }
    CExprSymbol(const char* name, Uint4 value);
    CExprSymbol(const char* name, Int4 value);
    CExprSymbol(const char* name, Uint8 value);
    CExprSymbol(const char* name, Int8 value);
    CExprSymbol(const char* name, bool value);
    CExprSymbol(const char* name, double value);
    CExprSymbol(const char* name, string value);
    CExprSymbol(const char* name, FIntFunc1 value);
    CExprSymbol(const char* name, FIntFunc2 value);
    CExprSymbol(const char* name, FFloatFunc1 value);
    CExprSymbol(const char* name, FFloatFunc2 value);
    CExprSymbol(const char* name, FBoolFunc1 value);
    CExprSymbol(const char* name, FBoolFunc2 value);
    CExprSymbol(const char* name, FStringFunc1 value);

    ~CExprSymbol(void);


public:
    enum ESymbol { 
        eVARIABLE, 
        eIFUNC1, 
        eIFUNC2,
        eFFUNC1, 
        eFFUNC2,
        eBFUNC1, 
        eBFUNC2,
        eSFUNC1
    };

public:
    ESymbol         m_Tag;
    union { 
        FIntFunc1   m_IntFunc1;
        FIntFunc2   m_IntFunc2;
        FFloatFunc1 m_FloatFunc1;
        FFloatFunc2 m_FloatFunc2;
        FBoolFunc1  m_BoolFunc1;
        FBoolFunc2  m_BoolFunc2;
        FStringFunc1 m_StringFunc1;
    };
    CExprValue      m_Val;
    string          m_Name;
    CExprSymbol*    m_Next;
};
           
////////////////////////////////////////////////////////////////////////////////
class NCBI_XNCBI_EXPORT CExprParserException : EXCEPTION_VIRTUAL_BASE public CException
{
public:
    enum EErrCode {
        eParseError,
        eTypeConversionError
    };


    CExprParserException(
        const CDiagCompileInfo& info,
        const CException* prev_exception, EErrCode err_code,
        const string& message, int pos, 
        EDiagSev severity = eDiag_Error)
    : CException(info, prev_exception, CException::eInvalid, message)
    , m_Pos(pos)
    NCBI_EXCEPTION_DEFAULT_IMPLEMENTATION(CExprParserException, CException);

    virtual const char* GetErrCodeString(void) const;

    virtual void ReportExtra(ostream& out) const;

public:
    int GetPos(void) const
    {
        return m_Pos;
    }

protected:
    virtual void x_Assign(const CException& src);

private:
    int    m_Pos;
}; 

////////////////////////////////////////////////////////////////////////////////
class NCBI_XNCBI_EXPORT CExprParser
{
public:
    /// Parser flags
    enum EAutoVar {
        fAllowAutoVar = 0,          //< create variables without previous declaration
        fDenyAutoVar  = (1 << 0),   //< call AddSymbol() to register a variable
        fNoDivision   = (1 << 1),   //< division is interpreted as a part of a symbol (to allow directories)

        // Legacy
        eAllowAutoVar = fAllowAutoVar,
        eDenyAutoVar  = fDenyAutoVar
    };

    typedef int TParserFlags;

    CExprParser(TParserFlags auto_var = 0);
    ~CExprParser(void);

public:
    void Parse(const char* str);

    const CExprValue& GetResult(void) const
    {
        if (m_v_sp != 1) {
            ReportError("Result is not available");
        }

        return m_VStack[0];
    }
    
    template <typename VT> 
    CExprSymbol* AddSymbol(const char* name, VT value);

private:
    enum EOperator { 
        eBEGIN, eOPERAND, eERROR, eEND, 
        eLPAR, eRPAR, eFUNC, ePOSTINC, ePOSTDEC,
        ePREINC, ePREDEC, ePLUS, eMINUS, eNOT, eCOM,
        ePOW,
        eMUL, eDIV, eMOD,
        eADD, eSUB, 
        eASL, eASR, eLSR, 
        eGT, eGE, eLT, eLE,     
        eEQ, eNE, 
        eAND,
        eXOR,
        eOR,
        eSET, eSETADD, eSETSUB, eSETMUL, eSETDIV, eSETMOD, eSETASL, eSETASR, eSETLSR, 
        eSETAND, eSETXOR, eSETOR, eSETPOW,
        eCOMMA,
        eTERMINALS
    };

private:
    EOperator Scan(bool operand);
    bool Assign(void);
    CExprSymbol* GetSymbol(const char* name) const;

    static void ReportError(int pos, const string& msg) 
    {
        NCBI_THROW2(CExprParserException, eParseError, msg, pos);
    }
    void ReportError(const string& msg) const { ReportError(m_Pos - 1, msg); }

    EOperator IfChar(
            char c, EOperator val, 
            EOperator val_def);
    EOperator IfElseChar(
            char c1, EOperator val1, 
            char c2, EOperator val2, 
            EOperator val_def);
    EOperator IfLongest2ElseChar(
            char c1, char c2, 
            EOperator val_true_longest, 
            EOperator val_true, 
            EOperator val_false, 
            EOperator val_def);

    TParserFlags AutoCreateVariable(void) const
    {
        return m_ParserFlags & fDenyAutoVar;
    }

    bool NoDivision(void) const { return (m_ParserFlags & fNoDivision) != 0;}

private:
    enum {hash_table_size = 1013};
    CExprSymbol* hash_table[hash_table_size];

    enum {max_stack_size = 256};
    enum {max_expression_length = 1024};

    static int sm_lpr[eTERMINALS];
    static int sm_rpr[eTERMINALS];

    CExprValue  m_VStack[max_stack_size];
    int         m_v_sp;
    EOperator   m_OStack[max_stack_size];
    int         m_o_sp;
    const char* m_Buf;
    int         m_Pos;
    int         m_TmpVarCount;
    TParserFlags    m_ParserFlags;
};


////////////////////////////////////////////////////////////////////////////////
// Inline methods.
//

NCBI_XNCBI_EXPORT
unsigned string_hash_function(const char* p);

template <typename VT> 
inline
CExprSymbol* CExprParser::AddSymbol(const char* name, VT value)
{
    CExprSymbol* sp = GetSymbol(name);

    if (!sp) {
        // Add ...
        sp = new CExprSymbol(name, value);

        unsigned h = string_hash_function(name) % hash_table_size;
        sp->m_Next = hash_table[h];
        hash_table[h] = sp;
    }

    return sp;
}

inline
CExprParser::EOperator 
CExprParser::IfChar(
        char c, EOperator val, 
        EOperator val_def)
{
    if (m_Buf[m_Pos] == c) { 
        m_Pos += 1;
        return val;
    }

    return val_def;
}

inline
CExprParser::EOperator 
CExprParser::IfElseChar(
        char c1, EOperator val1, 
        char c2, EOperator val2, 
        EOperator val_def)
{
    if (m_Buf[m_Pos] == c1) { 
        m_Pos += 1;
        return val1;
    } else if (m_Buf[m_Pos] == c2) { 
        m_Pos += 1;
        return val2;
    }

    return val_def;
}

inline
CExprParser::EOperator 
CExprParser::IfLongest2ElseChar(
        char c1, char c2, 
        EOperator val_true_longest, 
        EOperator val_true, 
        EOperator val_false, 
        EOperator val_def)
{
    if (m_Buf[m_Pos] == c1) { 
        m_Pos += 1;
        return IfChar(c2, val_true_longest, val_true);
    } else if (m_Buf[m_Pos] == c2) {
        m_Pos += 1;
        return val_false;
    }

    return val_def;
}

END_NCBI_SCOPE

#endif  /* CORELIB___EXPR__HPP */

