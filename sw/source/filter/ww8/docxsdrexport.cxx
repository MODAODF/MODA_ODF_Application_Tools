/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "docxsdrexport.hxx"
#include <com/sun/star/drawing/PointSequenceSequence.hpp>
#include <com/sun/star/xml/sax/XSAXSerializable.hpp>
#include <com/sun/star/xml/sax/Writer.hpp>
#include <com/sun/star/xml/dom/XNodeList.hpp>
#include <editeng/unoprnms.hxx>
#include <editeng/charrotateitem.hxx>
#include <editeng/lrspitem.hxx>
#include <editeng/ulspitem.hxx>
#include <editeng/shaditem.hxx>
#include <editeng/opaqitem.hxx>
#include <editeng/boxitem.hxx>
#include <svx/svdogrp.hxx>
#include <oox/helper/propertyset.hxx>
#include <oox/token/namespaces.hxx>
#include <oox/token/relationship.hxx>
#include <oox/token/properties.hxx>
#include <textboxhelper.hxx>
#include <fmtanchr.hxx>
#include <fmtsrnd.hxx>
#include <fmtcntnt.hxx>
#include <ndtxt.hxx>
#include <txatbase.hxx>
#include <fmtfsize.hxx>
#include <frmatr.hxx>
#include "docxattributeoutput.hxx"
#include "docxexportfilter.hxx"
#include <comphelper/processfactory.hxx>
#include <comphelper/seqstream.hxx>
#include <comphelper/sequence.hxx>
#include <comphelper/sequenceashashmap.hxx>
#include <o3tl/make_unique.hxx>
#include <sal/log.hxx>

#include <IDocumentDrawModelAccess.hxx>

using namespace com::sun::star;
using namespace oox;

namespace
{
uno::Sequence<beans::PropertyValue> lclGetProperty(const uno::Reference<drawing::XShape>& rShape,
                                                   const OUString& rPropName)
{
    uno::Sequence<beans::PropertyValue> aResult;
    uno::Reference<beans::XPropertySet> xPropertySet(rShape, uno::UNO_QUERY);
    uno::Reference<beans::XPropertySetInfo> xPropSetInfo;

    if (!xPropertySet.is())
        return aResult;

    xPropSetInfo = xPropertySet->getPropertySetInfo();
    if (xPropSetInfo.is() && xPropSetInfo->hasPropertyByName(rPropName))
    {
        xPropertySet->getPropertyValue(rPropName) >>= aResult;
    }
    return aResult;
}

OUString lclGetAnchorIdFromGrabBag(const SdrObject* pObj)
{
    OUString aResult;
    uno::Reference<drawing::XShape> xShape(const_cast<SdrObject*>(pObj)->getUnoShape(),
                                           uno::UNO_QUERY);
    OUString aGrabBagName;
    uno::Reference<lang::XServiceInfo> xServiceInfo(xShape, uno::UNO_QUERY);
    if (xServiceInfo->supportsService("com.sun.star.text.TextFrame"))
        aGrabBagName = "FrameInteropGrabBag";
    else
        aGrabBagName = "InteropGrabBag";
    uno::Sequence<beans::PropertyValue> propList = lclGetProperty(xShape, aGrabBagName);
    for (sal_Int32 nProp = 0; nProp < propList.getLength(); ++nProp)
    {
        OUString aPropName = propList[nProp].Name;
        if (aPropName == "AnchorId")
        {
            propList[nProp].Value >>= aResult;
            break;
        }
    }
    return aResult;
}

void lclMovePositionWithRotation(awt::Point& aPos, const Size& rSize, sal_Int64 nRotation)
{
    // code from ImplEESdrWriter::ImplFlipBoundingBox (filter/source/msfilter/eschesdo.cxx)
    // TODO: refactor

    if (nRotation == 0)
        return;

    if (nRotation < 0)
        nRotation = (36000 + nRotation) % 36000;
    if (nRotation % 18000 == 0)
        nRotation = 0;
    while (nRotation > 9000)
        nRotation = (18000 - (nRotation % 18000));

    double fVal = static_cast<double>(nRotation) * F_PI18000;
    double fCos = cos(fVal);
    double fSin = sin(fVal);

    double nWidthHalf = static_cast<double>(rSize.Width()) / 2;
    double nHeightHalf = static_cast<double>(rSize.Height()) / 2;

    double nXDiff = fSin * nHeightHalf + fCos * nWidthHalf - nWidthHalf;
    double nYDiff = fSin * nWidthHalf + fCos * nHeightHalf - nHeightHalf;

    aPos.X += nXDiff;
    aPos.Y += nYDiff;
}

/// Determines if the anchor is inside a paragraph.
bool IsAnchorTypeInsideParagraph(const ww8::Frame* pFrame)
{
    const SwFormatAnchor& rAnchor = pFrame->GetFrameFormat().GetAttrSet().GetAnchor();
    return rAnchor.GetAnchorId() != RndStdIds::FLY_AT_PAGE;
}
}

ExportDataSaveRestore::ExportDataSaveRestore(DocxExport& rExport, sal_uLong nStt, sal_uLong nEnd,
                                             ww8::Frame const* pParentFrame)
    : m_rExport(rExport)
{
    m_rExport.SaveData(nStt, nEnd);
    m_rExport.m_pParentFrame = pParentFrame;
}

ExportDataSaveRestore::~ExportDataSaveRestore() { m_rExport.RestoreData(); }

/// Holds data used by DocxSdrExport only.
struct DocxSdrExport::Impl
{
    DocxSdrExport& m_rSdrExport;
    DocxExport& m_rExport;
    sax_fastparser::FSHelperPtr m_pSerializer;
    oox::drawingml::DrawingML* m_pDrawingML;
    const Size* m_pFlyFrameSize;
    bool m_bTextFrameSyntax;
    bool m_bDMLTextFrameSyntax;
    rtl::Reference<sax_fastparser::FastAttributeList> m_pFlyAttrList;
    rtl::Reference<sax_fastparser::FastAttributeList> m_pTextboxAttrList;
    OStringBuffer m_aTextFrameStyle;
    bool m_bFrameBtLr;
    bool m_bDrawingOpen;
    bool m_bParagraphSdtOpen;
    bool m_bParagraphHasDrawing; ///Flag for checking drawing in a paragraph.
    bool m_bFlyFrameGraphic;
    rtl::Reference<sax_fastparser::FastAttributeList> m_pFlyFillAttrList;
    sax_fastparser::FastAttributeList* m_pFlyWrapAttrList;
    sax_fastparser::FastAttributeList* m_pBodyPrAttrList;
    rtl::Reference<sax_fastparser::FastAttributeList> m_pDashLineStyleAttr;
    bool m_bDMLAndVMLDrawingOpen;
    /// List of TextBoxes in this document: they are exported as part of their shape, never alone.
    /// Preserved rotation for TextFrames.
    sal_Int32 m_nDMLandVMLTextFrameRotation;

    Impl(DocxSdrExport& rSdrExport, DocxExport& rExport, sax_fastparser::FSHelperPtr pSerializer,
         oox::drawingml::DrawingML* pDrawingML)
        : m_rSdrExport(rSdrExport)
        , m_rExport(rExport)
        , m_pSerializer(std::move(pSerializer))
        , m_pDrawingML(pDrawingML)
        , m_pFlyFrameSize(nullptr)
        , m_bTextFrameSyntax(false)
        , m_bDMLTextFrameSyntax(false)
        , m_bFrameBtLr(false)
        , m_bDrawingOpen(false)
        , m_bParagraphSdtOpen(false)
        , m_bParagraphHasDrawing(false)
        , m_bFlyFrameGraphic(false)
        , m_pFlyWrapAttrList(nullptr)
        , m_pBodyPrAttrList(nullptr)
        , m_bDMLAndVMLDrawingOpen(false)
        , m_nDMLandVMLTextFrameRotation(0)
    {
    }

    /// Writes wp wrapper code around an SdrObject, which itself is written using drawingML syntax.

    void textFrameShadow(const SwFrameFormat& rFrameFormat);
    static bool isSupportedDMLShape(const uno::Reference<drawing::XShape>& xShape);
    /// Undo the text direction mangling done by the frame btLr handler in writerfilter::dmapper::DomainMapper::lcl_startCharacterGroup()
    bool checkFrameBtlr(SwNode* pStartNode, bool bDML, bool bIsVertical = false);
    oox::drawingml::DrawingML* getDrawingML() const { return m_pDrawingML; }
};

DocxSdrExport::DocxSdrExport(DocxExport& rExport, const sax_fastparser::FSHelperPtr& pSerializer,
                             oox::drawingml::DrawingML* pDrawingML)
    : m_pImpl(o3tl::make_unique<Impl>(*this, rExport, pSerializer, pDrawingML))
{
}

DocxSdrExport::~DocxSdrExport() = default;

void DocxSdrExport::setSerializer(const sax_fastparser::FSHelperPtr& pSerializer)
{
    m_pImpl->m_pSerializer = pSerializer;
}

const Size* DocxSdrExport::getFlyFrameSize() { return m_pImpl->m_pFlyFrameSize; }

bool DocxSdrExport::getTextFrameSyntax() { return m_pImpl->m_bTextFrameSyntax; }

bool DocxSdrExport::getDMLTextFrameSyntax() { return m_pImpl->m_bDMLTextFrameSyntax; }

rtl::Reference<sax_fastparser::FastAttributeList>& DocxSdrExport::getFlyAttrList()
{
    return m_pImpl->m_pFlyAttrList;
}

rtl::Reference<sax_fastparser::FastAttributeList>& DocxSdrExport::getTextboxAttrList()
{
    return m_pImpl->m_pTextboxAttrList;
}

OStringBuffer& DocxSdrExport::getTextFrameStyle() { return m_pImpl->m_aTextFrameStyle; }

bool DocxSdrExport::getFrameBtLr() { return m_pImpl->m_bFrameBtLr; }

bool DocxSdrExport::IsDrawingOpen() { return m_pImpl->m_bDrawingOpen; }

void DocxSdrExport::setParagraphSdtOpen(bool bParagraphSdtOpen)
{
    m_pImpl->m_bParagraphSdtOpen = bParagraphSdtOpen;
}

bool DocxSdrExport::IsDMLAndVMLDrawingOpen() { return m_pImpl->m_bDMLAndVMLDrawingOpen; }

bool DocxSdrExport::IsParagraphHasDrawing() { return m_pImpl->m_bParagraphHasDrawing; }

void DocxSdrExport::setParagraphHasDrawing(bool bParagraphHasDrawing)
{
    m_pImpl->m_bParagraphHasDrawing = bParagraphHasDrawing;
}

rtl::Reference<sax_fastparser::FastAttributeList>& DocxSdrExport::getFlyFillAttrList()
{
    return m_pImpl->m_pFlyFillAttrList;
}

sax_fastparser::FastAttributeList* DocxSdrExport::getFlyWrapAttrList()
{
    return m_pImpl->m_pFlyWrapAttrList;
}

sax_fastparser::FastAttributeList* DocxSdrExport::getBodyPrAttrList()
{
    return m_pImpl->m_pBodyPrAttrList;
}

rtl::Reference<sax_fastparser::FastAttributeList>& DocxSdrExport::getDashLineStyle()
{
    return m_pImpl->m_pDashLineStyleAttr;
}

void DocxSdrExport::setFlyWrapAttrList(sax_fastparser::FastAttributeList* pAttrList)
{
    m_pImpl->m_pFlyWrapAttrList = pAttrList;
}

void DocxSdrExport::startDMLAnchorInline(const SwFrameFormat* pFrameFormat, const Size& rSize)
{
    m_pImpl->m_bDrawingOpen = true;
    m_pImpl->m_bParagraphHasDrawing = true;
    m_pImpl->m_pSerializer->startElementNS(XML_w, XML_drawing, FSEND);

    const SvxLRSpaceItem aLRSpaceItem = pFrameFormat->GetLRSpace(false);
    const SvxULSpaceItem aULSpaceItem = pFrameFormat->GetULSpace(false);

    bool isAnchor;

    if (m_pImpl->m_bFlyFrameGraphic)
    {
        isAnchor = false; // make Graphic object inside DMLTextFrame & VMLTextFrame as Inline
    }
    else
    {
        isAnchor = pFrameFormat->GetAnchor().GetAnchorId() != RndStdIds::FLY_AS_CHAR;
    }

    // Count effectExtent values, their value is needed before dist{T,B,L,R} is written.
    SvxShadowItem aShadowItem = pFrameFormat->GetShadow();
    sal_Int32 nLeftExt = 0, nRightExt = 0, nTopExt = 0, nBottomExt = 0;
    if (aShadowItem.GetLocation() != SvxShadowLocation::NONE)
    {
        sal_Int32 nShadowWidth(TwipsToEMU(aShadowItem.GetWidth()));
        switch (aShadowItem.GetLocation())
        {
            case SvxShadowLocation::TopLeft:
                nTopExt = nLeftExt = nShadowWidth;
                break;
            case SvxShadowLocation::TopRight:
                nTopExt = nRightExt = nShadowWidth;
                break;
            case SvxShadowLocation::BottomLeft:
                nBottomExt = nLeftExt = nShadowWidth;
                break;
            case SvxShadowLocation::BottomRight:
                nBottomExt = nRightExt = nShadowWidth;
                break;
            case SvxShadowLocation::NONE:
            case SvxShadowLocation::End:
                break;
        }
    }
    else if (const SdrObject* pObject = pFrameFormat->FindRealSdrObject())
    {
        // No shadow, but we have an idea what was the original effectExtent.
        uno::Any aAny;
        pObject->GetGrabBagItem(aAny);
        comphelper::SequenceAsHashMap aGrabBag(aAny);
        auto it = aGrabBag.find("CT_EffectExtent");
        if (it != aGrabBag.end())
        {
            comphelper::SequenceAsHashMap aEffectExtent(it->second);
            for (std::pair<const OUString, uno::Any>& rDirection : aEffectExtent)
            {
                if (rDirection.first == "l" && rDirection.second.has<sal_Int32>())
                    nLeftExt = rDirection.second.get<sal_Int32>();
                else if (rDirection.first == "t" && rDirection.second.has<sal_Int32>())
                    nTopExt = rDirection.second.get<sal_Int32>();
                else if (rDirection.first == "r" && rDirection.second.has<sal_Int32>())
                    nRightExt = rDirection.second.get<sal_Int32>();
                else if (rDirection.first == "b" && rDirection.second.has<sal_Int32>())
                    nBottomExt = rDirection.second.get<sal_Int32>();
            }
        }
    }

    if (isAnchor)
    {
        sax_fastparser::FastAttributeList* attrList
            = sax_fastparser::FastSerializerHelper::createAttrList();
        bool bOpaque = pFrameFormat->GetOpaque().GetValue();
        awt::Point aPos(pFrameFormat->GetHoriOrient().GetPos(),
                        pFrameFormat->GetVertOrient().GetPos());
        const SdrObject* pObj = pFrameFormat->FindRealSdrObject();
        long nRotation = 0;
        if (pObj != nullptr)
        {
            // SdrObjects know their layer, consider that instead of the frame format.
            bOpaque = pObj->GetLayer()
                          != pFrameFormat->GetDoc()->getIDocumentDrawModelAccess().GetHellId()
                      && pObj->GetLayer()
                             != pFrameFormat->GetDoc()
                                    ->getIDocumentDrawModelAccess()
                                    .GetInvisibleHellId();

            nRotation = pObj->GetRotateAngle();
            lclMovePositionWithRotation(aPos, rSize, nRotation);
        }
        attrList->add(XML_behindDoc, bOpaque ? "0" : "1");
        // Extend distance with the effect extent if the shape is not rotated, which is the opposite
        // of the mapping done at import time.
        // The type of dist* attributes is unsigned, so make sure no negative value is written.
        sal_Int64 nTopExtDist = nRotation ? 0 : nTopExt;
        sal_Int64 nDistT = std::max(static_cast<sal_Int64>(0),
                                    TwipsToEMU(aULSpaceItem.GetUpper()) - nTopExtDist);
        attrList->add(XML_distT, OString::number(nDistT).getStr());
        sal_Int64 nBottomExtDist = nRotation ? 0 : nBottomExt;
        sal_Int64 nDistB = std::max(static_cast<sal_Int64>(0),
                                    TwipsToEMU(aULSpaceItem.GetLower()) - nBottomExtDist);
        attrList->add(XML_distB, OString::number(nDistB).getStr());
        sal_Int64 nLeftExtDist = nRotation ? 0 : nLeftExt;
        sal_Int64 nDistL = std::max(static_cast<sal_Int64>(0),
                                    TwipsToEMU(aLRSpaceItem.GetLeft()) - nLeftExtDist);
        attrList->add(XML_distL, OString::number(nDistL).getStr());
        sal_Int64 nRightExtDist = nRotation ? 0 : nRightExt;
        sal_Int64 nDistR = std::max(static_cast<sal_Int64>(0),
                                    TwipsToEMU(aLRSpaceItem.GetRight()) - nRightExtDist);
        attrList->add(XML_distR, OString::number(nDistR).getStr());
        attrList->add(XML_simplePos, "0");
        attrList->add(XML_locked, "0");
        attrList->add(XML_layoutInCell, "1");
        attrList->add(XML_allowOverlap, "1"); // TODO
        if (pObj != nullptr)
            // It seems 0 and 1 have special meaning: just start counting from 2 to avoid issues with that.
            attrList->add(XML_relativeHeight, OString::number(pObj->GetOrdNum() + 2));
        else
            // relativeHeight is mandatory attribute, if value is not present, we must write default value
            attrList->add(XML_relativeHeight, "0");
        if (pObj != nullptr)
        {
            OUString sAnchorId = lclGetAnchorIdFromGrabBag(pObj);
            if (!sAnchorId.isEmpty())
                attrList->addNS(XML_wp14, XML_anchorId,
                                OUStringToOString(sAnchorId, RTL_TEXTENCODING_UTF8));
        }
        sax_fastparser::XFastAttributeListRef xAttrList(attrList);
        m_pImpl->m_pSerializer->startElementNS(XML_wp, XML_anchor, xAttrList);
        m_pImpl->m_pSerializer->singleElementNS(XML_wp, XML_simplePos, XML_x, "0", XML_y, "0",
                                                FSEND); // required, unused
        const char* relativeFromH;
        const char* relativeFromV;
        const char* alignH = nullptr;
        const char* alignV = nullptr;
        switch (pFrameFormat->GetVertOrient().GetRelationOrient())
        {
            case text::RelOrientation::PAGE_PRINT_AREA:
                relativeFromV = "margin";
                break;
            case text::RelOrientation::PAGE_FRAME:
                relativeFromV = "page";
                break;
            case text::RelOrientation::FRAME:
                relativeFromV = "paragraph";
                break;
            case text::RelOrientation::TEXT_LINE:
            default:
                relativeFromV = "line";
                break;
        }
        switch (pFrameFormat->GetVertOrient().GetVertOrient())
        {
            case text::VertOrientation::TOP:
            case text::VertOrientation::CHAR_TOP:
            case text::VertOrientation::LINE_TOP:
                if (pFrameFormat->GetVertOrient().GetRelationOrient()
                    == text::RelOrientation::TEXT_LINE)
                    alignV = "bottom";
                else
                    alignV = "top";
                break;
            case text::VertOrientation::BOTTOM:
            case text::VertOrientation::CHAR_BOTTOM:
            case text::VertOrientation::LINE_BOTTOM:
                if (pFrameFormat->GetVertOrient().GetRelationOrient()
                    == text::RelOrientation::TEXT_LINE)
                    alignV = "top";
                else
                    alignV = "bottom";
                break;
            case text::VertOrientation::CENTER:
            case text::VertOrientation::CHAR_CENTER:
            case text::VertOrientation::LINE_CENTER:
                alignV = "center";
                break;
            default:
                break;
        }
        switch (pFrameFormat->GetHoriOrient().GetRelationOrient())
        {
            case text::RelOrientation::PAGE_PRINT_AREA:
                relativeFromH = "margin";
                break;
            case text::RelOrientation::PAGE_FRAME:
                relativeFromH = "page";
                break;
            case text::RelOrientation::CHAR:
                relativeFromH = "character";
                break;
            case text::RelOrientation::PAGE_RIGHT:
                relativeFromH = "rightMargin";
                break;
            case text::RelOrientation::PAGE_LEFT:
                relativeFromH = "leftMargin";
                break;
            case text::RelOrientation::FRAME:
            default:
                relativeFromH = "column";
                break;
        }
        switch (pFrameFormat->GetHoriOrient().GetHoriOrient())
        {
            case text::HoriOrientation::LEFT:
                alignH = "left";
                break;
            case text::HoriOrientation::RIGHT:
                alignH = "right";
                break;
            case text::HoriOrientation::CENTER:
                alignH = "center";
                break;
            case text::HoriOrientation::INSIDE:
                alignH = "inside";
                break;
            case text::HoriOrientation::OUTSIDE:
                alignH = "outside";
                break;
            default:
                break;
        }
        m_pImpl->m_pSerializer->startElementNS(XML_wp, XML_positionH, XML_relativeFrom,
                                               relativeFromH, FSEND);
        /**
        * Sizes of integral types
        * climits header defines constants with the limits of integral types for the specific system and compiler implementation used.
        * Use of this might cause platform dependent problem like posOffset exceed the limit.
        **/
        const sal_Int64 MAX_INTEGER_VALUE = SAL_MAX_INT32;
        const sal_Int64 MIN_INTEGER_VALUE = SAL_MIN_INT32;
        if (alignH != nullptr)
        {
            m_pImpl->m_pSerializer->startElementNS(XML_wp, XML_align, FSEND);
            m_pImpl->m_pSerializer->write(alignH);
            m_pImpl->m_pSerializer->endElementNS(XML_wp, XML_align);
        }
        else
        {
            m_pImpl->m_pSerializer->startElementNS(XML_wp, XML_posOffset, FSEND);
            sal_Int64 nPosXEMU = TwipsToEMU(aPos.X);

            /* Absolute Position Offset Value is of type Int. Hence it should not be greater than
             * Maximum value for Int OR Less than the Minimum value for Int.
             * - Maximum value for Int = 2147483647
             * - Minimum value for Int = -2147483648
             *
             * As per ECMA Specification : ECMA-376, Second Edition,
             * Part 1 - Fundamentals And Markup Language Reference[20.4.3.3 ST_PositionOffset (Absolute Position Offset Value)]
             *
             * Please refer : http://www.schemacentral.com/sc/xsd/t-xsd_int.html
             */

            if (nPosXEMU > MAX_INTEGER_VALUE)
            {
                nPosXEMU = MAX_INTEGER_VALUE;
            }
            else if (nPosXEMU < MIN_INTEGER_VALUE)
            {
                nPosXEMU = MIN_INTEGER_VALUE;
            }
            m_pImpl->m_pSerializer->write(nPosXEMU);
            m_pImpl->m_pSerializer->endElementNS(XML_wp, XML_posOffset);
        }
        m_pImpl->m_pSerializer->endElementNS(XML_wp, XML_positionH);
        m_pImpl->m_pSerializer->startElementNS(XML_wp, XML_positionV, XML_relativeFrom,
                                               relativeFromV, FSEND);

        sal_Int64 nPosYEMU = TwipsToEMU(aPos.Y);

        // tdf#93675, 0 below line/paragraph and/or top line/paragraph with
        // wrap top+bottom or other wraps is affecting the line directly
        // above the anchor line, which seems odd, but a tiny adjustment
        // here to bring the top down convinces msoffice to wrap like us
        if (nPosYEMU == 0
            && (strcmp(relativeFromV, "line") == 0 || strcmp(relativeFromV, "paragraph") == 0)
            && (!alignV || strcmp(alignV, "top") == 0))
        {
            alignV = nullptr;
            nPosYEMU = 635;
        }

        if (alignV != nullptr)
        {
            m_pImpl->m_pSerializer->startElementNS(XML_wp, XML_align, FSEND);
            m_pImpl->m_pSerializer->write(alignV);
            m_pImpl->m_pSerializer->endElementNS(XML_wp, XML_align);
        }
        else
        {
            m_pImpl->m_pSerializer->startElementNS(XML_wp, XML_posOffset, FSEND);
            if (nPosYEMU > MAX_INTEGER_VALUE)
            {
                nPosYEMU = MAX_INTEGER_VALUE;
            }
            else if (nPosYEMU < MIN_INTEGER_VALUE)
            {
                nPosYEMU = MIN_INTEGER_VALUE;
            }
            m_pImpl->m_pSerializer->write(nPosYEMU);
            m_pImpl->m_pSerializer->endElementNS(XML_wp, XML_posOffset);
        }
        m_pImpl->m_pSerializer->endElementNS(XML_wp, XML_positionV);
    }
    else
    {
        sax_fastparser::FastAttributeList* aAttrList
            = sax_fastparser::FastSerializerHelper::createAttrList();
        aAttrList->add(XML_distT, OString::number(TwipsToEMU(aULSpaceItem.GetUpper())).getStr());
        aAttrList->add(XML_distB, OString::number(TwipsToEMU(aULSpaceItem.GetLower())).getStr());
        aAttrList->add(XML_distL, OString::number(TwipsToEMU(aLRSpaceItem.GetLeft())).getStr());
        aAttrList->add(XML_distR, OString::number(TwipsToEMU(aLRSpaceItem.GetRight())).getStr());
        const SdrObject* pObj = pFrameFormat->FindRealSdrObject();
        if (pObj != nullptr)
        {
            OUString sAnchorId = lclGetAnchorIdFromGrabBag(pObj);
            if (!sAnchorId.isEmpty())
                aAttrList->addNS(XML_wp14, XML_anchorId,
                                 OUStringToOString(sAnchorId, RTL_TEXTENCODING_UTF8));
        }
        m_pImpl->m_pSerializer->startElementNS(XML_wp, XML_inline, aAttrList);
    }

    // now the common parts
    // extent of the image
    /**
    * Extent width is of type long ( i.e cx & cy ) as
    *
    * per ECMA-376, Second Edition, Part 1 - Fundamentals And Markup Language Reference
    * [ 20.4.2.7 extent (Drawing Object Size)]
    *
    * cy is of type a:ST_PositiveCoordinate.
    * Minimum inclusive: 0
    * Maximum inclusive: 27273042316900
    *
    * reference : http://www.schemacentral.com/sc/ooxml/e-wp_extent-1.html
    *
    *   Though ECMA mentions the max value as aforementioned. It appears that MSO does not
    *  handle for the same, infact it actually can handles a max value of int32 i.e
    *   2147483647( MAX_INTEGER_VALUE ).
    *  Therefore changing the following accordingly so that LO sync's up with MSO.
    **/
    sal_uInt64 cx = 0;
    sal_uInt64 cy = 0;
    const sal_Int64 MAX_INTEGER_VALUE = SAL_MAX_INT32;

    // the 'Size' type uses 'long' for width and height, so on
    // platforms where 'long' is 32 bits they can obviously never be
    // larger than the max signed 32-bit integer.
#if SAL_TYPES_SIZEOFLONG > 4
    if (rSize.Width() > MAX_INTEGER_VALUE)
        cx = MAX_INTEGER_VALUE;
    else
#endif
    {
        if (0 > rSize.Width())
            cx = 0;
        else
            cx = rSize.Width();
    }

#if SAL_TYPES_SIZEOFLONG > 4
    if (rSize.Height() > MAX_INTEGER_VALUE)
        cy = MAX_INTEGER_VALUE;
    else
#endif
    {
        if (0 > rSize.Height())
            cy = 0;
        else
            cy = rSize.Height();
    }

    OString aWidth(OString::number(TwipsToEMU(cx)));
    //we explicitly check the converted EMU value for the range as mentioned in above comment.
    aWidth = (aWidth.toInt64() > 0 ? (aWidth.toInt64() > MAX_INTEGER_VALUE ? I64S(MAX_INTEGER_VALUE)
                                                                           : aWidth.getStr())
                                   : "0");
    OString aHeight(OString::number(TwipsToEMU(cy)));
    aHeight
        = (aHeight.toInt64() > 0 ? (aHeight.toInt64() > MAX_INTEGER_VALUE ? I64S(MAX_INTEGER_VALUE)
                                                                          : aHeight.getStr())
                                 : "0");

    m_pImpl->m_pSerializer->singleElementNS(XML_wp, XML_extent, XML_cx, aWidth, XML_cy, aHeight,
                                            FSEND);

    // effectExtent, extent including the effect (shadow only for now)
    m_pImpl->m_pSerializer->singleElementNS(
        XML_wp, XML_effectExtent, XML_l, OString::number(nLeftExt), XML_t, OString::number(nTopExt),
        XML_r, OString::number(nRightExt), XML_b, OString::number(nBottomExt), FSEND);

    // See if we know the exact wrap type from grab-bag.
    sal_Int32 nWrapToken = 0;
    if (const SdrObject* pObject = pFrameFormat->FindRealSdrObject())
    {
        uno::Any aAny;
        pObject->GetGrabBagItem(aAny);
        comphelper::SequenceAsHashMap aGrabBag(aAny);
        auto it = aGrabBag.find("EG_WrapType");
        if (it != aGrabBag.end())
        {
            auto sType = it->second.get<OUString>();
            if (sType == "wrapTight")
                nWrapToken = XML_wrapTight;
            else if (sType == "wrapThrough")
                nWrapToken = XML_wrapThrough;
            else
                SAL_WARN("sw.ww8",
                         "DocxSdrExport::startDMLAnchorInline: unexpected EG_WrapType value");

            m_pImpl->m_pSerializer->startElementNS(XML_wp, nWrapToken, XML_wrapText, "bothSides",
                                                   FSEND);

            it = aGrabBag.find("CT_WrapPath");
            if (it != aGrabBag.end())
            {
                m_pImpl->m_pSerializer->startElementNS(XML_wp, XML_wrapPolygon, XML_edited, "0",
                                                       FSEND);
                auto aSeqSeq = it->second.get<drawing::PointSequenceSequence>();
                auto aPoints(comphelper::sequenceToContainer<std::vector<awt::Point>>(aSeqSeq[0]));
                for (auto i = aPoints.begin(); i != aPoints.end(); ++i)
                {
                    awt::Point& rPoint = *i;
                    m_pImpl->m_pSerializer->singleElementNS(
                        XML_wp, (i == aPoints.begin() ? XML_start : XML_lineTo), XML_x,
                        OString::number(rPoint.X), XML_y, OString::number(rPoint.Y), FSEND);
                }
                m_pImpl->m_pSerializer->endElementNS(XML_wp, XML_wrapPolygon);
            }

            m_pImpl->m_pSerializer->endElementNS(XML_wp, nWrapToken);
        }
    }

    // Or if we have a contour.
    if (!nWrapToken && pFrameFormat->GetSurround().IsContour())
    {
        if (const SwNoTextNode* pNd = sw::util::GetNoTextNodeFromSwFrameFormat(*pFrameFormat))
        {
            const tools::PolyPolygon* pPolyPoly = pNd->HasContour();
            if (pPolyPoly && pPolyPoly->Count())
            {
                nWrapToken = XML_wrapTight;
                m_pImpl->m_pSerializer->startElementNS(XML_wp, nWrapToken, XML_wrapText,
                                                       "bothSides", FSEND);

                m_pImpl->m_pSerializer->startElementNS(XML_wp, XML_wrapPolygon, XML_edited, "0",
                                                       FSEND);
                tools::Polygon aPoly = sw::util::CorrectWordWrapPolygonForExport(*pPolyPoly, pNd);
                for (sal_uInt16 i = 0; i < aPoly.GetSize(); ++i)
                    m_pImpl->m_pSerializer->singleElementNS(
                        XML_wp, (i == 0 ? XML_start : XML_lineTo), XML_x,
                        OString::number(aPoly[i].X()), XML_y, OString::number(aPoly[i].Y()), FSEND);
                m_pImpl->m_pSerializer->endElementNS(XML_wp, XML_wrapPolygon);

                m_pImpl->m_pSerializer->endElementNS(XML_wp, nWrapToken);
            }
        }
    }

    // No? Then just approximate based on what we have.
    if (isAnchor && !nWrapToken)
    {
        switch (pFrameFormat->GetSurround().GetValue())
        {
            case css::text::WrapTextMode_NONE:
                m_pImpl->m_pSerializer->singleElementNS(XML_wp, XML_wrapTopAndBottom, FSEND);
                break;
            case css::text::WrapTextMode_THROUGH:
                m_pImpl->m_pSerializer->singleElementNS(XML_wp, XML_wrapNone, FSEND);
                break;
            case css::text::WrapTextMode_PARALLEL:
                m_pImpl->m_pSerializer->singleElementNS(XML_wp, XML_wrapSquare, XML_wrapText,
                                                        "bothSides", FSEND);
                break;
            case css::text::WrapTextMode_DYNAMIC:
            default:
                m_pImpl->m_pSerializer->singleElementNS(XML_wp, XML_wrapSquare, XML_wrapText,
                                                        "largest", FSEND);
                break;
        }
    }
}

void DocxSdrExport::endDMLAnchorInline(const SwFrameFormat* pFrameFormat)
{
    bool isAnchor;
    if (m_pImpl->m_bFlyFrameGraphic)
    {
        isAnchor = false; // end Inline Graphic object inside DMLTextFrame
    }
    else
    {
        isAnchor = pFrameFormat->GetAnchor().GetAnchorId() != RndStdIds::FLY_AS_CHAR;
    }
    m_pImpl->m_pSerializer->endElementNS(XML_wp, isAnchor ? XML_anchor : XML_inline);

    m_pImpl->m_pSerializer->endElementNS(XML_w, XML_drawing);
    m_pImpl->m_bDrawingOpen = false;
}

void DocxSdrExport::writeVMLDrawing(const SdrObject* sdrObj, const SwFrameFormat& rFrameFormat)
{
    m_pImpl->m_pSerializer->startElementNS(XML_w, XML_pict, FSEND);
    m_pImpl->m_pDrawingML->SetFS(m_pImpl->m_pSerializer);
    // See WinwordAnchoring::SetAnchoring(), these are not part of the SdrObject, have to be passed around manually.

    const SwFormatHoriOrient& rHoriOri = rFrameFormat.GetHoriOrient();
    const SwFormatVertOrient& rVertOri = rFrameFormat.GetVertOrient();
    m_pImpl->m_rExport.VMLExporter().AddSdrObject(
        *sdrObj, rHoriOri.GetHoriOrient(), rVertOri.GetVertOrient(), rHoriOri.GetRelationOrient(),
        rVertOri.GetRelationOrient(), true);
    m_pImpl->m_pSerializer->endElementNS(XML_w, XML_pict);
}

static bool lcl_isLockedCanvas(const uno::Reference<drawing::XShape>& xShape)
{
    bool bRet = false;
    uno::Sequence<beans::PropertyValue> propList = lclGetProperty(xShape, "InteropGrabBag");
    for (sal_Int32 nProp = 0; nProp < propList.getLength(); ++nProp)
    {
        OUString propName = propList[nProp].Name;
        if (propName == "LockedCanvas")
        {
            /*
             * Export as Locked Canvas only if the property
             * is in the PropertySet
             */
            bRet = true;
            break;
        }
    }
    return bRet;
}

void DocxSdrExport::writeDMLDrawing(const SdrObject* pSdrObject, const SwFrameFormat* pFrameFormat,
                                    int nAnchorId)
{
    uno::Reference<drawing::XShape> xShape(const_cast<SdrObject*>(pSdrObject)->getUnoShape(),
                                           uno::UNO_QUERY_THROW);
    if (!Impl::isSupportedDMLShape(xShape))
        return;

    m_pImpl->m_rExport.DocxAttrOutput().GetSdtEndBefore(pSdrObject);

    sax_fastparser::FSHelperPtr pFS = m_pImpl->m_pSerializer;
    Size aSize(pSdrObject->GetLogicRect().GetWidth(), pSdrObject->GetLogicRect().GetHeight());
    startDMLAnchorInline(pFrameFormat, aSize);

    sax_fastparser::FastAttributeList* pDocPrAttrList
        = sax_fastparser::FastSerializerHelper::createAttrList();
    pDocPrAttrList->add(XML_id, OString::number(nAnchorId).getStr());
    pDocPrAttrList->add(XML_name,
                        OUStringToOString(pSdrObject->GetName(), RTL_TEXTENCODING_UTF8).getStr());
    if (!pSdrObject->GetTitle().isEmpty())
        pDocPrAttrList->add(XML_title,
                            OUStringToOString(pSdrObject->GetTitle(), RTL_TEXTENCODING_UTF8));
    if (!pSdrObject->GetDescription().isEmpty())
        pDocPrAttrList->add(XML_descr,
                            OUStringToOString(pSdrObject->GetDescription(), RTL_TEXTENCODING_UTF8));
    sax_fastparser::XFastAttributeListRef xDocPrAttrListRef(pDocPrAttrList);
    pFS->singleElementNS(XML_wp, XML_docPr, xDocPrAttrListRef);

    uno::Reference<lang::XServiceInfo> xServiceInfo(xShape, uno::UNO_QUERY_THROW);
    const char* pNamespace = "http://schemas.microsoft.com/office/word/2010/wordprocessingShape";
    if (xServiceInfo->supportsService("com.sun.star.drawing.GroupShape"))
        pNamespace = "http://schemas.microsoft.com/office/word/2010/wordprocessingGroup";
    else if (xServiceInfo->supportsService("com.sun.star.drawing.GraphicObjectShape"))
        pNamespace = "http://schemas.openxmlformats.org/drawingml/2006/picture";
    pFS->startElementNS(
        XML_a, XML_graphic, FSNS(XML_xmlns, XML_a),
        OUStringToOString(m_pImpl->m_rExport.GetFilter().getNamespaceURL(OOX_NS(dml)),
                          RTL_TEXTENCODING_UTF8)
            .getStr(),
        FSEND);
    pFS->startElementNS(XML_a, XML_graphicData, XML_uri, pNamespace, FSEND);

    bool bLockedCanvas = lcl_isLockedCanvas(xShape);
    if (bLockedCanvas)
        pFS->startElementNS(XML_lc, XML_lockedCanvas, FSNS(XML_xmlns, XML_lc),
                            OUStringToOString(m_pImpl->m_rExport.GetFilter().getNamespaceURL(
                                                  OOX_NS(dmlLockedCanvas)),
                                              RTL_TEXTENCODING_UTF8)
                                .getStr(),
                            FSEND);

    m_pImpl->m_rExport.OutputDML(xShape);

    if (bLockedCanvas)
        pFS->endElementNS(XML_lc, XML_lockedCanvas);
    pFS->endElementNS(XML_a, XML_graphicData);
    pFS->endElementNS(XML_a, XML_graphic);

    // Relative size of the drawing.
    if (pSdrObject->GetRelativeWidth())
    {
        // At the moment drawinglayer objects are always relative from page.
        pFS->startElementNS(XML_wp14, XML_sizeRelH, XML_relativeFrom,
                            (pSdrObject->GetRelativeWidthRelation() == text::RelOrientation::FRAME
                                 ? "margin"
                                 : "page"),
                            FSEND);
        pFS->startElementNS(XML_wp14, XML_pctWidth, FSEND);
        pFS->writeEscaped(
            OUString::number(*pSdrObject->GetRelativeWidth() * 100 * oox::drawingml::PER_PERCENT));
        pFS->endElementNS(XML_wp14, XML_pctWidth);
        pFS->endElementNS(XML_wp14, XML_sizeRelH);
    }
    if (pSdrObject->GetRelativeHeight())
    {
        pFS->startElementNS(XML_wp14, XML_sizeRelV, XML_relativeFrom,
                            (pSdrObject->GetRelativeHeightRelation() == text::RelOrientation::FRAME
                                 ? "margin"
                                 : "page"),
                            FSEND);
        pFS->startElementNS(XML_wp14, XML_pctHeight, FSEND);
        pFS->writeEscaped(
            OUString::number(*pSdrObject->GetRelativeHeight() * 100 * oox::drawingml::PER_PERCENT));
        pFS->endElementNS(XML_wp14, XML_pctHeight);
        pFS->endElementNS(XML_wp14, XML_sizeRelV);
    }

    endDMLAnchorInline(pFrameFormat);
}

void DocxSdrExport::Impl::textFrameShadow(const SwFrameFormat& rFrameFormat)
{
    const SvxShadowItem& aShadowItem = rFrameFormat.GetShadow();
    if (aShadowItem.GetLocation() == SvxShadowLocation::NONE)
        return;

    OString aShadowWidth(OString::number(double(aShadowItem.GetWidth()) / 20) + "pt");
    OString aOffset;
    switch (aShadowItem.GetLocation())
    {
        case SvxShadowLocation::TopLeft:
            aOffset = "-" + aShadowWidth + ",-" + aShadowWidth;
            break;
        case SvxShadowLocation::TopRight:
            aOffset = aShadowWidth + ",-" + aShadowWidth;
            break;
        case SvxShadowLocation::BottomLeft:
            aOffset = "-" + aShadowWidth + "," + aShadowWidth;
            break;
        case SvxShadowLocation::BottomRight:
            aOffset = aShadowWidth + "," + aShadowWidth;
            break;
        case SvxShadowLocation::NONE:
        case SvxShadowLocation::End:
            break;
    }
    if (aOffset.isEmpty())
        return;

    OString aShadowColor = msfilter::util::ConvertColor(aShadowItem.GetColor());
    m_pSerializer->singleElementNS(XML_v, XML_shadow, XML_on, "t", XML_color, "#" + aShadowColor,
                                   XML_offset, aOffset, FSEND);
}

bool DocxSdrExport::Impl::isSupportedDMLShape(const uno::Reference<drawing::XShape>& xShape)
{
    uno::Reference<lang::XServiceInfo> xServiceInfo(xShape, uno::UNO_QUERY_THROW);
    if (xServiceInfo->supportsService("com.sun.star.drawing.PolyPolygonShape")
        || xServiceInfo->supportsService("com.sun.star.drawing.PolyLineShape"))
        return false;

    // For signature line shapes, we don't want DML, just the VML shape.
    bool bIsSignatureLineShape = false;
    if (xServiceInfo->supportsService("com.sun.star.drawing.GraphicObjectShape"))
    {
        uno::Reference<beans::XPropertySet> xShapeProperties(xShape, uno::UNO_QUERY);
        xShapeProperties->getPropertyValue("IsSignatureLine") >>= bIsSignatureLineShape;
        if (bIsSignatureLineShape)
            return false;
    }

    return true;
}

void DocxSdrExport::writeDMLAndVMLDrawing(const SdrObject* sdrObj,
                                          const SwFrameFormat& rFrameFormat, int nAnchorId)
{
    bool bDMLAndVMLDrawingOpen = m_pImpl->m_bDMLAndVMLDrawingOpen;
    m_pImpl->m_bDMLAndVMLDrawingOpen = true;

    // Depending on the shape type, we actually don't write the shape as DML.
    OUString sShapeType;
    ShapeFlag nMirrorFlags = ShapeFlag::NONE;
    uno::Reference<drawing::XShape> xShape(const_cast<SdrObject*>(sdrObj)->getUnoShape(),
                                           uno::UNO_QUERY_THROW);

    // Locked canvas is OK inside DML.
    if (lcl_isLockedCanvas(xShape))
        bDMLAndVMLDrawingOpen = false;

    MSO_SPT eShapeType
        = EscherPropertyContainer::GetCustomShapeType(xShape, nMirrorFlags, sShapeType);

    // In case we are already inside a DML block, then write the shape only as VML, turn out that's allowed to do.
    // A common service created in util to check for VML shapes which are allowed to have textbox in content
    if ((msfilter::util::HasTextBoxContent(eShapeType)) && Impl::isSupportedDMLShape(xShape)
        && !bDMLAndVMLDrawingOpen)
    {
        m_pImpl->m_pSerializer->startElementNS(XML_mc, XML_AlternateContent, FSEND);

        auto pObjGroup = dynamic_cast<const SdrObjGroup*>(sdrObj);
        m_pImpl->m_pSerializer->startElementNS(XML_mc, XML_Choice, XML_Requires,
                                               (pObjGroup ? "wpg" : "wps"), FSEND);
        writeDMLDrawing(sdrObj, &rFrameFormat, nAnchorId);
        m_pImpl->m_pSerializer->endElementNS(XML_mc, XML_Choice);

        m_pImpl->m_pSerializer->startElementNS(XML_mc, XML_Fallback, FSEND);
        writeVMLDrawing(sdrObj, rFrameFormat);
        m_pImpl->m_pSerializer->endElementNS(XML_mc, XML_Fallback);

        m_pImpl->m_pSerializer->endElementNS(XML_mc, XML_AlternateContent);
    }
    else
        writeVMLDrawing(sdrObj, rFrameFormat);

    m_pImpl->m_bDMLAndVMLDrawingOpen = false;
}

// Converts ARGB transparency (0..255) to drawingml alpha (opposite, and 0..100000)
static OString lcl_ConvertTransparency(const Color& rColor)
{
    if (rColor.GetTransparency() > 0)
    {
        sal_Int32 nTransparencyPercent = 100 - float(rColor.GetTransparency()) / 2.55;
        return OString::number(nTransparencyPercent * oox::drawingml::PER_PERCENT);
    }

    return OString();
}

void DocxSdrExport::writeDMLEffectLst(const SwFrameFormat& rFrameFormat)
{
    const SvxShadowItem& aShadowItem = rFrameFormat.GetShadow();

    // Output effects
    if (aShadowItem.GetLocation() == SvxShadowLocation::NONE)
        return;

    // Distance is measured diagonally from corner
    double nShadowDist
        = sqrt(static_cast<double>(aShadowItem.GetWidth()) * aShadowItem.GetWidth() * 2.0);
    OString aShadowDist(OString::number(TwipsToEMU(nShadowDist)));
    OString aShadowColor = msfilter::util::ConvertColor(aShadowItem.GetColor());
    OString aShadowAlpha = lcl_ConvertTransparency(aShadowItem.GetColor());
    sal_uInt32 nShadowDir = 0;
    switch (aShadowItem.GetLocation())
    {
        case SvxShadowLocation::TopLeft:
            nShadowDir = 13500000;
            break;
        case SvxShadowLocation::TopRight:
            nShadowDir = 18900000;
            break;
        case SvxShadowLocation::BottomLeft:
            nShadowDir = 8100000;
            break;
        case SvxShadowLocation::BottomRight:
            nShadowDir = 2700000;
            break;
        case SvxShadowLocation::NONE:
        case SvxShadowLocation::End:
            break;
    }
    OString aShadowDir(OString::number(nShadowDir));

    m_pImpl->m_pSerializer->startElementNS(XML_a, XML_effectLst, FSEND);
    m_pImpl->m_pSerializer->startElementNS(XML_a, XML_outerShdw, XML_dist, aShadowDist.getStr(),
                                           XML_dir, aShadowDir.getStr(), FSEND);
    if (aShadowAlpha.isEmpty())
        m_pImpl->m_pSerializer->singleElementNS(XML_a, XML_srgbClr, XML_val, aShadowColor.getStr(),
                                                FSEND);
    else
    {
        m_pImpl->m_pSerializer->startElementNS(XML_a, XML_srgbClr, XML_val, aShadowColor.getStr(),
                                               FSEND);
        m_pImpl->m_pSerializer->singleElementNS(XML_a, XML_alpha, XML_val, aShadowAlpha.getStr(),
                                                FSEND);
        m_pImpl->m_pSerializer->endElementNS(XML_a, XML_srgbClr);
    }
    m_pImpl->m_pSerializer->endElementNS(XML_a, XML_outerShdw);
    m_pImpl->m_pSerializer->endElementNS(XML_a, XML_effectLst);
}

void DocxSdrExport::writeDiagram(const SdrObject* sdrObject, const SwFrameFormat& rFrameFormat,
                                 int nDiagramId)
{
    uno::Reference<drawing::XShape> xShape(const_cast<SdrObject*>(sdrObject)->getUnoShape(),
                                           uno::UNO_QUERY);

    // write necessary tags to document.xml
    Size aSize(sdrObject->GetSnapRect().GetWidth(), sdrObject->GetSnapRect().GetHeight());
    startDMLAnchorInline(&rFrameFormat, aSize);

    m_pImpl->getDrawingML()->WriteDiagram(xShape, nDiagramId);

    endDMLAnchorInline(&rFrameFormat);
}

void DocxSdrExport::writeOnlyTextOfFrame(ww8::Frame const* pParentFrame)
{
    const SwFrameFormat& rFrameFormat = pParentFrame->GetFrameFormat();
    const SwNodeIndex* pNodeIndex = rFrameFormat.GetContent().GetContentIdx();
    sax_fastparser::FSHelperPtr pFS = m_pImpl->m_pSerializer;

    sal_uLong nStt = pNodeIndex ? pNodeIndex->GetIndex() + 1 : 0;
    sal_uLong nEnd = pNodeIndex ? pNodeIndex->GetNode().EndOfSectionIndex() : 0;

    //Save data here and restore when out of scope
    ExportDataSaveRestore aDataGuard(m_pImpl->m_rExport, nStt, nEnd, pParentFrame);

    m_pImpl->m_pBodyPrAttrList = sax_fastparser::FastSerializerHelper::createAttrList();
    m_pImpl->m_bFrameBtLr
        = m_pImpl->checkFrameBtlr(m_pImpl->m_rExport.m_pDoc->GetNodes()[nStt], /*bDML=*/true);
    m_pImpl->m_bFlyFrameGraphic = true;
    m_pImpl->m_rExport.WriteText();
    m_pImpl->m_bFlyFrameGraphic = false;
    m_pImpl->m_bFrameBtLr = false;
}

void DocxSdrExport::writeBoxItemLine(const SvxBoxItem& rBox)
{
    const editeng::SvxBorderLine* pBorderLine = nullptr;

    if (rBox.GetTop())
    {
        pBorderLine = rBox.GetTop();
    }
    else if (rBox.GetLeft())
    {
        pBorderLine = rBox.GetLeft();
    }
    else if (rBox.GetBottom())
    {
        pBorderLine = rBox.GetBottom();
    }
    else if (rBox.GetRight())
    {
        pBorderLine = rBox.GetRight();
    }

    if (!pBorderLine)
    {
        return;
    }

    sax_fastparser::FSHelperPtr pFS = m_pImpl->m_pSerializer;
    double fConverted(editeng::ConvertBorderWidthToWord(pBorderLine->GetBorderLineStyle(),
                                                        pBorderLine->GetWidth()));
    OString sWidth(OString::number(TwipsToEMU(fConverted)));
    pFS->startElementNS(XML_a, XML_ln, XML_w, sWidth.getStr(), FSEND);

    pFS->startElementNS(XML_a, XML_solidFill, FSEND);
    OString sColor(msfilter::util::ConvertColor(pBorderLine->GetColor()));
    pFS->singleElementNS(XML_a, XML_srgbClr, XML_val, sColor, FSEND);
    pFS->endElementNS(XML_a, XML_solidFill);

    if (SvxBorderLineStyle::DASHED == pBorderLine->GetBorderLineStyle()) // Line Style is Dash type
        pFS->singleElementNS(XML_a, XML_prstDash, XML_val, "dash", FSEND);

    pFS->endElementNS(XML_a, XML_ln);
}

void DocxSdrExport::writeDMLTextFrame(ww8::Frame const* pParentFrame, int nAnchorId,
                                      bool bTextBoxOnly)
{
    bool bDMLAndVMLDrawingOpen = m_pImpl->m_bDMLAndVMLDrawingOpen;
    m_pImpl->m_bDMLAndVMLDrawingOpen = IsAnchorTypeInsideParagraph(pParentFrame);

    sax_fastparser::FSHelperPtr pFS = m_pImpl->m_pSerializer;
    const SwFrameFormat& rFrameFormat = pParentFrame->GetFrameFormat();
    const SwNodeIndex* pNodeIndex = rFrameFormat.GetContent().GetContentIdx();

    sal_uLong nStt = pNodeIndex ? pNodeIndex->GetIndex() + 1 : 0;
    sal_uLong nEnd = pNodeIndex ? pNodeIndex->GetNode().EndOfSectionIndex() : 0;

    //Save data here and restore when out of scope
    ExportDataSaveRestore aDataGuard(m_pImpl->m_rExport, nStt, nEnd, pParentFrame);

    // When a frame has some low height, but automatically expanded due
    // to lots of contents, this size contains the real size.
    const Size aSize = pParentFrame->GetSize();

    uno::Reference<drawing::XShape> xShape;
    const SdrObject* pSdrObj = rFrameFormat.FindRealSdrObject();
    if (pSdrObj)
        xShape.set(const_cast<SdrObject*>(pSdrObj)->getUnoShape(), uno::UNO_QUERY);
    uno::Reference<beans::XPropertySet> xPropertySet(xShape, uno::UNO_QUERY);
    uno::Reference<beans::XPropertySetInfo> xPropSetInfo;
    if (xPropertySet.is())
        xPropSetInfo = xPropertySet->getPropertySetInfo();

    m_pImpl->m_pBodyPrAttrList = sax_fastparser::FastSerializerHelper::createAttrList();
    {
        drawing::TextVerticalAdjust eAdjust = drawing::TextVerticalAdjust_TOP;
        if (xPropSetInfo.is() && xPropSetInfo->hasPropertyByName("TextVerticalAdjust"))
            xPropertySet->getPropertyValue("TextVerticalAdjust") >>= eAdjust;
        m_pImpl->m_pBodyPrAttrList->add(XML_anchor, oox::drawingml::GetTextVerticalAdjust(eAdjust));
    }

    if (!bTextBoxOnly)
    {
        startDMLAnchorInline(&rFrameFormat, aSize);

        sax_fastparser::FastAttributeList* pDocPrAttrList
            = sax_fastparser::FastSerializerHelper::createAttrList();
        pDocPrAttrList->add(XML_id, OString::number(nAnchorId).getStr());
        pDocPrAttrList->add(
            XML_name, OUStringToOString(rFrameFormat.GetName(), RTL_TEXTENCODING_UTF8).getStr());
        sax_fastparser::XFastAttributeListRef xDocPrAttrListRef(pDocPrAttrList);
        pFS->singleElementNS(XML_wp, XML_docPr, xDocPrAttrListRef);

        pFS->startElementNS(
            XML_a, XML_graphic, FSNS(XML_xmlns, XML_a),
            OUStringToOString(m_pImpl->m_rExport.GetFilter().getNamespaceURL(OOX_NS(dml)),
                              RTL_TEXTENCODING_UTF8)
                .getStr(),
            FSEND);
        pFS->startElementNS(XML_a, XML_graphicData, XML_uri,
                            "http://schemas.microsoft.com/office/word/2010/wordprocessingShape",
                            FSEND);
        pFS->startElementNS(XML_wps, XML_wsp, FSEND);
        pFS->singleElementNS(XML_wps, XML_cNvSpPr, XML_txBox, "1", FSEND);

        uno::Any aRotation;
        m_pImpl->m_nDMLandVMLTextFrameRotation = 0;
        if (xPropSetInfo.is() && xPropSetInfo->hasPropertyByName("FrameInteropGrabBag"))
        {
            uno::Sequence<beans::PropertyValue> propList;
            xPropertySet->getPropertyValue("FrameInteropGrabBag") >>= propList;
            for (sal_Int32 nProp = 0; nProp < propList.getLength(); ++nProp)
            {
                OUString propName = propList[nProp].Name;
                if (propName == "mso-rotation-angle")
                {
                    aRotation = propList[nProp].Value;
                    break;
                }
            }
        }
        aRotation >>= m_pImpl->m_nDMLandVMLTextFrameRotation;
        OString sRotation(OString::number(
            (OOX_DRAWINGML_EXPORT_ROTATE_CLOCKWISIFY(m_pImpl->m_nDMLandVMLTextFrameRotation))));
        // Shape properties
        pFS->startElementNS(XML_wps, XML_spPr, FSEND);
        if (m_pImpl->m_nDMLandVMLTextFrameRotation)
        {
            pFS->startElementNS(XML_a, XML_xfrm, XML_rot, sRotation.getStr(), FSEND);
        }
        else
        {
            pFS->startElementNS(XML_a, XML_xfrm, FSEND);
        }
        pFS->singleElementNS(XML_a, XML_off, XML_x, "0", XML_y, "0", FSEND);
        OString aWidth(OString::number(TwipsToEMU(aSize.Width())));
        OString aHeight(OString::number(TwipsToEMU(aSize.Height())));
        pFS->singleElementNS(XML_a, XML_ext, XML_cx, aWidth.getStr(), XML_cy, aHeight.getStr(),
                             FSEND);
        pFS->endElementNS(XML_a, XML_xfrm);
        OUString shapeType = "rect";
        if (xPropSetInfo.is() && xPropSetInfo->hasPropertyByName("FrameInteropGrabBag"))
        {
            uno::Sequence<beans::PropertyValue> propList;
            xPropertySet->getPropertyValue("FrameInteropGrabBag") >>= propList;
            for (sal_Int32 nProp = 0; nProp < propList.getLength(); ++nProp)
            {
                OUString propName = propList[nProp].Name;
                if (propName == "mso-orig-shape-type")
                {
                    propList[nProp].Value >>= shapeType;
                    break;
                }
            }
        }
        //Empty shapeType will lead to corruption so to avoid that shapeType is set to default i.e. "rect"
        if (shapeType.isEmpty())
            shapeType = "rect";

        pFS->singleElementNS(XML_a, XML_prstGeom, XML_prst,
                             OUStringToOString(shapeType, RTL_TEXTENCODING_UTF8).getStr(), FSEND);
        m_pImpl->m_bDMLTextFrameSyntax = true;
        m_pImpl->m_rExport.OutputFormat(pParentFrame->GetFrameFormat(), false, false, true);
        m_pImpl->m_bDMLTextFrameSyntax = false;
        writeDMLEffectLst(rFrameFormat);
        pFS->endElementNS(XML_wps, XML_spPr);
    }

    //first, loop through ALL of the chained textboxes to identify a unique ID for each chain, and sequence number for each textbox in that chain.
    if (!m_pImpl->m_rExport.m_bLinkedTextboxesHelperInitialized)
    {
        sal_Int32 nSeq = 0;
        for (auto& rEntry : m_pImpl->m_rExport.m_aLinkedTextboxesHelper)
        {
            //find the start of a textbox chain: has no PREVIOUS link, but does have NEXT link
            if (rEntry.second.sPrevChain.isEmpty() && !rEntry.second.sNextChain.isEmpty())
            {
                //assign this chain a unique ID and start a new sequence
                nSeq = 0;
                rEntry.second.nId = ++m_pImpl->m_rExport.m_nLinkedTextboxesChainId;
                rEntry.second.nSeq = nSeq;

                OUString sCheckForBrokenChains = rEntry.first;

                //follow the chain and assign the same id, and incremental sequence numbers.
                std::map<OUString, MSWordExportBase::LinkedTextboxInfo>::iterator followChainIter;
                followChainIter
                    = m_pImpl->m_rExport.m_aLinkedTextboxesHelper.find(rEntry.second.sNextChain);
                while (followChainIter != m_pImpl->m_rExport.m_aLinkedTextboxesHelper.end())
                {
                    //verify that the NEXT textbox also points to me as the PREVIOUS.
                    // A broken link indicates a leftover remnant that can be ignored.
                    if (followChainIter->second.sPrevChain != sCheckForBrokenChains)
                        break;

                    followChainIter->second.nId = m_pImpl->m_rExport.m_nLinkedTextboxesChainId;
                    followChainIter->second.nSeq = ++nSeq;

                    //empty next chain indicates the end of the linked chain.
                    if (followChainIter->second.sNextChain.isEmpty())
                        break;

                    sCheckForBrokenChains = followChainIter->first;
                    followChainIter = m_pImpl->m_rExport.m_aLinkedTextboxesHelper.find(
                        followChainIter->second.sNextChain);
                }
            }
        }
        m_pImpl->m_rExport.m_bLinkedTextboxesHelperInitialized = true;
    }

    m_pImpl->m_rExport.m_pParentFrame = nullptr;
    bool skipTxBxContent = false;
    bool isTxbxLinked = false;

    OUString sLinkChainName;
    if (xPropSetInfo.is())
    {
        if (xPropSetInfo->hasPropertyByName("LinkDisplayName"))
            xPropertySet->getPropertyValue("LinkDisplayName") >>= sLinkChainName;
        else if (xPropSetInfo->hasPropertyByName("ChainName"))
            xPropertySet->getPropertyValue("ChainName") >>= sLinkChainName;
    }

    // second, check if THIS textbox is linked and then decide whether to write the tag txbx or linkedTxbx
    std::map<OUString, MSWordExportBase::LinkedTextboxInfo>::iterator linkedTextboxesIter
        = m_pImpl->m_rExport.m_aLinkedTextboxesHelper.find(sLinkChainName);
    if (linkedTextboxesIter != m_pImpl->m_rExport.m_aLinkedTextboxesHelper.end())
    {
        if ((linkedTextboxesIter->second.nId != 0) && (linkedTextboxesIter->second.nSeq != 0))
        {
            //not the first in the chain, so write the tag as linkedTxbx
            pFS->singleElementNS(XML_wps, XML_linkedTxbx, XML_id,
                                 I32S(linkedTextboxesIter->second.nId), XML_seq,
                                 I32S(linkedTextboxesIter->second.nSeq), FSEND);
            /* no text content should be added to this tag,
               since the textbox is linked, the entire content
               is written in txbx block
            */
            skipTxBxContent = true;
        }
        else if ((linkedTextboxesIter->second.nId != 0) && (linkedTextboxesIter->second.nSeq == 0))
        {
            /* this is the first textbox in the chaining, we add the text content
               to this block*/
            //since the text box is linked, it needs an id.
            pFS->startElementNS(XML_wps, XML_txbx, XML_id, I32S(linkedTextboxesIter->second.nId),
                                FSEND);
            isTxbxLinked = true;
        }
    }

    if (!skipTxBxContent)
    {
        if (!isTxbxLinked)
            pFS->startElementNS(XML_wps, XML_txbx,
                                FSEND); //text box is not linked, therefore no id.

        pFS->startElementNS(XML_w, XML_txbxContent, FSEND);

        m_pImpl->m_bFrameBtLr
            = m_pImpl->checkFrameBtlr(m_pImpl->m_rExport.m_pDoc->GetNodes()[nStt], /*bDML=*/true);
        m_pImpl->m_bFlyFrameGraphic = true;
        m_pImpl->m_rExport.WriteText();
        if (m_pImpl->m_bParagraphSdtOpen)
        {
            m_pImpl->m_rExport.DocxAttrOutput().EndParaSdtBlock();
            m_pImpl->m_bParagraphSdtOpen = false;
        }
        m_pImpl->m_bFlyFrameGraphic = false;
        m_pImpl->m_bFrameBtLr = false;

        pFS->endElementNS(XML_w, XML_txbxContent);
        pFS->endElementNS(XML_wps, XML_txbx);
    }

    // We need to init padding to 0, if it's not set.
    // In LO the default is 0 and so ins attributes are not set when padding is 0
    // but in MSO the default is 254 / 127, so we need to set 0 padding explicitly
    if (m_pImpl->m_pBodyPrAttrList)
    {
        if (!m_pImpl->m_pBodyPrAttrList->hasAttribute(XML_lIns))
            m_pImpl->m_pBodyPrAttrList->add(XML_lIns, OString::number(0));
        if (!m_pImpl->m_pBodyPrAttrList->hasAttribute(XML_tIns))
            m_pImpl->m_pBodyPrAttrList->add(XML_tIns, OString::number(0));
        if (!m_pImpl->m_pBodyPrAttrList->hasAttribute(XML_rIns))
            m_pImpl->m_pBodyPrAttrList->add(XML_rIns, OString::number(0));
        if (!m_pImpl->m_pBodyPrAttrList->hasAttribute(XML_bIns))
            m_pImpl->m_pBodyPrAttrList->add(XML_bIns, OString::number(0));
    }

    sax_fastparser::XFastAttributeListRef xBodyPrAttrList(m_pImpl->m_pBodyPrAttrList);
    m_pImpl->m_pBodyPrAttrList = nullptr;
    if (!bTextBoxOnly)
    {
        pFS->startElementNS(XML_wps, XML_bodyPr, xBodyPrAttrList);
        // AutoSize of the Text Frame.
        const SwFormatFrameSize& rSize = rFrameFormat.GetFrameSize();
        pFS->singleElementNS(
            XML_a, (rSize.GetHeightSizeType() == ATT_VAR_SIZE ? XML_spAutoFit : XML_noAutofit),
            FSEND);
        pFS->endElementNS(XML_wps, XML_bodyPr);

        pFS->endElementNS(XML_wps, XML_wsp);
        pFS->endElementNS(XML_a, XML_graphicData);
        pFS->endElementNS(XML_a, XML_graphic);

        // Relative size of the Text Frame.
        const sal_uInt8 nWidthPercent = rSize.GetWidthPercent();
        if (nWidthPercent && nWidthPercent != SwFormatFrameSize::SYNCED)
        {
            pFS->startElementNS(XML_wp14, XML_sizeRelH, XML_relativeFrom,
                                (rSize.GetWidthPercentRelation() == text::RelOrientation::PAGE_FRAME
                                     ? "page"
                                     : "margin"),
                                FSEND);
            pFS->startElementNS(XML_wp14, XML_pctWidth, FSEND);
            pFS->writeEscaped(OUString::number(nWidthPercent * oox::drawingml::PER_PERCENT));
            pFS->endElementNS(XML_wp14, XML_pctWidth);
            pFS->endElementNS(XML_wp14, XML_sizeRelH);
        }
        const sal_uInt8 nHeightPercent = rSize.GetHeightPercent();
        if (nHeightPercent && nHeightPercent != SwFormatFrameSize::SYNCED)
        {
            pFS->startElementNS(
                XML_wp14, XML_sizeRelV, XML_relativeFrom,
                (rSize.GetHeightPercentRelation() == text::RelOrientation::PAGE_FRAME ? "page"
                                                                                      : "margin"),
                FSEND);
            pFS->startElementNS(XML_wp14, XML_pctHeight, FSEND);
            pFS->writeEscaped(OUString::number(nHeightPercent * oox::drawingml::PER_PERCENT));
            pFS->endElementNS(XML_wp14, XML_pctHeight);
            pFS->endElementNS(XML_wp14, XML_sizeRelV);
        }

        endDMLAnchorInline(&rFrameFormat);
    }
    m_pImpl->m_bDMLAndVMLDrawingOpen = bDMLAndVMLDrawingOpen;
}

void DocxSdrExport::writeVMLTextFrame(ww8::Frame const* pParentFrame, bool bTextBoxOnly)
{
    bool bDMLAndVMLDrawingOpen = m_pImpl->m_bDMLAndVMLDrawingOpen;
    m_pImpl->m_bDMLAndVMLDrawingOpen = IsAnchorTypeInsideParagraph(pParentFrame);

    sax_fastparser::FSHelperPtr pFS = m_pImpl->m_pSerializer;
    const SwFrameFormat& rFrameFormat = pParentFrame->GetFrameFormat();
    const SwNodeIndex* pNodeIndex = rFrameFormat.GetContent().GetContentIdx();

    sal_uLong nStt = pNodeIndex ? pNodeIndex->GetIndex() + 1 : 0;
    sal_uLong nEnd = pNodeIndex ? pNodeIndex->GetNode().EndOfSectionIndex() : 0;

    //Save data here and restore when out of scope
    ExportDataSaveRestore aDataGuard(m_pImpl->m_rExport, nStt, nEnd, pParentFrame);

    // When a frame has some low height, but automatically expanded due
    // to lots of contents, this size contains the real size.
    const Size aSize = pParentFrame->GetSize();
    m_pImpl->m_pFlyFrameSize = &aSize;

    m_pImpl->m_bTextFrameSyntax = true;
    m_pImpl->m_pFlyAttrList = sax_fastparser::FastSerializerHelper::createAttrList();
    m_pImpl->m_pTextboxAttrList = sax_fastparser::FastSerializerHelper::createAttrList();
    m_pImpl->m_aTextFrameStyle = "position:absolute";

    uno::Reference< drawing::XShape > xShape;
    const SdrObject* pSdrObj = rFrameFormat.FindRealSdrObject();
    if (pSdrObj)
        xShape.set(const_cast<SdrObject*>(pSdrObj)->getUnoShape(), uno::UNO_QUERY);
    uno::Reference< beans::XPropertySet > xPropertySet(xShape, uno::UNO_QUERY);
    uno::Reference< beans::XPropertySetInfo > xPropSetInfo;
    if (xPropertySet.is())
        xPropSetInfo = xPropertySet->getPropertySetInfo();

    bool bIsVertical = false;
    if (xPropSetInfo.is() && xPropSetInfo->hasPropertyByName("WritingMode"))
    {
        short nWritingDirection = text::WritingMode2::LR_TB;
        xPropertySet->getPropertyValue("WritingMode") >>= nWritingDirection;
        if(nWritingDirection == text::WritingMode2::TB_RL)
            bIsVertical = true;
    }

    if (!bTextBoxOnly)
    {
        OString sRotation(OString::number(m_pImpl->m_nDMLandVMLTextFrameRotation / -100));
        m_pImpl->m_rExport.SdrExporter().getTextFrameStyle().append(";rotation:").append(sRotation);
    }
    m_pImpl->m_rExport.OutputFormat(pParentFrame->GetFrameFormat(), false, false, true);
    m_pImpl->m_pFlyAttrList->add(XML_style, m_pImpl->m_aTextFrameStyle.makeStringAndClear());

    const SdrObject* pObject = pParentFrame->GetFrameFormat().FindRealSdrObject();
    if (pObject != nullptr)
    {
        OUString sAnchorId = lclGetAnchorIdFromGrabBag(pObject);
        if (!sAnchorId.isEmpty())
            m_pImpl->m_pFlyAttrList->addNS(XML_w14, XML_anchorId,
                                           OUStringToOString(sAnchorId, RTL_TEXTENCODING_UTF8));
    }
    sax_fastparser::XFastAttributeListRef xFlyAttrList(m_pImpl->m_pFlyAttrList.get());
    m_pImpl->m_pFlyAttrList.clear();
    m_pImpl->m_bFrameBtLr = m_pImpl->checkFrameBtlr(m_pImpl->m_rExport.m_pDoc->GetNodes()[nStt], /*bDML=*/false, bIsVertical);
    sax_fastparser::XFastAttributeListRef xTextboxAttrList(m_pImpl->m_pTextboxAttrList.get());
    m_pImpl->m_pTextboxAttrList.clear();
    m_pImpl->m_bTextFrameSyntax = false;
    m_pImpl->m_pFlyFrameSize = nullptr;
    m_pImpl->m_rExport.m_pParentFrame = nullptr;

    if (!bTextBoxOnly)
    {
        pFS->startElementNS(XML_w, XML_pict, FSEND);
        pFS->startElementNS(XML_v, XML_rect, xFlyAttrList);
        m_pImpl->textFrameShadow(rFrameFormat);
        if (m_pImpl->m_pFlyFillAttrList.is())
        {
            sax_fastparser::XFastAttributeListRef xFlyFillAttrList(
                m_pImpl->m_pFlyFillAttrList.get());
            m_pImpl->m_pFlyFillAttrList.clear();
            pFS->singleElementNS(XML_v, XML_fill, xFlyFillAttrList);
        }
        if (m_pImpl->m_pDashLineStyleAttr.is())
        {
            sax_fastparser::XFastAttributeListRef xDashLineStyleAttr(
                m_pImpl->m_pDashLineStyleAttr.get());
            m_pImpl->m_pDashLineStyleAttr.clear();
            pFS->singleElementNS(XML_v, XML_stroke, xDashLineStyleAttr);
        }
        pFS->startElementNS(XML_v, XML_textbox, xTextboxAttrList);
    }
    pFS->startElementNS(XML_w, XML_txbxContent, FSEND);
    m_pImpl->m_bFlyFrameGraphic = true;
    m_pImpl->m_rExport.WriteText();
    if (m_pImpl->m_bParagraphSdtOpen)
    {
        m_pImpl->m_rExport.DocxAttrOutput().EndParaSdtBlock();
        m_pImpl->m_bParagraphSdtOpen = false;
    }
    m_pImpl->m_bFlyFrameGraphic = false;
    pFS->endElementNS(XML_w, XML_txbxContent);
    if (!bTextBoxOnly)
    {
        pFS->endElementNS(XML_v, XML_textbox);

        if (m_pImpl->m_pFlyWrapAttrList)
        {
            sax_fastparser::XFastAttributeListRef xFlyWrapAttrList(m_pImpl->m_pFlyWrapAttrList);
            m_pImpl->m_pFlyWrapAttrList = nullptr;
            pFS->singleElementNS(XML_w10, XML_wrap, xFlyWrapAttrList);
        }

        pFS->endElementNS(XML_v, XML_rect);
        pFS->endElementNS(XML_w, XML_pict);
    }
    m_pImpl->m_bFrameBtLr = false;

    m_pImpl->m_bDMLAndVMLDrawingOpen = bDMLAndVMLDrawingOpen;
}

bool DocxSdrExport::Impl::checkFrameBtlr(SwNode* pStartNode, bool bDML, bool bVertical)
{
    // The intended usage is to pass either a valid VML or DML attribute list.
    if (bDML)
        assert(m_pBodyPrAttrList);
    else
        assert(m_pTextboxAttrList.is());

    if (!pStartNode->IsTextNode())
        return false;

    SwTextNode* pTextNode = pStartNode->GetTextNode();

    if (bVertical)
    {
        if (bDML)
            m_pBodyPrAttrList->add(XML_vert, "vert270");
        else
            m_pTextboxAttrList->add(XML_style, "layout-flow:vertical-ideographic");
        return true;
    }
    return false;
}

bool DocxSdrExport::isTextBox(const SwFrameFormat& rFrameFormat)
{
    return SwTextBoxHelper::isTextBox(&rFrameFormat, RES_FLYFRMFMT);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
