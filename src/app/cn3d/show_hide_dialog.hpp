/*  $Id: show_hide_dialog.hpp 103491 2007-05-04 17:18:18Z kazimird $
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
*      Class definition for the Show/Hide dialog
*
* ===========================================================================
*/

#ifndef CN3D_SHOW_HIDE_DIALOG__HPP
#define CN3D_SHOW_HIDE_DIALOG__HPP

#include <corelib/ncbistd.hpp>
#include <corelib/ncbistl.hpp>

#ifdef __WXMSW__
#include <windows.h>
#include <wx/msw/winundef.h>
#endif
#include <wx/wx.h>

#include <vector>

#include "show_hide_callback.hpp"


BEGIN_SCOPE(Cn3D)

class ShowHideDialog : public wxDialog
{
public:

    enum {
        // return values
        DONE,
        CANCEL,

        // control identifiers
        LISTBOX,
        B_DONE,
        B_CANCEL,
        B_APPLY
    };

    ShowHideDialog(
        const wxString items[],                 // must have items->size() wxStrings
        std::vector < bool > *itemsOn,          // modified to reflect user (de)selection(s)
        ShowHideCallbackObject *callback,       // to reflect changes as user performs acts
        bool useExtendedListStyle,              // whether to use wxLB_EXTENDED or wxLB_MULTIPLE
        wxWindow* parent,
        wxWindowID id,
        const wxString& title,
        const wxPoint& pos = wxDefaultPosition
    );

private:
    void OnSelection(wxCommandEvent& event);
    void OnButton(wxCommandEvent& event);
    void OnCloseWindow(wxCloseEvent& event);

    std::vector < bool > *itemsEnabled;
    std::vector < bool > originalItemsEnabled;  // save in case of 'Cancel'
    std::vector < bool > tempItemsEnabled;      // temporary storage
    bool haveApplied;

    ShowHideCallbackObject *callbackObject;
    wxListBox *listBox;
    wxButton *applyB, *cancelB;

    DECLARE_EVENT_TABLE()
};

END_SCOPE(Cn3D)

#endif // CN3D_SHOW_HIDE_DIALOG__HPP
