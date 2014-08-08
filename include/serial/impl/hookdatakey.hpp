#ifndef HOOKDATAKEY__HPP
#define HOOKDATAKEY__HPP

/*  $Id: hookdatakey.hpp 184468 2010-03-01 15:45:11Z gouriano $
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
* Author: Eugene Vasilchenko
*
* File Description:
*   !!! PUT YOUR DESCRIPTION HERE !!!
*/

#include <corelib/ncbistd.hpp>
#include <corelib/ncbiobj.hpp>

#include <vector>

/** @addtogroup HookSupport
 *
 * @{
 */


BEGIN_NCBI_SCOPE

class CHookDataBase;

class NCBI_XSERIAL_EXPORT CLocalHookSetBase
{
public:
    typedef CHookDataBase THookData;
    typedef CObject THook;

    CLocalHookSetBase(void);
    ~CLocalHookSetBase(void);

    void Clear(void);
    bool IsEmpty(void) const
    {
        return m_Hooks.empty();
    }

    typedef pair<THookData*, CRef<THook> > TValue;
    typedef vector<TValue> THooks;

protected:
    void ResetHook(THookData* key);
    void SetHook(THookData* key, THook* hook);
    const THook* GetHook(const THookData* key) const;

private:
    CLocalHookSetBase(const CLocalHookSetBase&);
    CLocalHookSetBase& operator=(const CLocalHookSetBase&);

    friend class CHookDataBase;

    struct Compare
    {
        bool operator()(const TValue& v1, const TValue& v2) const
            {
                return v1.first < v2.first;
            }
	bool operator()(const THookData* key, const TValue& value) const
            {
                return key < value.first;
            }
        bool operator()(const TValue& value, const THookData* key) const
            {
                return value.first < key;
            }
    };

    THooks::iterator x_Find(const THookData* key);
    THooks::const_iterator x_Find(const THookData* key) const;
    bool x_Found(THooks::const_iterator it, const THookData* key) const;

    THooks m_Hooks;
};


template<class Hook>
class CLocalHookSet : public CLocalHookSetBase
{
    typedef CLocalHookSetBase CParent;
public:
    typedef CParent::THookData THookData;
    typedef Hook THook;

protected:
    friend class CHookDataBase;
    void SetHook(THookData* key, THook* hook)
        {
            CParent::SetHook(key, hook);
        }
    THook* GetHook(THookData* key) const
        {
            return static_cast<THook*>(CParent::GetHook(key));
        }
};

END_NCBI_SCOPE

#endif  /* HOOKDATAKEY__HPP */


/* @} */
