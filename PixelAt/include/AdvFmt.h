/* This file was created by Avtem using Visual Studio IDE.
 * 
 * Created: 12/18/2021 6:00:16 PM 
 * Version 1.0.0:	dd.mm.yyyy*/

#pragma once

#include <string>
#include <av.h>

std::wstring processAdvFmt(cwstr fmt, const std::vector <int>& args);

namespace AdvFmt
{
    const WCHAR printfRegEx[] = L"^%[+-0 #]*([0-9]{0,2}|\\*)?(\\.([0-9]+|\\*))?[diuoxXfFeEgGaAcn]";

    // returns number of characters it processed
    int parseFmt(cwstr fmt, std::wstring& strOut); 
}