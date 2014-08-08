#ifndef CORELIB___STR_UTIL__HPP
#define CORELIB___STR_UTIL__HPP

/*  $Id: ncbistr_util.hpp 376964 2012-10-05 15:27:44Z ucko $
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
 * Authors:  Eugene Vasilchenko, Aaron Ucko, Denis Vakatov, Anatoliy Kuznetsov
 *
 * File Description:
 *      String algorithms
 *
 */

/// @file ncbistr_util.hpp
/// Algorithms for string processing

#include <corelib/ncbistr.hpp>

BEGIN_NCBI_SCOPE


/** @addtogroup String
 *
 * @{
 */

class CStrTokenizeBase;

/// Do-nothing token position container
///
struct CStrDummyTokenPos
{
    void push_back(string::size_type /*pos*/) {}
    void reserve(string::size_type) {}
};

/// Do nothing token counter 
///
struct CStrDummyTokenCount
{
    static
    size_t Count(CStrTokenizeBase& /*tokenizer*/)
    {
        return 0;
    }
};

/// Do nothing target reservation trait 
///
/// applies for list<> or cases when reservation only makes things slower
/// (may be the case if we use deque<> as a target container and tokenize 
/// large text)
///
template<class TV, class TP>
struct CStrDummyTargetReserve
{
    static
    void Reserve(CStrTokenizeBase& /*tokenizer*/, 
                 TV&               /*target*/,
                 TP&               /*token_pos*/)
    {}
};


/// Singly-linked list of substrings that will constitute a single
/// Split/Tokenize piece, optimized for the typical one-node case.
class CTempStringList
{
public:
    CTempStringList() : m_LastNode(NULL) { }

    void   Add(const CTempString& s);
    void   Clear(void);
    void   Join(string* s) const;
    void   Join(CTempStringEx* s) const;
    size_t GetSize(void) const;

private:
    struct SNode
    {
        SNode() { }
        SNode(const CTempString& s) : str(s) { }

        CTempString     str;
        auto_ptr<SNode> next;
    };

    SNode  m_FirstNode;
    SNode *m_LastNode;
};

inline
void CTempStringList::Add(const CTempString& s)
{
    if (m_LastNode == NULL) {
        m_FirstNode.str = s;
        m_LastNode = &m_FirstNode;
    } else {
        m_LastNode->next.reset(new SNode(s));
        m_LastNode = m_LastNode->next.get();
    }
}

inline
void CTempStringList::Clear(void)
{
    m_FirstNode.str.clear();
    m_FirstNode.next.reset();
    m_LastNode = NULL;
}

class CStrTokenizeBase
{
public:
    typedef NStr::TSplitFlags TFlags;

    CStrTokenizeBase(const CTempString& str, const CTempString& delim,
                     TFlags flags);

    SIZE_TYPE GetPos(void) const { return m_Pos; }
    bool      AtEnd (void) const { return m_Pos == NPOS; }

    bool Advance   (CTempStringList* part_collector);
    void SkipDelims(void);

    void SetDelim(const CTempString& delim);

protected:
    const CTempString& m_Str;
    CTempString        m_Delim;
    SIZE_TYPE          m_Pos;
    TFlags             m_Flags;

private:
    void x_ExtendInternalDelim(void);

    CTempStringEx m_InternalDelim;
};

inline
CStrTokenizeBase::CStrTokenizeBase(const CTempString& str,
                                   const CTempString& delim,
                                   TFlags flags)
    : m_Str(str), m_Pos(0), m_Flags(flags)
{
    SetDelim(delim);
}

inline
void CStrTokenizeBase::SetDelim(const CTempString& delim)
{
    m_Delim = delim;
    if ((m_Flags & NStr::fSplit_ByPattern) == 0) {
        m_InternalDelim = m_Delim;
    } else {
        m_InternalDelim.assign(m_Delim, 0, 1);
    }

    if ((m_Flags & (NStr::fSplit_CanEscape | NStr::fSplit_CanQuote)) != 0) {
        x_ExtendInternalDelim();
    }
}

/// Main tokenization algorithm
///
/// TStr    - string type (must follow std::string interface)
/// TV      - container type for tokens (std::vector)
/// TP      - target container type for token positions (vector<size_t>)
/// TCount  - token count trait for the target container space reservation
/// TReserve- target space reservation trait
///
template<class TStr, 
         class TV, 
         class TP = CStrDummyTokenPos, 
         class TCount = CStrDummyTokenCount,
         class TReserve = CStrDummyTargetReserve<TV, TP> > 
class CStrTokenize : public CStrTokenizeBase
{
public:
    typedef TStr     TString;
    typedef TV       TContainer;
    typedef TP       TPosContainer;
    typedef TCount   TCountTrait;
    typedef TReserve TReserveTrait;

    /// Constructor
    ///
    /// @param str
    ///   String to be tokenized.
    /// @param delim
    ///   Set of char delimiters used to tokenize string "str".
    ///   If delimiter is empty, then input string is appended to "arr" as is.
    /// @param flags
    ///   Flags governing splitting.
    ///   Without NStr::fSplit_MergeDelims, delimiters that immediately follow
    ///   each other are treated as separate delimiters - empty string(s) 
    ///   appear in the target output.
    ///
    CStrTokenize(const TString& str, const TString& delim, TFlags flags)
        : CStrTokenizeBase(str, delim, flags) 
        { }

    /// Tokenize the string using the specified set of char delimiters.
    ///
    /// @param target
    ///   Output container. 
    ///   Tokens defined in "str" by using symbols from "delim" are added
    ///   to the list "target".
    /// @param token_pos
    ///   Container for the tokens' positions in "str".
    /// @param empty_str
    ///   String added to target when there are no other characters between
    ///   delimiters
    ///
    void Do(TContainer&        target,
            TPosContainer&     token_pos,
            const TString&     empty_str = TString())
    {
        // Special cases
        if (m_Str.empty()) {
            return;
        } else if (m_Delim.empty()) {
            target.push_back(m_Str);
            token_pos.push_back(0);
            return;
        }

        // Do target space reservation (if applicable)
        TReserveTrait::Reserve(*this, target, token_pos);

        // Reposition to just after any initial delimiters
        m_Pos = 0;
        SkipDelims();

        // Tokenization
        //
        CTempStringList part_collector;
        SIZE_TYPE       prev_pos;
        do {
            prev_pos = m_Pos;
            if (Advance(&part_collector)) {
                target.push_back(empty_str);
                part_collector.Join(&target.back());
                part_collector.Clear();
                token_pos.push_back(prev_pos);
            }
        } while ( !AtEnd() );
    }
};


/// token count trait for std::string
///
struct CStringTokenCount
{
    static
    size_t Count(CStrTokenizeBase& tokenizer)
    {
        size_t tokens = 0;

        do {
            if (tokenizer.Advance(NULL)) {
                ++tokens;
            }
        } while ( !tokenizer.AtEnd() );

        return tokens;
    }
};


/// Target reservation trait (applies for vector<>)
///
template<class TV, class TP, class TCount>
struct CStrTargetReserve
{
    static
    void Reserve(CStrTokenizeBase& tokenizer,
                 TV&               target,
                 TP&               token_pos)
    {
        // Reserve vector size only for empty vectors.  For vectors which
        // already have items this has been known to work more slowly.
        if ( target.empty() ) {
            size_t tokens = TCount::Count(tokenizer);
            if (tokens) { 
                token_pos.reserve(tokens);
                target.reserve(tokens);
            }
        }
    }
};


/// Adapter for token position container pointer(NULL legal)
/// Makes pointer to a container look as a legal container
///
template<class TPosContainer>
class CStrTokenPosAdapter
{
public:
    /// If token_pos construction parameter is NULL all calls are ignored
    CStrTokenPosAdapter(TPosContainer* token_pos)
        : m_TokenPos(token_pos)
    {}

    void push_back(string::size_type pos)
    {
        if (m_TokenPos) m_TokenPos->push_back(pos);
    }
    void reserve(string::size_type capacity)
    {
        if (m_TokenPos) m_TokenPos->reserve(capacity);
    }
private:
    TPosContainer* m_TokenPos;
};



 /*
 * @}
 */

/////////////////////////////////////////////////////////////////////////////


END_NCBI_SCOPE

#endif  // CORELIB___STR_UTIL__HPP
