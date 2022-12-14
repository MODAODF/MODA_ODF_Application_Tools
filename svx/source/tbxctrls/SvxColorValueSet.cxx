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

#include <svx/SvxColorValueSet.hxx>
#include <svx/xtable.hxx>
#include <vcl/builderfactory.hxx>
#include <vcl/svapp.hxx>
#include <vcl/settings.hxx>
#include <osl/diagnose.h>

SvxColorValueSet::SvxColorValueSet(vcl::Window* _pParent, WinBits nWinStyle)
:   ValueSet(_pParent, nWinStyle)
{
    SetEdgeBlending(true);
}

ColorValueSet::ColorValueSet(std::unique_ptr<weld::ScrolledWindow> pWindow)
    : SvtValueSet(std::move(pWindow))
{
    SetEdgeBlending(true);
}

VCL_BUILDER_FACTORY_CONSTRUCTOR(SvxColorValueSet, WB_TABSTOP)

sal_uInt32 SvxColorValueSet::getMaxRowCount()
{
    return StyleSettings::GetColorValueSetMaximumRowCount();
}

sal_uInt32 SvxColorValueSet::getEntryEdgeLength()
{
    const StyleSettings& rStyleSettings = Application::GetSettings().GetStyleSettings();

    return rStyleSettings.GetListBoxPreviewDefaultPixelSize().Height() + 1;
}

sal_uInt32 SvxColorValueSet::getColumnCount()
{
    const StyleSettings& rStyleSettings = Application::GetSettings().GetStyleSettings();

    return rStyleSettings.GetColorValueSetColumnCount();
}

void SvxColorValueSet::addEntriesForXColorList(const XColorList& rXColorList, sal_uInt32 nStartIndex)
{
    const sal_uInt32 nColorCount(rXColorList.Count());

    for(sal_uInt32 nIndex(0); nIndex < nColorCount; nIndex++, nStartIndex++)
    {
        const XColorEntry* pEntry = rXColorList.GetColor(nIndex);

        if(pEntry)
        {
            InsertItem(nStartIndex, pEntry->GetColor(), pEntry->GetName());
        }
        else
        {
            OSL_ENSURE(false, "OOps, XColorList with empty entries (!)");
        }
    }
}

void ColorValueSet::addEntriesForXColorList(const XColorList& rXColorList, sal_uInt32 nStartIndex)
{
    const sal_uInt32 nColorCount(rXColorList.Count());

    for(sal_uInt32 nIndex(0); nIndex < nColorCount; nIndex++, nStartIndex++)
    {
        const XColorEntry* pEntry = rXColorList.GetColor(nIndex);

        if(pEntry)
        {
            InsertItem(nStartIndex, pEntry->GetColor(), pEntry->GetName());
        }
        else
        {
            OSL_ENSURE(false, "OOps, XColorList with empty entries (!)");
        }
    }
}

void ColorValueSet::addEntriesForColorSet(const std::set<Color>& rColorSet, const OUString& rNamePrefix)
{
    sal_uInt32 nStartIndex = 1;
    if(rNamePrefix.getLength() != 0)
    {
        for(std::set<Color>::const_iterator it = rColorSet.begin();
            it != rColorSet.end(); ++it, nStartIndex++)
        {
            InsertItem(nStartIndex, *it, rNamePrefix + OUString::number(nStartIndex));
        }
    }
    else
    {
        for(std::set<Color>::const_iterator it = rColorSet.begin();
            it != rColorSet.end(); ++it, nStartIndex++)
        {
            InsertItem(nStartIndex, *it, "");
        }
    }
}

void SvxColorValueSet::addEntriesForColorSet(const std::set<Color>& rColorSet, const OUString& rNamePrefix)
{
    sal_uInt32 nStartIndex = 1;
    if(rNamePrefix.getLength() != 0)
    {
        for(std::set<Color>::const_iterator it = rColorSet.begin();
            it != rColorSet.end(); ++it, nStartIndex++)
        {
            InsertItem(nStartIndex, *it, rNamePrefix + OUString::number(nStartIndex));
        }
    }
    else
    {
        for(std::set<Color>::const_iterator it = rColorSet.begin();
            it != rColorSet.end(); ++it, nStartIndex++)
        {
            InsertItem(nStartIndex, *it, "");
        }
    }
}

Size ColorValueSet::layoutAllVisible(sal_uInt32 nEntryCount)
{
    if(!nEntryCount)
    {
        nEntryCount++;
    }

    const sal_uInt32 nRowCount(ceil(double(nEntryCount)/SvxColorValueSet::getColumnCount()));
    const Size aItemSize(SvxColorValueSet::getEntryEdgeLength() - 2, SvxColorValueSet::getEntryEdgeLength() - 2);
    const WinBits aWinBits(GetStyle() & ~WB_VSCROLL);

    if (nRowCount > SvxColorValueSet::getMaxRowCount())
    {
        SetStyle(aWinBits|WB_VSCROLL);
    }
    else
    {
        SetStyle(aWinBits);
    }

    SetColCount(SvxColorValueSet::getColumnCount());
    SetLineCount(std::min(nRowCount, SvxColorValueSet::getMaxRowCount()));
    SetItemWidth(aItemSize.Width());
    SetItemHeight(aItemSize.Height());

    return CalcWindowSizePixel(aItemSize);
}

Size SvxColorValueSet::layoutAllVisible(sal_uInt32 nEntryCount)
{
    if(!nEntryCount)
    {
        nEntryCount++;
    }

    const sal_uInt32 nRowCount(ceil(double(nEntryCount)/getColumnCount()));
    const Size aItemSize(getEntryEdgeLength() - 2, getEntryEdgeLength() - 2);
    const WinBits aWinBits(GetStyle() & ~WB_VSCROLL);

    if(nRowCount > getMaxRowCount())
    {
        SetStyle(aWinBits|WB_VSCROLL);
    }
    else
    {
        SetStyle(aWinBits);
    }

    SetColCount(getColumnCount());
    SetLineCount(std::min(nRowCount, getMaxRowCount()));
    SetItemWidth(aItemSize.Width());
    SetItemHeight(aItemSize.Height());

    return CalcWindowSizePixel(aItemSize);
}

void SvxColorValueSet::Resize()
{
    layoutToGivenHeight(GetSizePixel().Height(), GetItemCount());
    ValueSet::Resize();
}

void ColorValueSet::Resize()
{
    layoutToGivenHeight(GetOutputSizePixel().Height(), GetItemCount());
    SvtValueSet::Resize();
}

Size SvxColorValueSet::layoutToGivenHeight(sal_uInt32 nHeight, sal_uInt32 nEntryCount)
{
    if(!nEntryCount)
    {
        nEntryCount++;
    }

    const Size aItemSize(getEntryEdgeLength() - 2, getEntryEdgeLength() - 2);
    const WinBits aWinBits(GetStyle() & ~WB_VSCROLL);

    // get size with all fields disabled
    const WinBits aWinBitsNoScrollNoFields(GetStyle() & ~(WB_VSCROLL|WB_NAMEFIELD|WB_NONEFIELD));
    SetStyle(aWinBitsNoScrollNoFields);
    const Size aSizeNoScrollNoFields(CalcWindowSizePixel(aItemSize, getColumnCount()));

    // get size with all needed fields
    SetStyle(aWinBits);
    Size aNewSize(CalcWindowSizePixel(aItemSize, getColumnCount()));

    const Size aItemSizePixel(CalcItemSizePixel(aItemSize));
    // calculate field height and available height for requested height
    const sal_uInt32 nFieldHeight(aNewSize.Height() - aSizeNoScrollNoFields.Height());
    const sal_uInt32 nAvailableHeight(nHeight >= nFieldHeight ? nHeight - nFieldHeight + aItemSizePixel.Height() - 1 : 0);

    // calculate how many lines can be shown there
    const sal_uInt32 nLineCount(nAvailableHeight / aItemSizePixel.Height());
    const sal_uInt32 nLineMax(ceil(double(nEntryCount)/getColumnCount()));

    if(nLineMax > nLineCount)
    {
        SetStyle(aWinBits|WB_VSCROLL);
    }

    // set height to wanted height
    aNewSize.setHeight( nHeight );

    SetItemWidth(aItemSize.Width());
    SetItemHeight(aItemSize.Height());
    SetColCount(getColumnCount());
    SetLineCount(nLineCount);

    return aNewSize;
}

Size ColorValueSet::layoutToGivenHeight(sal_uInt32 nHeight, sal_uInt32 nEntryCount)
{
    if(!nEntryCount)
    {
        nEntryCount++;
    }

    const Size aItemSize(SvxColorValueSet::getEntryEdgeLength() - 2, SvxColorValueSet::getEntryEdgeLength() - 2);
    const WinBits aWinBits(GetStyle() & ~WB_VSCROLL);

    // get size with all fields disabled
    const WinBits aWinBitsNoScrollNoFields(GetStyle() & ~(WB_VSCROLL|WB_NAMEFIELD|WB_NONEFIELD));
    SetStyle(aWinBitsNoScrollNoFields);
    const Size aSizeNoScrollNoFields(CalcWindowSizePixel(aItemSize, SvxColorValueSet::getColumnCount()));

    // get size with all needed fields
    SetStyle(aWinBits);
    Size aNewSize(CalcWindowSizePixel(aItemSize, SvxColorValueSet::getColumnCount()));

    const Size aItemSizePixel(CalcItemSizePixel(aItemSize));
    // calculate field height and available height for requested height
    const sal_uInt32 nFieldHeight(aNewSize.Height() - aSizeNoScrollNoFields.Height());
    const sal_uInt32 nAvailableHeight(nHeight >= nFieldHeight ? nHeight - nFieldHeight + aItemSizePixel.Height() - 1 : 0);

    // calculate how many lines can be shown there
    const sal_uInt32 nLineCount(nAvailableHeight / aItemSizePixel.Height());
    const sal_uInt32 nLineMax(ceil(double(nEntryCount)/SvxColorValueSet::getColumnCount()));

    if(nLineMax > nLineCount)
    {
        SetStyle(aWinBits|WB_VSCROLL);
    }

    // set height to wanted height
    aNewSize.setHeight( nHeight );

    SetItemWidth(aItemSize.Width());
    SetItemHeight(aItemSize.Height());
    SetColCount(SvxColorValueSet::getColumnCount());
    SetLineCount(nLineCount);

    return aNewSize;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
