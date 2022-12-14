/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <svl/undo.hxx>
#include <unotools/textsearch.hxx>
#include <unotools/localedatawrapper.hxx>
#include <unotools/collatorwrapper.hxx>
#include <sfx2/app.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/basedlgs.hxx>

#include <acredlin.hxx>
#include <global.hxx>
#include <reffact.hxx>
#include <document.hxx>
#include <docsh.hxx>
#include <scresid.hxx>
#include <strings.hrc>
#include <simpref.hxx>
#include <scmod.hxx>
#include <tabvwsh.hxx>

// defines -------------------------------------------------------------------

#define RD_SPECIAL_NONE         0
#define RD_SPECIAL_CONTENT      1
#define RD_SPECIAL_VISCONTENT   2

//  class ScRedlinData

ScRedlinData::ScRedlinData()
    :RedlinData()
{
    nInfo=RD_SPECIAL_NONE;
    nActionNo=0;
    pData=nullptr;
    bDisabled=false;
    bIsRejectable=false;
    bIsAcceptable=false;
    nTable=SCTAB_MAX;
    nCol=SCCOL_MAX;
    nRow=SCROW_MAX;
}

ScRedlinData::~ScRedlinData()
{
    nInfo=RD_SPECIAL_NONE;
    nActionNo=0;
    pData=nullptr;
    bDisabled=false;
    bIsRejectable=false;
    bIsAcceptable=false;
}

//  class ScAcceptChgDlg

ScAcceptChgDlg::ScAcceptChgDlg(SfxBindings* pB, SfxChildWindow* pCW, vcl::Window* pParent,
    ScViewData* ptrViewData)
    : SfxModelessDialog(pB, pCW, pParent,
        "AcceptRejectChangesDialog", "svx/ui/acceptrejectchangesdialog.ui"),
        aSelectionIdle("ScAcceptChgDlg SelectionIdle"),
        aReOpenIdle("ScAcceptChgDlg ReOpenIdle"),
        m_xPopup(get_menu("calcmenu")),
        pViewData       ( ptrViewData ),
        pDoc            ( ptrViewData->GetDocument() ),
        aStrInsertCols       (ScResId(STR_CHG_INSERT_COLS)),
        aStrInsertRows       (ScResId(STR_CHG_INSERT_ROWS)),
        aStrInsertTabs       (ScResId(STR_CHG_INSERT_TABS)),
        aStrDeleteCols       (ScResId(STR_CHG_DELETE_COLS)),
        aStrDeleteRows       (ScResId(STR_CHG_DELETE_ROWS)),
        aStrDeleteTabs       (ScResId(STR_CHG_DELETE_TABS)),
        aStrMove             (ScResId(STR_CHG_MOVE)),
        aStrContent          (ScResId(STR_CHG_CONTENT)),
        aStrReject           (ScResId(STR_CHG_REJECT)),
        aStrAllAccepted      (ScResId(STR_CHG_ACCEPTED)),
        aStrAllRejected      (ScResId(STR_CHG_REJECTED)),
        aStrNoEntry          (ScResId(STR_CHG_NO_ENTRY)),
        aStrContentWithChild (ScResId(STR_CHG_CONTENT_WITH_CHILD)),
        aStrChildContent     (ScResId(STR_CHG_CHILD_CONTENT)),
        aStrChildOrgContent  (ScResId(STR_CHG_CHILD_ORGCONTENT)),
        aStrEmpty            (ScResId(STR_CHG_EMPTY)),
        aUnknown("Unknown"),
        bIgnoreMsg(false),
        bNoSelection(false),
        bHasFilterEntry(false),
        bUseColor(false)
{
    m_pAcceptChgCtr = VclPtr<SvxAcceptChgCtr>::Create(get_content_area(), this);
    nAcceptCount=0;
    nRejectCount=0;
    aReOpenIdle.SetInvokeHandler(LINK( this, ScAcceptChgDlg, ReOpenTimerHdl ));

    pTPFilter=m_pAcceptChgCtr->GetFilterPage();
    pTPView=m_pAcceptChgCtr->GetViewPage();
    pTheView=pTPView->GetTableControl();
    aSelectionIdle.SetInvokeHandler(LINK( this, ScAcceptChgDlg, UpdateSelectionHdl ));
    aSelectionIdle.SetDebugName( "ScAcceptChgDlg  aSelectionIdle" );

    pTPFilter->SetReadyHdl(LINK( this, ScAcceptChgDlg, FilterHandle ));
    pTPFilter->SetRefHdl(LINK( this, ScAcceptChgDlg, RefHandle ));
    pTPFilter->HideRange(false);
    pTPView->InsertCalcHeader();
    pTPView->SetRejectClickHdl( LINK( this, ScAcceptChgDlg,RejectHandle));
    pTPView->SetAcceptClickHdl( LINK(this, ScAcceptChgDlg, AcceptHandle));
    pTPView->SetRejectAllClickHdl( LINK( this, ScAcceptChgDlg,RejectAllHandle));
    pTPView->SetAcceptAllClickHdl( LINK(this, ScAcceptChgDlg, AcceptAllHandle));
    pTheView->SetCalcView();
    pTheView->SetStyle(pTheView->GetStyle()|WB_HASLINES|WB_CLIPCHILDREN|WB_HASBUTTONS|WB_HASBUTTONSATROOT|WB_HSCROLL);
    pTheView->SetExpandingHdl( LINK(this, ScAcceptChgDlg, ExpandingHandle));
    pTheView->SetSelectHdl( LINK(this, ScAcceptChgDlg, SelectHandle));
    pTheView->SetDeselectHdl( LINK(this, ScAcceptChgDlg, SelectHandle));
    pTheView->SetCommandHdl( LINK(this, ScAcceptChgDlg, CommandHdl));
    pTheView->SetColCompareHdl( LINK(this, ScAcceptChgDlg,ColCompareHdl));
    pTheView->SetSelectionMode(SelectionMode::Multiple);
    pTheView->SetHighlightRange(1);

    Init();

    UpdateView();
    SvTreeListEntry* pEntry=pTheView->First();
    if(pEntry!=nullptr)
    {
        pTheView->Select(pEntry);
    }
}

ScAcceptChgDlg::~ScAcceptChgDlg()
{
    disposeOnce();
}

void ScAcceptChgDlg::dispose()
{
    ClearView();
    ScChangeTrack* pChanges=pDoc->GetChangeTrack();

    if(pChanges!=nullptr)
    {
        Link<ScChangeTrack&,void> aLink;
        pChanges->SetModifiedLink(aLink);
    }

    m_xPopup.clear();
    m_pAcceptChgCtr.disposeAndClear();
    pTPFilter.clear();
    pTPView.clear();
    pTheView.clear();
    SfxModelessDialog::dispose();
}

void ScAcceptChgDlg::ReInit(ScViewData* ptrViewData)
{
    pViewData=ptrViewData;
    if(pViewData!=nullptr)
        pDoc=ptrViewData->GetDocument();
    else
        pDoc=nullptr;

    bNoSelection=false;
    bIgnoreMsg=false;
    nAcceptCount=0;
    nRejectCount=0;

    //  don't call Init here (switching between views), just set link below
    //  (dialog is just hidden, not deleted anymore, when switching views)
    ClearView();
    UpdateView();

    if ( pDoc )
    {
        ScChangeTrack* pChanges = pDoc->GetChangeTrack();
        if ( pChanges )
            pChanges->SetModifiedLink( LINK( this, ScAcceptChgDlg, ChgTrackModHdl ) );
    }
}

void ScAcceptChgDlg::Init()
{
    OSL_ENSURE( pViewData && pDoc, "ViewData or Document not found!" );

    ScChangeTrack* pChanges=pDoc->GetChangeTrack();

    if(pChanges!=nullptr)
    {
        pChanges->SetModifiedLink( LINK( this, ScAcceptChgDlg,ChgTrackModHdl));
        aChangeViewSet.SetTheAuthorToShow(pChanges->GetUser());
        pTPFilter->ClearAuthors();
        const std::set<OUString>& rUserColl = pChanges->GetUserCollection();
        std::set<OUString>::const_iterator it = rUserColl.begin(), itEnd = rUserColl.end();
        for (; it != itEnd; ++it)
            pTPFilter->InsertAuthor(*it);
    }

    ScChangeViewSettings* pViewSettings=pDoc->GetChangeViewSettings();
    if ( pViewSettings!=nullptr )
        aChangeViewSet = *pViewSettings;
    // adjust TimeField for filter tabpage
    aChangeViewSet.AdjustDateMode( *pDoc );

    pTPFilter->CheckDate(aChangeViewSet.HasDate());
    pTPFilter->SetFirstDate(aChangeViewSet.GetTheFirstDateTime());
    pTPFilter->SetFirstTime(aChangeViewSet.GetTheFirstDateTime());
    pTPFilter->SetLastDate(aChangeViewSet.GetTheLastDateTime());
    pTPFilter->SetLastTime(aChangeViewSet.GetTheLastDateTime());
    pTPFilter->SetDateMode(static_cast<sal_uInt16>(aChangeViewSet.GetTheDateMode()));
    pTPFilter->CheckComment(aChangeViewSet.HasComment());
    pTPFilter->SetComment(aChangeViewSet.GetTheComment());

    pTPFilter->CheckAuthor(aChangeViewSet.HasAuthor());
    OUString aString=aChangeViewSet.GetTheAuthorToShow();
    if(!aString.isEmpty())
    {
        pTPFilter->SelectAuthor(aString);
        if(pTPFilter->GetSelectedAuthor()!=aString)
        {
            pTPFilter->InsertAuthor(aString);
            pTPFilter->SelectAuthor(aString);
        }
    }
    else
        pTPFilter->SelectedAuthorPos(0);

    pTPFilter->CheckRange(aChangeViewSet.HasRange());

    aRangeList=aChangeViewSet.GetTheRangeList();

    if( !aChangeViewSet.GetTheRangeList().empty() )
    {
        const ScRange & rRangeEntry = aChangeViewSet.GetTheRangeList().front();
        OUString aRefStr(rRangeEntry.Format(ScRefFlags::RANGE_ABS_3D, pDoc));
        pTPFilter->SetRange(aRefStr);
    }

    // init filter
    if(pTPFilter->IsDate()||pTPFilter->IsRange()||
        pTPFilter->IsAuthor()||pTPFilter->IsComment())
    {
        pTheView->SetFilterDate(pTPFilter->IsDate());
        pTheView->SetDateTimeMode(pTPFilter->GetDateMode());
        pTheView->SetFirstDate(pTPFilter->GetFirstDate());
        pTheView->SetLastDate(pTPFilter->GetLastDate());
        pTheView->SetFirstTime(pTPFilter->GetFirstTime());
        pTheView->SetLastTime(pTPFilter->GetLastTime());
        pTheView->SetFilterAuthor(pTPFilter->IsAuthor());
        pTheView->SetAuthor(pTPFilter->GetSelectedAuthor());

        pTheView->SetFilterComment(pTPFilter->IsComment());

        utl::SearchParam aSearchParam( pTPFilter->GetComment(),
                utl::SearchParam::SearchType::Regexp,false );

        pTheView->SetCommentParams(&aSearchParam);

        pTheView->UpdateFilterTest();
    }
}

void ScAcceptChgDlg::ClearView()
{
    nAcceptCount=0;
    nRejectCount=0;
    pTheView->SetUpdateMode(false);

    pTheView->Clear();
    pTheView->SetUpdateMode(true);
}

OUString* ScAcceptChgDlg::MakeTypeString(ScChangeActionType eType)
{
    OUString* pStr;

    switch(eType)
    {

        case SC_CAT_INSERT_COLS:    pStr=&aStrInsertCols;break;
        case SC_CAT_INSERT_ROWS:    pStr=&aStrInsertRows;break;
        case SC_CAT_INSERT_TABS:    pStr=&aStrInsertTabs;break;
        case SC_CAT_DELETE_COLS:    pStr=&aStrDeleteCols;break;
        case SC_CAT_DELETE_ROWS:    pStr=&aStrDeleteRows;break;
        case SC_CAT_DELETE_TABS:    pStr=&aStrDeleteTabs;break;
        case SC_CAT_MOVE:           pStr=&aStrMove;break;
        case SC_CAT_CONTENT:        pStr=&aStrContent;break;
        case SC_CAT_REJECT:         pStr=&aStrReject;break;
        default:                    pStr=&aUnknown;break;
    }
    return pStr;
}

bool ScAcceptChgDlg::IsValidAction(const ScChangeAction* pScChangeAction)
{
    if(pScChangeAction==nullptr) return false;

    bool bFlag = false;

    ScRange aRef=pScChangeAction->GetBigRange().MakeRange();
    OUString aUser=pScChangeAction->GetUser();
    DateTime aDateTime=pScChangeAction->GetDateTime();

    ScChangeActionType eType=pScChangeAction->GetType();
    OUString aDesc;

    OUString aComment = pScChangeAction->GetComment().replaceAll("\n", "");

    if(eType==SC_CAT_CONTENT)
    {
        if(!pScChangeAction->IsDialogParent())
            pScChangeAction->GetDescription(aDesc, pDoc, true);
    }
    else
        pScChangeAction->GetDescription(aDesc, pDoc, !pScChangeAction->IsMasterDelete());

    if (!aDesc.isEmpty())
    {
        aComment += " (" + aDesc + ")";
    }

    if (pTheView->IsValidEntry(aUser, aDateTime, aComment))
    {
        if(pTPFilter->IsRange())
        {
            for ( size_t i = 0, nRanges = aRangeList.size(); i < nRanges; ++i )
            {
                ScRange const & rRangeEntry = aRangeList[ i ];
                if (rRangeEntry.Intersects(aRef)) {
                    bFlag = true;
                    break;
                }
            }
        }
        else
            bFlag=true;
    }

    return bFlag;
}

SvTreeListEntry* ScAcceptChgDlg::AppendChangeAction(
    const ScChangeAction* pScChangeAction,
    SvTreeListEntry* pParent, bool bDelMaster,bool bDisabled)
{
    ScChangeTrack* pChanges=pDoc->GetChangeTrack();

    if(pScChangeAction==nullptr || pChanges==nullptr) return nullptr;

    SvTreeListEntry* pEntry=nullptr;

    bool bFlag = false;

    ScRange aRef=pScChangeAction->GetBigRange().MakeRange();
    OUString aUser=pScChangeAction->GetUser();
    DateTime aDateTime=pScChangeAction->GetDateTime();

    OUString aRefStr;
    ScChangeActionType eType=pScChangeAction->GetType();
    OUStringBuffer aBuf;
    OUString aDesc;

    ScRedlinData* pNewData=new ScRedlinData;
    pNewData->pData=const_cast<ScChangeAction *>(pScChangeAction);
    pNewData->nActionNo=pScChangeAction->GetActionNumber();
    pNewData->bIsAcceptable=pScChangeAction->IsClickable();
    pNewData->bIsRejectable=pScChangeAction->IsRejectable();
    pNewData->bDisabled=!pNewData->bIsAcceptable || bDisabled;
    pNewData->aDateTime=aDateTime;
    pNewData->nRow  = aRef.aStart.Row();
    pNewData->nCol  = aRef.aStart.Col();
    pNewData->nTable= aRef.aStart.Tab();

    if(eType==SC_CAT_CONTENT)
    {
        if(pScChangeAction->IsDialogParent())
        {
            aBuf.append(aStrContentWithChild);
            pNewData->nInfo=RD_SPECIAL_VISCONTENT;
            pNewData->bIsRejectable=false;
            pNewData->bIsAcceptable=false;
        }
        else
        {
            aBuf.append(*MakeTypeString(eType));
            pScChangeAction->GetDescription( aDesc, pDoc, true);
        }
    }
    else
    {
        aBuf.append(aStrContentWithChild);

        if(bDelMaster)
        {
            pScChangeAction->GetDescription( aDesc, pDoc,true);
            pNewData->bDisabled=true;
            pNewData->bIsRejectable=false;
        }
        else
            pScChangeAction->GetDescription( aDesc, pDoc,!pScChangeAction->IsMasterDelete());

    }

    pScChangeAction->GetRefString(aRefStr, pDoc, true);

    aBuf.append('\t');
    aBuf.append(aRefStr);
    aBuf.append('\t');

    bool bIsGenerated = false;

    if(!pChanges->IsGenerated(pScChangeAction->GetActionNumber()))
    {
        aBuf.append(aUser);
        aBuf.append('\t');
        aBuf.append(ScGlobal::pLocaleData->getDate(aDateTime));
        aBuf.append(' ');
        aBuf.append(ScGlobal::pLocaleData->getTime(aDateTime));
        aBuf.append('\t');

        bIsGenerated = false;
    }
    else
    {
        aBuf.append('\t');
        aBuf.append('\t');
        bIsGenerated = true;
    }

    OUString aComment = pScChangeAction->GetComment().replaceAll("\n", "");

    if (!aDesc.isEmpty())
    {
        aComment +=  " (" + aDesc + ")";
    }

    aBuf.append(aComment);

    if (pTheView->IsValidEntry(aUser, aDateTime) || bIsGenerated)
    {
        if (pTheView->IsValidComment(aComment))
        {
            if(pTPFilter->IsRange())
            {
                for ( size_t i = 0, nRanges = aRangeList.size(); i < nRanges; ++i )
                {
                    ScRange const & rRangeEntry = aRangeList[ i ];
                    if( rRangeEntry.Intersects(aRef) )
                    {
                        bHasFilterEntry=true;
                        bFlag=true;
                        break;
                    }
                }
            }
            else if(!bIsGenerated)
            {
                bHasFilterEntry=true;
                bFlag=true;
            }
        }
    }

    if(!bFlag&& bUseColor&& pParent==nullptr)
    {
        pEntry = pTheView->InsertEntry(
            aBuf.makeStringAndClear() ,pNewData, COL_LIGHTBLUE, pParent, TREELIST_APPEND);
    }
    else if(bFlag&& bUseColor&& pParent!=nullptr)
    {
        pEntry = pTheView->InsertEntry(
            aBuf.makeStringAndClear(), pNewData, COL_GREEN, pParent, TREELIST_APPEND);
        SvTreeListEntry* pExpEntry=pParent;

        while(pExpEntry!=nullptr && !pTheView->IsExpanded(pExpEntry))
        {
            SvTreeListEntry* pTmpEntry=pTheView->GetParent(pExpEntry);

            if(pTmpEntry!=nullptr) pTheView->Expand(pExpEntry);

            pExpEntry=pTmpEntry;
        }
    }
    else
    {
        pEntry = pTheView->InsertEntry(
            aBuf.makeStringAndClear(), pNewData, pParent, TREELIST_APPEND);
    }
    return pEntry;
}

SvTreeListEntry* ScAcceptChgDlg::AppendFilteredAction(
    const ScChangeAction* pScChangeAction, ScChangeActionState eState,
    SvTreeListEntry* pParent, bool bDelMaster, bool bDisabled)
{

    ScChangeTrack* pChanges=pDoc->GetChangeTrack();

    if(pScChangeAction==nullptr || pChanges==nullptr) return nullptr;

    bool bIsGenerated = pChanges->IsGenerated(pScChangeAction->GetActionNumber());

    SvTreeListEntry* pEntry=nullptr;

    bool bFlag = false;

    ScRange aRef=pScChangeAction->GetBigRange().MakeRange();
    OUString aUser=pScChangeAction->GetUser();
    DateTime aDateTime=pScChangeAction->GetDateTime();

    if (pTheView->IsValidEntry(aUser, aDateTime) || bIsGenerated)
    {
        if(pTPFilter->IsRange())
        {
            for ( size_t i = 0, nRanges = aRangeList.size(); i < nRanges; ++i )
            {
                ScRange const & rRangeEntry=aRangeList[ i ];
                if( rRangeEntry.Intersects(aRef) )
                {
                    if( pScChangeAction->GetState()==eState )
                        bFlag = true;
                    break;
                }
            }
        }
        else if(pScChangeAction->GetState()==eState && !bIsGenerated)
            bFlag = true;
    }

    if(bFlag)
    {

        OUString aRefStr;
        ScChangeActionType eType=pScChangeAction->GetType();
        OUString aString;
        OUString aDesc;

        ScRedlinData* pNewData=new ScRedlinData;
        pNewData->pData=const_cast<ScChangeAction *>(pScChangeAction);
        pNewData->nActionNo=pScChangeAction->GetActionNumber();
        pNewData->bIsAcceptable=pScChangeAction->IsClickable();
        pNewData->bIsRejectable=pScChangeAction->IsRejectable();
        pNewData->bDisabled=!pNewData->bIsAcceptable || bDisabled;
        pNewData->aDateTime=aDateTime;
        pNewData->nRow  = aRef.aStart.Row();
        pNewData->nCol  = aRef.aStart.Col();
        pNewData->nTable= aRef.aStart.Tab();

        if(eType==SC_CAT_CONTENT)
        {
            if(pScChangeAction->IsDialogParent())
            {
                aString=aStrContentWithChild;
                pNewData->nInfo=RD_SPECIAL_VISCONTENT;
                pNewData->bIsRejectable=false;
                pNewData->bIsAcceptable=false;
            }
            else
            {
                aString=*MakeTypeString(eType);
                pScChangeAction->GetDescription( aDesc, pDoc, true);
            }
        }
        else
        {
            aString=*MakeTypeString(eType);

            if(bDelMaster)
            {
                pScChangeAction->GetDescription( aDesc, pDoc,true);
                pNewData->bDisabled=true;
                pNewData->bIsRejectable=false;
            }
            else
                pScChangeAction->GetDescription( aDesc, pDoc,!pScChangeAction->IsMasterDelete());

        }

        aString += "\t";
        pScChangeAction->GetRefString(aRefStr, pDoc, true);
        aString += aRefStr + "\t";

        if(!bIsGenerated)
        {
            aString += aUser
                    + "\t"
                    + ScGlobal::pLocaleData->getDate(aDateTime)
                    + " "
                    + ScGlobal::pLocaleData->getTime(aDateTime)
                    + "\t";
        }
        else
        {
            aString += "\t";
            aString += "\t";
        }

        OUString aComment = pScChangeAction->GetComment().replaceAll("\n", "");

        if (!aDesc.isEmpty())
        {
            aComment += " (" + aDesc + ")";
        }
        if (pTheView->IsValidComment(aComment))
        {
            aString+=aComment;
            pEntry=pTheView->InsertEntry(aString,pNewData,pParent,TREELIST_APPEND);
        }
        else
            delete pNewData;
    }
    return pEntry;
}

SvTreeListEntry* ScAcceptChgDlg::InsertChangeActionContent(const ScChangeActionContent* pScChangeAction,
                                                          SvTreeListEntry* pParent, sal_uLong nSpecial)
{
    ScChangeTrack* pChanges=pDoc->GetChangeTrack();
    SvTreeListEntry* pEntry=nullptr;

    if(pScChangeAction==nullptr || pChanges==nullptr) return nullptr;

    bool bIsGenerated = pChanges->IsGenerated(pScChangeAction->GetActionNumber());

    bool bFlag = false;

    ScRange aRef=pScChangeAction->GetBigRange().MakeRange();
    OUString aUser=pScChangeAction->GetUser();
    DateTime aDateTime=pScChangeAction->GetDateTime();

    if (pTheView->IsValidEntry(aUser, aDateTime) || bIsGenerated)
    {
        if(pTPFilter->IsRange())
        {
            for ( size_t i = 0, nRanges = aRangeList.size(); i < nRanges; ++i )
            {
                ScRange const & rRangeEntry = aRangeList[ i ];
                if( rRangeEntry.Intersects(aRef) )
                {
                    bFlag=true;
                    break;
                }
            }
        }
        else if(!bIsGenerated)
            bFlag=true;
    }

    OUString aRefStr;
    OUString aString;
    OUString a2String;
    OUString aDesc;

    if(nSpecial==RD_SPECIAL_CONTENT)
    {
        OUString aTmp;
        pScChangeAction->GetOldString(aTmp, pDoc);
        a2String = aTmp;
        if(a2String.isEmpty()) a2String=aStrEmpty;

        //aString+="\'";
        aString+=a2String;
        //aString+="\'";

        aDesc = aStrChildOrgContent + ": ";
    }
    else
    {
        OUString aTmp;
        pScChangeAction->GetNewString(aTmp, pDoc);
        a2String = aTmp;
        if(a2String.isEmpty())
        {
            a2String = aStrEmpty;
            aString += a2String;
        }
        else
        {
            aString += "\'" + a2String + "\'";
            a2String = aString;
        }
        aDesc = aStrChildContent;

    }

    aDesc += a2String;
    aString += "\t";
    pScChangeAction->GetRefString(aRefStr, pDoc, true);
    aString += aRefStr + "\t";

    if(!bIsGenerated)
    {
        aString += aUser + "\t"
                +  ScGlobal::pLocaleData->getDate(aDateTime) + " "
                +  ScGlobal::pLocaleData->getTime(aDateTime) + "\t";
    }
    else
    {
        aString += "\t\t";
    }

    OUString aComment = pScChangeAction->GetComment().replaceAll("\n", "");

    if(!aDesc.isEmpty())
    {
        aComment += " (" + aDesc + ")";
    }

    aString += aComment;

    ScRedlinData* pNewData=new ScRedlinData;
    pNewData->nInfo=nSpecial;
    pNewData->pData=const_cast<ScChangeActionContent *>(pScChangeAction);
    pNewData->nActionNo=pScChangeAction->GetActionNumber();
    pNewData->bIsAcceptable=pScChangeAction->IsClickable();
    pNewData->bIsRejectable=false;
    pNewData->bDisabled=!pNewData->bIsAcceptable;
    pNewData->aDateTime=aDateTime;
    pNewData->nRow  = aRef.aStart.Row();
    pNewData->nCol  = aRef.aStart.Col();
    pNewData->nTable= aRef.aStart.Tab();

    if (pTheView->IsValidComment(aComment) && bFlag)
    {
        bHasFilterEntry=true;
        pEntry=pTheView->InsertEntry(aString,pNewData,pParent);
    }
    else
        pEntry=pTheView->InsertEntry(aString,pNewData,COL_LIGHTBLUE,pParent);
    return pEntry;
}

void ScAcceptChgDlg::UpdateView()
{
    SvTreeListEntry* pParent=nullptr;
    ScChangeTrack* pChanges=nullptr;
    const ScChangeAction* pScChangeAction=nullptr;
    SetPointer(Pointer(PointerStyle::Wait));
    pTheView->SetUpdateMode(false);
    bool bFilterFlag = pTPFilter->IsDate() || pTPFilter->IsRange() ||
        pTPFilter->IsAuthor() || pTPFilter->IsComment();

    bUseColor = bFilterFlag;

    if(pDoc!=nullptr)
    {
        pChanges=pDoc->GetChangeTrack();
        if(pChanges!=nullptr)
            pScChangeAction=pChanges->GetFirst();
    }
    bool bTheFlag = false;

    while(pScChangeAction!=nullptr)
    {
        bHasFilterEntry=false;
        switch(pScChangeAction->GetState())
        {
            case SC_CAS_VIRGIN:

                if(pScChangeAction->IsDialogRoot())
                {
                    if(pScChangeAction->IsDialogParent())
                        pParent=AppendChangeAction(pScChangeAction);
                    else
                        pParent=AppendFilteredAction(pScChangeAction,SC_CAS_VIRGIN);
                }
                else
                    pParent=nullptr;

                bTheFlag=true;
                break;

            case SC_CAS_ACCEPTED:
                pParent=nullptr;
                nAcceptCount++;
                break;

            case SC_CAS_REJECTED:
                pParent=nullptr;
                nRejectCount++;
                break;
        }

        if(pParent!=nullptr && pScChangeAction->IsDialogParent())
        {
            if(!bFilterFlag)
                pParent->EnableChildrenOnDemand();
            else
            {
                bool bTestFlag = bHasFilterEntry;
                bHasFilterEntry=false;
                if(Expand(pChanges,pScChangeAction,pParent,!bTestFlag)&&!bTestFlag)
                    pTheView->RemoveEntry(pParent);
            }
        }

        pScChangeAction=pScChangeAction->GetNext();
    }

    if( bTheFlag && (!pDoc->IsDocEditable() || pChanges->IsProtected()) )
        bTheFlag=false;

    pTPView->EnableAccept(bTheFlag);
    pTPView->EnableAcceptAll(bTheFlag);
    pTPView->EnableReject(bTheFlag);
    pTPView->EnableRejectAll(bTheFlag);

    if(nAcceptCount>0)
    {
        pParent=pTheView->InsertEntry(
            aStrAllAccepted, static_cast< RedlinData * >(nullptr),
            static_cast< SvTreeListEntry * >(nullptr));
        pParent->EnableChildrenOnDemand();
    }
    if(nRejectCount>0)
    {
        pParent=pTheView->InsertEntry(
            aStrAllRejected, static_cast< RedlinData * >(nullptr),
            static_cast< SvTreeListEntry * >(nullptr));
        pParent->EnableChildrenOnDemand();
    }
    pTheView->SetUpdateMode(true);
    SetPointer(Pointer(PointerStyle::Arrow));
    SvTreeListEntry* pEntry=pTheView->First();
    if(pEntry!=nullptr)
        pTheView->Select(pEntry);
}

IMPL_LINK_NOARG(ScAcceptChgDlg, RefHandle, SvxTPFilter*, void)
{
    sal_uInt16 nId  =ScSimpleRefDlgWrapper::GetChildWindowId();

    ScSimpleRefDlgWrapper::SetDefaultPosSize(GetPosPixel(),GetSizePixel());

    SC_MOD()->SetRefDialog( nId, true );

    SfxViewFrame* pViewFrm = pViewData->GetViewShell()->GetViewFrame();
    ScSimpleRefDlgWrapper* pWnd = static_cast<ScSimpleRefDlgWrapper*>(pViewFrm->GetChildWindow( nId ));

    if(pWnd!=nullptr)
    {
        sal_uInt16 nAcceptId=ScAcceptChgDlgWrapper::GetChildWindowId();
        pViewFrm->ShowChildWindow(nAcceptId,false);
        pWnd->SetCloseHdl(LINK( this, ScAcceptChgDlg,RefInfoHandle));
        pWnd->SetRefString(pTPFilter->GetRange());
        ScSimpleRefDlgWrapper::SetAutoReOpen(false);
        vcl::Window* pWin=pWnd->GetWindow();
        pWin->SetPosSizePixel(GetPosPixel(),GetSizePixel());
        Hide();
        pWin->SetText(GetText());
        pWnd->StartRefInput();
    }
}

IMPL_LINK( ScAcceptChgDlg, RefInfoHandle, const OUString*, pResult, void)
{
    sal_uInt16 nId;

    ScSimpleRefDlgWrapper::SetAutoReOpen(true);

    SfxViewFrame* pViewFrm = pViewData->GetViewShell()->GetViewFrame();
    if(pResult!=nullptr)
    {
        pTPFilter->SetRange(*pResult);
        FilterHandle(pTPFilter);

        nId = ScSimpleRefDlgWrapper::GetChildWindowId();
        ScSimpleRefDlgWrapper* pWnd = static_cast<ScSimpleRefDlgWrapper*>(pViewFrm->GetChildWindow( nId ));

        if(pWnd!=nullptr)
        {
            vcl::Window* pWin=pWnd->GetWindow();
            Size aWinSize=pWin->GetSizePixel();
            aWinSize.setWidth(GetSizePixel().Width() );
            SetPosSizePixel(pWin->GetPosPixel(),aWinSize);
            Invalidate();
        }
        nId = ScAcceptChgDlgWrapper::GetChildWindowId();
        pViewFrm->ShowChildWindow( nId );
    }
    else
    {
        nId = ScAcceptChgDlgWrapper::GetChildWindowId();
        pViewFrm->SetChildWindow( nId, false );
    }
}

IMPL_LINK( ScAcceptChgDlg, FilterHandle, SvxTPFilter*, pRef, void )
{
    if(pRef!=nullptr)
    {
        ClearView();
        aRangeList.RemoveAll();
        aRangeList.Parse(pTPFilter->GetRange(),pDoc);
        UpdateView();
    }
}

IMPL_LINK( ScAcceptChgDlg, RejectHandle, SvxTPView*, pRef, void )
{
    SetPointer(Pointer(PointerStyle::Wait));

    bIgnoreMsg=true;
    ScChangeTrack* pChanges=pDoc->GetChangeTrack();

    if(pRef!=nullptr)
    {
        SvTreeListEntry* pEntry=pTheView->FirstSelected();
        while(pEntry!=nullptr)
        {
            ScRedlinData *pEntryData=static_cast<ScRedlinData *>(pEntry->GetUserData());
            if(pEntryData!=nullptr)
            {
                ScChangeAction* pScChangeAction=
                        static_cast<ScChangeAction*>(pEntryData->pData);

                if(pScChangeAction->GetType()==SC_CAT_INSERT_TABS)
                    pViewData->SetTabNo(0);

                pChanges->Reject(pScChangeAction);
            }
            pEntry = pTheView->NextSelected(pEntry);
        }
        ScDocShell* pDocSh=pViewData->GetDocShell();
        pDocSh->PostPaintExtras();
        pDocSh->PostPaintGridAll();
        pDocSh->GetUndoManager()->Clear();
        pDocSh->SetDocumentModified();
        ClearView();
        UpdateView();
    }
    SetPointer(Pointer(PointerStyle::Arrow));

    bIgnoreMsg=false;
}
IMPL_LINK( ScAcceptChgDlg, AcceptHandle, SvxTPView*, pRef, void )
{
    SetPointer(Pointer(PointerStyle::Wait));

    ScChangeTrack* pChanges=pDoc->GetChangeTrack();
    bIgnoreMsg=true;
    if(pRef!=nullptr)
    {
        SvTreeListEntry* pEntry=pTheView->FirstSelected();
        while(pEntry!=nullptr)
        {
            ScRedlinData *pEntryData=static_cast<ScRedlinData *>(pEntry->GetUserData());
            if(pEntryData!=nullptr)
            {
                ScChangeAction* pScChangeAction=
                        static_cast<ScChangeAction*>(pEntryData->pData);
                if(pScChangeAction->GetType()==SC_CAT_CONTENT)
                {
                    if(pEntryData->nInfo==RD_SPECIAL_CONTENT)
                        pChanges->SelectContent(pScChangeAction,true);
                    else
                        pChanges->SelectContent(pScChangeAction);
                }
                else
                    pChanges->Accept(pScChangeAction);
            }
            pEntry = pTheView->NextSelected(pEntry);
        }
        ScDocShell* pDocSh=pViewData->GetDocShell();
        pDocSh->PostPaintExtras();
        pDocSh->PostPaintGridAll();
        pDocSh->SetDocumentModified();
        ClearView();
        UpdateView();
    }
    bIgnoreMsg=false;
}

void ScAcceptChgDlg::RejectFiltered()
{
    if(pDoc==nullptr) return;
    ScChangeTrack* pChanges=pDoc->GetChangeTrack();
    const ScChangeAction* pScChangeAction=nullptr;

    if(pChanges!=nullptr)
    {
        pScChangeAction=pChanges->GetLast();
    }

    while(pScChangeAction!=nullptr)
    {
        if(pScChangeAction->IsDialogRoot())
            if(IsValidAction(pScChangeAction))
                pChanges->Reject(const_cast<ScChangeAction*>(pScChangeAction));

        pScChangeAction=pScChangeAction->GetPrev();
    }
}
void ScAcceptChgDlg::AcceptFiltered()
{
    if(pDoc==nullptr) return;
    ScChangeTrack* pChanges=pDoc->GetChangeTrack();
    const ScChangeAction* pScChangeAction=nullptr;

    if(pChanges!=nullptr)
        pScChangeAction=pChanges->GetLast();

    while(pScChangeAction!=nullptr)
    {
        if(pScChangeAction->IsDialogRoot())
            if(IsValidAction(pScChangeAction))
                pChanges->Accept(const_cast<ScChangeAction*>(pScChangeAction));

        pScChangeAction=pScChangeAction->GetPrev();
    }
}

IMPL_LINK_NOARG(ScAcceptChgDlg, RejectAllHandle, SvxTPView*, void)
{
    SetPointer(Pointer(PointerStyle::Wait));
    bIgnoreMsg=true;
    ScChangeTrack* pChanges=pDoc->GetChangeTrack();
    if(pChanges!=nullptr)
    {
        if(pTPFilter->IsDate()||pTPFilter->IsAuthor()||pTPFilter->IsRange()||pTPFilter->IsComment())
            RejectFiltered();
        else
            pChanges->RejectAll();

        pViewData->SetTabNo(0);

        ScDocShell* pDocSh=pViewData->GetDocShell();
        pDocSh->PostPaintExtras();
        pDocSh->PostPaintGridAll();
        pDocSh->GetUndoManager()->Clear();
        pDocSh->SetDocumentModified();
        ClearView();
        UpdateView();
    }
    SetPointer(Pointer(PointerStyle::Arrow));

    bIgnoreMsg=false;
}

IMPL_LINK_NOARG(ScAcceptChgDlg, AcceptAllHandle, SvxTPView*, void)
{
    SetPointer(Pointer(PointerStyle::Wait));

    bIgnoreMsg=true;
    ScChangeTrack* pChanges=pDoc->GetChangeTrack();
    if(pChanges!=nullptr)
    {
        if(pTPFilter->IsDate()||pTPFilter->IsAuthor()||pTPFilter->IsRange()||pTPFilter->IsComment())
            AcceptFiltered();
        else
            pChanges->AcceptAll();

        ScDocShell* pDocSh=pViewData->GetDocShell();
        pDocSh->PostPaintExtras();
        pDocSh->PostPaintGridAll();
        pDocSh->SetDocumentModified();
        ClearView();
        UpdateView();
    }
    bIgnoreMsg=false;
    SetPointer(Pointer(PointerStyle::Arrow));
}

IMPL_LINK_NOARG(ScAcceptChgDlg, SelectHandle, SvTreeListBox*, void)
{
    if(!bNoSelection)
        aSelectionIdle.Start();

    bNoSelection=false;
}

void ScAcceptChgDlg::GetDependents(  const ScChangeAction* pScChangeAction,
                                    ScChangeActionMap& aActionMap,
                                    SvTreeListEntry* pEntry)
{
    ScChangeTrack* pChanges=pDoc->GetChangeTrack();

    SvTreeListEntry* pParent=pTheView->GetParent(pEntry);
    if(pParent!=nullptr)
    {
        ScRedlinData *pParentData=static_cast<ScRedlinData *>(pParent->GetUserData());
        ScChangeAction* pParentAction=static_cast<ScChangeAction*>(pParentData->pData);

        if(pParentAction!=pScChangeAction)
            pChanges->GetDependents(const_cast<ScChangeAction*>(pScChangeAction),
                        aActionMap,pScChangeAction->IsMasterDelete());
        else
            pChanges->GetDependents( const_cast<ScChangeAction*>(pScChangeAction),
                        aActionMap );
    }
    else
        pChanges->GetDependents(const_cast<ScChangeAction*>(pScChangeAction),
                    aActionMap, pScChangeAction->IsMasterDelete() );
}

bool ScAcceptChgDlg::InsertContentChildren(ScChangeActionMap* pActionMap,SvTreeListEntry* pParent)
{
    bool bTheTestFlag = true;
    ScRedlinData *pEntryData=static_cast<ScRedlinData *>(pParent->GetUserData());
    const ScChangeAction* pScChangeAction = static_cast<ScChangeAction*>(pEntryData->pData);
    bool bParentInserted = false;
    // If the parent is a MatrixOrigin then place it in the right order before
    // the MatrixReferences. Also if it is the first content change at this
    // position don't insert the first dependent MatrixReference as the special
    // content (original value) but insert the predecessor of the MatrixOrigin
    // itself instead.
    if ( pScChangeAction->GetType() == SC_CAT_CONTENT &&
            static_cast<const ScChangeActionContent*>(pScChangeAction)->IsMatrixOrigin() )
    {
        pActionMap->insert( ::std::make_pair( pScChangeAction->GetActionNumber(),
            const_cast<ScChangeAction*>( pScChangeAction ) ) );
        bParentInserted = true;
    }
    SvTreeListEntry* pEntry=nullptr;

    ScChangeActionMap::iterator itChangeAction = pActionMap->begin();
    while( itChangeAction != pActionMap->end() )
    {
        if( itChangeAction->second->GetState()==SC_CAS_VIRGIN )
            break;
        ++itChangeAction;
    }

    if( itChangeAction == pActionMap->end() )
        return true;

    SvTreeListEntry* pOriginal = InsertChangeActionContent(
        dynamic_cast<const ScChangeActionContent*>( itChangeAction->second ),
        pParent, RD_SPECIAL_CONTENT );

    if(pOriginal!=nullptr)
    {
        bTheTestFlag=false;
        ScRedlinData *pParentData=static_cast<ScRedlinData *>(pOriginal->GetUserData());
        pParentData->pData=const_cast<ScChangeAction *>(pScChangeAction);
        pParentData->nActionNo=pScChangeAction->GetActionNumber();
        pParentData->bIsAcceptable=pScChangeAction->IsRejectable(); // select old value
        pParentData->bIsRejectable=false;
        pParentData->bDisabled=false;
    }
    while( itChangeAction != pActionMap->end() )
    {
        if( itChangeAction->second->GetState() == SC_CAS_VIRGIN )
        {
            pEntry = InsertChangeActionContent( dynamic_cast<const ScChangeActionContent*>( itChangeAction->second ),
                pParent, RD_SPECIAL_NONE );

            if(pEntry!=nullptr)
                bTheTestFlag=false;
        }
        ++itChangeAction;
    }

    if ( !bParentInserted )
    {
        pEntry=InsertChangeActionContent(static_cast<const ScChangeActionContent*>(
                                pScChangeAction),pParent,RD_SPECIAL_NONE);

        if(pEntry!=nullptr)
        {
            bTheTestFlag=false;
            ScRedlinData *pParentData=static_cast<ScRedlinData *>(pEntry->GetUserData());
            pParentData->pData=const_cast<ScChangeAction *>(pScChangeAction);
            pParentData->nActionNo=pScChangeAction->GetActionNumber();
            pParentData->bIsAcceptable=pScChangeAction->IsClickable();
            pParentData->bIsRejectable=false;
            pParentData->bDisabled=false;
        }
    }

    return bTheTestFlag;

}

bool ScAcceptChgDlg::InsertAcceptedORejected(SvTreeListEntry* pParent)
{
    ScChangeTrack* pChanges=pDoc->GetChangeTrack();
    bool bTheTestFlag = true;

    ScChangeActionState eState = SC_CAS_VIRGIN;
    OUString aString = pTheView->GetEntryText(pParent);
    OUString a2String = aString.copy(0, aStrAllAccepted.getLength());
    if (a2String == aStrAllAccepted)
        eState=SC_CAS_ACCEPTED;
    else
    {
        a2String = aString.copy(0, aStrAllRejected.getLength());
        if (a2String == aStrAllRejected)
            eState=SC_CAS_REJECTED;
    }

    ScChangeAction* pScChangeAction=pChanges->GetFirst();
    while(pScChangeAction!=nullptr)
    {
        if(pScChangeAction->GetState()==eState &&
            AppendFilteredAction(pScChangeAction,eState,pParent)!=nullptr)
            bTheTestFlag=false;
        pScChangeAction=pScChangeAction->GetNext();
    }
    return bTheTestFlag;
}

bool ScAcceptChgDlg::InsertChildren(ScChangeActionMap* pActionMap,SvTreeListEntry* pParent)
{
    ScChangeTrack* pChanges=pDoc->GetChangeTrack();
    bool bTheTestFlag = true;
    ScChangeActionMap::iterator itChangeAction;

    for( itChangeAction = pActionMap->begin(); itChangeAction != pActionMap->end(); ++itChangeAction )
    {
        SvTreeListEntry* pEntry=AppendChangeAction( itChangeAction->second, pParent, false, true );

        if(pEntry!=nullptr)
        {
            bTheTestFlag=false;

            ScRedlinData *pEntryData=static_cast<ScRedlinData *>(pEntry->GetUserData());
            pEntryData->bIsRejectable=false;
            pEntryData->bIsAcceptable=false;
            pEntryData->bDisabled=true;

            if( itChangeAction->second->IsDialogParent() )
                Expand( pChanges, itChangeAction->second, pEntry );
        }
    }
    return bTheTestFlag;
}

bool ScAcceptChgDlg::InsertDeletedChildren(const ScChangeAction* pScChangeAction,
                                         ScChangeActionMap* pActionMap,SvTreeListEntry* pParent)
{
    ScChangeTrack* pChanges=pDoc->GetChangeTrack();
    bool bTheTestFlag = true;
    SvTreeListEntry* pEntry=nullptr;
    ScChangeActionMap::iterator itChangeAction;

    for( itChangeAction = pActionMap->begin(); itChangeAction != pActionMap->end(); ++itChangeAction )
    {

        if( pScChangeAction != itChangeAction->second )
            pEntry = AppendChangeAction( itChangeAction->second, pParent, false, true );
        else
            pEntry = AppendChangeAction( itChangeAction->second, pParent, true, true );

        if(pEntry!=nullptr)
        {
            ScRedlinData *pEntryData=static_cast<ScRedlinData *>(pEntry->GetUserData());
            pEntryData->bIsRejectable=false;
            pEntryData->bIsAcceptable=false;
            pEntryData->bDisabled=true;

            bTheTestFlag=false;

            if( itChangeAction->second->IsDialogParent() )
                Expand( pChanges, itChangeAction->second, pEntry );
        }
    }
    return bTheTestFlag;
}

bool ScAcceptChgDlg::Expand(
    const ScChangeTrack* pChanges, const ScChangeAction* pScChangeAction,
    SvTreeListEntry* pEntry, bool bFilter)
{
    bool bTheTestFlag = true;

    if(pChanges!=nullptr &&pEntry!=nullptr &&pScChangeAction!=nullptr)
    {
        ScChangeActionMap aActionMap;

        GetDependents( pScChangeAction, aActionMap, pEntry );

        switch(pScChangeAction->GetType())
        {
            case SC_CAT_CONTENT:
            {
                InsertContentChildren( &aActionMap, pEntry );
                bTheTestFlag=!bHasFilterEntry;
                break;
            }
            case SC_CAT_DELETE_COLS:
            case SC_CAT_DELETE_ROWS:
            case SC_CAT_DELETE_TABS:
            {
                InsertDeletedChildren( pScChangeAction, &aActionMap, pEntry );
                bTheTestFlag=!bHasFilterEntry;
                break;
            }
            default:
            {
                if(!bFilter)
                    bTheTestFlag = InsertChildren( &aActionMap, pEntry );
                break;
            }
        }
        aActionMap.clear();
    }
    return bTheTestFlag;
}

IMPL_LINK( ScAcceptChgDlg, ExpandingHandle, SvTreeListBox*, pTable, bool )
{
    ScChangeTrack* pChanges=pDoc->GetChangeTrack();
    SetPointer(Pointer(PointerStyle::Wait));
    if(pTable!=nullptr && pChanges!=nullptr)
    {
        ScChangeActionMap aActionMap;
        SvTreeListEntry* pEntry=pTheView->GetHdlEntry();
        if(pEntry!=nullptr)
        {
            ScRedlinData *pEntryData=static_cast<ScRedlinData *>(pEntry->GetUserData());

            if(pEntry->HasChildrenOnDemand())
            {
                bool bTheTestFlag = true;
                pEntry->EnableChildrenOnDemand(false);
                SvTreeListEntry* pChildEntry = pTheView->FirstChild(pEntry);
                if (pChildEntry)
                    pTheView->RemoveEntry(pChildEntry);

                if(pEntryData!=nullptr)
                {
                    ScChangeAction* pScChangeAction=static_cast<ScChangeAction*>(pEntryData->pData);

                    GetDependents( pScChangeAction, aActionMap, pEntry );

                    switch(pScChangeAction->GetType())
                    {
                        case SC_CAT_CONTENT:
                        {
                            bTheTestFlag = InsertContentChildren( &aActionMap, pEntry );
                            break;
                        }
                        case SC_CAT_DELETE_COLS:
                        case SC_CAT_DELETE_ROWS:
                        case SC_CAT_DELETE_TABS:
                        {
                            bTheTestFlag = InsertDeletedChildren( pScChangeAction, &aActionMap, pEntry );
                            break;
                        }
                        default:
                        {
                            bTheTestFlag = InsertChildren( &aActionMap, pEntry );
                            break;
                        }
                    }
                    aActionMap.clear();

                }
                else
                {
                    bTheTestFlag=InsertAcceptedORejected(pEntry);
                }
                if(bTheTestFlag) pTheView->InsertEntry(aStrNoEntry,nullptr,COL_GRAY,pEntry);
            }

        }
    }
    SetPointer(Pointer(PointerStyle::Arrow));
    return true;
}

void ScAcceptChgDlg::AppendChanges(const ScChangeTrack* pChanges,sal_uLong nStartAction,
                                   sal_uLong nEndAction)
{
    if(pChanges!=nullptr)
    {
        SvTreeListEntry* pParent=nullptr;
        SetPointer(Pointer(PointerStyle::Wait));
        pTheView->SetUpdateMode(false);

        bool bTheFlag = false;

        bool bFilterFlag = pTPFilter->IsDate() || pTPFilter->IsRange() ||
            pTPFilter->IsAuthor() || pTPFilter->IsComment();

        bUseColor = bFilterFlag;

        for(sal_uLong i=nStartAction;i<=nEndAction;i++)
        {
            const ScChangeAction* pScChangeAction=pChanges->GetAction(i);
            if(pScChangeAction==nullptr) continue;

            switch(pScChangeAction->GetState())
            {
                case SC_CAS_VIRGIN:

                    if(pScChangeAction->IsDialogRoot())
                    {
                        if(pScChangeAction->IsDialogParent())
                            pParent=AppendChangeAction(pScChangeAction);
                        else
                            pParent=AppendFilteredAction(pScChangeAction,SC_CAS_VIRGIN);
                    }
                    else
                        pParent=nullptr;

                    bTheFlag=true;
                    break;

                case SC_CAS_ACCEPTED:
                    pParent=nullptr;
                    nAcceptCount++;
                    break;

                case SC_CAS_REJECTED:
                    pParent=nullptr;
                    nRejectCount++;
                    break;
            }

            if(pParent!=nullptr && pScChangeAction->IsDialogParent())
            {
                if(!bFilterFlag)
                    pParent->EnableChildrenOnDemand();
                else
                {
                    bool bTestFlag = bHasFilterEntry;
                    bHasFilterEntry = false;
                    if(Expand(pChanges,pScChangeAction,pParent,!bTestFlag)&&!bTestFlag)
                        pTheView->RemoveEntry(pParent);
                }
            }
        }

        if( bTheFlag && (!pDoc->IsDocEditable() || pChanges->IsProtected()) )
            bTheFlag=false;

        pTPView->EnableAccept(bTheFlag);
        pTPView->EnableAcceptAll(bTheFlag);
        pTPView->EnableReject(bTheFlag);
        pTPView->EnableRejectAll(bTheFlag);

        pTheView->SetUpdateMode(true);
        SetPointer(Pointer(PointerStyle::Arrow));
    }
}

void ScAcceptChgDlg::RemoveEntrys(sal_uLong nStartAction,sal_uLong nEndAction)
{

    pTheView->SetUpdateMode(false);

    SvTreeListEntry* pEntry=pTheView->GetCurEntry();

    ScRedlinData *pEntryData=nullptr;

    if(pEntry!=nullptr)
        pEntryData=static_cast<ScRedlinData *>(pEntry->GetUserData());

    sal_uLong nAction=0;
    if(pEntryData!=nullptr)
        nAction=pEntryData->nActionNo;

    if(nAction>=nStartAction && nAction<=nEndAction)
        pTheView->SetCurEntry(pTheView->GetModel()->GetEntry(0));


    // MUST do it backwards, don't delete parents before children and GPF
    pEntry=pTheView->Last();
    while(pEntry!=nullptr)
    {
        bool bRemove = false;
        pEntryData=static_cast<ScRedlinData *>(pEntry->GetUserData());
        if(pEntryData!=nullptr)
        {
            nAction=pEntryData->nActionNo;

            if(nStartAction<=nAction && nAction<=nEndAction) bRemove=true;

        }
        SvTreeListEntry* pPrevEntry = pTheView->Prev(pEntry);

        if(bRemove)
            pTheView->RemoveEntry(pEntry);

        pEntry=pPrevEntry;
    }
    pTheView->SetUpdateMode(true);

}

void ScAcceptChgDlg::UpdateEntrys(const ScChangeTrack* pChgTrack, sal_uLong nStartAction,sal_uLong nEndAction)
{
    pTheView->SetUpdateMode(false);

    SvTreeListEntry* pEntry=pTheView->First();
    SvTreeListEntry* pLastEntry=nullptr;
    while(pEntry!=nullptr)
    {
        bool bRemove = false;
        ScRedlinData *pEntryData=static_cast<ScRedlinData *>(pEntry->GetUserData());
        if(pEntryData!=nullptr)
        {
            ScChangeAction* pScChangeAction=
                    static_cast<ScChangeAction*>(pEntryData->pData);

            sal_uLong nAction=pScChangeAction->GetActionNumber();

            if(nStartAction<=nAction && nAction<=nEndAction) bRemove=true;
        }

        SvTreeListEntry* pNextEntry;
        if(bRemove)
        {
            pTheView->RemoveEntry(pEntry);

            if(pLastEntry==nullptr) pLastEntry=pTheView->First();
            if(pLastEntry!=nullptr)
            {
                pNextEntry=pTheView->Next(pLastEntry);

                if(pNextEntry==nullptr)
                {
                    pNextEntry=pLastEntry;
                    pLastEntry=nullptr;
                }
            }
            else
                pNextEntry=nullptr;

        }
        else
        {
            pLastEntry = pEntry;
            pNextEntry = pTheView->Next(pEntry);
        }
        pEntry=pNextEntry;
    }

    AppendChanges(pChgTrack,nStartAction,nEndAction);

    pTheView->SetUpdateMode(true);
}

IMPL_LINK( ScAcceptChgDlg, ChgTrackModHdl, ScChangeTrack&, rChgTrack, void)
{
    ScChangeTrackMsgQueue::iterator iter;
    ScChangeTrackMsgQueue& aMsgQueue= rChgTrack.GetMsgQueue();

    sal_uLong   nStartAction;
    sal_uLong   nEndAction;

    for (iter = aMsgQueue.begin(); iter != aMsgQueue.end(); ++iter)
    {
        nStartAction=(*iter)->nStartAction;
        nEndAction=(*iter)->nEndAction;

        if(!bIgnoreMsg)
        {
            bNoSelection=true;

            switch((*iter)->eMsgType)
            {
                case SC_CTM_APPEND: AppendChanges(&rChgTrack,nStartAction,nEndAction);
                                    break;
                case SC_CTM_REMOVE: RemoveEntrys(nStartAction,nEndAction);
                                    break;
                case SC_CTM_PARENT:
                case SC_CTM_CHANGE: //bNeedsUpdate=true;
                                    UpdateEntrys(&rChgTrack,nStartAction,nEndAction);
                                    break;
                default:
                {
                    // added to avoid warnings
                }
            }
        }
        delete *iter;
    }

    aMsgQueue.clear();
}
IMPL_LINK_NOARG(ScAcceptChgDlg, ReOpenTimerHdl, Timer *, void)
{
    ScSimpleRefDlgWrapper::SetAutoReOpen(true);
    m_pAcceptChgCtr->ShowFilterPage();
    RefHandle(nullptr);
}

IMPL_LINK_NOARG(ScAcceptChgDlg, UpdateSelectionHdl, Timer *, void)
{
    ScTabView* pTabView = pViewData->GetView();

    bool bAcceptFlag = true;
    bool bRejectFlag = true;
    bool bContMark = false;

    pTabView->DoneBlockMode();  // clears old marking
    SvTreeListEntry* pEntry = pTheView->FirstSelected();
    while( pEntry )
    {
        ScRedlinData* pEntryData = static_cast<ScRedlinData*>(pEntry->GetUserData());
        if( pEntryData )
        {
            bRejectFlag &= pEntryData->bIsRejectable;
            bAcceptFlag &= pEntryData->bIsAcceptable;

            const ScChangeAction* pScChangeAction = static_cast<ScChangeAction*>(pEntryData->pData);
            if( pScChangeAction && (pScChangeAction->GetType() != SC_CAT_DELETE_TABS) &&
                    (!pEntryData->bDisabled || pScChangeAction->IsVisible()) )
            {
                const ScBigRange& rBigRange = pScChangeAction->GetBigRange();
                if( rBigRange.IsValid( pDoc ) && IsActive() )
                {
                    bool bSetCursor = !pTheView->NextSelected( pEntry );
                    pTabView->MarkRange( rBigRange.MakeRange(), bSetCursor, bContMark );
                    bContMark = true;
                }
            }
        }
        else
        {
            bAcceptFlag = false;
            bRejectFlag = false;
        }

        pEntry = pTheView->NextSelected( pEntry );
    }

    ScChangeTrack* pChanges = pDoc->GetChangeTrack();
    bool bEnable = pDoc->IsDocEditable() && pChanges && !pChanges->IsProtected();
    pTPView->EnableAccept( bAcceptFlag && bEnable );
    pTPView->EnableReject( bRejectFlag && bEnable );
}

IMPL_LINK_NOARG(ScAcceptChgDlg, CommandHdl, SvSimpleTable*, void)
{

    const CommandEvent aCEvt(pTheView->GetCommandEvent());

    if(aCEvt.GetCommand()==CommandEventId::ContextMenu)
    {
        m_xPopup->SetMenuFlags(MenuFlags::HideDisabledEntries);

        SvTreeListEntry* pEntry=pTheView->GetCurEntry();
        if(pEntry!=nullptr)
        {
            pTheView->Select(pEntry);
        }
        else
        {
            m_xPopup->Deactivate();
        }

        const sal_uInt16 nSubSortId = m_xPopup->GetItemId("calcsort");
        PopupMenu *pSubMenu = m_xPopup->GetPopupMenu(nSubSortId);
        const sal_uInt16 nActionId = pSubMenu->GetItemId("calcaction");

        sal_uInt16 nSortedCol = pTheView->GetSortedCol();
        if (nSortedCol != 0xFFFF)
            pSubMenu->CheckItem(nActionId + nSortedCol);

        const sal_uInt16 nEditId = m_xPopup->GetItemId("calcedit");

        m_xPopup->EnableItem(nEditId, false);

        if(pDoc->IsDocEditable() && pEntry!=nullptr)
        {
            ScRedlinData *pEntryData=static_cast<ScRedlinData *>(pEntry->GetUserData());
            if(pEntryData!=nullptr)
            {
                ScChangeAction* pScChangeAction=
                        static_cast<ScChangeAction*>(pEntryData->pData);
                if (pScChangeAction!=nullptr && !pTheView->GetParent(pEntry))
                    m_xPopup->EnableItem(nEditId);
            }
        }

        sal_uInt16 nCommand = m_xPopup->Execute(this, GetPointerPosPixel());

        if(nCommand)
        {
            if (nCommand == nEditId)
            {
                if(pEntry!=nullptr)
                {
                    ScRedlinData *pEntryData=static_cast<ScRedlinData *>(pEntry->GetUserData());
                    if(pEntryData!=nullptr)
                    {
                        ScChangeAction* pScChangeAction=
                                static_cast<ScChangeAction*>(pEntryData->pData);

                        pViewData->GetDocShell()->ExecuteChangeCommentDialog(pScChangeAction, GetFrameWeld(), false);
                    }
                }
            }
            else
            {
                bool bSortDir = pTheView->GetSortDirection();
                sal_uInt16 nDialogCol = nCommand - nActionId;
                if(nSortedCol==nDialogCol) bSortDir=!bSortDir;
                pTheView->SortByCol(nDialogCol,bSortDir);
                /*
                0, sort by action
                1, sort by position
                2, sort by author
                3, sort by date
                4, sort by comment
                */
            }
        }
    }
}

namespace
{
    //at one point we were writing multiple AcceptChgDat strings,
    //so strip all of them and keep the results of the last one
    OUString lcl_StripAcceptChgDat(OUString &rExtraString)
    {
        OUString aStr;
        while (true)
        {
            sal_Int32 nPos = rExtraString.indexOf("AcceptChgDat:");
            if (nPos == -1)
                break;
            // Try to read the alignment string "ALIGN:(...)"; if it is missing
            // we have an old version
            sal_Int32 n1 = rExtraString.indexOf('(', nPos);
            if ( n1 != -1 )
            {
                sal_Int32 n2 = rExtraString.indexOf(')', n1);
                if ( n2 != -1 )
                {
                    // cut out alignment string
                    aStr = rExtraString.copy(nPos, n2 - nPos + 1);
                    rExtraString = rExtraString.replaceAt(nPos, n2 - nPos + 1, "");
                    aStr = aStr.copy( n1-nPos+1 );
                }
            }
        }
        return aStr;
    }
}

void ScAcceptChgDlg::Initialize(SfxChildWinInfo *pInfo)
{
    OUString aStr;
    if (pInfo && !pInfo->aExtraString.isEmpty())
        aStr = lcl_StripAcceptChgDat(pInfo->aExtraString);

    SfxModelessDialog::Initialize(pInfo);

    if ( !aStr.isEmpty())
    {
        sal_uInt16 nCount=static_cast<sal_uInt16>(aStr.toInt32());

        for(sal_uInt16 i=0;i<nCount;i++)
        {
            sal_Int32 n1 = aStr.indexOf(';');
            aStr = aStr.copy( n1+1 );
            pTheView->SetTab(i, static_cast<sal_uInt16>(aStr.toInt32()), MapUnit::MapPixel);
        }
    }
}

void ScAcceptChgDlg::FillInfo(SfxChildWinInfo& rInfo) const
{
    SfxModelessDialog::FillInfo(rInfo);
    //remove any old one before adding a new one
    lcl_StripAcceptChgDat(rInfo.aExtraString);
    rInfo.aExtraString += "AcceptChgDat:(";

    sal_uInt16  nCount=pTheView->TabCount();

    rInfo.aExtraString += OUString::number(nCount);
    rInfo.aExtraString += ";";
    for(sal_uInt16 i=0;i<nCount;i++)
    {
        rInfo.aExtraString += OUString::number(pTheView->GetTab(i));
        rInfo.aExtraString += ";";
    }
    rInfo.aExtraString += ")";
}

#define CALC_DATE       3
#define CALC_POS        1

IMPL_LINK( ScAcceptChgDlg, ColCompareHdl, const SvSortData*, pSortData, sal_Int32 )
{
    sal_Int32 nCompare = 0;
    SCCOL nSortCol= static_cast<SCCOL>(pTheView->GetSortedCol());

    if(pSortData)
    {
        SvTreeListEntry* pLeft = const_cast<SvTreeListEntry*>(pSortData->pLeft );
        SvTreeListEntry* pRight = const_cast<SvTreeListEntry*>(pSortData->pRight );

        if(CALC_DATE==nSortCol)
        {
            RedlinData *pLeftData=static_cast<RedlinData *>(pLeft->GetUserData());
            RedlinData *pRightData=static_cast<RedlinData *>(pRight->GetUserData());

            if(pLeftData!=nullptr && pRightData!=nullptr)
            {
                if(pLeftData->aDateTime < pRightData->aDateTime)
                    nCompare = -1;
                else if(pLeftData->aDateTime > pRightData->aDateTime)
                    nCompare = 1;
                return nCompare;
            }
        }
        else if(CALC_POS==nSortCol)
        {
            ScRedlinData *pLeftData=static_cast<ScRedlinData *>(pLeft->GetUserData());
            ScRedlinData *pRightData=static_cast<ScRedlinData *>(pRight->GetUserData());

            if(pLeftData!=nullptr && pRightData!=nullptr)
            {
                nCompare = 1;

                if(pLeftData->nTable < pRightData->nTable)
                    nCompare = -1;
                else if(pLeftData->nTable == pRightData->nTable)
                {
                    if(pLeftData->nRow < pRightData->nRow)
                        nCompare = -1;
                    else if(pLeftData->nRow == pRightData->nRow)
                    {
                        if(pLeftData->nCol < pRightData->nCol)
                            nCompare = -1;
                        else if(pLeftData->nCol == pRightData->nCol)
                            nCompare = 0;
                    }
                }

                return nCompare;
            }
        }

        SvLBoxItem* pLeftItem = pTheView->GetEntryAtPos( pLeft, static_cast<sal_uInt16>(nSortCol));
        SvLBoxItem* pRightItem = pTheView->GetEntryAtPos( pRight, static_cast<sal_uInt16>(nSortCol));

        if(pLeftItem != nullptr && pRightItem != nullptr)
        {
            SvLBoxItemType nLeftKind = pLeftItem->GetType();
            SvLBoxItemType nRightKind = pRightItem->GetType();

            if (nRightKind == SvLBoxItemType::String &&
                 nLeftKind == SvLBoxItemType::String)
            {
                nCompare = ScGlobal::GetCaseCollator()->compareString(
                                        static_cast<SvLBoxString*>(pLeftItem)->GetText(),
                                        static_cast<SvLBoxString*>(pRightItem)->GetText());
            }
        }

    }
    return nCompare;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
