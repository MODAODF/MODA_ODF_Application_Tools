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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <msiquery.h>

#include <cassert>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <strsafe.h>
#include <sstream>

#include "seterror.hxx"


static std::wstring GetMsiPropertyW( MSIHANDLE handle, const std::wstring& sProperty )
{
    std::wstring   result;
    WCHAR szDummy[1] = L"";
    DWORD nChars = 0;

    if ( MsiGetPropertyW( handle, sProperty.c_str(), szDummy, &nChars ) == ERROR_MORE_DATA )
    {
        DWORD nBytes = ++nChars * sizeof(WCHAR);
        PWSTR buffer = static_cast<PWSTR>(_alloca(nBytes));
        ZeroMemory( buffer, nBytes );
        MsiGetPropertyW( handle, sProperty.c_str(), buffer, &nChars );
        result = buffer;
    }

    return  result;
}


#ifdef DEBUG
inline void OutputDebugStringFormatW( PCWSTR pFormat, ... )
{
    WCHAR    buffer[1024];
    va_list  args;

    va_start( args, pFormat );
    StringCchVPrintfW( buffer, sizeof(buffer)/sizeof(buffer[0]), pFormat, args );
    OutputDebugStringW( buffer );
    va_end(args);
}
#else
static void OutputDebugStringFormatW( PCWSTR, ... )
{
}
#endif

static void CustomRegistration(LPCWSTR lpSubKey, LPCWSTR lpValueName,
                                LPCWSTR oldValue, LPCWSTR newValue)
{
    HKEY    hKey = nullptr;
    LONG    lResult = RegOpenKeyExW( HKEY_CLASSES_ROOT, lpSubKey, 0,
                                     KEY_QUERY_VALUE|KEY_SET_VALUE, &hKey );
    if ( ERROR_SUCCESS == lResult )
    {
        WCHAR   szBuffer[1024];
        DWORD   nSize = sizeof( szBuffer );
        lResult = RegQueryValueExW( hKey, lpValueName, nullptr, nullptr, reinterpret_cast<LPBYTE>(szBuffer), &nSize );
        if ( ERROR_SUCCESS == lResult )
        {
            szBuffer[nSize/sizeof(*szBuffer)] = L'\0';
            if ( wcscmp( szBuffer, oldValue) == 0)
            {
                size_t iLen = wcslen(newValue);
                WCHAR valBuffer[50];
                wcsncpy(valBuffer, newValue, iLen+1);
                RegSetValueExW( hKey, lpValueName, 0,
                                REG_SZ, reinterpret_cast<LPBYTE>(valBuffer), sizeof(valBuffer) );
            }
        }
        RegCloseKey( hKey );
    }
}

static void MODA_Registration(MSIHANDLE handle, LPCWSTR appname, LPCWSTR appEXE)
{
    std::wstring sOfficeInstallPath = GetMsiPropertyW(handle, L"INSTALLLOCATION");
    std::wstring sKey = L"Local Settings\\Software\\Microsoft\\Windows\\Shell\\MuiCache";
    std::wstring OldValueA = L"National Development Council";
    std::wstring NewValueA = L"Ministry of Digital Affairs";

    std::wstring appName(appname);
    std::wstring OldValue_OfficeF = L"NDC ODF Application Tools" + appName;
    std::wstring NewValue_OfficeF = L"MODA ODF 應用工具" + appName;

    std::wstring SubKey_officeA = sOfficeInstallPath + L"program\\" + appEXE + L".exe.ApplicationCompany";
    std::wstring SubKey_officeF = sOfficeInstallPath + L"program\\" + appEXE + L".exe.FriendlyAppName";

    CustomRegistration(sKey.c_str(), SubKey_officeA.c_str(), OldValueA.c_str(), NewValueA.c_str());
    CustomRegistration(sKey.c_str(), SubKey_officeF.c_str(), OldValue_OfficeF.c_str(), NewValue_OfficeF.c_str());
}

extern "C" __declspec(dllexport) UINT __stdcall RegistryCheck( MSIHANDLE handle )
{
    std::wstring sOfficeInstallPath = GetMsiPropertyW(handle, L"INSTALLLOCATION");
    // MessageBoxW(NULL, sOfficeInstallPath.c_str(), L"RegistryCheck", MB_OK | MB_ICONINFORMATION);

    auto RegValue = [](HKEY hRoot, const WCHAR* sKey, const WCHAR* sVal) {
        std::wstring sResult;
        WCHAR buf[32767]; // max longpath
        DWORD bufsize = sizeof(buf); // yes, it is the number of bytes
        if (RegGetValueW(hRoot, sKey, sVal, RRF_RT_REG_SZ, nullptr, buf, &bufsize) == ERROR_SUCCESS)
            sResult = buf; // RegGetValueW null-terminates strings

        return sResult;
    };

    MODA_Registration(handle, L" Base", L"sbase");
    MODA_Registration(handle, L" Calc", L"scalc");
    MODA_Registration(handle, L" Writer", L"swriter");
    MODA_Registration(handle, L" Impress", L"simpress");
    MODA_Registration(handle, L" Draw", L"sdraw");
    MODA_Registration(handle, L"", L"soffice");

    return ERROR_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
