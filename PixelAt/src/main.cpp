// ====== This project was created from "AvApp" template 
// ====== version with dialog 1.1.0 (released 02.04.2021).
// ====== Created: 5/6/2021 8:30:31 PM
// ====== 
// ====== <3 WinAPI <3
/*
    Depends on "av.dll" library version $Insert_current_av_dll_version_just_before_release$
    30.09.2021 - added source control. Yay!!
*/

// manifest for supporting themes
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <vector>
#include <HtmlHelp.h>

// av stuff
#include <av.h>
#include "../res/rc files/resource.h"
#include <ListView.h>
#include "LoadSave.h"
#include "AdvFmt.h"

// ### Classes, structs
struct DisplayingData 
{
    COLORREF color;
    HBITMAP frozenBitmap;
    POINT screenshotXY;
    POINT cursorPos;
    POINT frozenCursorOffset;  // when user uses WASD while it's frozen
    bool frozen;
    bool colorByPicker;

    void setColor(const COLORREF &Color);
    void onColorChanged();
};

// ### global variables
const wchar_t appName[] = L"PixelAt";
const wchar_t releaseDate[] = L"05.02.2022";
constexpr int HOTKEY_FREEZE = 1;
HICON appIcon = 0;
HWND mainWnd = 0;
HWND tooltip = 0;
DisplayingData dispData = {0};
HWND currColorWnd = 0; 
HWND zoomMaxPercentage = 0;
HWND resultStrWnd = 0;
HDC dcZoomArea = 0;
HDC dcMemTemp = 0;
HDC dcMainWnd = 0;
HDC dcCurrColor = 0;
HDC dcScreen = 0;
WNDPROC zoomAreaOldProc = 0;
WNDPROC currColorOldProc = 0;
WNDPROC zoomSliderOldProc = 0;
RECT staticWndRect = {0};
HHOOK mouseHook = 0;
TOOLINFO toolInfo = {0};
int pixelThickness = 1;
AvTitleBtn toTrayBtn;
AvTitleBtn onTopBtn;
ListView* listView = nullptr;
const int tooltipTimerID  = 1;
const int redrawTimerID   = 2;
const int keyboardTimerID = 3;
const int btnDownTimerID = 4;
const int AVM_HOOK_MESSAGE = WM_USER;
std::vector <int> pixelSizes;
HFONT fontArial = CreateFont(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L"Arial");

// ### function declarations / definitions
BOOL CALLBACK mainDialogProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
BOOL CALLBACK aboutDialogProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
LRESULT CALLBACK mouseHookProc(int code, WPARAM wparam, LPARAM lparam);
LRESULT CALLBACK zoomAreaProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
LRESULT CALLBACK currColorProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
LRESULT CALLBACK zoomSliderProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
void onInitDialog(HWND wnd);
void onCommand(WPARAM wparam, LPARAM lparam);
void onHScroll(HWND wnd = 0); // on Zoom Slider change
void saveScreenshot();
void onDestroy();
void onHotkey(int id);
void registerHotkeys();
void unregisterHotkeys();
POINT getMoveDist(RECT& moveDist, bool speedy, bool up, bool down, bool left, bool right);
void onAddItem();
void moveCursor(const POINT& dist);
void setButtonTexts();
void tryToOpenChm(cwstr fileName, cwstr strToAppend = L"");
void onMoveDownUp(UINT controlID);
void onAbout();
void toggleFreeze();
void onBtnTimer();
void onHelpHotkeys();
void onMousePosChanged();
std::vector<int> convertToIntArgs(const std::vector<char>& charArgs);
void onFrozenChanged();
void onTimer(WPARAM wparam);
void copyMousePosToClipboard();
void copyResultStrToClipboard();
std::vector <int> getArgs();
cwstr getFmt();
void showTooltipCopied(UINT controlID);
void assignVariables(HWND hwnd);
void hideTooltipCopied();
void copyFileToTempDir(cwstr fileName);
POINT getFinalMousePos();
int getFinalX();
int getFinalY();
void setColorByPicker(bool colorByPicker);
void onKeyDown(); 
void onRedrawTimer();
void onColorPicker();
void createTooltipCopied();
void displayMousePos();
void displayRGBValues();
void updateCurrColor();
void drawZoomedArea();
void onMouseHook();
void onVersionHistory();
void onHelpFmtBtn();
BOOL onEndSession(HWND wnd, BOOL fEnding);
BOOL onNotify(HWND hwnd, INT idCtrl, NMHDR* pnmh);
void updateMousePos(); // cursor pos changed (KB or MOUSE). Try to assign new value
void updateResultCtl();
void fillPixelSizes();
void resizeZoomedAreaControl(HWND wnd);
void drawHudInZoomedArea(const HDC &hdc, const float & controlSize);
void initZoomSlider(HWND wnd);
INT_PTR onCtlColorStatic(WPARAM wparam, LPARAM lparam);
HWND dlgWnd(const int & windowID, HWND wnd = 0);  // get any window from the dialog!
HBITMAP drawZoomedAreaBmp();  // delete the bitmap after use!
int getZoomAreaSize();
bool focusedControlIsEdit();
bool keyMsgShouldBeSuspended(const MSG &msg);
std::wstring processFormatStr();
std::wstring processSimpleFmt();
std::wstring processAdvancedFormat();

// ### application main entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, char* args, int nCmdShow)
{
    (void)nCmdShow, args, hInstance;

    CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOG_MAIN), 0, mainDialogProc);

    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0))
    {
        if(msg.message == WM_KEYDOWN)
        {
            if (keyMsgShouldBeSuspended(msg))
                continue;
            if(msg.wParam == VK_F1 && !(GetKeyState(VK_CONTROL) < 0))
                tryToOpenChm(L"userGuide.chm");
        }

        if(false == IsDialogMessage(mainWnd, &msg))
        {
            DispatchMessage(&msg);
            TranslateMessage(&msg);
        }
    }

    return 0;
}

// ### window procedure definition
BOOL CALLBACK mainDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:         onInitDialog(hwnd);            return true;
        case WM_COMMAND:            onCommand(wParam, lParam);     return true;
        case WM_TIMER:              onTimer(wParam);               return 0;
        case WM_HOTKEY:             onHotkey(wParam);              return 0;
        case WM_CTLCOLORSTATIC:          return onCtlColorStatic(wParam, lParam); 
        case WM_HSCROLL:            onHScroll();                   return true;
        case AVM_HOOK_MESSAGE:      onMouseHook();                 return true;
        HANDLE_MSG(hwnd, WM_NOTIFY, onNotify);
        HANDLE_MSG(hwnd, WM_ENDSESSION, onEndSession);
    }

    return false; // we return TRUE if we processe a message 
                  // and FALSE if we do not.
}

BOOL CALLBACK aboutDialogProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    (void)lparam;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            AvVersionInfo info;
            std::wstring ver = L"Version: " + std::wstring(info.productVersion());
            std::wstring rel = L"Released: " + std::wstring(releaseDate);
            SetDlgItemTextW(hwnd, IDC_TXT_VERSION,  ver.c_str());
            SetDlgItemTextW(hwnd, IDC_TXT_RELEASED, rel.c_str());
        } 
            break;
        case WM_COMMAND:
            if(wparam == IDOK || wparam == IDCANCEL)
                EndDialog(hwnd, 0);
            return true;
    }

    return false;
}

LRESULT CALLBACK mouseHookProc(int code, WPARAM wparam, LPARAM lparam)
{
    if(code == HC_ACTION && !dispData.frozen) // mouse moved, pressed, released, etc.!
    {
        PostMessage(mainWnd, AVM_HOOK_MESSAGE, 0, 0);
    }

    return CallNextHookEx(0, code, wparam, lparam);
}

LRESULT CALLBACK zoomAreaProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch(message)
    {
        case WM_ERASEBKGND: return 1;
        case WM_PAINT:
        {
            ValidateRect(hwnd, nullptr);
            
            HBITMAP bmp = drawZoomedAreaBmp();
            SelectObject(dcMemTemp, bmp);
            BitBlt(dcZoomArea, 0, 0, getZoomAreaSize(), getZoomAreaSize(),
                   dcMemTemp, 0, 0, SRCCOPY);
            DeleteObject(bmp);            
        } return 0;
    }

    return CallWindowProc(zoomAreaOldProc, hwnd, message, wparam, lparam);
}

LRESULT CALLBACK currColorProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{ 
    switch(message)
    {
        case WM_ERASEBKGND:  return 1;
        case WM_PAINT:
        {
            ValidateRect(hwnd, nullptr);
            HBRUSH currBrush = CreateSolidBrush(dispData.color);
            FillRect(dcCurrColor, &staticWndRect, currBrush);
            DeleteObject(currBrush);
        } return 0;
    }

    return CallWindowProc(currColorOldProc, hwnd, message, wparam, lparam);
}

LRESULT CALLBACK zoomSliderProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    if(message == WM_MOUSEWHEEL)
        wparam = MAKEWPARAM(GET_KEYSTATE_WPARAM(wparam), -GET_WHEEL_DELTA_WPARAM(wparam));

    return CallWindowProc(zoomSliderOldProc, hwnd, message, wparam, lparam);
}

void onInitDialog(HWND wnd)
{
    mainWnd = wnd;
    listView = new ListView(GetDlgItem(wnd, IDC_LISTVIEW));

    // create windows
    assignVariables(wnd);
    toTrayBtn.createToTray(wnd, 1);
    onTopBtn.createToggleTopmost(wnd, 2);
    createTooltipCopied();
    SetTimer(wnd, redrawTimerID, 50, 0);
    SetTimer(wnd, keyboardTimerID, 30, 0);
    SetTimer(wnd, btnDownTimerID, 30, 0);

    // change appearance
    SendMessage(wnd, WM_SETICON, ICON_BIG, (LPARAM) appIcon);
    setButtonTexts();

    resizeZoomedAreaControl(wnd);
    initZoomSlider(wnd);
    onHScroll(wnd);

    // init hooks
    mouseHook = SetWindowsHookEx(WH_MOUSE_LL, mouseHookProc, 0, 0);
    
    setLoadSaveVars(wnd, listView);
    loadAllSettings(*toTrayBtn.getTrayIcon()); // this function can't be called earlier!

    copyFileToTempDir(L"userGuide.chm");
    copyFileToTempDir(L"versionHistory.chm");

    registerHotkeys();
}

void onCommand(WPARAM wparam, LPARAM lparam)
{
    (void) lparam;

    uint controlID = LOWORD(wparam);
    switch(controlID)
    {
        case IDC_MOVE_DOWN:
        case IDC_MOVE_UP:        onMoveDownUp(controlID);       break;
        case ID_FILE_EXIT:
        case IDOK:
        case IDCANCEL:           onDestroy();                   break;
        case ID_HELP_ABOUT:      onAbout();                     break;
        case IDC_EDIT_ITEM:      listView->editCurrItem();      break;
        case IDC_COLOR_PICKER:   onColorPicker();               break;
        case IDC_COPY_MOUSEPOS:  copyMousePosToClipboard();     break;
        case IDC_FREEZE:         onFrozenChanged();             break;
        case IDC_COPY_RESULT:    copyResultStrToClipboard();    break;
        case ID_HELP_USERGUIDE:  onHelpFmtBtn();                break;
        case ID_HELP_HOTKEYS:    onHelpHotkeys();               break;
        case ID_HELP_VERSIONHISTORY: onVersionHistory();        break;
        case IDC_ADD_FTM:        onAddItem();                   break;
        case IDC_DELETE_FMT:     listView->deleteItem();        break;
    }
}


void onHScroll(HWND wnd)
{
    if(!wnd)
        wnd = mainWnd;

    int index = SendMessage(dlgWnd(IDC_ZOOM_SLIDER, wnd), TBM_GETPOS, 0, 0);
    pixelThickness = 1;
    if(index >= 0 && index < (int)pixelSizes.size())
        pixelThickness = pixelSizes.at(index);

    // set current zoom static text
    wchar_t text[20] = {0};
    wsprintf(text, L"&Zoom: %ix", (int) pixelThickness);
    SetDlgItemText(wnd, IDC_MAX_PERCENTAGE, text);

    drawZoomedArea();
}


void saveScreenshot()
{
    // copy whole thingy!
    int controlSize = getZoomAreaSize();
    dispData.frozenBitmap = CreateCompatibleBitmap(dcScreen, controlSize, controlSize);
    SelectObject(dcMemTemp, dispData.frozenBitmap);
    
    dispData.screenshotXY = {dispData.cursorPos.x -controlSize/2, 
                             dispData.cursorPos.y -controlSize/2};
    BitBlt(dcMemTemp, 0, 0, controlSize, controlSize, 
           dcScreen, dispData.screenshotXY.x, dispData.screenshotXY.y, SRCCOPY);
}

INT_PTR onCtlColorStatic(WPARAM wparam, LPARAM lparam)
{
    if ((HWND)lparam == dlgWnd(IDC_RVALUE))
        SetTextColor((HDC)wparam, av::red);
    else if (lparam == (LPARAM)dlgWnd(IDC_GVALUE))
        SetTextColor((HDC)wparam, av::darkGreen);
    else if (lparam == (LPARAM)dlgWnd(IDC_BVALUE))
        SetTextColor((HDC)wparam, av::blue);

    SetBkColor((HDC)wparam, GetSysColor(COLOR_BTNFACE));
    return (INT_PTR)GetSysColorBrush(COLOR_BTNFACE);
}

void onDestroy()
{
    UnhookWindowsHookEx(mouseHook);
    unregisterHotkeys();
    
    saveAllSettings();

    KillTimer(mainWnd, tooltipTimerID);
    KillTimer(mainWnd, redrawTimerID);
    KillTimer(mainWnd, keyboardTimerID);
    KillTimer(mainWnd, btnDownTimerID);

    ReleaseDC(mainWnd, dcMainWnd);
    ReleaseDC(dlgWnd(IDC_ZOOMED_RECT), dcZoomArea);
    ReleaseDC(dlgWnd(IDC_COLOR_RECT), dcCurrColor);
    ReleaseDC(HWND_DESKTOP, dcScreen);
    DeleteDC(dcMemTemp);
    delete listView;

    DeleteObject(fontArial);

    PostQuitMessage(0);
}

void onHotkey(int id)
{
    switch (id)
    {
        case HOTKEY_FREEZE:     toggleFreeze();   break;
    }
}

void registerHotkeys()
{
    BOOL res = true;
    res &= RegisterHotKey(mainWnd, HOTKEY_FREEZE, MOD_CONTROL, VK_F1);

    if(!res)
        MessageBox(mainWnd, L"Could not register global hotkey!",
                   L"Error", MB_ICONERROR);
}

void unregisterHotkeys()
{
    UnregisterHotKey(mainWnd, HOTKEY_FREEZE);
}

POINT getMoveDist(RECT& moveDist, bool speedy, bool up, bool down, 
                  bool left, bool right)
{
    int x = 0, y = 0;   // distance the cursor will move by this call
    int dist = speedy ? 4 : 1;  // one-call step length

    // w count, a count, etc.
    long& wc = moveDist.top;
    long& sc = moveDist.bottom;
    long& ac = moveDist.left;
    long& dc = moveDist.right;

    // after 15 "ticks" we move without stopping
    constexpr int waitCount = 15;

    // increase btn holding counter
    up    ? ++wc : wc = 0;
    left  ? ++ac : ac = 0;
    down  ? ++sc : sc = 0;
    right ? ++dc : dc = 0;

    // pressing SHIFT disables waiting ↓↓ +making smooth diag.movement
    bool moving = speedy || wc > waitCount || ac > waitCount
        || sc > waitCount || dc > waitCount;
    if (moving && wc > 1 && wc <= waitCount)
        wc = waitCount;
    if (moving && ac > 1 && ac <= waitCount)
        ac = waitCount;
    if (moving && sc > 1 && sc <= waitCount)
        sc = waitCount;
    if (moving && dc > 1 && dc <= waitCount)
        dc = waitCount;

    // calculate dist.to move in x & y axes for this call
    if (wc == 1 || wc > waitCount)
        y -= dist;
    if (ac == 1 || ac > waitCount)
        x -= dist;
    if (sc == 1 || sc > waitCount)
        y += dist;
    if (dc == 1 || dc > waitCount)
        x += dist;

    return { x, y };
}

void onAddItem()
{
    listView->addItem(L"New");
    
    listView->setCurrItem(listView->getItemCount() -1);
    listView->editCurrItem();
}

void moveCursor(const POINT& dist)
{
    if (dispData.frozen)
    {
        DisplayingData& dp = dispData;
        BITMAP bmp = { 0 };
        GetObject(dp.frozenBitmap, sizeof(BITMAP), &bmp);

        POINT newPos = {
            min(dp.screenshotXY.x + bmp.bmWidth -1, // 1 off rule
                max(dp.screenshotXY.x, getFinalX() +dist.x)),
            min(dp.screenshotXY.y + bmp.bmHeight -1, // 1 off rule
                max(dp.screenshotXY.y, getFinalY() +dist.y))
        };

        if (NULL == MonitorFromPoint(newPos, MONITOR_DEFAULTTONULL))
            return;

        dp.frozenCursorOffset.x = newPos.x -dp.cursorPos.x;
        dp.frozenCursorOffset.y = newPos.y -dp.cursorPos.y;
        updateCurrColor();
    }
    else
    {
        POINT currPos = av::getMousePos();
        SetCursorPos(currPos.x +dist.x, currPos.y +dist.y);
    }

    updateMousePos(); // we have to call it because SetCursorPos ↑↑ won't trigger hook
}

void setButtonTexts()
{
    SetDlgItemTextW(mainWnd, IDC_MOVE_DOWN, L"▼");
    SetDlgItemTextW(mainWnd, IDC_MOVE_UP, L"▲");


    // cursor poss
    for(int i=IDC_CUR_UP; i <= IDC_CUR_DOWN; ++i)
        SendMessage(dlgWnd(i), WM_SETFONT, (WPARAM)fontArial, false);

    SetDlgItemTextW(mainWnd, IDC_CUR_DOWN, L"▼");
    SetDlgItemTextW(mainWnd, IDC_CUR_UP,   L"▲");
    SetDlgItemTextW(mainWnd, IDC_CUR_LEFT, L"◄");
    SetDlgItemTextW(mainWnd, IDC_CUR_RIGHT,L"►");
}

void tryToOpenChm(cwstr fileName, cwstr strToAppend)
{
    WCHAR* buff;
    SHGetKnownFolderPath(FOLDERID_ProgramData, 0, nullptr, &buff);
    std::wstring newPath(buff);
    CoTaskMemFree(buff);

    newPath.append(L"\\AvApp\\");
    newPath.append(appName);
    newPath.push_back(L'\\');
    newPath.append(fileName);

    if(*strToAppend)
        newPath.append(strToAppend);

    cwstr path = newPath.c_str();

    if (*strToAppend == 0 && !PathFileExists(path))
    {
        MessageBox(mainWnd,
                   (L"Oh no! The file was not found!"
                    "\nPlease ensure that file exists:\n"
                    + std::wstring(path)).c_str(),
                   L"File was not found", MB_OK | MB_ICONERROR);
    }
    else if (*strToAppend == 0 &&
         av::strContains(path, L'#') || av::strContains(path, L'%'))
    {
        MessageBox(mainWnd,
                   L"Oh no! The path to this program has \'#\' "
                   "or \'%\' character in it.\nPlease remove the characters "
                   "or move this program with its files to another folder "
                   "in order to view the file.",
                   L"File can not be opened!", MB_OK | MB_ICONINFORMATION);
    }
    else
    {
        HtmlHelpW(mainWnd, path, HH_DISPLAY_TOPIC, 0);
    }
}

void onMoveDownUp(UINT controlID)
{
    listView->moveCurrItem(controlID == IDC_MOVE_UP);
}

void onAbout()
{
    DialogBoxW(avInst, MAKEINTRESOURCE(IDD_ABOUT), mainWnd, aboutDialogProc);
}

void toggleFreeze()
{
    Button_SetCheck(dlgWnd(IDC_FREEZE), 
                    !Button_GetCheck(dlgWnd(IDC_FREEZE)));

    onFrozenChanged();
}

void onBtnTimer()
{
    static RECT moveDist;

    POINT dist = getMoveDist(moveDist, GetKeyState(VK_SHIFT) < 0,
        SendMessage(dlgWnd(IDC_CUR_UP), BM_GETSTATE, 0, 0) & BST_PUSHED,
        SendMessage(dlgWnd(IDC_CUR_DOWN), BM_GETSTATE, 0, 0) & BST_PUSHED,
        SendMessage(dlgWnd(IDC_CUR_LEFT), BM_GETSTATE, 0, 0) & BST_PUSHED,
        SendMessage(dlgWnd(IDC_CUR_RIGHT), BM_GETSTATE, 0, 0) & BST_PUSHED
    );

    moveCursor(dist);
}

void onHelpHotkeys()
{
    tryToOpenChm(L"userGuide.chm", L"::/Hotkeys.htm");
}

// sent only when it CHANGED
void onMousePosChanged()
{
    displayMousePos();     // in the read-only edit
    updateResultCtl();
    drawZoomedArea();
}

std::vector<int> convertToIntArgs(const std::vector<char>& charArgs)
{
    int r = GetRValue(dispData.color);
    int g = GetGValue(dispData.color);
    int b = GetBValue(dispData.color);
    int x = dispData.cursorPos.x;
    int y = dispData.cursorPos.y;

    std::vector <int> vec;
    for(const char& c : charArgs)
        switch (c)
        {
            case 'r':   vec.push_back(r);   break;
            case 'g':   vec.push_back(g);   break;
            case 'b':   vec.push_back(b);   break;
            case 'x':   vec.push_back(x);   break;
            case 'y':   vec.push_back(y);   break;
        }

    return vec;
}

void onFrozenChanged()
{
    dispData.frozen = Button_GetCheck(GetDlgItem(mainWnd, IDC_FREEZE));

    dispData.frozenCursorOffset = {0,0};

    if(dispData.frozen == true)
        saveScreenshot();
    else 
    {
        setColorByPicker(false);

        DeleteObject(dispData.frozenBitmap);
        dispData.frozenBitmap = 0;
    }
}

void onTimer(WPARAM wparam)
{
    switch(wparam)
    {
        case redrawTimerID:   onRedrawTimer();                  break;
        case tooltipTimerID:  hideTooltipCopied();              break;
        case keyboardTimerID: onKeyDown();                      break;
        case btnDownTimerID:  onBtnTimer();                     break;
    }
}

void copyMousePosToClipboard()
{ 
    wchar_t text[13] = {0};
    GetDlgItemTextW(mainWnd, IDC_MOUSEPOS, text, 13);
    
    int byteCount = (lstrlenW(text) +1) *sizeof(wchar_t); // +1 for "NULL"
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, byteCount);
    if(hGlobal) {        
        memcpy(GlobalLock(hGlobal), text, byteCount);
        GlobalUnlock(hGlobal);
        
        if(!OpenClipboard(mainWnd))
            return;
        EmptyClipboard();
        SetClipboardData(CF_UNICODETEXT, hGlobal);
    }

    CloseClipboard();
    showTooltipCopied(IDC_COPY_MOUSEPOS);
}

void copyResultStrToClipboard()
{
    wchar_t text[2048] = {0};
    GetDlgItemTextW(mainWnd, IDC_RESULT, text, 2048);
    
    int byteCount = (lstrlenW(text) +1) *sizeof(wchar_t); // +1 for "NULL"
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, byteCount);
    if(hGlobal)
    {        
        memcpy(GlobalLock(hGlobal), text, byteCount);
        GlobalUnlock(hGlobal);
        
        if(!OpenClipboard(mainWnd))
            return;
        EmptyClipboard();
        SetClipboardData(CF_UNICODETEXT, hGlobal);
    }

    CloseClipboard();
    showTooltipCopied(IDC_COPY_RESULT);
}

std::vector <int> getArgs()
{
    // if user is editing args, get them from edit
    if (listView->isEditingArgs())
    {
        auto args = listView->getArgs(av::getEditText(listView->getEdit()).c_str());
        return convertToIntArgs(args);
    }
    else
        return convertToIntArgs(listView->getArgs(listView->getCurrItemStr(LV_IND_ARGS)));
}

cwstr getFmt()
{
    // if user is editing format, get it from edit
    if(listView->isEditingFmt())
    {
        static std::wstring editStr;
        editStr = av::getEditText(listView->getEdit());
        return editStr.c_str();
    }
    else
        return listView->getCurrItemStr(LV_IND_FORMAT);
}

BOOL onEndSession(HWND wnd, BOOL fEnding)
{
    (void) wnd;

    if(fEnding)
        onDestroy();

    return 0;
}

void showTooltipCopied(UINT controlID)
{
    SendMessage(tooltip, TTM_TRACKACTIVATE, 1, (LPARAM)&toolInfo);
    HWND buttonCopied = GetDlgItem(mainWnd, controlID);
    SendMessage(tooltip, TTM_TRACKPOSITION, 0, 
                MAKELONG(av::getWndRect(buttonCopied).left, 
                         av::getWndRect(buttonCopied).top -20) );
    SetTimer(mainWnd, tooltipTimerID, 3000, 0);
}

void assignVariables(HWND hwnd)
{
    appIcon = LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(IDI_APPICON));

    resultStrWnd = GetDlgItem(hwnd, IDC_RESULT);
    zoomMaxPercentage = GetDlgItem(hwnd, IDC_MAX_PERCENTAGE);
    currColorWnd = GetDlgItem(hwnd, IDC_COLOR_RECT);

    dcMainWnd = GetDC(hwnd);
    dcScreen = GetDC(HWND_DESKTOP);
    dcMemTemp = CreateCompatibleDC(dcScreen);
    dcZoomArea = GetDC(dlgWnd(IDC_ZOOMED_RECT, hwnd));
    dcCurrColor = GetDC(dlgWnd(IDC_COLOR_RECT, hwnd));

    GetClientRect(currColorWnd, &staticWndRect);

    zoomAreaOldProc  = (WNDPROC) SetWindowLong(dlgWnd(IDC_ZOOMED_RECT, hwnd), GWL_WNDPROC, 
                                               (LONG)zoomAreaProc);
    currColorOldProc = (WNDPROC) SetWindowLong(currColorWnd, GWL_WNDPROC, 
                                               (LONG)currColorProc);
    zoomSliderOldProc = (WNDPROC) SetWindowLong(dlgWnd(IDC_ZOOM_SLIDER, hwnd), GWL_WNDPROC,
                                               (LONG)zoomSliderProc);
}

void hideTooltipCopied()
{
    SendMessage(tooltip, TTM_TRACKACTIVATE, 0, (LPARAM)&toolInfo);
    KillTimer(mainWnd, tooltipTimerID);
}

void copyFileToTempDir(cwstr fileName)
{
    std::wstring oldPath = av::getExeDir() + L"\\" + fileName;

    WCHAR* buff;
    SHGetKnownFolderPath(FOLDERID_ProgramData, 0, nullptr, &buff);
    std::wstring newPath(buff);
    CoTaskMemFree(buff);

    newPath.append(L"\\AvApp\\");
    newPath.append(appName);
    newPath.push_back(L'\\');
    newPath.append(fileName);

    WCHAR dir[MAX_PATH]{0};
    StrCpyW(dir, newPath.c_str());
    PathRemoveFileSpecW(dir);
    SHCreateDirectoryEx(0, dir, 0);

    CopyFile(oldPath.c_str(), newPath.c_str(), false);
}

POINT getFinalMousePos()
{
    return { getFinalX(), getFinalY() } ;
}

int getFinalX()
{
    return dispData.cursorPos.x + dispData.frozenCursorOffset.x;
}

int getFinalY()
{
    return dispData.cursorPos.y + dispData.frozenCursorOffset.y;
}

void setColorByPicker(bool colorByPicker)
{
    static bool prev = !colorByPicker;
    if(prev == colorByPicker)
        return;

    prev = dispData.colorByPicker = colorByPicker;

    SetDlgItemText(mainWnd, IDC_COLOR_AT_CURSOR, colorByPicker ? L"Color (by Color picker)"
                                                               : L"Color (at cursor)");
}

void onKeyDown()
{
    if(focusedControlIsEdit()
        || GetForegroundWindow() != mainWnd
        || GetFocus() == listView->getHWND()
        || GetKeyState(VK_CONTROL) < 0)
        return;

    if(GetKeyState('Z') < 0 )
    {
        SetFocus(GetDlgItem(mainWnd, IDC_ZOOM_SLIDER));
        return;
    }

    static RECT moveDist;
    POINT dist = getMoveDist(moveDist, GetKeyState(VK_SHIFT) < 0,
                             GetKeyState('W') < 0,
                             GetKeyState('S') < 0,
                             GetKeyState('A') < 0,
                             GetKeyState('D') < 0 );

    moveCursor(dist);
}

void onRedrawTimer()
{
    if (!dispData.frozen)
        updateCurrColor();

    drawZoomedArea();
}

void onColorPicker()
{
    CHOOSECOLOR cc = {0};
    COLORREF    crCustColors[16];
    cc.lStructSize = sizeof(cc);
    cc.Flags = CC_RGBINIT | CC_FULLOPEN;
    cc.hInstance = (HWND) GetModuleHandle(0);
    cc.hwndOwner = mainWnd;
    cc.lpCustColors = crCustColors;

    bool userClickedOK = ChooseColor(&cc);

    if(userClickedOK) 
    {
        setColorByPicker(true);

        Button_SetCheck(GetDlgItem(mainWnd, IDC_FREEZE), BST_CHECKED);
        dispData.setColor(cc.rgbResult);
        onFrozenChanged();
    }
}

bool focusedControlIsEdit()
{
    static wchar_t className[200];
    className[0] = 0;
    GetClassName(GetFocus(), className, 199);
    return 0 == lstrcmpiW(L"Edit", className);
}

void createTooltipCopied()
{
    tooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, 0, WS_POPUP | TTS_ALWAYSTIP, 
                           CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                           mainWnd, 0, avInst, 0);

    toolInfo.cbSize = sizeof(TOOLINFO) -4;
    toolInfo.uFlags = TTF_IDISHWND | TTF_TRACK;
    toolInfo.hwnd   = mainWnd;
    toolInfo.uId    = (UINT_PTR)GetDlgItem(mainWnd, IDC_COPY_MOUSEPOS);
    static wchar_t buff[] = L"Copied!";
    toolInfo.lpszText = buff  ;
    
    SendMessage(tooltip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
}

void displayMousePos()
{
    POINT currPos = getFinalMousePos();

    wchar_t str[20] = { 0 };
    wsprintfW(str, L"%i,%i", currPos.x, currPos.y);
    SetDlgItemTextW(mainWnd, IDC_MOUSEPOS, str);
}

void displayRGBValues()
{
    wchar_t str[20] = { 0 };
    
    wsprintfW(str, L"R: %3i", GetRValue(dispData.color));
    SetDlgItemTextW(mainWnd, IDC_RVALUE, str);
    
    wsprintfW(str, L"G: %3i", GetGValue(dispData.color));
    SetDlgItemTextW(mainWnd, IDC_GVALUE, str);

    wsprintfW(str, L"B: %3i", GetBValue(dispData.color));
    SetDlgItemTextW(mainWnd, IDC_BVALUE, str);
}

void updateCurrColor()
{
    if(!dispData.frozen) // color by timer or mouse, etc.
    {
        HDC screenDC = GetDC(0);
        dispData.setColor(GetPixel(screenDC, dispData.cursorPos.x, dispData.cursorPos.y));
        ReleaseDC(0, screenDC);
    }
    else if(dispData.frozen && !dispData.colorByPicker) // color in frozen changed by keys
    {
        DisplayingData &dp = dispData;

        SelectObject(dcMemTemp, dp.frozenBitmap);
        
        int x = getFinalX() -dp.screenshotXY.x;
        int y = getFinalY() -dp.screenshotXY.y;
        dp.setColor(GetPixel(dcMemTemp, x, y));
    }
}

void drawZoomedArea()
{
    InvalidateRect(dlgWnd(IDC_ZOOMED_RECT), nullptr, false);
}

void onMouseHook()
{
    updateCurrColor();
    updateMousePos();
}

void onVersionHistory()
{
    tryToOpenChm(L"versionHistory.chm");
}

void onHelpFmtBtn()
{
    tryToOpenChm(L"userGuide.chm");
}

BOOL onNotify(HWND wndParent, INT idCtrl, NMHDR* nmh)
{
    if(!listView)
        return false;

    // redirect notify messages to ListView:
    if(nmh->hwndFrom == listView->getHWND())
        listView->onNotify(wndParent, idCtrl, nmh);


    if(nmh->hwndFrom == listView->getEdit()                             // Edit change
    || nmh->code == LVN_ITEMCHANGED && listView->getCurrIndex() != -1)  // ListView change
    {
        updateResultCtl();
    }

    return true;
}

// sends a signal only if mouse pos CHANGED
void updateMousePos()
{
    static POINT prevPos = { 999999, 999999 };
    if(!dispData.frozen) // set mouse position
        dispData.cursorPos = av::getMousePos();

    POINT newPos = getFinalMousePos();

    if(newPos.x != prevPos.x || newPos.y != prevPos.y)
    {
        onMousePosChanged();
        prevPos = newPos;
    }
}

void updateResultCtl()  // convert whatever user typed as text
{
    // process all the escape characters
    SetWindowText(resultStrWnd, processFormatStr().c_str());
}

void fillPixelSizes()
{
    pixelSizes.clear();

    int zoomAreaSize = getZoomAreaSize();
    int prevSrcWidth = -1;

    for(int i = 1; true ; i++) {  // 3 pix/per screen is max zoom
        int srcWidth = zoomAreaSize /i;
        srcWidth += srcWidth%2 ? 0 :1;
        
        if(srcWidth != prevSrcWidth) {
            float pixSize = (float)getZoomAreaSize() /(float)srcWidth;

            pixelSizes.push_back(int(round(pixSize)));
            prevSrcWidth = srcWidth;
        }

        if(srcWidth == 3)
            return;
    }
}

void resizeZoomedAreaControl(HWND wnd)
{
    int zoomedAreaSize = 151;   // must be uneven (51x51 i.e.)
    av::setWndWidth(dlgWnd(IDC_ZOOMED_RECT, wnd), zoomedAreaSize);
    av::setWndHeight(dlgWnd(IDC_ZOOMED_RECT, wnd), zoomedAreaSize);
}

void drawHudInZoomedArea(const HDC &hdc, const float &controlSize)
{
    int halfWidth = int(controlSize /2);

    // single pixel rect
    RECT hudRect = { halfWidth,    halfWidth,
                     halfWidth +1, halfWidth +1 }; // 1 off rule

    int inflateBy = pixelThickness /2;
    InflateRect(&hudRect, inflateBy, inflateBy);
    InflateRect(&hudRect, 1, 1);    // make frame outside the pixel

    HRGN hudRgn = CreateRectRgnIndirect(&hudRect);
    InflateRect(&hudRect, -1, -1);
    HRGN innerRgn = CreateRectRgnIndirect(&hudRect);

    CombineRgn(hudRgn, hudRgn, innerRgn, RGN_XOR);
    InvertRgn(hdc, hudRgn);

    DeleteObject(hudRgn);
    DeleteObject(innerRgn);
}

HBITMAP drawZoomedAreaBmp()
{
    int controlSize = getZoomAreaSize();

    HBITMAP bmp = CreateCompatibleBitmap(dcScreen, controlSize, controlSize);
    SelectObject(dcMemTemp, bmp);
            
    int srcWidth = controlSize /pixelThickness;
    srcWidth += srcWidth%2 ? 0 :1;

    if (dispData.frozen) 
    {
        HDC hdcSShot = CreateCompatibleDC(dcScreen);
        SelectObject(hdcSShot, dispData.frozenBitmap);

        // guh... really? do i have to delete it/??
        //int newX = round(((1.0 -1.0/(double)pixelThickness) *(double)controlSize) /2.0);
        //int newY = round(((1.0 -1.0/(double)pixelThickness) *(double)controlSize) /2.0);
        int newX = getFinalX() -srcWidth/2;
        int newY = getFinalY() -srcWidth/2;
        newX -= dispData.screenshotXY.x;
        newY -= dispData.screenshotXY.y;

        StretchBlt(dcMemTemp, 0, 0, controlSize, controlSize,
                   hdcSShot, newX, newY, srcWidth, srcWidth, SRCCOPY);  
        DeleteDC(hdcSShot);
    }
    else 
    {
        int xSrc = getFinalX() -srcWidth/2;
        int ySrc = getFinalY() -srcWidth/2;
        
        StretchBlt(dcMemTemp, 0, 0, controlSize, controlSize,
           dcScreen, xSrc, ySrc, srcWidth, srcWidth, SRCCOPY);    
    }
    
    drawHudInZoomedArea(dcMemTemp, (const float&)controlSize);
    
    return bmp;
}

void initZoomSlider(HWND wnd)
{
    fillPixelSizes();
    SendMessage(dlgWnd(IDC_ZOOM_SLIDER, wnd), TBM_SETRANGE, true, 
                MAKELONG(0, pixelSizes.size() -1));
    SendMessage(dlgWnd(IDC_ZOOM_SLIDER, wnd), TBM_SETPOS, true, 0);
}

HWND dlgWnd(const int & windowID, HWND wnd)
{
    if(!wnd)
        wnd = mainWnd;
    
    return GetDlgItem(wnd, windowID);
}

int getZoomAreaSize()
{
    return 151;

    // avTodo: hardcode zoom area size?
    //RECT rect;
    //GetClientRect(dlgWnd(IDC_ZOOMED_RECT), &rect);
    //return rect.right -rect.left;
}

bool keyMsgShouldBeSuspended(const MSG &msg)
{
    if (focusedControlIsEdit()
    ||  GetAsyncKeyState(VK_CONTROL) < 0
    ||  GetAsyncKeyState(VK_MENU)    < 0
    ||  GetAsyncKeyState(VK_LWIN)    < 0
    ||  GetAsyncKeyState(VK_RWIN)    < 0)
        return false;

    switch(msg.wParam)
    {
        case 'W':
        case 'A':
        case 'S':
        case 'D':
            setColorByPicker(false);
            return true;    // message must be suspended
    }

    return false;
}

std::wstring processFormatStr()
{
    return listView->isSelItemChecked() ? processAdvancedFormat()
                                        : processSimpleFmt();
}

std::wstring processSimpleFmt()
{
    // #[h][0]r
    // # - escape character
    // h H - hexadecimal (decimals are by default)
    // 0 - padding with zeroes (3 for dec, 2 for hex)
    // r g b - red, green, blue values

    static const wchar_t escChar = L'#';

    int r = GetRValue(dispData.color);
    int g = GetGValue(dispData.color);
    int b = GetBValue(dispData.color);

    std::wstring result;
    const wchar_t *fmtStr = getFmt();
    const int len = lstrlenW(fmtStr);
    static wchar_t buff[10] = {0};

    while(*fmtStr)
    {
        if(*fmtStr != escChar) // boring literal character
        {
            result.push_back(*fmtStr);
        }
        else if (*(++fmtStr)) // escaped sequence!
        {
            std::wstring printfFMT = L"%d";  // decimal is default
            bool hex = *fmtStr == 'h' || *fmtStr == 'H';
            if(hex)
            {
                if(*(++fmtStr) == 0)   // end of fmt string
                    return result;
                printfFMT = *(fmtStr -1) == 'h' ? L"%x" : L"%X";
            }
            if(*fmtStr == L'0') // padding with 0
            {
                if(*(++fmtStr) == 0)   // end of fmt string
                    return result;

                printfFMT.insert(1, hex ? L"02" : L"03");
            }

            buff[0] = 0;
            switch (*fmtStr)
            {
                case escChar:  buff[0] = escChar; buff[1] = 0;     break;
                case 'r':   swprintf_s(buff, 10, printfFMT.c_str(), r);  break;
                case 'g':   swprintf_s(buff, 10, printfFMT.c_str(), g);  break;
                case 'b':   swprintf_s(buff, 10, printfFMT.c_str(), b);  break;
                case 'x':   swprintf_s(buff, 10, printfFMT.c_str(), getFinalX());  break;
                case 'y':   swprintf_s(buff, 10, printfFMT.c_str(), getFinalY());  break;
                default:    break;  // ignore unknown spec.characters
            }
            result.append(buff);
        }
        else    // it's a null character (end of the format string)!
        {
            return result;
        }
        
        ++fmtStr;
    }

    return result;
}

std::wstring processAdvancedFormat()
{
    std::vector <int> args = getArgs();
    const wchar_t* fmtStr = getFmt();
    
    return processAdvFmt(fmtStr, args).c_str();
}

// sent only when CurrColor CHANGED
void DisplayingData::setColor(const COLORREF &Color)
{
    if(this->color == Color)    // nothing changed!
        return; 
        
    this->color = Color;
    onColorChanged();
}

void DisplayingData::onColorChanged()
{
    displayRGBValues();
    InvalidateRect(currColorWnd, nullptr, false);   // color changed

    updateResultCtl();
}
