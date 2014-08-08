/*  $Id: show_hide_dialog.cpp 103491 2007-05-04 17:18:18Z kazimird $
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

#include <ncbi_pch.hpp>
#include <corelib/ncbistd.hpp>

#include "remove_header_conflicts.hpp"

#include "show_hide_dialog.hpp"
#include "cn3d_tools.hpp"

USING_NCBI_SCOPE;


BEGIN_SCOPE(Cn3D)

BEGIN_EVENT_TABLE(ShowHideDialog, wxDialog)
    EVT_LISTBOX     (LISTBOX,   ShowHideDialog::OnSelection)
    EVT_BUTTON      (B_APPLY,   ShowHideDialog::OnButton)
    EVT_BUTTON      (B_CANCEL,  ShowHideDialog::OnButton)
    EVT_BUTTON      (B_DONE,    ShowHideDialog::OnButton)
    EVT_CLOSE                  (ShowHideDialog::OnCloseWindow)
END_EVENT_TABLE()

#define MIN_SIZE 250,300
#define B_WIDTH 50
#define MARGIN 10

ShowHideDialog::ShowHideDialog(
    const wxString items[],
    vector < bool > *itemsOn,
    ShowHideCallbackObject *callback,
    bool useExtendedListStyle,
    wxWindow* parent,
    wxWindowID id,
    const wxString& title,
    const wxPoint& pos
) :
    wxDialog(parent, id, title, pos, wxSize(MIN_SIZE), wxCAPTION | wxRESIZE_BORDER),
    itemsEnabled(itemsOn), haveApplied(false), callbackObject(callback), applyB(NULL)
{
    SetAutoLayout(true);
    SetSizeHints(MIN_SIZE);

	wxButton *doneB;
    if (callbackObject) {
        doneB = new wxButton(this, B_DONE, "Done");
        wxLayoutConstraints *c1 = new wxLayoutConstraints();
        c1->bottom.SameAs       (this,      wxBottom,   MARGIN);
        c1->right.SameAs        (this,      wxRight,    MARGIN);
        c1->width.Absolute      (B_WIDTH);
        c1->height.AsIs         ();
        doneB->SetConstraints(c1);

        applyB = new wxButton(this, B_APPLY, "Apply");
        wxLayoutConstraints *c2 = new wxLayoutConstraints();
        c2->bottom.SameAs       (this,      wxBottom,   MARGIN);
        c2->centreX.SameAs      (this,      wxCentreX);
        c2->width.Absolute      (B_WIDTH);
        c2->top.SameAs          (doneB,     wxTop);
        applyB->SetConstraints(c2);
        applyB->Enable(false);

        cancelB = new wxButton(this, B_CANCEL, "Cancel");
        wxLayoutConstraints *c3 = new wxLayoutConstraints();
        c3->bottom.SameAs       (this,      wxBottom,   MARGIN);
        c3->left.SameAs         (this,      wxLeft,     MARGIN);
        c3->width.Absolute      (B_WIDTH);
        c3->top.SameAs          (doneB,     wxTop);
        cancelB->SetConstraints(c3);
        cancelB->Enable(false);
    }

    // don't include Apply button if no callback object given
    else {
        doneB = new wxButton(this, B_DONE, "Done");
        wxLayoutConstraints *c1 = new wxLayoutConstraints();
        c1->bottom.SameAs       (this,      wxBottom,   MARGIN);
        c1->left.SameAs         (this,      wxCentreX,  MARGIN);
        c1->width.Absolute      (B_WIDTH);
        c1->height.AsIs         ();
        doneB->SetConstraints(c1);

        cancelB = new wxButton(this, B_CANCEL, "Cancel");
        wxLayoutConstraints *c3 = new wxLayoutConstraints();
        c3->bottom.SameAs       (this,      wxBottom,   MARGIN);
        c3->right.SameAs        (this,      wxCentreX,  MARGIN);
        c3->width.Absolute      (B_WIDTH);
        c3->top.SameAs          (doneB,     wxTop);
        cancelB->SetConstraints(c3);
        cancelB->Enable(false);
    }

    listBox = new wxListBox(this, LISTBOX, wxDefaultPosition, wxDefaultSize,
        itemsEnabled->size(), items,
        (useExtendedListStyle ? wxLB_EXTENDED : wxLB_MULTIPLE) | wxLB_HSCROLL | wxLB_NEEDED_SB);
    wxLayoutConstraints *c4 = new wxLayoutConstraints();
    c4->top.SameAs          (this,      wxTop,      MARGIN);
    c4->left.SameAs         (this,      wxLeft,     MARGIN);
    c4->right.SameAs        (this,      wxRight,    MARGIN);
    c4->bottom.Above        (doneB,                 -MARGIN);
    listBox->SetConstraints(c4);

    Layout();

    // set initial item selection (kludge: reverse order, so scroll bar is initially at the top)
    originalItemsEnabled.resize(itemsEnabled->size());
    for (int i=itemsEnabled->size()-1; i>=0; --i) {
        listBox->SetSelection(i, true);
        listBox->SetSelection(i, (*itemsEnabled)[i]);
        originalItemsEnabled[i] = (*itemsEnabled)[i];
    }
    tempItemsEnabled.resize(itemsEnabled->size());
}

void ShowHideDialog::OnSelection(wxCommandEvent& event)
{
    unsigned int i;

    // get states as they are before this selection
    for (i=0; i<listBox->GetCount(); ++i) tempItemsEnabled[i] = (*itemsEnabled)[i];

    // update from current listbox selections
    for (i=0; i<listBox->GetCount(); ++i) (*itemsEnabled)[i] = listBox->IsSelected(i);

    // do the selection changed callback, and adjust selections according to what's changed
    if (callbackObject && callbackObject->SelectionChangedCallback(tempItemsEnabled, *itemsEnabled)) {

        // apply changes
        int pV = listBox->GetScrollPos(wxVERTICAL);
        for (i=0; i<listBox->GetCount(); ++i) listBox->SetSelection(i, (*itemsEnabled)[i]);

        // horrible kludge to keep vertical scroll at same position...
        if (pV >= 0 && pV < (int) listBox->GetCount()) {
            listBox->SetSelection(listBox->GetCount() - 1, true);
            listBox->SetSelection(listBox->GetCount() - 1, (*itemsEnabled)[listBox->GetCount() - 1]);
            listBox->SetSelection(pV, true);
            listBox->SetSelection(pV, (*itemsEnabled)[pV]);
        }
    }

    cancelB->Enable(true);
    if (applyB) applyB->Enable(true);
}

void ShowHideDialog::OnButton(wxCommandEvent& event)
{
    bool doCallback = (applyB && applyB->IsEnabled());

    if (event.GetId() == B_CANCEL) {
        // restore item list to original state
        for (unsigned int i=0; i<originalItemsEnabled.size(); ++i) (*itemsEnabled)[i] = originalItemsEnabled[i];
        doCallback = haveApplied;
    }

    // only do this if selection has changed since last time
    if (callbackObject && doCallback) {
        TRACEMSG("calling ShowHideCallbackFunction()");
        // do the callback with the current selection state
        callbackObject->ShowHideCallbackFunction(*itemsEnabled);
        if (applyB) applyB->Enable(false);
    }

    if (event.GetId() == B_CANCEL)
        EndModal(wxCANCEL);
    else if (event.GetId() == B_DONE)
        EndModal(wxOK);
    else if (event.GetId() == B_APPLY)
        haveApplied = true;
}

void ShowHideDialog::OnCloseWindow(wxCloseEvent& event)
{
    EndModal(wxOK);
}

END_SCOPE(Cn3D)
