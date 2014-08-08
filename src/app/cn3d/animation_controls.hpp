/*  $Id: animation_controls.hpp 128326 2008-05-21 15:57:16Z thiessen $
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
* Authors:  Paul Thiessen
*
* File Description:
*      dialog for setting animation controls
*
* ===========================================================================
*/

#ifndef CN3D_ANIMATION_CONTROLS__HPP
#define CN3D_ANIMATION_CONTROLS__HPP

#include <corelib/ncbistd.hpp>

#ifdef __WXMSW__
#include <windows.h>
#include <wx/msw/winundef.h>
#endif
#include <wx/wx.h>

#include <algo/structure/wx_tools/wx_tools.hpp>


BEGIN_SCOPE(Cn3D)

class AnimationControls : public wxDialog
{
public:
    AnimationControls(wxWindow *parent);
    ~AnimationControls(void);

private:
    // event callbacks
    void OnCloseWindow(wxCloseEvent& event);
    void OnButton(wxCommandEvent& event);

    // GUI elements
    ncbi::IntegerSpinCtrl *iSpinDelay, *iFrameDelay;
    ncbi::FloatingPointSpinCtrl *fSpinIncrement;

    DECLARE_EVENT_TABLE()
};

END_SCOPE(Cn3D)

#endif // CN3D_ANIMATION_CONTROLS__HPP
