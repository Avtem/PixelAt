#include "AdvFmt.h"
#include <regex>

constexpr int ERR_FMT = -2;

std::wstring processAdvFmt(cwstr fmt, const std::vector<int>& args)
{
    std::wstring res;
    std::wstring tempFmt;

    // process whole Format string provided by user
    size_t argI = 0;
    for(size_t i=0; i < wcslen(fmt); ++i)
    {
        if(fmt[i] != L'%') // push back normal characters
            res.push_back(fmt[i]);
        else if(fmt[i] == L'%' && fmt[i+1] && fmt[i+1] == L'%') // escaped '%' char
            res.push_back(fmt[++i]);
        else // ok, we got a format thingy
        {            
            int funcRes = AdvFmt::parseFmt(&fmt[i], tempFmt);
            if(funcRes == ERR_FMT) // ignore bad formats
                break;
            
            i += funcRes;
            // do printf stuff
            static WCHAR buff[200]{0};
            buff[0] = 0;
            if(argI < args.size())
            {
                swprintf_s(buff, 200, tempFmt.c_str(), args.at(argI));
                ++argI;
            }
            else
                swprintf_s(buff, 200, L"#ERR_NEA#");
            
            res.append(buff);
        }
    }
    if(args.size() > argI)
        res.append(L"#ERR_TMA#");

    return res;
}

// returns number of characters it processed
int AdvFmt::parseFmt(cwstr fmt, std::wstring& strOut)
{
    static std::wregex pattern(printfRegEx);
    std::wcmatch matches;
    std::regex_search(fmt, matches, pattern);
    if(matches.empty())
    {
        strOut = L"#ERR_SPEC#"; // something wrong happened        
        return ERR_FMT;
    }
    else
    {
        strOut = matches.str(0);
        return strOut.size() -1;
    }
}