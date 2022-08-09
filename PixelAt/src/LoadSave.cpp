#include "LoadSave.h"

// av stuff
#include <resource.h>
#include <av.h>
#include <sstream>

static AvIni ini(L"settings.ini");
static HWND wndMain = 0;
static ListView* listView = nullptr;

void onHScroll(HWND wnd = 0); // on Zoom Slider change
static std::vector<std::wstring>splitStr(const std::wstring& str, const WCHAR delim);

//
// CODE ↓↓↓
//

void setLoadSaveVars(HWND wnd, ListView* lv)
{
    ::wndMain = wnd;
    ::listView = lv;
}

SIZE getWndSize(HWND wnd)
{
    return { av::getWndWidth(wnd), av::getWndHeight(wnd) };
}

void loadWndPos(AvTrayIcon & trayIcon, bool noSize)
{
    av::setTopmostStyle(wndMain, ini.loadBoolean(L"Is topmost", false));

    WINDOWPLACEMENT wndPlac{0};
    ini.loadBinary(L"Window placement", &wndPlac, sizeof(wndPlac));
    // loading failed. Position window by default
    if(GetLastError() != ERROR_SUCCESS)
    {
        ShowWindow(wndMain, SW_SHOW);
        return;
    }
    
// BLOCK1
    // This block (BLOCK1) ensures that user will always be able to move the main window. 
    // Imagine what happens if window was positioned in other monitor which
    // no longer accessible while loading (window will be nowhere and there is no way
    // user will be able to change this)
    RECT* wndRect = &wndPlac.rcNormalPosition;
    const int btnTitleCount = 3 +2; // 3 (min,max,close) + 2 (totray,topmost)
    const int horzOffset = GetSystemMetrics(SM_CXSIZE) *btnTitleCount;
    POINT ptLeftTop = { wndRect->left, wndRect->top };
    POINT ptRightTop = { wndRect->right -horzOffset, wndRect->top };

    HMONITOR currMonitorPtL = MonitorFromPoint(ptLeftTop, MONITOR_DEFAULTTONULL);
    HMONITOR currMonitorPtR = MonitorFromPoint(ptRightTop, MONITOR_DEFAULTTONULL);
    if (!currMonitorPtL && !currMonitorPtR)
    {
        // returning will cause main window display in the center of primary monitor
        ShowWindow(wndMain, SW_SHOW);
        return;
    }
// end of BLOCK1


    if(ini.loadBoolean(L"In tray", false))
    {
        wndPlac.showCmd = SW_HIDE; // avBug: forces wnd to show, because i don't like when app starts minimized
        trayIcon.show();
    }

    // "don't load" window size
    SIZE normSize = getWndSize(wndMain);
    if(noSize)
    {
        RECT& r = wndPlac.rcNormalPosition;
        r.right  = r.left +normSize.cx;
        r.bottom = r.top  +normSize.cy;
    }
    SetWindowPlacement(wndMain, &wndPlac);
}

std::vector<std::wstring> splitStr(const std::wstring& str, const WCHAR delim)
{
    std::vector<std::wstring> res;
    res.reserve(4);

    // parse presets
    std::wstring info;
    const WCHAR* ptr = str.data();
    while (*ptr)
    {
        if (*ptr != delim)
        {
            info.push_back(*ptr);
        }
        else
        {
            res.push_back(info);
            info.clear();
        }

        ++ptr;
    }
    res.push_back(info); // push last elem

    return res;
}

void saveWndPos()
{
    WINDOWPLACEMENT wndPlac;
    wndPlac.length = sizeof(wndPlac);
    GetWindowPlacement(wndMain, &wndPlac);
    
    ini.saveBinary(L"Window placement", &wndPlac, sizeof(wndPlac));
    ini.saveBoolean(L"Is topmost", av::isTopMost(wndMain));
    ini.saveBoolean(L"In tray", IsWindowVisible(wndMain) == false);
}

void saveZoomPercentage()
{
    int zoomSliderPos = SendMessage(GetDlgItem(wndMain, IDC_ZOOM_SLIDER), TBM_GETPOS, 0, 0);
    ini.saveString(L"Zoom slider pos", std::to_wstring(zoomSliderPos).c_str());
}

void loadLVColWidths()
{
    WCHAR key[100]{ 0 };

    for (int i = 0; i < 3; ++i)
    {
        wsprintf(key, L"colWidth #%i", i);
        int w = stoi(ini.loadString(key, L"-1"));
        // there was no setting.ini file or something
        if(w == -1)
            return;

        listView->setColWidth(i, w);
    }
}

void saveLVColWidths()
{
    WCHAR key[100]{ 0 };
    WCHAR width[100]{ 0 };

    for(int i=0; i < 3; ++i)
    {
        wsprintf(key, L"colWidth #%i", i);
        wsprintf(width, L"%i", listView->getColWidth(i));
        ini.saveString(key, width);
    }
}

void loadLVItems()
{
    int presetCount = stoi(ini.loadString(L"Preset Count", L"0", L"Presets"));

    static const wchar_t separator = L'\4'; // EOT (End of text)
    for (int i = 0; i < presetCount; ++i)
    {
        static WCHAR key[10]{ 0 };
        swprintf_s(key, sizeof(key), L"%02i", i);

        std::wstring pres = ini.loadString(key, L"\4\4\4\4", L"Presets");
        auto vec = splitStr(pres, separator);
        cwstr subItems[] = {vec[1].c_str(), vec[2].c_str()};
        listView->addItem(vec[0].c_str(), subItems, 2);
        listView->setCheckedState(i, stoi(vec[3]));
    }
}

void saveLVItems()
{
    // clear "Preset" section
    ini.clearSection(L"Presets");

    int presetCount = listView->getItemCount();
    ini.saveString(L"Preset Count", std::to_wstring(presetCount).c_str(), L"Presets");

    for(int i=0; i < presetCount; ++i)
    {
        static WCHAR key[10]{ 0 };
        swprintf_s(key, sizeof(key), L"%02i", i);
        
        static WCHAR preset[2048]{ 0 };
        static const wchar_t separator = L'\4'; // EOT (End of text)
        swprintf_s(preset, sizeof(preset)/sizeof(preset[0]), L"%s\4%s\4%s\4%i\4", 
                   listView->getItem(i, 0).c_str(), listView->getItem(i, 1).c_str(),
                   listView->getItem(i, 2).c_str(), listView->isItemChecked(i));

        ini.saveString(key, preset, L"Presets");
    }
}

void loadLVSelectionInd()
{
    listView->setCurrItem(ini.loadInt(L"Selected item index", -1));
}

void saveLVSelectionInd()
{
    ini.saveInt(L"Selected item index", listView->getCurrIndex());
}

void loadZoomPercentage()
{
    std::wstring percStr = ini.loadString(L"Zoom slider pos", L"0");
    int zoomSliderPos = std::stoi(percStr);
    SendMessage(GetDlgItem(wndMain, IDC_ZOOM_SLIDER), TBM_SETPOS, true, zoomSliderPos);
    onHScroll();
}

void saveAllSettings()
{
    saveWndPos();

    saveZoomPercentage();
    saveLVColWidths();
    saveLVItems();
    saveLVSelectionInd();
}

void loadAllSettings(AvTrayIcon& trayIcon)
{
    loadWndPos(trayIcon);

    loadZoomPercentage();
    loadLVColWidths();
    loadLVItems();
    loadLVSelectionInd(); // must be after loading items
}
