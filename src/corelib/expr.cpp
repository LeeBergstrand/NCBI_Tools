/*  $Id: expr.cpp 390714 2013-03-01 15:58:48Z vakatov $
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

#include <ncbi_pch.hpp>

#include <corelib/expr.hpp>

#include <math.h>
#include <limits>


#define BINARY(opd) (opd >= ePOW)


BEGIN_NCBI_SCOPE

////////////////////////////////////////////////////////////////////////////////
CExprValue::CExprValue(void)
: ival(0)
, m_sval("")
, m_Var(NULL)
, m_Pos(0)
, m_Tag()
{
}

CExprValue::CExprValue(string value)
: ival(0),
m_sval(value),
m_Var(NULL),
m_Pos(0),
m_Tag(eSTRING)
{
}


CExprValue::CExprValue(const CExprValue& value)
: ival(value.ival)
, m_sval("")
, m_Var(value.m_Var)
, m_Pos(value.m_Pos)
, m_Tag(value.m_Tag)
{
}

CExprValue::CExprValue(Uint4 value)
: ival(value)
, m_sval("")
, m_Var(NULL)
, m_Pos(0)
, m_Tag(eINT)
{
}

CExprValue::CExprValue(Int4 value)
: ival(value)
, m_sval("")
, m_Var(NULL)
, m_Pos(0)
, m_Tag(eINT)
{
}

CExprValue::CExprValue(Uint8 value)
: ival(0)
, m_sval("")
, m_Var(NULL)
, m_Pos(0)
, m_Tag(eINT)
{
    if (static_cast<Uint8>(numeric_limits<Int8>::max()) < value) {
        NCBI_THROW2(CExprParserException, 
                eTypeConversionError, 
                "Value too big to fit in the 8-byte signed integer type", 
                m_Pos);
    }

    ival = static_cast<Int8>(value);
}


CExprValue::CExprValue(Int8 value)
: ival(value)
, m_sval("")
, m_Var(NULL)
, m_Pos(0)
, m_Tag(eINT)
{
}

CExprValue::CExprValue(double value)
: fval(value)
, m_sval("")
, m_Var(NULL)
, m_Pos(0)
, m_Tag(eFLOAT)
{
}

CExprValue::CExprValue(bool value)
: bval(value)
, m_sval("")
, m_Var(NULL)
, m_Pos(0)
, m_Tag(eBOOL)
{
}

////////////////////////////////////////////////////////////////////////////////
CExprSymbol::CExprSymbol(void)
: m_Tag()
, m_IntFunc1(NULL)
, m_Val()
, m_Next(NULL)
{
}

CExprSymbol::~CExprSymbol(void)
{
    delete m_Next;
}

CExprSymbol::CExprSymbol(const char* name, Uint4 value)
: m_Tag(eVARIABLE)
, m_IntFunc1(NULL)
, m_Val(value)
, m_Name(name)
, m_Next(NULL)
{
}

CExprSymbol::CExprSymbol(const char* name, Int4 value)
: m_Tag(eVARIABLE)
, m_IntFunc1(NULL)
, m_Val(value)
, m_Name(name)
, m_Next(NULL)
{
}

CExprSymbol::CExprSymbol(const char* name, Uint8 value)
: m_Tag(eVARIABLE)
, m_IntFunc1(NULL)
, m_Val(value)
, m_Name(name)
, m_Next(NULL)
{
}

CExprSymbol::CExprSymbol(const char* name, Int8 value)
: m_Tag(eVARIABLE)
, m_IntFunc1(NULL)
, m_Val(value)
, m_Name(name)
, m_Next(NULL)
{
}

CExprSymbol::CExprSymbol(const char* name, bool value)
: m_Tag(eVARIABLE)
, m_IntFunc1(NULL)
, m_Val(value)
, m_Name(name)
, m_Next(NULL)
{
}

CExprSymbol::CExprSymbol(const char* name, double value)
: m_Tag(eVARIABLE)
, m_IntFunc1(NULL)
, m_Val(value)
, m_Name(name)
, m_Next(NULL)
{
}

CExprSymbol::CExprSymbol(const char* name, string value)
: m_Tag(eVARIABLE)
, m_IntFunc1(NULL)
, m_Val(value)
, m_Name(name)
, m_Next(NULL)
{
}

CExprSymbol::CExprSymbol(const char* name, FIntFunc1 value)
: m_Tag(eIFUNC1)
, m_IntFunc1(value)
, m_Val((Int8)0)
, m_Name(name)
, m_Next(NULL)
{
}

CExprSymbol::CExprSymbol(const char* name, FIntFunc2 value)
: m_Tag(eIFUNC2)
, m_IntFunc2(value)
, m_Val((Int8)0)
, m_Name(name)
, m_Next(NULL)
{
}

CExprSymbol::CExprSymbol(const char* name, FFloatFunc1 value)
: m_Tag(eFFUNC1)
, m_FloatFunc1(value)
, m_Val((Int8)0)
, m_Name(name)
, m_Next(NULL)
{
}

CExprSymbol::CExprSymbol(const char* name, FFloatFunc2 value)
: m_Tag(eFFUNC2)
, m_FloatFunc2(value)
, m_Val((Int8)0)
, m_Name(name)
, m_Next(NULL)
{
}

CExprSymbol::CExprSymbol(const char* name, FBoolFunc1 value)
: m_Tag(eBFUNC1)
, m_BoolFunc1(value)
, m_Val((Int8)0)
, m_Name(name)
, m_Next(NULL)
{
}

CExprSymbol::CExprSymbol(const char* name, FBoolFunc2 value)
: m_Tag(eBFUNC2)
, m_BoolFunc2(value)
, m_Val((Int8)0)
, m_Name(name)
, m_Next(NULL)
{
}

CExprSymbol::CExprSymbol(const char* name, FStringFunc1 value)
: m_Tag(eSFUNC1)
, m_StringFunc1(value)
, m_Val((Int8)0)
, m_Name(name)
, m_Next(NULL)
{
}

////////////////////////////////////////////////////////////////////////////////
static
Int8 to_int(Int8 m_Val) 
{ 
    return m_Val; 
}

static
double to_float(double m_Val) 
{ 
    return m_Val; 
}

static
Int8 gcd(Int8 x, Int8 y) 
{
    while (x) {
        Int8 r = y%x;
        y=x;
        x=r;
    }

    return y;
}

static
Int8 invmod(Int8 x, Int8 y) 
{
    Int8 m = y;
    Int8 u = 1, v = 0;
    Int8 s = 0, t = 1;

    while (x) {
        Int8 q = y/x;
        Int8 r = y%x;
        Int8 a = s - q*u;
        Int8 b = t - q*v;
        y=x; s=u; t=v;
        x=r; u=a; v=b;
    }

    if (y!=1) return 0;
    
    while (s<0) s+=m;
    
    return s;
}

static
Int8 prime(Int8 n) 
{ 
    if (n <= 3) { 
        return n;
    }
    n |= 1; 
    while (true) {
        Int8 m = (Int8)sqrt((double)n) + 1;
        Int8 k = 3;
        while (true) { 
            if (k > m) { 
                return n;
            }
            if (n % k == 0) break;
            k += 2;
        }
        n += 2;
    }
}

/*
static
char* 
to_bin(int nval, char* pbuf, int nbufsize)
{
    int ncnt;
    int bcnt;
    int nlen = sizeof(int) * 8 + sizeof(int);

    if (pbuf != NULL && nbufsize > nlen)
    {
        pbuf[nlen] = '\0';
        pbuf[nlen - 1] = 'b';
        for(ncnt = 0, bcnt = nlen - 2; ncnt < nlen; ncnt ++, bcnt --)
        {
            if (ncnt > 0 && (ncnt % 8) == 0)
            {
                pbuf[bcnt] = '.';
                bcnt --;
            }
            pbuf[bcnt] = (nval & (1 << ncnt))? '1' : '0';
        }
    }

    return pbuf;
}
*/

unsigned string_hash_function(const char* p) 
{ 
    unsigned h = 0, g;
    while(*p) { 
        h = (h << 4) + *p++;
        if ((g = h & 0xF0000000) != 0) { 
            h ^= g >> 24;
        }
        h &= ~g;
    }
    return h;
}

////////////////////////////////////////////////////////////////////////////////
const char* CExprParserException::GetErrCodeString(void) const
{
    switch (GetErrCode()) {
        case eParseError: return "eParseError";
        case eTypeConversionError: return "eTypeConversionError";
        default:
            break;
    }

    return CException::GetErrCodeString();
}

void CExprParserException::ReportExtra(ostream& out) const
{
    out << "pos: " << m_Pos;
}

void CExprParserException::x_Assign(const CException& src)
{
    CException::x_Assign(src);

    const CExprParserException& other = dynamic_cast<const CExprParserException&>(src);
    m_Pos = other.m_Pos;
}

////////////////////////////////////////////////////////////////////////////////
CExprParser::CExprParser(CExprParser::TParserFlags ParserFlags)
: m_Buf(NULL)
, m_Pos(0)
, m_TmpVarCount(0)
, m_ParserFlags(ParserFlags)
{
    memset(hash_table, 0, sizeof(hash_table));

    AddSymbol("abs",    (CExprSymbol::FFloatFunc1)fabs);
    AddSymbol("acos",   (CExprSymbol::FFloatFunc1)acos);
    AddSymbol("asin",   (CExprSymbol::FFloatFunc1)asin);
    AddSymbol("atan",   (CExprSymbol::FFloatFunc1)atan);
    AddSymbol("atan2",  (CExprSymbol::FFloatFunc2)atan2);
    AddSymbol("cos",    (CExprSymbol::FFloatFunc1)cos);
    AddSymbol("cosh",   (CExprSymbol::FFloatFunc1)cosh);
    AddSymbol("exp",    (CExprSymbol::FFloatFunc1)exp);
    AddSymbol("log",    (CExprSymbol::FFloatFunc1)log);
    AddSymbol("log10",  (CExprSymbol::FFloatFunc1)log10);
    AddSymbol("sin",    (CExprSymbol::FFloatFunc1)sin);
    AddSymbol("sinh",   (CExprSymbol::FFloatFunc1)sinh);
    AddSymbol("tan",    (CExprSymbol::FFloatFunc1)tan);
    AddSymbol("tanh",   (CExprSymbol::FFloatFunc1)tanh);
    AddSymbol("sqrt",   (CExprSymbol::FFloatFunc1)sqrt);

    AddSymbol("float",  to_float);
    AddSymbol("int",    to_int);

    AddSymbol("gcd",    gcd);
    AddSymbol("invmod", invmod);

    AddSymbol("prime",  prime);

    AddSymbol("pi",     3.1415926535897932385E0);
    AddSymbol("e",      2.7182818284590452354E0);
}

CExprParser::~CExprParser(void)
{
    for (int i = 0; i < hash_table_size; ++i) {
        delete hash_table[i];
    }
}

int CExprParser::sm_lpr[eTERMINALS] = {
    2, 0, 0, 0,       // eBEGIN, eOPERAND, eERROR, eEND, 
    4, 4,             // eLPAR, eRPAR 
    5, 98, 98,        // eFUNC, ePOSTINC, ePOSTDEC,
    98, 98, 98, 98, 98, 98, // ePREINC, ePREDEC, ePLUS, eMINUS, eNOT, eCOM,
    90,               // ePOW,
    80, 80, 80,       // eMUL, eDIV, eMOD,
    70, 70,           // eADD, eSUB, 
    60, 60, 60,       // eASL, eASR, eLSR, 
    50, 50, 50, 50,   // eGT, eGE, eLT, eLE,     
    40, 40,           // eEQ, eNE, 
    38,               // eAND,
    36,               // eXOR,
    34,               // eOR,
    20, 20, 20, 20, 20, 20, 20, //eSET, eSETADD, eSETSUB, eSETMUL, eSETDIV, eSETMOD, 
    20, 20, 20, 20, 20, 20, // eSETASL, eSETASR, eSETLSR, eSETAND, eSETXOR, eSETOR,
    10               // eCOMMA
};

int CExprParser::sm_rpr[eTERMINALS] = {
    0, 0, 0, 1,       // eBEGIN, eOPERAND, eERROR, eEND, 
    110, 3,           // eLPAR, eRPAR 
    120, 99, 99,      // eFUNC, ePOSTINC, ePOSTDEC
    99, 99, 99, 99, 99, 99, // ePREINC, ePREDEC, ePLUS, eMINUS, eNOT, eCOM,
    95,               // ePOW,
    80, 80, 80,       // eMUL, eDIV, eMOD,
    70, 70,           // eADD, eSUB, 
    60, 60, 60,       // eASL, eASR, eLSR, 
    50, 50, 50, 50,   // eGT, eGE, eLT, eLE,     
    40, 40,           // eEQ, eNE, 
    38,               // eAND,
    36,               // eXOR,
    34,               // eOR,
    25, 25, 25, 25, 25, 25, 25, //eSET, eSETADD, eSETSUB, eSETMUL, eSETDIV, eSETMOD, 
    25, 25, 25, 25, 25, 25, // eSETASL, eSETASR, eSETLSR, eSETAND, eSETXOR, eSETOR,
    15               // eCOMMA
};

CExprSymbol* CExprParser::GetSymbol(const char* name) const
{
    unsigned h = string_hash_function(name) % hash_table_size;
    CExprSymbol* sp = NULL;

    for (sp = hash_table[h]; sp != NULL; sp = sp->m_Next) { 
        if (sp->m_Name.compare(name) == 0) { 
            return sp;
        }
    }

    return sp;
}

CExprParser::EOperator 
CExprParser::Scan(bool operand)
{
    char sym_name[max_expression_length], *np;

    while (isspace(m_Buf[m_Pos])) m_Pos += 1;

    switch (m_Buf[m_Pos++]) { 
      case '\0':
        return eEND;
      case '(':
        return eLPAR;
      case ')':
        return eRPAR;
      case '+':
        if (m_Buf[m_Pos] == '+') { 
            m_Pos += 1;
            return operand ? ePREINC : ePOSTINC;
        } else if (m_Buf[m_Pos] == '=') { 
            m_Pos += 1;
            return eSETADD;
        }
        return operand ? ePLUS : eADD;
      case '-':
        if (m_Buf[m_Pos] == '-') { 
            m_Pos += 1;
            return operand ? ePREDEC : ePOSTDEC;
        } else if (m_Buf[m_Pos] == '=') { 
            m_Pos += 1;
            return eSETSUB;
        }
        return operand ? eMINUS : eSUB;
      case '!':
        return IfChar('=', eNE, eNOT);
      case '~':
        return eCOM;
      case '*':
        return IfLongest2ElseChar('*', '=', eSETPOW, ePOW, eSETMUL, eMUL);
      case '/':
        if(NoDivision())
            goto l_SkipDivision;            // goto to default is justified - complete code refactoring otherwise (MZ)
        return IfChar('=', eSETDIV, eDIV);
      case '%':
        return IfChar('=', eSETMOD, eMOD);
      case '<':
        return IfLongest2ElseChar('<', '=', eSETASL, eASL, eLE, eLT);
      case '>':
        if (m_Buf[m_Pos] == '>') { 
            if (m_Buf[m_Pos+1] == '>') { 
                if (m_Buf[m_Pos+2] == '=') { 
                    m_Pos += 3;
                    return eSETLSR;
                }
                m_Pos += 2;
                return eLSR;
            } else if (m_Buf[m_Pos+1] == '=') { 
                m_Pos += 2;
                return eSETASR;
            } else { 
                m_Pos += 1;
                return eASR;
            }
        } else if (m_Buf[m_Pos] == '=') { 
            m_Pos += 1;
            return eGE;
        } 
        return eGT;
      case '=':
        return IfChar('=', eEQ, eSET);
      case '&':
        return IfElseChar(
                '&', eAND, 
                '=', eSETAND, 
                eAND);
      case '|':
        return IfElseChar(
                '|', eOR, 
                '=', eSETOR, 
                eOR);
      case '^':
        return IfChar('=', eSETXOR, eXOR);
      case ',':
        return eCOMMA;
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        {
            Int8 ival;
            double fval;            
            int ierr, ferr;
            char *ipos;
            char *fpos;

#ifdef NCBI_OS_MSWIN
            int n = 0;
            ierr = sscanf(m_Buf+m_Pos-1, "%I64i%n", &ival, &n) != 1;
            ipos = const_cast<char*>(m_Buf+m_Pos-1+n);
#else
            errno = 0;             
#if SIZEOF_LONG == 8
            ival = strtoul(m_Buf+m_Pos-1, &ipos, 0); 
#else
            ival = strtoull(m_Buf+m_Pos-1, &ipos, 0);
#endif
            ierr = errno;
#endif
            errno = 0; 
            fval = strtod(m_Buf+m_Pos-1, &fpos); 
            ferr = errno;

            if (ierr && ferr) { 
                ReportError("bad numeric constant");
                return eERROR;
            }

            if (m_v_sp == max_stack_size) { 
                ReportError("stack overflow");
                return eERROR;
            }

            if (!ierr && ipos >= fpos) { 
                m_VStack[m_v_sp].SetType(CExprValue::eINT);
                m_VStack[m_v_sp].ival = ival;
                m_Pos = (int)(ipos - m_Buf);
            } else { 
                m_VStack[m_v_sp].SetType(CExprValue::eFLOAT);
                m_VStack[m_v_sp].fval = fval;
                m_Pos = (int)(fpos - m_Buf);
            } 

            m_VStack[m_v_sp].m_Pos = m_Pos;
            m_VStack[m_v_sp++].m_Var = NULL;

            return eOPERAND;
        }

      case '"':             // String literal case
        {
            string StringConstant(m_Buf+m_Pos);     // Not including the \"
            size_t pos=StringConstant.find('"');

            StringConstant = StringConstant.substr(0, pos);

            m_VStack[m_v_sp].SetType(CExprValue::eSTRING);

            m_VStack[m_v_sp].m_sval = StringConstant;     //assign string
            m_Pos += pos+1;                               // move m_Pos

            m_VStack[m_v_sp].m_Pos = m_Pos;
            m_VStack[m_v_sp].m_Var = NULL;
            m_v_sp++;

            return eOPERAND;
        }

      default:
      l_SkipDivision:
        m_Pos -= 1;
        np = sym_name;

        while (isalnum(m_Buf[m_Pos]) || m_Buf[m_Pos] == '$' || m_Buf[m_Pos] == '_' || (m_Buf[m_Pos]=='/' && NoDivision())) {
            *np++ = m_Buf[m_Pos++];
        }

        if (np == m_Buf) { 
            ReportError("Bad character");
            return eERROR;
        }

        *np = '\0';

        // Check for true/false ...
        if (strcmp(sym_name, "true") == 0) {
            m_VStack[m_v_sp].SetType(CExprValue::eBOOL);
            m_VStack[m_v_sp].bval = true;

            m_VStack[m_v_sp].m_Pos = m_Pos;
            m_VStack[m_v_sp++].m_Var = NULL;

            return eOPERAND;
        } else if (strcmp(sym_name, "false") == 0) {
            m_VStack[m_v_sp].SetType(CExprValue::eBOOL);
            m_VStack[m_v_sp].bval = false;

            m_VStack[m_v_sp].m_Pos = m_Pos;
            m_VStack[m_v_sp++].m_Var = NULL;

            return eOPERAND;
        }

        CExprSymbol* sym = NULL;

        if (AutoCreateVariable() == eAllowAutoVar) {
            sym = AddSymbol(sym_name, Int8(0));
        } else {
            sym = GetSymbol(sym_name);
            if (!sym) {
                ReportError(string("Invalid symbol name: ") + sym_name);
                return eERROR;
            }
        }

        if (m_v_sp == max_stack_size) { 
            ReportError("stack overflow");
            return eERROR;
        }

        m_VStack[m_v_sp] = sym->m_Val;
        m_VStack[m_v_sp].m_Pos = m_Pos;
        m_VStack[m_v_sp++].m_Var = sym;

        return (sym->m_Tag == CExprSymbol::eVARIABLE) ? eOPERAND : eFUNC;
    }
}

bool CExprParser::Assign(void) 
{ 
    CExprValue& v = m_VStack[m_v_sp-1];
    if (v.m_Var == NULL) { 
        ReportError(v.m_Pos, "variable expected");
        return false;
    } else { 
        v.m_Var->m_Val = v;
        return true;
    }
}

void CExprParser::Parse(const char* str)
{
    // char var_name[16];
    m_Buf = str;
    m_v_sp = 0;
    m_o_sp = 0;
    m_Pos = 0;
    m_OStack[m_o_sp++] = eBEGIN;
    bool operand = true;
    int n_args = 0;

    while (true) { 
      next_token:
        int op_pos = m_Pos;

        EOperator oper = Scan(operand);

        if (oper == eERROR) {
            return;
        }

        if (!operand) { 
            if (!BINARY(oper) && oper != eEND && oper != ePOSTINC 
                && oper != ePOSTDEC && oper != eRPAR) 
            { 
                ReportError(op_pos, "operator expected");
                return;
            }
            if (oper != ePOSTINC && oper != ePOSTDEC && oper != eRPAR) { 
                operand = true;
            }
        } else { 
            if (oper == eOPERAND) { 
                operand = false;
                n_args += 1;
                continue;
            }
            if (BINARY(oper) || oper == eRPAR) {
                ReportError(op_pos, "operand expected");
                return;
            }
        }

        int n_args = 1;

        while (sm_lpr[m_OStack[m_o_sp-1]] >= sm_rpr[oper]) { 
            int cop = m_OStack[--m_o_sp]; 

            switch (cop) { 
              case eBEGIN:
                if (oper == eRPAR) { 
                    ReportError("Unmatched ')'");
                    return;
                }

                if (oper != eEND) { 
                    ReportError("Unexpected end of input");
                }

                if (m_v_sp == 1) {
                    /*
                    sprintf(var_name, "$%d", ++m_TmpVarCount);
                    printf("%s = ", var_name);
                    CExprSymbol::add(CExprSymbol::eVARIABLE, var_name)->m_Val = m_VStack[0];
                    if (m_VStack[0].GetType() == CExprValue::eINT) { 
                        printf("%" INT_FORMAT "d [%#" INT_FORMAT "x %#" INT_FORMAT "o]\n", 
                               m_VStack[0].ival,  m_VStack[0].ival, m_VStack[0].ival);
                    } else { 
                        printf("%.10g\n", m_VStack[0].fval);
                    }
                    */
                    return;
                } else if (m_v_sp != 0) { 
                    ReportError("Unexpected end of expression");
                }

                return;
              case eCOMMA:
                n_args += 1;
                continue;
              case eADD:
              case eSETADD:
                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eINT && m_VStack[m_v_sp-2].GetType() == CExprValue::eINT) {
                    m_VStack[m_v_sp-2].ival += m_VStack[m_v_sp-1].ival;
                } else if(m_VStack[m_v_sp-1].GetType() == CExprValue::eSTRING && m_VStack[m_v_sp-2].GetType() == CExprValue::eSTRING) {
                    m_VStack[m_v_sp-2].m_sval += m_VStack[m_v_sp-1].m_sval;     // string concatenation
                } else {
                    m_VStack[m_v_sp-2].fval = 
                        m_VStack[m_v_sp-2].GetDouble() + m_VStack[m_v_sp-1].GetDouble();
                    m_VStack[m_v_sp-2].SetType(CExprValue::eFLOAT);
                }

                m_v_sp -= 1;

                if (cop == eSETADD) { 
                    if (!Assign()) return;
                }

                m_VStack[m_v_sp-1].m_Var = NULL;
                break;
              case eSUB:
              case eSETSUB:
                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eSTRING || m_VStack[m_v_sp-2].GetType() == CExprValue::eSTRING)
                    ReportError("Can not subtract string literals");
                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eINT && m_VStack[m_v_sp-2].GetType() == CExprValue::eINT) {
                    m_VStack[m_v_sp-2].ival -= m_VStack[m_v_sp-1].ival;
                } else { 
                    m_VStack[m_v_sp-2].fval = 
                        m_VStack[m_v_sp-2].GetDouble() - m_VStack[m_v_sp-1].GetDouble();
                    m_VStack[m_v_sp-2].SetType(CExprValue::eFLOAT);
                }

                m_v_sp -= 1;

                if (cop == eSETSUB) { 
                    if (!Assign()) return;
                }

                m_VStack[m_v_sp-1].m_Var = NULL;

                break;
              case eMUL:
              case eSETMUL:
                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eSTRING || m_VStack[m_v_sp-2].GetType() == CExprValue::eSTRING)
                    ReportError("Can not mupltiply string literals");

                 if (m_VStack[m_v_sp-1].GetType() == CExprValue::eINT && m_VStack[m_v_sp-2].GetType() == CExprValue::eINT) {
                    m_VStack[m_v_sp-2].ival *= m_VStack[m_v_sp-1].ival;
                } else { 
                    m_VStack[m_v_sp-2].fval = 
                        m_VStack[m_v_sp-2].GetDouble() * m_VStack[m_v_sp-1].GetDouble();
                    m_VStack[m_v_sp-2].SetType(CExprValue::eFLOAT);
                }

                m_v_sp -= 1;

                if (cop == eSETMUL) { 
                    if (!Assign()) return;
                }
                m_VStack[m_v_sp-1].m_Var = NULL;

                break;
              case eDIV:
              case eSETDIV:
                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eSTRING || m_VStack[m_v_sp-2].GetType() == CExprValue::eSTRING)
                    ReportError("Can not divide string literals");

                if (m_VStack[m_v_sp-1].GetDouble() == 0.0) {
                    ReportError(m_VStack[m_v_sp-2].m_Pos, "Division by zero");
                    return;
                }
                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eINT && m_VStack[m_v_sp-2].GetType() == CExprValue::eINT) {
                    m_VStack[m_v_sp-2].ival /= m_VStack[m_v_sp-1].ival;
                } else {
                    m_VStack[m_v_sp-2].fval = 
                        m_VStack[m_v_sp-2].GetDouble() / m_VStack[m_v_sp-1].GetDouble();
                    m_VStack[m_v_sp-2].SetType(CExprValue::eFLOAT);
                }
                m_v_sp -= 1;
                if (cop == eSETDIV) { 
                    if (!Assign()) return;
                }
                m_VStack[m_v_sp-1].m_Var = NULL;
                break;
              case eMOD:
              case eSETMOD:
                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eSTRING || m_VStack[m_v_sp-2].GetType() == CExprValue::eSTRING)
                    ReportError("Can not divide string literals");

                if (m_VStack[m_v_sp-1].GetDouble() == 0.0) {
                    ReportError(m_VStack[m_v_sp-2].m_Pos, "Division by zero");
                    return;
                }
                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eINT && m_VStack[m_v_sp-2].GetType() == CExprValue::eINT) {
                    m_VStack[m_v_sp-2].ival %= m_VStack[m_v_sp-1].ival;
                } else { 
                    m_VStack[m_v_sp-2].fval = 
                        fmod(m_VStack[m_v_sp-2].GetDouble(), m_VStack[m_v_sp-1].GetDouble());
                    m_VStack[m_v_sp-2].SetType(CExprValue::eFLOAT);
                }
                m_v_sp -= 1;
                if (cop == eSETMOD) { 
                    if (!Assign()) return;
                }
                m_VStack[m_v_sp-1].m_Var = NULL;
                break;
              case ePOW:
              case eSETPOW:
                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eSTRING || m_VStack[m_v_sp-2].GetType() == CExprValue::eSTRING)
                    ReportError("Illegal operation on string literals");

                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eINT && m_VStack[m_v_sp-2].GetType() == CExprValue::eINT) {
                    m_VStack[m_v_sp-2].ival = 
                        (Int8)pow((double)m_VStack[m_v_sp-2].ival, 
                                   (double)m_VStack[m_v_sp-1].ival);
                } else { 
                    m_VStack[m_v_sp-2].fval = 
                        pow(m_VStack[m_v_sp-2].GetDouble(), m_VStack[m_v_sp-1].GetDouble());
                    m_VStack[m_v_sp-2].SetType(CExprValue::eFLOAT);
                }
                m_v_sp -= 1;
                if (cop == eSETPOW) { 
                    if (!Assign()) return;
                }
                m_VStack[m_v_sp-1].m_Var = NULL;
                break;                
              case eAND:
              case eSETAND:
                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eSTRING || m_VStack[m_v_sp-2].GetType() == CExprValue::eSTRING)
                    ReportError("Illegal operation on string literals");

                 if (m_VStack[m_v_sp-1].GetType() == CExprValue::eINT && m_VStack[m_v_sp-2].GetType() == CExprValue::eINT) {
                    m_VStack[m_v_sp-2].ival &= m_VStack[m_v_sp-1].ival;
                } else if (m_VStack[m_v_sp-2].GetType() == CExprValue::eBOOL) { 
                    m_VStack[m_v_sp-2].bval = 
                        m_VStack[m_v_sp-2].bval && m_VStack[m_v_sp-1].GetBool();
                } else { 
                    m_VStack[m_v_sp-2].ival = 
                        m_VStack[m_v_sp-2].GetInt() & m_VStack[m_v_sp-1].GetInt();
                    m_VStack[m_v_sp-2].SetType(CExprValue::eINT);
                }
                m_v_sp -= 1;
                if (cop == eSETAND) { 
                    if (!Assign()) return;
                }
                m_VStack[m_v_sp-1].m_Var = NULL;
                break;
              case eOR:
              case eSETOR:
                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eSTRING || m_VStack[m_v_sp-2].GetType() == CExprValue::eSTRING)
                    ReportError("Illegal operation on string literals");

                 if (m_VStack[m_v_sp-1].GetType() == CExprValue::eINT && m_VStack[m_v_sp-2].GetType() == CExprValue::eINT) {
                    m_VStack[m_v_sp-2].ival |= m_VStack[m_v_sp-1].ival;
                } else if (m_VStack[m_v_sp-2].GetType() == CExprValue::eBOOL) { 
                    m_VStack[m_v_sp-2].bval = 
                        m_VStack[m_v_sp-2].bval || m_VStack[m_v_sp-1].GetBool();
                } else { 
                    m_VStack[m_v_sp-2].ival = 
                        m_VStack[m_v_sp-2].GetInt() | m_VStack[m_v_sp-1].GetInt();
                    m_VStack[m_v_sp-2].SetType(CExprValue::eINT);
                }
                m_v_sp -= 1;
                if (cop == eSETOR) { 
                    if (!Assign()) return;
                }
                m_VStack[m_v_sp-1].m_Var = NULL;
                break;
              case eXOR:
              case eSETXOR:
                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eSTRING || m_VStack[m_v_sp-2].GetType() == CExprValue::eSTRING)
                    ReportError("Illegal operation on string literals");

                 if (m_VStack[m_v_sp-1].GetType() == CExprValue::eINT && m_VStack[m_v_sp-2].GetType() == CExprValue::eINT) {
                    m_VStack[m_v_sp-2].ival ^= m_VStack[m_v_sp-1].ival;
                } else if (m_VStack[m_v_sp-2].GetType() == CExprValue::eBOOL) { 
                    m_VStack[m_v_sp-2].bval = 
                        m_VStack[m_v_sp-2].bval != m_VStack[m_v_sp-1].GetBool();
                } else { 
                    m_VStack[m_v_sp-2].ival = 
                        m_VStack[m_v_sp-2].GetInt() ^ m_VStack[m_v_sp-1].GetInt();
                    m_VStack[m_v_sp-2].SetType(CExprValue::eINT);
                }
                m_v_sp -= 1;
                if (cop == eSETXOR) { 
                    if (!Assign()) return;
                }
                m_VStack[m_v_sp-1].m_Var = NULL;
                break;
              case eASL:
              case eSETASL:
                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eSTRING || m_VStack[m_v_sp-2].GetType() == CExprValue::eSTRING)
                    ReportError("Illegal operation on string literals");

                 if (m_VStack[m_v_sp-1].GetType() == CExprValue::eINT && m_VStack[m_v_sp-2].GetType() == CExprValue::eINT) {
                    m_VStack[m_v_sp-2].ival <<= m_VStack[m_v_sp-1].ival;
                } else { 
                    m_VStack[m_v_sp-2].ival = 
                        m_VStack[m_v_sp-2].GetInt() << m_VStack[m_v_sp-1].GetInt();
                    m_VStack[m_v_sp-2].SetType(CExprValue::eINT);
                }
                m_v_sp -= 1;
                if (cop == eSETASL) { 
                    if (!Assign()) return;
                }
                m_VStack[m_v_sp-1].m_Var = NULL;
                break;
              case eASR:
              case eSETASR:
                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eSTRING || m_VStack[m_v_sp-2].GetType() == CExprValue::eSTRING)
                    ReportError("Illegal operation on string literals");

                 if (m_VStack[m_v_sp-1].GetType() == CExprValue::eINT && m_VStack[m_v_sp-2].GetType() == CExprValue::eINT) {
                    m_VStack[m_v_sp-2].ival >>= m_VStack[m_v_sp-1].ival;
                } else { 
                    m_VStack[m_v_sp-2].ival = 
                        m_VStack[m_v_sp-2].GetInt() >> m_VStack[m_v_sp-1].GetInt();
                    m_VStack[m_v_sp-2].SetType(CExprValue::eINT);
                }
                m_v_sp -= 1;
                if (cop == eSETASR) { 
                    if (!Assign()) return;
                }
                m_VStack[m_v_sp-1].m_Var = NULL;
                break;
              case eLSR:
              case eSETLSR:
                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eSTRING || m_VStack[m_v_sp-2].GetType() == CExprValue::eSTRING)
                    ReportError("Illegal operation on string literals");

                 if (m_VStack[m_v_sp-1].GetType() == CExprValue::eINT && m_VStack[m_v_sp-2].GetType() == CExprValue::eINT) {
                    m_VStack[m_v_sp-2].ival = 
                        (Uint8)m_VStack[m_v_sp-2].ival >> m_VStack[m_v_sp-1].ival;
                } else { 
                    m_VStack[m_v_sp-2].ival = (Uint8)m_VStack[m_v_sp-2].GetInt()
                        >> m_VStack[m_v_sp-1].GetInt();
                    m_VStack[m_v_sp-2].SetType(CExprValue::eINT);
                }
                m_v_sp -= 1;
                if (cop == eSETLSR) { 
                    if (!Assign()) return;
                }
                m_VStack[m_v_sp-1].m_Var = NULL;
                break;
              case eEQ:
                 if (m_VStack[m_v_sp-1].GetType() == CExprValue::eINT && m_VStack[m_v_sp-2].GetType() == CExprValue::eINT) {
                    m_VStack[m_v_sp-2].bval = 
                        m_VStack[m_v_sp-2].ival == m_VStack[m_v_sp-1].ival;
                    m_VStack[m_v_sp-2].SetType(CExprValue::eBOOL);
                } else if (m_VStack[m_v_sp-2].GetType() == CExprValue::eBOOL) { 
                    m_VStack[m_v_sp-2].bval = 
                        m_VStack[m_v_sp-2].bval == m_VStack[m_v_sp-1].GetBool();
                } else if (m_VStack[m_v_sp-1].GetType() == CExprValue::eSTRING && m_VStack[m_v_sp-2].GetType() == CExprValue::eSTRING) {
                    m_VStack[m_v_sp-2].bval = 
                        m_VStack[m_v_sp-2].m_sval == m_VStack[m_v_sp-1].GetString();
                    m_VStack[m_v_sp-2].SetType(CExprValue::eBOOL);
                } else { 
                    m_VStack[m_v_sp-2].bval = 
                        m_VStack[m_v_sp-2].GetDouble() == m_VStack[m_v_sp-1].GetDouble();
                    m_VStack[m_v_sp-2].SetType(CExprValue::eBOOL);
                }
                m_v_sp -= 1;
                m_VStack[m_v_sp-1].m_Var = NULL;
                break;                
              case eNE:
                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eINT && m_VStack[m_v_sp-2].GetType() == CExprValue::eINT) {
                    m_VStack[m_v_sp-2].bval = 
                        m_VStack[m_v_sp-2].ival != m_VStack[m_v_sp-1].ival;
                    m_VStack[m_v_sp-2].SetType(CExprValue::eBOOL);
                } else if (m_VStack[m_v_sp-2].GetType() == CExprValue::eBOOL) { 
                    m_VStack[m_v_sp-2].bval = 
                        m_VStack[m_v_sp-2].bval != m_VStack[m_v_sp-1].GetBool();
                } else if (m_VStack[m_v_sp-1].GetType() == CExprValue::eSTRING && m_VStack[m_v_sp-2].GetType() == CExprValue::eSTRING) {
                    m_VStack[m_v_sp-2].bval = 
                        m_VStack[m_v_sp-2].m_sval != m_VStack[m_v_sp-1].GetString();
                    m_VStack[m_v_sp-2].SetType(CExprValue::eBOOL);
                } else {
                    m_VStack[m_v_sp-2].bval = 
                        m_VStack[m_v_sp-2].GetDouble() != m_VStack[m_v_sp-1].GetDouble();
                    m_VStack[m_v_sp-2].SetType(CExprValue::eBOOL);
                }
                m_v_sp -= 1;
                m_VStack[m_v_sp-1].m_Var = NULL;
                break;                
              case eGT:
                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eSTRING || m_VStack[m_v_sp-2].GetType() == CExprValue::eSTRING)
                    ReportError("Illegal operation on string literals");

                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eINT && m_VStack[m_v_sp-2].GetType() == CExprValue::eINT) {
                    m_VStack[m_v_sp-2].bval = 
                        m_VStack[m_v_sp-2].ival > m_VStack[m_v_sp-1].ival;
                    m_VStack[m_v_sp-2].SetType(CExprValue::eBOOL);
                } else if (m_VStack[m_v_sp-2].GetType() == CExprValue::eBOOL) { 
                    m_VStack[m_v_sp-2].bval = 
                        m_VStack[m_v_sp-2].bval > m_VStack[m_v_sp-1].GetBool();
                } else { 
                    m_VStack[m_v_sp-2].bval = 
                        m_VStack[m_v_sp-2].GetDouble() > m_VStack[m_v_sp-1].GetDouble();
                    m_VStack[m_v_sp-2].SetType(CExprValue::eBOOL);
                }
                m_v_sp -= 1;
                m_VStack[m_v_sp-1].m_Var = NULL;
                break;                
              case eGE:
               if (m_VStack[m_v_sp-1].GetType() == CExprValue::eSTRING || m_VStack[m_v_sp-2].GetType() == CExprValue::eSTRING)
                    ReportError("Illegal operation on string literals");

               if (m_VStack[m_v_sp-1].GetType() == CExprValue::eINT && m_VStack[m_v_sp-2].GetType() == CExprValue::eINT) {
                    m_VStack[m_v_sp-2].bval = 
                        m_VStack[m_v_sp-2].ival >= m_VStack[m_v_sp-1].ival;
                    m_VStack[m_v_sp-2].SetType(CExprValue::eBOOL);
                } else if (m_VStack[m_v_sp-2].GetType() == CExprValue::eBOOL) { 
                    m_VStack[m_v_sp-2].bval = 
                        m_VStack[m_v_sp-2].bval >= m_VStack[m_v_sp-1].GetBool();
                } else { 
                    m_VStack[m_v_sp-2].bval = 
                        m_VStack[m_v_sp-2].GetDouble() >= m_VStack[m_v_sp-1].GetDouble();
                    m_VStack[m_v_sp-2].SetType(CExprValue::eBOOL);
                }
                m_v_sp -= 1;
                m_VStack[m_v_sp-1].m_Var = NULL;
                break;                
              case eLT:
               if (m_VStack[m_v_sp-1].GetType() == CExprValue::eSTRING || m_VStack[m_v_sp-2].GetType() == CExprValue::eSTRING)
                    ReportError("Illegal operation on string literals");

                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eINT && m_VStack[m_v_sp-2].GetType() == CExprValue::eINT) {
                    m_VStack[m_v_sp-2].bval = 
                        m_VStack[m_v_sp-2].ival < m_VStack[m_v_sp-1].ival;
                    m_VStack[m_v_sp-2].SetType(CExprValue::eBOOL);
                } else if (m_VStack[m_v_sp-2].GetType() == CExprValue::eBOOL) { 
                    m_VStack[m_v_sp-2].bval = 
                        m_VStack[m_v_sp-2].bval < m_VStack[m_v_sp-1].GetBool();
                } else { 
                    m_VStack[m_v_sp-2].bval = 
                        m_VStack[m_v_sp-2].GetDouble() < m_VStack[m_v_sp-1].GetDouble();
                    m_VStack[m_v_sp-2].SetType(CExprValue::eBOOL);
                }
                m_v_sp -= 1;
                m_VStack[m_v_sp-1].m_Var = NULL;
                break;                
              case eLE:
                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eSTRING || m_VStack[m_v_sp-2].GetType() == CExprValue::eSTRING)
                    ReportError("Illegal operation on string literals");

               if (m_VStack[m_v_sp-1].GetType() == CExprValue::eINT && m_VStack[m_v_sp-2].GetType() == CExprValue::eINT) {
                    m_VStack[m_v_sp-2].bval = 
                        m_VStack[m_v_sp-2].ival <= m_VStack[m_v_sp-1].ival;
                    m_VStack[m_v_sp-2].SetType(CExprValue::eBOOL);
                } else if (m_VStack[m_v_sp-2].GetType() == CExprValue::eBOOL) { 
                    m_VStack[m_v_sp-2].bval = 
                        m_VStack[m_v_sp-2].bval <= m_VStack[m_v_sp-1].GetBool();
                } else { 
                    m_VStack[m_v_sp-2].bval = 
                        m_VStack[m_v_sp-2].GetDouble() <= m_VStack[m_v_sp-1].GetDouble();
                    m_VStack[m_v_sp-2].SetType(CExprValue::eBOOL);
                }
                m_v_sp -= 1;
                m_VStack[m_v_sp-1].m_Var = NULL;
                break;                
              case ePREINC:
                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eSTRING || m_VStack[m_v_sp-2].GetType() == CExprValue::eSTRING)
                    ReportError("Illegal operation on string literals");

               if (m_VStack[m_v_sp-1].GetType() == CExprValue::eINT) { 
                    m_VStack[m_v_sp-1].ival += 1;
                } else { 
                    m_VStack[m_v_sp-1].fval += 1;
                } 
                if (!Assign()) return;
                m_VStack[m_v_sp-1].m_Var = NULL;
                break;
              case ePREDEC:
                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eINT) { 
                    m_VStack[m_v_sp-1].ival -= 1;
                } else { 
                    m_VStack[m_v_sp-1].fval -= 1;
                } 
                if (!Assign()) return;
                m_VStack[m_v_sp-1].m_Var = NULL;
                break;
              case ePOSTINC:
                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eSTRING || m_VStack[m_v_sp-2].GetType() == CExprValue::eSTRING)
                    ReportError("Illegal operation on string literals");

               if (m_VStack[m_v_sp-1].m_Var == NULL) { 
                    ReportError(m_VStack[m_v_sp-1].m_Pos, "Varaibale expected");
                    return;
                } 
                if (m_VStack[m_v_sp-1].m_Var->m_Val.GetType() == CExprValue::eINT) { 
                    m_VStack[m_v_sp-1].m_Var->m_Val.ival += 1;
                } else { 
                    m_VStack[m_v_sp-1].m_Var->m_Val.fval += 1;
                } 
                m_VStack[m_v_sp-1].m_Var = NULL;
                break;
              case ePOSTDEC:
                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eSTRING || m_VStack[m_v_sp-2].GetType() == CExprValue::eSTRING)
                    ReportError("Illegal operation on string literals");

               if (m_VStack[m_v_sp-1].m_Var == NULL) { 
                    ReportError(m_VStack[m_v_sp-1].m_Pos, "Varaibale expected");
                    return;
                } 
                if (m_VStack[m_v_sp-1].m_Var->m_Val.GetType() == CExprValue::eINT) { 
                    m_VStack[m_v_sp-1].m_Var->m_Val.ival -= 1;
                } else { 
                    m_VStack[m_v_sp-1].m_Var->m_Val.fval -= 1;
                } 
                m_VStack[m_v_sp-1].m_Var = NULL;
                break;
              case eSET:
                if (m_VStack[m_v_sp-2].m_Var == NULL) { 
                    ReportError(m_VStack[m_v_sp-2].m_Pos, "Variable expected");
                    return;
                } else { 
                    m_VStack[m_v_sp-2]=m_VStack[m_v_sp-2].m_Var->m_Val=m_VStack[m_v_sp-1];
                }                    
                m_v_sp -= 1;
                m_VStack[m_v_sp-1].m_Var = NULL;
                break;
              case eNOT:
                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eSTRING || m_VStack[m_v_sp-2].GetType() == CExprValue::eSTRING)
                    ReportError("Illegal operation on string literals");

               if (m_VStack[m_v_sp-1].GetType() == CExprValue::eINT) { 
                    m_VStack[m_v_sp-1].ival = !m_VStack[m_v_sp-1].ival;
                } else if (m_VStack[m_v_sp-1].GetType() == CExprValue::eBOOL) { 
                    m_VStack[m_v_sp-1].bval = !m_VStack[m_v_sp-1].bval;
                } else { 
                    m_VStack[m_v_sp-1].ival = !m_VStack[m_v_sp-1].fval;
                    m_VStack[m_v_sp-1].SetType(CExprValue::eINT);
                }
                m_VStack[m_v_sp-1].m_Var = NULL;
                break;
              case eMINUS:
               if (m_VStack[m_v_sp-1].GetType() == CExprValue::eSTRING || m_VStack[m_v_sp-2].GetType() == CExprValue::eSTRING)
                    ReportError("Illegal operation on string literals");

                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eINT) { 
                    m_VStack[m_v_sp-1].ival = -m_VStack[m_v_sp-1].ival;
                } else { 
                    m_VStack[m_v_sp-1].fval = -m_VStack[m_v_sp-1].fval;
                }
                // no break
              case ePLUS:
                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eSTRING || m_VStack[m_v_sp-2].GetType() == CExprValue::eSTRING)
                    ReportError("Illegal operation on string literals");

               m_VStack[m_v_sp-1].m_Var = NULL;
                break;
              case eCOM:
               if (m_VStack[m_v_sp-1].GetType() == CExprValue::eSTRING || m_VStack[m_v_sp-2].GetType() == CExprValue::eSTRING)
                    ReportError("Illegal operation on string literals");

                if (m_VStack[m_v_sp-1].GetType() == CExprValue::eINT) { 
                    m_VStack[m_v_sp-1].ival = ~m_VStack[m_v_sp-1].ival;
                } else { 
                    m_VStack[m_v_sp-1].ival = ~(int)m_VStack[m_v_sp-1].fval;
                    m_VStack[m_v_sp-1].SetType(CExprValue::eINT);
                }
                m_VStack[m_v_sp-1].m_Var = NULL;
                break;                
              case eRPAR:
                ReportError("mismatched ')'");
                return;
              case eFUNC:
                ReportError("'(' expected");
                return;
              case eLPAR:
                if (oper != eRPAR) { 
                    ReportError("')' expected");
                    return;
                }
                if (m_OStack[m_o_sp-1] == eFUNC) { 
                    CExprSymbol* sym = m_VStack[m_v_sp-n_args-1].m_Var;

                    switch(sym->m_Tag) {
                    case CExprSymbol::eIFUNC1:
                        if (n_args != 1) { 
                            ReportError(m_VStack[m_v_sp-n_args-1].m_Pos, 
                                "Function should take one argument");
                            return;
                        }
                        m_VStack[m_v_sp-2].ival = 
                        (*sym->m_IntFunc1)(m_VStack[m_v_sp-1].GetInt());
                        m_VStack[m_v_sp-2].SetType(CExprValue::eINT);
                        m_v_sp -= 1;
                        break;
                    case CExprSymbol::eIFUNC2:
                        if (n_args != 2) { 
                            ReportError(m_VStack[m_v_sp-n_args-1].m_Pos, 
                                "Function should take two arguments");
                            return;
                        }
                        m_VStack[m_v_sp-3].ival =
                        (*sym->m_IntFunc2)
                        (m_VStack[m_v_sp-2].GetInt(), m_VStack[m_v_sp-1].GetInt());
                        m_VStack[m_v_sp-3].SetType(CExprValue::eINT);
                        m_v_sp -= 2;
                        break;
                    case CExprSymbol::eFFUNC1:
                        if (n_args != 1) { 
                            ReportError(m_VStack[m_v_sp-n_args-1].m_Pos, 
                                "Function should take one argument");
                            return;
                        }
                        m_VStack[m_v_sp-2].fval = 
                        (*sym->m_FloatFunc1)(m_VStack[m_v_sp-1].GetDouble());
                        m_VStack[m_v_sp-2].SetType(CExprValue::eFLOAT);
                        m_v_sp -= 1;
                        break;
                    case CExprSymbol::eFFUNC2:
                        if (n_args != 2) { 
                            ReportError(m_VStack[m_v_sp-n_args-1].m_Pos, 
                                "Function should take two arguments");
                            return;
                        }
                        m_VStack[m_v_sp-3].fval = 
                            (*sym->m_FloatFunc2)
                            (m_VStack[m_v_sp-2].GetDouble(), m_VStack[m_v_sp-1].GetDouble());
                        m_VStack[m_v_sp-3].SetType(CExprValue::eFLOAT);
                        m_v_sp -= 2;
                        break;
                    case CExprSymbol::eBFUNC1:
                        if (n_args != 1) { 
                            ReportError(m_VStack[m_v_sp-n_args-1].m_Pos, 
                                "Function should take one argument");
                            return;
                        }
                        m_VStack[m_v_sp-2].bval = 
                        (*sym->m_BoolFunc1)(m_VStack[m_v_sp-1].GetBool());
                        m_VStack[m_v_sp-2].SetType(CExprValue::eBOOL);
                        m_v_sp -= 1;
                        break;
                    case CExprSymbol::eBFUNC2:
                        if (n_args != 2) { 
                            ReportError(m_VStack[m_v_sp-n_args-1].m_Pos, 
                                "Function should take two arguments");
                            return;
                        }
                        m_VStack[m_v_sp-3].bval = 
                            (*sym->m_BoolFunc2)
                            (m_VStack[m_v_sp-2].GetBool(), m_VStack[m_v_sp-1].GetBool());
                        m_VStack[m_v_sp-3].SetType(CExprValue::eBOOL);
                        m_v_sp -= 2;
                        break;
                    case CExprSymbol::eSFUNC1:
                        if (n_args != 1) { 
                            ReportError(m_VStack[m_v_sp-n_args-1].m_Pos, 
                                "Function should take one argument");
                            return;
                        }
                        m_VStack[m_v_sp-2].bval =
                        (*sym->m_StringFunc1)(m_VStack[m_v_sp-1].GetString());
                        m_VStack[m_v_sp-2].SetType(CExprValue::eBOOL);
                        m_v_sp -= 1;
                        break;
;
                    default: 
                        ReportError("Invalid expression");
                    }

                    m_VStack[m_v_sp-1].m_Var = NULL; 
                    m_o_sp -= 1;
                    n_args = 1;
                } else if (n_args != 1) { 
                    ReportError("Function call expected");
                    return;
                }

                goto next_token;

              default:
                ReportError("syntax ReportError");
            }
        }

        if (m_o_sp == max_stack_size) { 
            ReportError("operator stack overflow");
            return;
        }

        m_OStack[m_o_sp++] = oper;
    }
}


END_NCBI_SCOPE
