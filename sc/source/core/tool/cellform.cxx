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

#include <cellform.hxx>

#include <sfx2/objsh.hxx>
#include <svl/zforlist.hxx>
#include <svl/sharedstring.hxx>

#include <formulacell.hxx>
#include <document.hxx>
#include <cellvalue.hxx>
#include <formula/errorcodes.hxx>
#include <sc.hrc>
#include <editutil.hxx>

void ScCellFormat::GetString( ScRefCellValue& rCell, sal_uInt32 nFormat, OUString& rString,
                              Color** ppColor, SvNumberFormatter& rFormatter, const ScDocument* pDoc,
                              bool bNullVals, bool bFormula, bool bUseStarFormat, bool bZeroToMinus )
{
    *ppColor = nullptr;

    switch (rCell.meType)
    {
        case CELLTYPE_STRING:
            rFormatter.GetOutputString(rCell.mpString->getString(), nFormat, rString, ppColor, bUseStarFormat);
        break;
        case CELLTYPE_EDIT:
            rFormatter.GetOutputString(rCell.getString(pDoc), nFormat, rString, ppColor );
        break;
        case CELLTYPE_VALUE:
        {
            const double & nValue = rCell.mfValue;
            // show zero to minus
            if (bZeroToMinus && nValue == 0.0)
                rString = "-";

            if (!bNullVals && nValue == 0.0)
                rString.clear();
            else
                rFormatter.GetOutputString( nValue, nFormat, rString, ppColor, bUseStarFormat );
        }
        break;
        case CELLTYPE_FORMULA:
        {
            ScFormulaCell*  pFCell = rCell.mpFormula;
            if ( bFormula )
            {
                pFCell->GetFormula( rString );
            }
            else
            {
                // A macro started from the interpreter, which has
                // access to Formula Cells, becomes a CellText, even if
                // that triggers further interpretation, except if those
                // cells are already being interpreted.
                // IdleCalc generally doesn't trigger further interpretation,
                // as not to get Err522 (circular).
                if ( pFCell->GetDocument()->IsInInterpreter() &&
                        (!pFCell->GetDocument()->GetMacroInterpretLevel()
                        || pFCell->IsRunning()) )
                {
                    rString = "...";
                }
                else
                {
                    const FormulaError nErrCode = pFCell->GetErrCode();

                    if (nErrCode != FormulaError::NONE)
                        rString = ScGlobal::GetErrorString(nErrCode);
                    else if ( pFCell->IsEmptyDisplayedAsString() )
                        rString.clear();
                    else if ( pFCell->IsValue() )
                    {
                        double fValue = pFCell->GetValue();
                        if ( !bNullVals && fValue == 0.0 )
                            rString.clear();
                        else
                            rFormatter.GetOutputString( fValue, nFormat, rString, ppColor, bUseStarFormat );
                    }
                    else
                    {
                        rFormatter.GetOutputString( pFCell->GetString().getString(),
                                                    nFormat, rString, ppColor, bUseStarFormat );
                    }
                }
            }
        }
        break;
        default:
            rString.clear();
            break;
    }
}

OUString ScCellFormat::GetString(
    ScDocument& rDoc, const ScAddress& rPos, sal_uInt32 nFormat, Color** ppColor,
    SvNumberFormatter& rFormatter, bool bNullVals, bool bFormula, bool bZeroToMinus )
{
    OUString aString;
    *ppColor = nullptr;

    ScRefCellValue aCell(rDoc, rPos);
    GetString(aCell, nFormat, aString, ppColor, rFormatter, &rDoc, bNullVals, bFormula, bZeroToMinus);
    return aString;
}

void ScCellFormat::GetInputString(
    ScRefCellValue& rCell, sal_uInt32 nFormat, OUString& rString, SvNumberFormatter& rFormatter, const ScDocument* pDoc )
{
    switch (rCell.meType)
    {
        case CELLTYPE_STRING:
        case CELLTYPE_EDIT:
            rString = rCell.getString(pDoc);
        break;
        case CELLTYPE_VALUE:
            rFormatter.GetInputLineString(rCell.mfValue, nFormat, rString );
        break;
        case CELLTYPE_FORMULA:
        {
            ScFormulaCell* pFC = rCell.mpFormula;
            if (pFC->IsEmptyDisplayedAsString())
                rString = EMPTY_OUSTRING;
            else if (pFC->IsValue())
                rFormatter.GetInputLineString(pFC->GetValue(), nFormat, rString);
            else
                rString = pFC->GetString().getString();

            const FormulaError nErrCode = pFC->GetErrCode();
            if (nErrCode != FormulaError::NONE)
                rString = EMPTY_OUSTRING;
        }
        break;
        default:
            rString = EMPTY_OUSTRING;
            break;
    }
}

OUString ScCellFormat::GetOutputString( ScDocument& rDoc, const ScAddress& rPos, ScRefCellValue& rCell )
{
    if (rCell.isEmpty())
        return EMPTY_OUSTRING;

    OUString aVal;

    if (rCell.meType == CELLTYPE_EDIT)
    {
        //  GetString converts line breaks into spaces in EditCell,
        //  but here we need the line breaks
        const EditTextObject* pData = rCell.mpEditText;
        if (pData)
        {
            ScFieldEditEngine& rEngine = rDoc.GetEditEngine();
            rEngine.SetText(*pData);
            aVal = rEngine.GetText();
        }
        //  also do not format EditCells as numbers
        //  (fitting to output)
    }
    else
    {
        //  like in GetString for document (column)
        Color* pColor;
        sal_uInt32 nNumFmt = rDoc.GetNumberFormat(rPos);
        GetString(rCell, nNumFmt, aVal, &pColor, *rDoc.GetFormatTable(), &rDoc);
    }
    return aVal;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
