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
#include <osl/process.h>
#include <sal/log.hxx>
#include <osl/diagnose.h>
#include <rtl/character.hxx>
#include <vcl/layout.hxx>
#include <vcl/weld.hxx>
#include <vcl/svapp.hxx>
#include <vcl/settings.hxx>

#include <tools/stream.hxx>
#include <rtl/bootstrap.hxx>
#include <unotools/configmgr.hxx>
#include <unotools/bootstrap.hxx>
#include <com/sun/star/uno/Any.h>
#include <vcl/graph.hxx>
#include <vcl/graphicfilter.hxx>
#include <svtools/langhelp.hxx>

#include <com/sun/star/system/SystemShellExecuteFlags.hpp>
#include <com/sun/star/system/SystemShellExecute.hpp>
#include <comphelper/processfactory.hxx>
#include <comphelper/anytostring.hxx>
#include <cppuhelper/exc_hlp.hxx>
#include <cppuhelper/bootstrap.hxx>
#include <basegfx/numeric/ftools.hxx>
#include <com/sun/star/geometry/RealRectangle2D.hpp>
#include <svtools/optionsdrawinglayer.hxx>

#include <sfx2/sfxuno.hxx>
#include <about.hxx>
#include <config_buildid.h>
#include <sfx2/app.hxx>
#include <rtl/ustrbuf.hxx>
#include <vcl/bitmap.hxx>

#if HAVE_FEATURE_OPENCL
#include <opencl/openclwrapper.hxx>
#endif
#include <officecfg/Office/Common.hxx>
#include <officecfg/Office/Calc.hxx>

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star;

AboutDialog::AboutDialog(vcl::Window* pParent)
    : SfxModalDialog(pParent, "AboutDialog", "cui/ui/aboutdialog.ui")
{
    get(m_pLogoReplacement, "logoreplacement");
    get(m_pLogoImage, "logo");
    get(m_pVersion, "version");
    get(m_pDescriptionText, "description");
    get(m_pCopyrightText, "copyright");
    get(m_pBuildIdLink, "buildIdLink");
    m_aCopyrightTextStr = m_pCopyrightText->GetText();
    get(m_pWebsiteButton, "website");
    get(m_pCreditsButton, "credits");
    m_aCreditsLinkStr = get<FixedText>("link")->GetText();
    m_sBuildStr = get<FixedText>("buildid")->GetText();
    m_aVendorTextStr = get<FixedText>("vendor")->GetText();
    m_aVersionTextStr = m_pVersion->GetText();
    m_aBasedTextStr = get<FixedText>("libreoffice")->GetText();
    m_aBasedDerivedTextStr = get<FixedText>("derived")->GetText();
    m_aLocaleStr = get<FixedText>("locale")->GetText();
    m_aUILocaleStr = get<FixedText>("uilocale")->GetText();
    m_buildIdLinkString = m_pBuildIdLink->GetText();

    m_pVersion->SetText(GetVersionString());

    OUString aCopyrightString = GetCopyrightString();
    m_pCopyrightText->SetText( aCopyrightString );

    SetBuildIdLink();

    StyleControls();

    SetLogo();

    // Connect all handlers
    m_pCreditsButton->SetClickHdl( LINK( this, AboutDialog, HandleClick ) );
    m_pWebsiteButton->SetClickHdl( LINK( this, AboutDialog, HandleClick ) );

    get<PushButton>("close")->GrabFocus();
}

AboutDialog::~AboutDialog()
{
    disposeOnce();
}

void AboutDialog::dispose()
{
    m_pVersion.clear();
    m_pDescriptionText.clear();
    m_pCopyrightText.clear();
    m_pLogoImage.clear();
    m_pLogoReplacement.clear();
    m_pCreditsButton.clear();
    m_pWebsiteButton.clear();
    m_pBuildIdLink.clear();
    SfxModalDialog::dispose();
}

IMPL_LINK( AboutDialog, HandleClick, Button*, pButton, void )
{
    OUString sURL = "";

    // Find which button was pressed and from this, get the URL to be opened
    if (pButton == m_pCreditsButton)
        sURL = m_aCreditsLinkStr;
    else if (pButton == m_pWebsiteButton)
    {
        sURL = officecfg::Office::Common::Help::StartCenter::InfoURL::get();
    }

    // If the URL is empty, don't do anything
    if ( sURL.isEmpty() )
        return;
    try
    {
        Reference< css::system::XSystemShellExecute > xSystemShellExecute(
            css::system::SystemShellExecute::create(::comphelper::getProcessComponentContext() ) );
        xSystemShellExecute->execute( sURL, OUString(), css::system::SystemShellExecuteFlags::URIS_ONLY );
    }
    catch (const Exception&)
    {
        Any exc( ::cppu::getCaughtException() );
        OUString msg( ::comphelper::anyToString( exc ) );
        const SolarMutexGuard guard;
        std::unique_ptr<weld::MessageDialog> xErrorBox(Application::CreateMessageDialog(pButton->GetFrameWeld(),
                                                       VclMessageType::Warning, VclButtonsType::Ok, msg));
        xErrorBox->set_title(GetText());
        xErrorBox->run();
    }
}

void AboutDialog::SetBuildIdLink()
{
    const OUString buildId = GetBuildId();

    //~ if (IsStringValidGitHash(buildId))
    if (0)
    {
        if (m_buildIdLinkString.indexOf("$GITHASH") == -1)
        {
            SAL_WARN( "cui.dialogs", "translated git hash string in translations doesn't contain $GITHASH placeholder" );
            m_buildIdLinkString += " $GITHASH";
        }

        m_pBuildIdLink->SetText(m_buildIdLinkString.replaceAll("$GITHASH", buildId));
        m_pBuildIdLink->SetURL("https://hub.libreoffice.org/git-core/" + buildId);
    }
    else
    {
        m_pBuildIdLink->Hide();
    }
}

void AboutDialog::StyleControls()
{
    // Make all the controls have a transparent background
    m_pLogoImage->SetBackground();
    m_pLogoReplacement->SetPaintTransparent(true);
    m_pVersion->SetPaintTransparent(true);
    m_pDescriptionText->SetPaintTransparent(true);
    m_pCopyrightText->SetPaintTransparent(true);

    const StyleSettings& rStyleSettings = Application::GetSettings().GetStyleSettings();

    const vcl::Font& aLabelFont = rStyleSettings.GetLabelFont();
    vcl::Font aLargeFont = aLabelFont;
    aLargeFont.SetFontSize(Size( 0, aLabelFont.GetFontSize().Height() * 3));

    // Logo Replacement Text
    m_pLogoReplacement->SetControlFont(aLargeFont);

    // Description Text
    aLargeFont.SetFontSize(Size(0, aLabelFont.GetFontSize().Height() * 1.3));
    m_pDescriptionText->SetControlFont(aLargeFont);
}

void AboutDialog::SetLogo()
{
    long nWidth = get_content_area()->get_preferred_size().Width();

    // fdo#67401 set AntiAliasing for SVG logo
    SvtOptionsDrawinglayer aDrawOpt;
    bool bOldAntiAliasSetting = aDrawOpt.IsAntiAliasing();
    aDrawOpt.SetAntiAliasing(true);

    // load svg logo, specify desired width, scale height isotropically
    if (SfxApplication::loadBrandSvg("flat_logo", aLogoBitmap, nWidth) &&
        !aLogoBitmap.IsEmpty())
    {
        m_pLogoImage->SetImage(Image(aLogoBitmap));
        m_pLogoReplacement->Hide();
        m_pLogoImage->Show();
    }
    else
    {
        m_pLogoImage->Hide();
        m_pLogoReplacement->Show();
    }
    aDrawOpt.SetAntiAliasing(bOldAntiAliasSetting);
}

void AboutDialog::Resize()
{
    SfxModalDialog::Resize();

    // Load background image
    if (isInitialLayout(this) && !(Application::GetSettings().GetStyleSettings().GetHighContrastMode()))
    {
        SfxApplication::loadBrandSvg("shell/about", aBackgroundBitmap, GetSizePixel().Width());
    }
}

void AboutDialog::Paint(vcl::RenderContext& rRenderContext, const ::tools::Rectangle& rRect)
{
    rRenderContext.SetClipRegion(vcl::Region(rRect));

    Size aSize(GetOutputSizePixel());
    Point aPos(aSize.Width() - aBackgroundBitmap.GetSizePixel().Width(),
               aSize.Height() - aBackgroundBitmap.GetSizePixel().Height());

    rRenderContext.DrawBitmapEx(aPos, aBackgroundBitmap);
}

OUString AboutDialog::GetBuildId()
{
    OUString sDefault;
    OUString sBuildId(utl::Bootstrap::getBuildVersion(sDefault));
    if (!sBuildId.isEmpty())
        return sBuildId;

    sBuildId = utl::Bootstrap::getBuildIdData(sDefault);

    if (!sBuildId.isEmpty())
    {
        sal_Int32 nIndex = 0;
        return sBuildId.getToken( 0, '-', nIndex );
    }

    OSL_ENSURE( !sBuildId.isEmpty(), "No BUILDID in bootstrap file" );
    return sBuildId;
}

OUString AboutDialog::GetLocaleString()
{
    OUString aLocaleStr;
    rtl_Locale * pLocale;

    osl_getProcessLocale( &pLocale );

    if ( pLocale && pLocale->Language )
    {
        if (pLocale->Country && rtl_uString_getLength( pLocale->Country) > 0)
            aLocaleStr = OUString(pLocale->Language) + "_" + OUString(pLocale->Country);
        else
            aLocaleStr = OUString(pLocale->Language);
        if (pLocale->Variant && rtl_uString_getLength( pLocale->Variant) > 0)
            aLocaleStr += OUString(pLocale->Variant);
    }

    return aLocaleStr;
}

bool AboutDialog::IsStringValidGitHash(const OUString& hash)
{
    for (int i = 0; i < hash.getLength(); i++)
    {
        if (!rtl::isAsciiHexDigit(hash[i]))
        {
            return false;
        }
    }

    return true;
}

OUString AboutDialog::GetVersionString()
{
    OUString sVersion = m_aVersionTextStr;

#ifdef _WIN64
    sVersion += " (x64)";
#elif defined(_WIN32)
    sVersion += " (x86)";
#endif

    OUString sBuildId = GetBuildId();

    OUString aLocaleStr = Application::GetSettings().GetLanguageTag().getBcp47() + " (" + GetLocaleString() + ")";
    OUString aUILocaleStr = Application::GetSettings().GetUILanguageTag().getBcp47();

    if (!sBuildId.trim().isEmpty())
    {
        sVersion += "\n";
        if (m_sBuildStr.indexOf("$BUILDID") == -1)
        {
            SAL_WARN( "cui.dialogs", "translated Build Id string in translations doesn't contain $BUILDID placeholder" );
            m_sBuildStr += " $BUILDID";
        }
        sVersion += m_sBuildStr.replaceAll("$BUILDID", sBuildId);
    }

    sVersion += "\n" + Application::GetHWOSConfInfo();

    bool const extra = EXTRA_BUILDID[0] != '\0';
        // extracted from the 'if' to avoid Clang -Wunreachable-code
    if (extra)
    {
        sVersion += "\n" EXTRA_BUILDID;
    }

    if (m_aLocaleStr.indexOf("$LOCALE") == -1)
    {
        SAL_WARN( "cui.dialogs", "translated locale string in translations doesn't contain $LOCALE placeholder" );
        m_aLocaleStr += " $LOCALE";
    }
    sVersion += "\n" + m_aLocaleStr.replaceAll("$LOCALE", aLocaleStr);

    if (m_aUILocaleStr.indexOf("$LOCALE") == -1)
    {
        SAL_WARN( "cui.dialogs", "translated uilocale string in translations doesn't contain $LOCALE placeholder" );
        m_aUILocaleStr += " $LOCALE";
    }
    sVersion += "; " + m_aUILocaleStr.replaceAll("$LOCALE", aUILocaleStr);

    OUString aCalcMode = "Calc: "; // Calc calculation mode

#if HAVE_FEATURE_OPENCL
    bool bOpenCL = openclwrapper::GPUEnv::isOpenCLEnabled();
    if (bOpenCL)
        aCalcMode += "CL";
#else
    const bool bOpenCL = false;
#endif

    static const bool bThreadingProhibited = std::getenv("SC_NO_THREADED_CALCULATION");
    bool bThreadedCalc = officecfg::Office::Calc::Formula::Calculation::UseThreadedCalculationForFormulaGroups::get();

    if (!bThreadingProhibited && !bOpenCL && bThreadedCalc)
    {
        if (!aCalcMode.endsWith(" "))
            aCalcMode += " ";
        aCalcMode += "threaded";
    }

    sVersion += "\n" + aCalcMode;

    return sVersion;
}

OUString AboutDialog::GetCopyrightString()
{
    OUString aCopyrightString  = m_aVendorTextStr + "\n"
                               + m_aCopyrightTextStr + "\n";

    if (utl::ConfigManager::getProductName() == "LibreOffice")
        aCopyrightString += m_aBasedTextStr;
    else
        aCopyrightString += m_aBasedDerivedTextStr;

    return aCopyrightString;
}

bool AboutDialog::Close()
{
    EndDialog( RET_OK );
    return false;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
