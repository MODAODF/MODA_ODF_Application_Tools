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

#include <sfx2/request.hxx>
#include <svl/eitem.hxx>
#include <svl/stritem.hxx>
#include <editeng/numitem.hxx>
#include <editeng/brushitem.hxx>
#include <numrule.hxx>

#include <cmdid.h>
#include <wrtsh.hxx>
#include <view.hxx>
#include <viewopt.hxx>
#include <wdocsh.hxx>
#include <poolfmt.hxx>
#include <textsh.hxx>
#include <swabstdlg.hxx>
#include <SwStyleNameMapper.hxx>
#include <sfx2/tabdlg.hxx>
#include <svx/nbdtmg.hxx>
#include <svx/nbdtmgfact.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/bindings.hxx>
#include <memory>

#define DOUBLEMAXLEVEL 20

enum NumList
{
    nARABIC = 1,
    nCHARS_UPPER_LETTER,
    nROMAN_UPPER,
    nFULLWIDTH_ARABIC,
    nNUMBER_LOWER_ZH,
    nNUMBER_UPPER_ZH_TW,
    nTIAN_GAN_ZH,
    nDI_ZI_ZH
};

const tools::Long ARA_fontIndent[ DOUBLEMAXLEVEL ] = {
    181, -181, 454, -272, 675, -221, 1009, -335, 1174, -164,
    1435, -261, 1435, 0, 1435, 0, 1435, 0, 1435, 0
};

const tools::Long ARA_offset[ DOUBLEMAXLEVEL ] = {
    28, -28, 79, -51, 125, -45, 181, -57, 210, -28,
    261, -51, 261, 0, 261, 0, 261, 0, 261, 0
};

const tools::Long LZ_fontIndent[ DOUBLEMAXLEVEL ] = {
    482, -482, 879, -397, 1060, -181, 1338, -278, 1571, -232,
    1905, -335, 1905, 0, 1905, 0, 1905, 0, 1905, 0
};

const tools::Long LZ_offset[ DOUBLEMAXLEVEL ] = {
    79, -79, 147, -68, 176, -28, 221, -45, 261, -40,
    318, -57, 318, 0, 318, 0, 318, 0, 318, 0
};

const tools::Long UZT_fontIndent[ DOUBLEMAXLEVEL ] = {
    482, -482, 964, -482, 1361, -397, 1542, -181, 1820, -278,
    2296, -476, 2693, -397, 2926, -232, 3255, -329, 3430, -176
};

const tools::Long UZT_offset[ DOUBLEMAXLEVEL ] = {
    79, -79, 159, -79, 227, -68, 255, -28, 306, -51,
    386, -79, 454, -68, 493, -40, 550, -57, 573, -23
};

const OUString ARABIC_MORPHEME[DOUBLEMAXLEVEL] = {
    "", ".",
    "(" ,")",
    "", ".",
    "(", ")",
    "", ".",
    "(" ,")",
    "", ".",
    "", ".",
    "", ".",
    "", ".",
};

const OUString NUMBER_LOWER_ZH_MORPHEME[DOUBLEMAXLEVEL] = {
    "", u"\x3001",
    "(" ,")",
    "", ".",
    "(", ")",
    "", ".",
    "(" ,")",
    "", ".",
    "(", ")",
    "", ".",
    "", ".",
};

const OUString NUMBER_UPPER_ZH_TW_MORPHEME[DOUBLEMAXLEVEL] = {
    "", u"\x3001",
    "" ,u"\x3001",
    "(", ")",
    "", ".",
    "(" ,")",
    "", u"\x3001",
    "(" ,")",
    "", ".",
    "(" ,")",
    "", ".",
};

const SvxNumType ARABIC_TYPE[ MAXLEVEL ] = {
    SVX_NUM_ARABIC,
    SVX_NUM_ARABIC,
    SVX_NUM_CHARS_UPPER_LETTER,
    SVX_NUM_CHARS_UPPER_LETTER,
    SVX_NUM_CHARS_LOWER_LETTER,
    SVX_NUM_CHARS_LOWER_LETTER,
    SVX_NUM_ARABIC,
    SVX_NUM_ARABIC,
    SVX_NUM_ARABIC,
    SVX_NUM_ARABIC
};

const SvxNumType NUMBER_LOWER_ZH_TYPE[ MAXLEVEL ] = {
    SVX_NUM_NUMBER_LOWER_ZH,
    SVX_NUM_NUMBER_LOWER_ZH,
    SVX_NUM_ARABIC,
    SVX_NUM_ARABIC,
    SVX_NUM_CHARS_UPPER_LETTER,
    SVX_NUM_CHARS_UPPER_LETTER,
    SVX_NUM_CHARS_LOWER_LETTER,
    SVX_NUM_CHARS_LOWER_LETTER,
    SVX_NUM_CHARS_LOWER_LETTER,
    SVX_NUM_CHARS_LOWER_LETTER
};

const SvxNumType NUMBER_UPPER_ZH_TW_TYPE[ MAXLEVEL ] = {

    SVX_NUM_NUMBER_UPPER_ZH_TW,
    SVX_NUM_NUMBER_LOWER_ZH,
    SVX_NUM_NUMBER_LOWER_ZH,
    SVX_NUM_ARABIC,
    SVX_NUM_ARABIC,
    SVX_NUM_TIAN_GAN_ZH,
    SVX_NUM_TIAN_GAN_ZH,
    SVX_NUM_CHARS_UPPER_LETTER,
    SVX_NUM_CHARS_UPPER_LETTER,
    SVX_NUM_CHARS_LOWER_LETTER
};

const SvxNumType* cType(int chooseindex)
{
    switch(chooseindex)
    {
        case NumList::nARABIC:
            return ARABIC_TYPE;
        case NumList::nNUMBER_LOWER_ZH:
            return NUMBER_LOWER_ZH_TYPE;
        case NumList::nNUMBER_UPPER_ZH_TW:
            return NUMBER_UPPER_ZH_TW_TYPE;
        default:
            return ARABIC_TYPE;
    }
}

const tools::Long* cIndent(int chooseindex)
{
    switch(chooseindex)
    {
        case NumList::nARABIC:
            return ARA_fontIndent;
        case NumList::nNUMBER_LOWER_ZH:
            return LZ_fontIndent;
        case NumList::nNUMBER_UPPER_ZH_TW:
            return UZT_fontIndent;
        default:
            return ARA_fontIndent;
    }
}

const tools::Long* cOffset(int chooseindex)
{
     switch(chooseindex)
     {
        case NumList::nARABIC:
            return ARA_offset;
        case NumList::nNUMBER_LOWER_ZH:
            return LZ_offset;
        case NumList::nNUMBER_UPPER_ZH_TW:
            return UZT_offset;
         default:
            return ARA_offset;
     }
}

const OUString* cMorpheme(int chooseindex)
{
    switch(chooseindex)
    {
        case NumList::nARABIC:
            return ARABIC_MORPHEME;
        case NumList::nNUMBER_LOWER_ZH:
            return NUMBER_LOWER_ZH_MORPHEME;
        case NumList::nNUMBER_UPPER_ZH_TW:
            return NUMBER_UPPER_ZH_TW_MORPHEME;
        default:
            return ARABIC_MORPHEME;
    }
}



void SwTextShell::ExecEnterNum(SfxRequest &rReq)
{
    //Because the record before any shell exchange.
    switch(rReq.GetSlot())
    {
    case FN_NUM_NUMBERING_ON:
    {
        GetShell().StartAllAction();
        const SfxBoolItem* pItem = rReq.GetArg<SfxBoolItem>(FN_PARAM_1);
        bool bMode = !GetShell().SelectionHasNumber(); // #i29560#
        if ( pItem )
            bMode = pItem->GetValue();
        else
            rReq.AppendItem( SfxBoolItem( FN_PARAM_1, bMode ) );

        if ( bMode != (GetShell().SelectionHasNumber()) ) // #i29560#
        {
            rReq.Done();
            if( bMode )
                GetShell().NumOn();
            else
                GetShell().NumOrBulletOff(); // #i29560#
        }
        bool bNewResult = GetShell().SelectionHasNumber();
        if (bNewResult!=bMode) {
            SfxBindings& rBindings = GetView().GetViewFrame()->GetBindings();
            SfxBoolItem aItem(FN_NUM_NUMBERING_ON,!bNewResult);
            rBindings.SetState(aItem);
            SfxBoolItem aNewItem(FN_NUM_NUMBERING_ON,bNewResult);
            rBindings.SetState(aNewItem);
        }
        GetShell().EndAllAction();
    }
    break;
    case FN_NUM_BULLET_ON:
    {
        GetShell().StartAllAction();
        const SfxBoolItem* pItem = rReq.GetArg<SfxBoolItem>(FN_PARAM_1);
        bool bMode = !GetShell().SelectionHasBullet(); // #i29560#
        if ( pItem )
            bMode = pItem->GetValue();
        else
            rReq.AppendItem( SfxBoolItem( FN_PARAM_1, bMode ) );

        if ( bMode != (GetShell().SelectionHasBullet()) ) // #i29560#
        {
            rReq.Done();
            if( bMode )
                GetShell().BulletOn();
            else
                GetShell().NumOrBulletOff(); // #i29560#
        }
        bool bNewResult = GetShell().SelectionHasBullet();
        if (bNewResult!=bMode) {
            SfxBindings& rBindings = GetView().GetViewFrame()->GetBindings();
            SfxBoolItem aItem(FN_NUM_BULLET_ON,!bNewResult);
            rBindings.SetState(aItem);
            SfxBoolItem aNewItem(FN_NUM_BULLET_ON,bNewResult);
            rBindings.SetState(aNewItem);
        }
        GetShell().EndAllAction();
    }
    break;

    case FN_NUMBER_BULLETS:
    case SID_OUTLINE_BULLET:
    {
        SfxItemSet aSet( GetPool(),
                         svl::Items<SID_HTML_MODE, SID_HTML_MODE,
                         SID_ATTR_NUMBERING_RULE, SID_PARAM_CUR_NUM_LEVEL>{} );
        SwDocShell* pDocSh = GetView().GetDocShell();
        const bool bHtml = dynamic_cast<SwWebDocShell*>( pDocSh  ) !=  nullptr;
        const SwNumRule* pNumRuleAtCurrentSelection = GetShell().GetNumRuleAtCurrentSelection();
        if ( pNumRuleAtCurrentSelection != nullptr )
        {
            SvxNumRule aRule = pNumRuleAtCurrentSelection->MakeSvxNumRule();

            //convert type of linked bitmaps from SVX_NUM_BITMAP to (SVX_NUM_BITMAP|LINK_TOKEN)
            for ( sal_uInt16 i = 0; i < aRule.GetLevelCount(); i++ )
            {
                SvxNumberFormat aFormat( aRule.GetLevel( i ) );
                if ( SVX_NUM_BITMAP == aFormat.GetNumberingType() )
                {
                    const SvxBrushItem* pBrush = aFormat.GetBrush();
                    if(pBrush && !pBrush->GetGraphicLink().isEmpty())
                        aFormat.SetNumberingType(SvxNumType(SVX_NUM_BITMAP|LINK_TOKEN));
                    aRule.SetLevel(i, aFormat, aRule.Get(i) != nullptr);
                }
            }
            if(bHtml)
                aRule.SetFeatureFlag(SvxNumRuleFlags::ENABLE_EMBEDDED_BMP, false);

            aSet.Put(SvxNumBulletItem(aRule));
            OSL_ENSURE( GetShell().GetNumLevel() < MAXLEVEL,
                    "<SwTextShell::ExecEnterNum()> - numbered node without valid list level. Serious defect." );
            sal_uInt16 nLevel = GetShell().GetNumLevel();
            if( nLevel < MAXLEVEL )
            {
                nLevel = 1 << nLevel;
                aSet.Put( SfxUInt16Item( SID_PARAM_CUR_NUM_LEVEL, nLevel ) );
            }
        }
        else
        {
            SwNumRule aRule( GetShell().GetUniqueNumRuleName(),
                             // #i89178#
                             numfunc::GetDefaultPositionAndSpaceMode() );
            SvxNumRule aSvxRule = aRule.MakeSvxNumRule();
            const bool bRightToLeft = GetShell().IsInRightToLeftText();

            if ( bHtml || bRightToLeft )
            {
                for ( sal_uInt8 n = 0; n < MAXLEVEL; ++n )
                {
                    SvxNumberFormat aFormat( aSvxRule.GetLevel( n ) );
                    if ( n && bHtml )
                    {
                        // 1/2" for HTML
                        aFormat.SetAbsLSpace(n * 720);
                    }
                    // #i38904#  Default alignment for
                    // numbering/bullet should be rtl in rtl paragraph:
                    if ( bRightToLeft )
                    {
                        aFormat.SetNumAdjust( SvxAdjust::Right );
                    }
                    aSvxRule.SetLevel( n, aFormat, false );
                }
                aSvxRule.SetFeatureFlag(SvxNumRuleFlags::ENABLE_EMBEDDED_BMP, false);
            }
            aSet.Put( SvxNumBulletItem( aSvxRule ) );
        }

        aSet.Put( SfxBoolItem( SID_PARAM_NUM_PRESET,false ));

        // Before the dialogue of the HTML mode will be dropped at the Docshell.
        pDocSh->PutItem(SfxUInt16Item(SID_HTML_MODE, ::GetHtmlMode(pDocSh)));

        SwAbstractDialogFactory* pFact = SwAbstractDialogFactory::Create();
        weld::Window *pParent = rReq.GetFrameWeld();
        VclPtr<SfxAbstractTabDialog> pDlg(pFact->CreateSvxNumBulletTabDialog(pParent, &aSet, GetShell()));
        const SfxStringItem* pPageItem = rReq.GetArg<SfxStringItem>(FN_PARAM_1);
        if ( pPageItem )
            pDlg->SetCurPageId( OUStringToOString( pPageItem->GetValue(), RTL_TEXTENCODING_UTF8 ) );

        auto pRequest = std::make_shared<SfxRequest>(rReq);
        rReq.Ignore(); // the 'old' request is not relevant any more

        pDlg->StartExecuteAsync([aSet, pDlg, pNumRuleAtCurrentSelection, pRequest, this](sal_Int32 nResult){
            if (RET_OK == nResult)
            {
                const SfxPoolItem* pItem;
                if (SfxItemState::SET == pDlg->GetOutputItemSet()->GetItemState(SID_ATTR_NUMBERING_RULE, false, &pItem))
                {
                    pRequest->AppendItem(*pItem);
                    pRequest->Done();
                    SvxNumRule* pSetRule = static_cast<const SvxNumBulletItem*>(pItem)->GetNumRule();
                    pSetRule->UnLinkGraphics();
                    SwNumRule aSetRule(pNumRuleAtCurrentSelection != nullptr
                                       ? pNumRuleAtCurrentSelection->GetName()
                                       : GetShell().GetUniqueNumRuleName(),
                        numfunc::GetDefaultPositionAndSpaceMode());
                    aSetRule.SetSvxRule(*pSetRule, GetShell().GetDoc());
                    aSetRule.SetAutoRule(true);
                    // No start of new list, if an existing list style is edited.
                    // Otherwise start a new list.
                    const bool bCreateList = (pNumRuleAtCurrentSelection == nullptr);
                    GetShell().SetCurNumRule(aSetRule, bCreateList);
                }
                // If the Dialog was leaved with OK but nothing was chosen then the
                // numbering must be at least activated, if it is not already.
                else if (pNumRuleAtCurrentSelection == nullptr
                         && SfxItemState::SET == aSet.GetItemState(SID_ATTR_NUMBERING_RULE, false, &pItem))
                {
                    pRequest->AppendItem(*pItem);
                    pRequest->Done();
                    SvxNumRule* pSetRule = static_cast<const SvxNumBulletItem*>(pItem)->GetNumRule();
                    SwNumRule aSetRule(
                        GetShell().GetUniqueNumRuleName(),
                        numfunc::GetDefaultPositionAndSpaceMode());
                    aSetRule.SetSvxRule(*pSetRule, GetShell().GetDoc());
                    aSetRule.SetAutoRule(true);
                    // start new list
                    GetShell().SetCurNumRule(aSetRule, true);
                }
            }
            else if (RET_USER == nResult)
                GetShell().DelNumRules();
            pDlg->disposeOnce();
        });
    }
    break;

    default:
        OSL_FAIL("wrong dispatcher");
        return;
    }
}


void SwTextShell::ExecSetNumber(SfxRequest const &rReq)
{
    const sal_uInt16 nSlot = rReq.GetSlot();
    switch ( nSlot )
    {
    case FN_SVX_SET_NUMBER:
    case FN_SVX_SET_BULLET:
    case FN_SVX_SET_OUTLINE:
        {
            const SfxUInt16Item* pItem = rReq.GetArg<SfxUInt16Item>(nSlot);
            if ( pItem != nullptr )
            {
                const sal_uInt16 nChosenItemIdx = pItem->GetValue();
                svx::sidebar::NBOType nNBOType = svx::sidebar::NBOType::Bullets;
                if ( nSlot == FN_SVX_SET_NUMBER )
                    nNBOType = svx::sidebar::NBOType::Numbering;
                else if ( nSlot == FN_SVX_SET_OUTLINE )
                    nNBOType = svx::sidebar::NBOType::Outline;

                svx::sidebar::NBOTypeMgrBase* pNBOTypeMgr = svx::sidebar::NBOutlineTypeMgrFact::CreateInstance( nNBOType );

                if ( pNBOTypeMgr != nullptr )
                {
                    const SwNumRule* pNumRuleAtCurrentSelection = GetShell().GetNumRuleAtCurrentSelection();
                    sal_uInt16 nActNumLvl = USHRT_MAX;
                    if ( pNumRuleAtCurrentSelection != nullptr )
                    {
                        const sal_uInt16 nLevel = GetShell().GetNumLevel();
                        if ( nLevel < MAXLEVEL )
                        {
                            nActNumLvl = 1 << nLevel;
                        }
                    }
                    SwNumRule aNewNumRule(
                        pNumRuleAtCurrentSelection != nullptr ? pNumRuleAtCurrentSelection->GetName() : GetShell().GetUniqueNumRuleName(),
                        numfunc::GetDefaultPositionAndSpaceMode() );
                    SvxNumRule aNewSvxNumRule = pNumRuleAtCurrentSelection != nullptr
                                                    ? pNumRuleAtCurrentSelection->MakeSvxNumRule()
                                                    : aNewNumRule.MakeSvxNumRule();

                    OUString aNumCharFormat, aBulletCharFormat;
                    SwStyleNameMapper::FillUIName( RES_POOLCHR_NUM_LEVEL, aNumCharFormat );
                    SwStyleNameMapper::FillUIName( RES_POOLCHR_BULLET_LEVEL, aBulletCharFormat );

                    SfxAllItemSet aSet( GetPool() );
                    aSet.Put( SfxStringItem( SID_NUM_CHAR_FMT, aNumCharFormat ) );
                    aSet.Put( SfxStringItem( SID_BULLET_CHAR_FMT, aBulletCharFormat ) );
                    aSet.Put( SvxNumBulletItem( aNewSvxNumRule, SID_ATTR_NUMBERING_RULE ) );

                    pNBOTypeMgr->SetItems( &aSet );
                    pNBOTypeMgr->ApplyNumRule( aNewSvxNumRule, nChosenItemIdx - 1, nActNumLvl );

                    // use NDC rule for NumberingPopup
                    if (nSlot == FN_SVX_SET_NUMBER)
                    {
                        SwDocShell* pDocSh = GetView().GetDocShell();
                        OutputDevice* rOutDev = pDocSh->GetDocumentRefDev();
                        vcl::Font bFont( rOutDev->GetFont() );
                        Size bSize = bFont.GetFontSize();
                        sal_uInt16 multiple = (bSize.Height() - 240)/40;

                        sal_uInt8 n;
                        for( n = 0; n < MAXLEVEL; ++n )
                        {
                            SvxNumberFormat nFmt(aNewSvxNumRule.GetLevel(n));
                            nFmt.SetLabelFollowedBy( SvxNumberFormat::NOTHING ); // Numbering use NOTHING

                            if ( nChosenItemIdx == NumList::nARABIC ||
                                nChosenItemIdx == NumList::nNUMBER_LOWER_ZH ||
                                nChosenItemIdx == NumList::nNUMBER_UPPER_ZH_TW )
                            {
                                nFmt.SetNumberingType(cType(nChosenItemIdx)[n]);
                                nFmt.SetListtabPos( cIndent(nChosenItemIdx)[n*2] + ( cOffset(nChosenItemIdx)[n*2] * multiple ) );
                                nFmt.SetIndentAt( cIndent(nChosenItemIdx)[n*2] + ( cOffset(nChosenItemIdx)[n*2] * multiple ) );
                                nFmt.SetFirstLineIndent( cIndent(nChosenItemIdx)[n*2+1] + ( cOffset(nChosenItemIdx)[n*2+1] * multiple ) );
                                nFmt.SetPrefix(cMorpheme(nChosenItemIdx)[ n*2 ]);
                                nFmt.SetSuffix(cMorpheme(nChosenItemIdx)[ n*2+1 ]);
                            }
                            aNewSvxNumRule.SetLevel(n, nFmt);
                            pNBOTypeMgr->RelplaceNumRule( aNewSvxNumRule, nChosenItemIdx - 1, nActNumLvl );
                        }
                    }

                    // BulletPopup followdby use SPACE
                    if (nSlot == FN_SVX_SET_BULLET)
                    {
                        sal_uInt8 k;
                        for( k = 0; k < MAXLEVEL; ++k )
                        {
                            SvxNumberFormat bFmt(aNewSvxNumRule.GetLevel(k));
                            bFmt.SetLabelFollowedBy( SvxNumberFormat::SPACE ); // Bullet use SPACE
                            aNewSvxNumRule.SetLevel(k, bFmt);
                            pNBOTypeMgr->RelplaceNumRule( aNewSvxNumRule, nChosenItemIdx - 1, nActNumLvl );
                         }
                    }

                    aNewNumRule.SetSvxRule( aNewSvxNumRule, GetShell().GetDoc() );
                    aNewNumRule.SetAutoRule( true );
                    const bool bCreateNewList = ( pNumRuleAtCurrentSelection == nullptr );
                    GetShell().SetCurNumRule( aNewNumRule, bCreateNewList );
                }
            }
        }
        break;

    default:
        OSL_ENSURE(false, "wrong Dispatcher");
        return;
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
