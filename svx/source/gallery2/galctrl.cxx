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

#include <config_features.h>

#include <vcl/svapp.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/dispatch.hxx>
#include <avmedia/mediaplayer.hxx>
#include <helpids.h>
#include <galbrws2.hxx>
#include <svx/dialmgr.hxx>
#include <svx/galtheme.hxx>
#include <svx/galmisc.hxx>
#include <svx/galctrl.hxx>
#include <editeng/AccessibleStringWrap.hxx>
#include <editeng/svxfont.hxx>
#include <galobj.hxx>
#include <avmedia/mediawindow.hxx>
#include <svx/strings.hrc>
#include <vcl/graphicfilter.hxx>
#include <vcl/settings.hxx>
#include <vcl/builderfactory.hxx>
#include <bitmaps.hlst>

#define GALLERY_BRWBOX_TITLE    1

GalleryPreview::GalleryPreview(vcl::Window* pParent, WinBits nStyle, GalleryTheme* pTheme)
    : Window(pParent, nStyle)
    , DropTargetHelper(this)
    , DragSourceHelper(this)
    , mpTheme(pTheme)
{
    SetHelpId( HID_GALLERY_WINDOW );
    InitSettings();
}

Size GalleryPreview::GetOptimalSize() const
{
    return LogicToPixel(Size(70, 88), MapMode(MapUnit::MapAppFont));
}

bool GalleryPreview::SetGraphic( const INetURLObject& _aURL )
{
    bool bRet = true;
    Graphic aGraphic;
#if HAVE_FEATURE_AVMEDIA
    if( ::avmedia::MediaWindow::isMediaURL( _aURL.GetMainURL( INetURLObject::DecodeMechanism::Unambiguous ), "" ) )
    {
        aGraphic = BitmapEx(RID_SVXBMP_GALLERY_MEDIA);
    }
    else
#endif
    {
        GraphicFilter& rFilter = GraphicFilter::GetGraphicFilter();
        GalleryProgress aProgress( &rFilter );
        if( rFilter.ImportGraphic( aGraphic, _aURL ) )
            bRet = false;
    }

    SetGraphic( aGraphic );
    Invalidate();
    return bRet;
}

void GalleryPreview::InitSettings()
{
    SetBackground( Wallpaper( GALLERY_BG_COLOR ) );
    SetControlBackground( GALLERY_BG_COLOR );
    SetControlForeground( GALLERY_FG_COLOR );
}

void GalleryPreview::DataChanged( const DataChangedEvent& rDCEvt )
{
    if ( ( rDCEvt.GetType() == DataChangedEventType::SETTINGS ) && ( rDCEvt.GetFlags() & AllSettingsFlags::STYLE ) )
        InitSettings();
    else
        Window::DataChanged( rDCEvt );
}

bool GalleryPreview::ImplGetGraphicCenterRect( const Graphic& rGraphic, tools::Rectangle& rResultRect ) const
{
    const Size  aWinSize( GetOutputSizePixel() );
    Size        aNewSize( LogicToPixel( rGraphic.GetPrefSize(), rGraphic.GetPrefMapMode() ) );
    bool        bRet = false;

    if( aNewSize.Width() && aNewSize.Height() )
    {
        // scale to fit window
        const double fGrfWH = static_cast<double>(aNewSize.Width()) / aNewSize.Height();
        const double fWinWH = static_cast<double>(aWinSize.Width()) / aWinSize.Height();

        if ( fGrfWH < fWinWH )
        {
            aNewSize.setWidth( static_cast<long>( aWinSize.Height() * fGrfWH ) );
            aNewSize.setHeight( aWinSize.Height() );
        }
        else
        {
            aNewSize.setWidth( aWinSize.Width() );
            aNewSize.setHeight( static_cast<long>( aWinSize.Width() / fGrfWH) );
        }

        const Point aNewPos( ( aWinSize.Width()  - aNewSize.Width() ) >> 1,
                             ( aWinSize.Height() - aNewSize.Height() ) >> 1 );

        rResultRect = tools::Rectangle( aNewPos, aNewSize );
        bRet = true;
    }

    return bRet;
}

void GalleryPreview::Paint(vcl::RenderContext& rRenderContext, const tools::Rectangle& rRect)
{
    Window::Paint(rRenderContext, rRect);

    if (ImplGetGraphicCenterRect(aGraphicObj.GetGraphic(), aPreviewRect))
    {
        const Point aPos( aPreviewRect.TopLeft() );
        const Size  aSize( aPreviewRect.GetSize() );

        if( aGraphicObj.IsAnimated() )
            aGraphicObj.StartAnimation(&rRenderContext, aPos, aSize);
        else
            aGraphicObj.Draw(&rRenderContext, aPos, aSize);
    }
}

void GalleryPreview::MouseButtonDown(const MouseEvent& rMEvt)
{
    if (mpTheme && (rMEvt.GetClicks() == 2))
        static_cast<GalleryBrowser2*>(GetParent())->TogglePreview();
}

void GalleryPreview::Command(const CommandEvent& rCEvt)
{
    Window::Command(rCEvt);

    if (mpTheme && (rCEvt.GetCommand() == CommandEventId::ContextMenu))
    {
        GalleryBrowser2* pGalleryBrowser = static_cast<GalleryBrowser2*>(GetParent());
        pGalleryBrowser->ShowContextMenu(rCEvt.IsMouseEvent() ? &rCEvt.GetMousePosPixel() : nullptr);
    }
}

void GalleryPreview::KeyInput(const KeyEvent& rKEvt)
{
    if(mpTheme)
    {
        GalleryBrowser2* pBrowser = static_cast< GalleryBrowser2* >( GetParent() );

        switch( rKEvt.GetKeyCode().GetCode() )
        {
            case KEY_BACKSPACE:
                pBrowser->TogglePreview();
            break;

            case KEY_HOME:
                pBrowser->Travel( GalleryBrowserTravel::First );
            break;

            case KEY_END:
                pBrowser->Travel( GalleryBrowserTravel::Last );
            break;

            case KEY_LEFT:
            case KEY_UP:
                pBrowser->Travel( GalleryBrowserTravel::Previous );
            break;

            case KEY_RIGHT:
            case KEY_DOWN:
                pBrowser->Travel( GalleryBrowserTravel::Next );
            break;

            default:
            {
                if (!pBrowser->KeyInput(rKEvt, this))
                    Window::KeyInput(rKEvt);
            }
            break;
        }
    }
    else
    {
        Window::KeyInput(rKEvt);
    }
}

sal_Int8 GalleryPreview::AcceptDrop( const AcceptDropEvent& /*rEvt*/ )
{
    sal_Int8 nRet;

    if (mpTheme)
        nRet = static_cast<GalleryBrowser2*>(GetParent())->AcceptDrop(*this);
    else
        nRet = DND_ACTION_NONE;

    return nRet;
}

sal_Int8 GalleryPreview::ExecuteDrop( const ExecuteDropEvent& rEvt )
{
    sal_Int8 nRet;

    if (mpTheme)
        nRet = static_cast<GalleryBrowser2*>(GetParent())->ExecuteDrop(rEvt);
    else
        nRet = DND_ACTION_NONE;

    return nRet;
}

void GalleryPreview::StartDrag( sal_Int8, const Point& )
{
    if(mpTheme)
        static_cast<GalleryBrowser2*>(GetParent())->StartDrag();
}

void GalleryPreview::PreviewMedia( const INetURLObject& rURL )
{
#if HAVE_FEATURE_AVMEDIA
    if (rURL.GetProtocol() != INetProtocol::NotValid)
    {
        ::avmedia::MediaFloater* pFloater = avmedia::getMediaFloater();

        if (!pFloater)
        {
            SfxViewFrame::Current()->GetBindings().GetDispatcher()->Execute( SID_AVMEDIA_PLAYER, SfxCallMode::SYNCHRON );
            pFloater = avmedia::getMediaFloater();
        }

        if (pFloater)
            pFloater->setURL( rURL.GetMainURL( INetURLObject::DecodeMechanism::Unambiguous ), "", true );
    }
#else
    (void) rURL;
#endif
}

SvxGalleryPreview::SvxGalleryPreview()
{
}

void SvxGalleryPreview::SetDrawingArea(weld::DrawingArea* pDrawingArea)
{
    CustomWidgetController::SetDrawingArea(pDrawingArea);
    Size aSize(pDrawingArea->get_ref_device().LogicToPixel(Size(70, 88), MapMode(MapUnit::MapAppFont)));
    pDrawingArea->set_size_request(aSize.Width(), aSize.Height());
    pDrawingArea->set_help_id(HID_GALLERY_WINDOW);
}

bool SvxGalleryPreview::SetGraphic( const INetURLObject& _aURL )
{
    bool bRet = true;
    Graphic aGraphic;
#if HAVE_FEATURE_AVMEDIA
    if( ::avmedia::MediaWindow::isMediaURL( _aURL.GetMainURL( INetURLObject::DecodeMechanism::Unambiguous ), "" ) )
    {
        aGraphic = BitmapEx(RID_SVXBMP_GALLERY_MEDIA);
    }
    else
#endif
    {
        GraphicFilter& rFilter = GraphicFilter::GetGraphicFilter();
        GalleryProgress aProgress( &rFilter );
        if( rFilter.ImportGraphic( aGraphic, _aURL ) )
            bRet = false;
    }

    SetGraphic( aGraphic );
    Invalidate();
    return bRet;
}

bool SvxGalleryPreview::ImplGetGraphicCenterRect( const Graphic& rGraphic, tools::Rectangle& rResultRect ) const
{
    const Size  aWinSize(GetOutputSizePixel());
    Size        aNewSize(GetDrawingArea()->get_ref_device().LogicToPixel(rGraphic.GetPrefSize(), rGraphic.GetPrefMapMode()));
    bool        bRet = false;

    if( aNewSize.Width() && aNewSize.Height() )
    {
        // scale to fit window
        const double fGrfWH = static_cast<double>(aNewSize.Width()) / aNewSize.Height();
        const double fWinWH = static_cast<double>(aWinSize.Width()) / aWinSize.Height();

        if ( fGrfWH < fWinWH )
        {
            aNewSize.setWidth( static_cast<long>( aWinSize.Height() * fGrfWH ) );
            aNewSize.setHeight( aWinSize.Height() );
        }
        else
        {
            aNewSize.setWidth( aWinSize.Width() );
            aNewSize.setHeight( static_cast<long>( aWinSize.Width() / fGrfWH) );
        }

        const Point aNewPos( ( aWinSize.Width()  - aNewSize.Width() ) >> 1,
                             ( aWinSize.Height() - aNewSize.Height() ) >> 1 );

        rResultRect = tools::Rectangle( aNewPos, aNewSize );
        bRet = true;
    }

    return bRet;
}

void SvxGalleryPreview::Paint(vcl::RenderContext& rRenderContext, const tools::Rectangle&)
{
    rRenderContext.SetBackground(Wallpaper(GALLERY_BG_COLOR));

    if (ImplGetGraphicCenterRect(aGraphicObj.GetGraphic(), aPreviewRect))
    {
        const Point aPos( aPreviewRect.TopLeft() );
        const Size  aSize( aPreviewRect.GetSize() );

        if( aGraphicObj.IsAnimated() )
            aGraphicObj.StartAnimation(&rRenderContext, aPos, aSize);
        else
            aGraphicObj.Draw(&rRenderContext, aPos, aSize);
    }
}

static void drawTransparenceBackground(vcl::RenderContext& rOut, const Point& rPos, const Size& rSize)
{

    // draw checkered background
    static const sal_uInt32 nLen(8);
    static const Color aW(COL_WHITE);
    static const Color aG(0xef, 0xef, 0xef);

    rOut.DrawCheckered(rPos, rSize, nLen, aW, aG);
}

GalleryIconView::GalleryIconView( GalleryBrowser2* pParent, GalleryTheme* pTheme ) :
        ValueSet( pParent, WB_TABSTOP | WB_3DLOOK | WB_BORDER | WB_ITEMBORDER | WB_DOUBLEBORDER | WB_VSCROLL | WB_FLATVALUESET ),
        DropTargetHelper( this ),
        DragSourceHelper( this ),
        mpTheme ( pTheme )
{

    EnableFullItemMode( false );

    SetHelpId( HID_GALLERY_WINDOW );
    InitSettings();
    SetExtraSpacing( 2 );
    SetItemWidth( S_THUMB + 6 );
    SetItemHeight( S_THUMB + 6 );
}

void GalleryIconView::InitSettings()
{
    SetBackground( Wallpaper( GALLERY_BG_COLOR ) );
    SetControlBackground( GALLERY_BG_COLOR );
    SetControlForeground( GALLERY_FG_COLOR );
    SetColor( GALLERY_BG_COLOR );
}

void GalleryIconView::DataChanged( const DataChangedEvent& rDCEvt )
{
    if ( ( rDCEvt.GetType() == DataChangedEventType::SETTINGS ) && ( rDCEvt.GetFlags() & AllSettingsFlags::STYLE ) )
        InitSettings();
    else
        ValueSet::DataChanged( rDCEvt );
}

void GalleryIconView::UserDraw(const UserDrawEvent& rUDEvt)
{
    const sal_uInt16 nId = rUDEvt.GetItemId();

    if (!nId || !mpTheme)
        return;

    const tools::Rectangle& rRect = rUDEvt.GetRect();
    const Size aSize(rRect.GetWidth(), rRect.GetHeight());
    BitmapEx aBitmapEx;
    Size aPreparedSize;
    OUString aItemTextTitle;
    OUString aItemTextPath;

    mpTheme->GetPreviewBitmapExAndStrings(nId - 1, aBitmapEx, aPreparedSize, aItemTextTitle, aItemTextPath);

    bool bNeedToCreate(aBitmapEx.IsEmpty());

    if (!bNeedToCreate && aItemTextTitle.isEmpty())
    {
        bNeedToCreate = true;
    }

    if (!bNeedToCreate && aPreparedSize != aSize)
    {
        bNeedToCreate = true;
    }

    if (bNeedToCreate)
    {
        std::unique_ptr<SgaObject> pObj = mpTheme->AcquireObject(nId - 1);

        if(pObj)
        {
            aBitmapEx = pObj->createPreviewBitmapEx(aSize);
            aItemTextTitle = GalleryBrowser2::GetItemText(*mpTheme, *pObj, GalleryItemFlags::Title);

            mpTheme->SetPreviewBitmapExAndStrings(nId - 1, aBitmapEx, aSize, aItemTextTitle, aItemTextPath);
        }
    }

    if (!aBitmapEx.IsEmpty())
    {
        const Size aBitmapExSizePixel(aBitmapEx.GetSizePixel());
        const Point aPos(
            ((aSize.Width() - aBitmapExSizePixel.Width()) >> 1) + rRect.Left(),
            ((aSize.Height() - aBitmapExSizePixel.Height()) >> 1) + rRect.Top());
        OutputDevice* pDev = rUDEvt.GetRenderContext();

        if(aBitmapEx.IsTransparent())
        {
            // draw checkered background for full rectangle.
            drawTransparenceBackground(*pDev, rRect.TopLeft(), rRect.GetSize());
        }

        pDev->DrawBitmapEx(aPos, aBitmapEx);
    }

    SetItemText(nId, aItemTextTitle);
}

void GalleryIconView::MouseButtonDown(const MouseEvent& rMEvt)
{
    ValueSet::MouseButtonDown(rMEvt);

    if (rMEvt.GetClicks() == 2)
        static_cast<GalleryBrowser2*>(GetParent())->TogglePreview();
}

void GalleryIconView::Command(const CommandEvent& rCEvt)
{
    ValueSet::Command(rCEvt);

    if (rCEvt.GetCommand() == CommandEventId::ContextMenu)
    {
        GalleryBrowser2* pGalleryBrowser = static_cast<GalleryBrowser2*>(GetParent());
        pGalleryBrowser->ShowContextMenu(rCEvt.IsMouseEvent() ? &rCEvt.GetMousePosPixel() : nullptr);
    }
}

void GalleryIconView::KeyInput(const KeyEvent& rKEvt)
{
    if (!mpTheme || !static_cast<GalleryBrowser2*>(GetParent())->KeyInput(rKEvt, this))
        ValueSet::KeyInput(rKEvt);
}

sal_Int8 GalleryIconView::AcceptDrop(const AcceptDropEvent& /*rEvt*/)
{
    return static_cast<GalleryBrowser2*>(GetParent())->AcceptDrop(*this);
}

sal_Int8 GalleryIconView::ExecuteDrop(const ExecuteDropEvent& rEvt)
{
    return static_cast<GalleryBrowser2*>(GetParent())->ExecuteDrop(rEvt);
}

void GalleryIconView::StartDrag(sal_Int8, const Point&)
{
    const CommandEvent aEvt(GetPointerPosPixel(), CommandEventId::StartDrag, true);
    vcl::Region aRegion;

    // call this to initiate dragging for ValueSet
    ValueSet::StartDrag(aEvt, aRegion);
    static_cast<GalleryBrowser2*>(GetParent())->StartDrag();
}

GalleryListView::GalleryListView( GalleryBrowser2* pParent, GalleryTheme* pTheme ) :
    BrowseBox( pParent, WB_TABSTOP | WB_3DLOOK | WB_BORDER ),
    mpTheme( pTheme ),
    mnCurRow( 0 )
{

    SetHelpId( HID_GALLERY_WINDOW );

    InitSettings();

    SetMode( BrowserMode::AUTO_VSCROLL | BrowserMode::AUTOSIZE_LASTCOL | BrowserMode::AUTO_HSCROLL );
    SetDataRowHeight( 28 );
    InsertDataColumn( GALLERY_BRWBOX_TITLE, SvxResId(RID_SVXSTR_GALLERY_TITLE), 256  );
}

void GalleryListView::InitSettings()
{
    SetBackground( Wallpaper( GALLERY_BG_COLOR ) );
    SetControlBackground( GALLERY_BG_COLOR );
    SetControlForeground( GALLERY_FG_COLOR );
}

void GalleryListView::DataChanged( const DataChangedEvent& rDCEvt )
{
    if ( ( rDCEvt.GetType() == DataChangedEventType::SETTINGS ) && ( rDCEvt.GetFlags() & AllSettingsFlags::STYLE ) )
        InitSettings();
    else
        BrowseBox::DataChanged( rDCEvt );
}

bool GalleryListView::SeekRow( long nRow )
{
    mnCurRow = nRow;
    return true;
}

OUString GalleryListView::GetCellText(long _nRow, sal_uInt16 /*nColumnId*/) const
{
    OUString sRet;
    if( mpTheme && ( _nRow < static_cast< long >( mpTheme->GetObjectCount() ) ) )
    {
        std::unique_ptr<SgaObject> pObj = mpTheme->AcquireObject( _nRow );

        if( pObj )
        {
            sRet = GalleryBrowser2::GetItemText( *mpTheme, *pObj, GalleryItemFlags::Title );
        }
    }

    return sRet;
}

tools::Rectangle GalleryListView::GetFieldCharacterBounds(sal_Int32 _nRow,sal_Int32 _nColumnPos,sal_Int32 nIndex)
{
    DBG_ASSERT(_nColumnPos >= 0 && _nColumnPos <= SAL_MAX_UINT16, "GalleryListView::GetFieldCharacterBounds: _nColumnId overflow");
    tools::Rectangle aRect;
    if ( SeekRow(_nRow) )
    {
        SvxFont aFont( GetFont() );
        AccessibleStringWrap aStringWrap( *this, aFont, GetCellText(_nRow, sal::static_int_cast<sal_uInt16>( GetColumnId( sal::static_int_cast<sal_uInt16>(_nColumnPos) ) ) ) );

        // get the bounds inside the string
        aStringWrap.GetCharacterBounds(nIndex, aRect);

        // offset to
    }
    return aRect;
}

sal_Int32 GalleryListView::GetFieldIndexAtPoint(sal_Int32 _nRow,sal_Int32 _nColumnPos,const Point& _rPoint)
{
    DBG_ASSERT(_nColumnPos >= 0 && _nColumnPos <= SAL_MAX_UINT16, "GalleryListView::GetFieldIndexAtPoint: _nColumnId overflow");
    sal_Int32 nRet = -1;
    if ( SeekRow(_nRow) )
    {
        SvxFont aFont( GetFont() );
        AccessibleStringWrap aStringWrap( *this, aFont, GetCellText(_nRow, sal::static_int_cast<sal_uInt16>(GetColumnId(sal::static_int_cast<sal_uInt16>(_nColumnPos)))) );
        nRet = aStringWrap.GetIndexAtPoint(_rPoint);
    }
    return nRet;
}

void GalleryListView::PaintField(vcl::RenderContext& rDev, const tools::Rectangle& rRect, sal_uInt16 /*nColumnId*/) const
{
    rDev.Push( PushFlags::CLIPREGION );
    rDev.IntersectClipRegion( rRect );

    if( mpTheme && ( mnCurRow < mpTheme->GetObjectCount() ) )
    {
        const Size aSize(rRect.GetHeight(), rRect.GetHeight());
        BitmapEx aBitmapEx;
        Size aPreparedSize;
        OUString aItemTextTitle;
        OUString aItemTextPath;

        mpTheme->GetPreviewBitmapExAndStrings(mnCurRow, aBitmapEx, aPreparedSize, aItemTextTitle, aItemTextPath);

        bool bNeedToCreate(aBitmapEx.IsEmpty());

        if(!bNeedToCreate && (aItemTextTitle.isEmpty() || aPreparedSize != aSize))
            bNeedToCreate = true;

        if(bNeedToCreate)
        {
            std::unique_ptr<SgaObject> pObj = mpTheme->AcquireObject(mnCurRow);

            if(pObj)
            {
                aBitmapEx = pObj->createPreviewBitmapEx(aSize);
                aItemTextTitle = GalleryBrowser2::GetItemText(*mpTheme, *pObj, GalleryItemFlags::Title);
                aItemTextPath = GalleryBrowser2::GetItemText(*mpTheme, *pObj, GalleryItemFlags::Path);

                mpTheme->SetPreviewBitmapExAndStrings(mnCurRow, aBitmapEx, aSize, aItemTextTitle, aItemTextPath);
            }
        }

        const long nTextPosY(rRect.Top() + ((rRect.GetHeight() - rDev.GetTextHeight()) >> 1));

        if(!aBitmapEx.IsEmpty())
        {
            const Size aBitmapExSizePixel(aBitmapEx.GetSizePixel());
            const Point aPos(
                ((aSize.Width() - aBitmapExSizePixel.Width()) >> 1) + rRect.Left(),
                ((aSize.Height() - aBitmapExSizePixel.Height()) >> 1) + rRect.Top());

            if(aBitmapEx.IsTransparent())
            {
                // draw checkered background
                drawTransparenceBackground(rDev, aPos, aBitmapExSizePixel);
            }

            rDev.DrawBitmapEx(aPos, aBitmapEx);
        }

        rDev.DrawText(Point(rRect.Left() + rRect.GetHeight() + 6, nTextPosY), aItemTextTitle);
    }

    rDev.Pop();
}

void GalleryListView::Command( const CommandEvent& rCEvt )
{
    BrowseBox::Command( rCEvt );

    if( rCEvt.GetCommand() == CommandEventId::ContextMenu )
    {
        const Point* pPos = nullptr;

        if( rCEvt.IsMouseEvent() && ( GetRowAtYPosPixel( rCEvt.GetMousePosPixel().Y() ) != BROWSER_ENDOFSELECTION ) )
            pPos = &rCEvt.GetMousePosPixel();

        static_cast<GalleryBrowser2*>( GetParent() )->ShowContextMenu( pPos );
    }
}

void GalleryListView::KeyInput( const KeyEvent& rKEvt )
{
    if( !mpTheme || !static_cast< GalleryBrowser2* >( GetParent() )->KeyInput( rKEvt, this ) )
        BrowseBox::KeyInput( rKEvt );
}

void GalleryListView::DoubleClick( const BrowserMouseEvent& rEvt )
{
    BrowseBox::DoubleClick( rEvt );

    if( rEvt.GetRow() != BROWSER_ENDOFSELECTION )
        static_cast<GalleryBrowser2*>( GetParent() )->TogglePreview();
}

void GalleryListView::Select()
{
    maSelectHdl.Call( this );
}

sal_Int8 GalleryListView::AcceptDrop( const BrowserAcceptDropEvent& )
{
    sal_Int8 nRet = DND_ACTION_NONE;

    if( mpTheme && !mpTheme->IsReadOnly() )
        nRet = DND_ACTION_COPY;

    return nRet;
}

sal_Int8 GalleryListView::ExecuteDrop( const BrowserExecuteDropEvent& rEvt )
{
    ExecuteDropEvent aEvt( rEvt );

    aEvt.maPosPixel.AdjustY(GetTitleHeight() );

    return static_cast<GalleryBrowser2*>( GetParent() )->ExecuteDrop( aEvt );
}

void GalleryListView::StartDrag( sal_Int8, const Point& rPosPixel )
{
    static_cast<GalleryBrowser2*>( GetParent() )->StartDrag( &rPosPixel );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
