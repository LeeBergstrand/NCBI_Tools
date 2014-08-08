#ifndef CONNECT_SERVICES__NET_OBJECT_HPP
#define CONNECT_SERVICES__NET_OBJECT_HPP

/*  $Id: netcomponent.hpp 353379 2012-02-15 16:24:52Z kazimird $
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
 * Authors:  Dmitry Kazimirov
 *
 * File Description:
 *
 */

#include <connect/connect_export.h>

#include <corelib/ncbiobj.hpp>
#include <corelib/ncbicntr.hpp>

BEGIN_NCBI_SCOPE

///////////////////////////////////////////////////////////////////////////
//

/// To create a void (uninitialized) component instance
enum EVoid {
    eVoid   ///< To create a void (uninitialized) instance of a component
};

template <class S>
class CNetComponentCounterLocker : public CObjectCounterLocker
{
public:
    void Lock(const S* object) const
    {
        CObjectCounterLocker::Lock(reinterpret_cast<const CObject*>(object));
    }

    void Relock(const S* object) const
    {
        CObjectCounterLocker::Relock(reinterpret_cast<const CObject*>(object));
    }

    void Unlock(const S* object) const
    {
        CObjectCounterLocker::Unlock(reinterpret_cast<const CObject*>(object));
    }

    void UnlockRelease(const S* object) const
    {
        CObjectCounterLocker::UnlockRelease(
            reinterpret_cast<const CObject*>(object));
    }
};


#define NCBI_NET_COMPONENT_WITH_DEFAULT_CTOR(component) \
    protected: \
    CRef<S##component##Impl, \
        CNetComponentCounterLocker<S##component##Impl> > m_Impl; \
    public: \
    typedef S##component##Impl* TInstance; \
    C##component(S##component##Impl* impl) : m_Impl(impl) {} \
    C##component& operator =(S##component##Impl* impl) \
        {m_Impl = impl; return *this;} \
    operator S##component##Impl*() {return m_Impl.GetPointer();} \
    operator const S##component##Impl*() const {return m_Impl.GetPointer();} \
    S##component##Impl* operator ->() {return m_Impl.GetPointer();} \
    const S##component##Impl* operator ->() const \
        {return m_Impl.GetPointer();} \
    C##component(EVoid /* create_void */) {}

#define NCBI_NET_COMPONENT(component) \
    NCBI_NET_COMPONENT_WITH_DEFAULT_CTOR(component) \
    C##component() {}

END_NCBI_SCOPE

#endif // CONNECT_SERVICES__SERVER_CONN_HPP
