//#include <commctrl.h>

// avtem includes
#include "ListView.h"
#include <resource.h>
#include <windowsx.h>

WNDPROC ListView::oldListViewProc = 0;

ListView::ListView(HWND hwndExistingLV)
    :wnd(hwndExistingLV)
{
    createHeaders();
    
    HWND tip = ListView_GetToolTips(hwndExistingLV);
    av::setTopmostStyle(tip, true);

    SetWindowLongPtr(wnd, GWLP_USERDATA, (LONG) this);
    setLVRequiredStyles();

    oldListViewProc = WNDPROC(SetWindowLongPtr(wnd, GWLP_WNDPROC, LONG(listViewProc)));

    ListView_SetItemState(wnd, 0, LVIS_SELECTED, LVIS_SELECTED);
}

bool rectsAreEqual(const RECT& r1, const RECT& r2)
{
    return r1.left   == r2.left
        && r1.top    == r2.top
        && r1.right  == r2.right
        && r1.bottom == r2.bottom;
}

BOOL ListView::onNotify(HWND hwndParent, int ctlID, NMHDR* nmhdr)
{
    (void) ctlID, hwndParent;

    // ignore notifications from other windows
    ListView* This = (ListView*) GetWindowLongPtr(nmhdr->hwndFrom, GWLP_USERDATA);
    if(nmhdr->hwndFrom != This->wnd)
        return false;

    if(nmhdr->code == NM_RCLICK)
    {
        // show context MENU
        NMITEMACTIVATE* currItem = (NMITEMACTIVATE*) (nmhdr);
        HMENU menu = LoadMenu(avInst, MAKEINTRESOURCE(IDR_LVITEM_MENU));
        UINT resMenuIndex = TrackPopupMenu(GetSubMenu(menu,0),
              TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON | TPM_NOANIMATION, 
              av::getMousePos().x, av::getMousePos().y, 0,
              hwndParent, 0);
        DestroyMenu(menu);

        switch (resMenuIndex)
        {
            case IDC_DELETE_FMT:
                This->deleteItem();
                break;
            case IDC_EDIT_ITEM:    
                This->editCurrItem();
                break;
            case IDC_MOVE_DOWN:
                This->swapItems(currItem->iItem, currItem->iItem +1);
                This->setCurrItem(currItem->iItem +1);
                break;
            case IDC_MOVE_UP:       
                This->swapItems(currItem->iItem, currItem->iItem -1);
                This->setCurrItem(currItem->iItem -1);
                break;
        }
    }
    else if (nmhdr->code == LVN_BEGINSCROLL && This->edit) // kill edit on scroll start
    {
        This->applyChanges = false;
        DestroyWindow(This->edit);
        return true;
    }
    else if (nmhdr->code == NM_CLICK)                 // double click logic
    {
        NMITEMACTIVATE* currItem = (NMITEMACTIVATE*) (nmhdr);
        RECT currClickedRect = This->getItemRect(currItem->iItem, currItem->iSubItem);
        if(!PtInRect(&currClickedRect, currItem->ptAction)) // user clicked outside item rect..
            return true;

        static UINT mPrevClick = 0;

        UINT newClick = GetTickCount();
        bool dblClick = newClick -mPrevClick < GetDoubleClickTime();
        mPrevClick = newClick;

        static RECT prevClickedRect{0};
        if (dblClick)
        {
            mPrevClick = 0;

            if(rectsAreEqual(currClickedRect, prevClickedRect))
                This->createEditForItem(currItem->iItem, currItem->iSubItem);
        }

        prevClickedRect = currClickedRect;
    }

    return true;
}

void ListView::destroyEdit()
{
    if (!edit)
        return;

    static std::wstring editText;
    editText = av::getEditText(edit);

    if (applyChanges)
    {
        editingItem.pszText = (WCHAR*) (editText.c_str());
        ListView_SetItem(wnd, &editingItem);
    }
    else if(editingItem.iSubItem == 1)
    {
        SetWindowText(edit, editingItem.pszText);
     //send notification about destruction
        static NMHDR nmhdr{ 0 };
        nmhdr.code = EN_CHANGE;
        nmhdr.hwndFrom = HWND(edit);
        SendMessage(GetParent(wnd), WM_NOTIFY, 0, LPARAM(&nmhdr));   
    }

    DestroyWindow(edit);
    edit = 0;
    editingItem = {0};
}

void ListView::onEditTab()
{
    bool shift = GetKeyState(VK_SHIFT) < 0;

    int item = editingItem.iItem;
    int sub  = editingItem.iSubItem +(shift ? -1 : +1);
    if(sub == -1)
        sub = 2;
    else
        sub %= COL_COUNT;

    DestroyWindow(edit);

    createEditForItem(item, sub);
}

// for Edit control (triple click)
void ListView::onLbuttonUp()
{
    static const int maxTimeDiff = GetDoubleClickTime();
    
    static int click1 = 0;
    static int click2 = 0;
    int click3 = GetTickCount();

    if(click3 - click2 < maxTimeDiff    // triple click!
    && click2 - click1 < maxTimeDiff)
    {
        SendMessage(edit, EM_SETSEL, 0, LPARAM(-1));
        click1 = click2 = 0;
        return;
    }

    click1 = click2;
    click2 = click3;
}

int ListView::getColWidth(int col0b) const
{
    return ListView_GetColumnWidth(wnd, col0b);
}

void ListView::setCurrItem(int index)
{
    if(index < 0 || index >= getItemCount())
        return;

    ListView_SetItemState(wnd, index, LVIS_SELECTED | LVIS_FOCUSED, 
                                      LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(wnd, index, false);
}

void ListView::setColWidth(int col0b, int width)
{
    ListView_SetColumnWidth(wnd, col0b, width);
}

void ListView::setEditingItem(int itemInd0b, int subItem0b)
{
    static WCHAR buff[1024]{0};
    buff[0] = 0;
    // get item string from LV
    ListView_GetItemText(wnd, itemInd0b, subItem0b, buff, 1024);

    editingItem.iItem = itemInd0b;
    editingItem.iSubItem = subItem0b;
    editingItem.pszText = buff;
    editingItem.cchTextMax = 1024;
    editingItem.mask = LVIF_TEXT;
}

void ListView::createEditForItem(int itemInd0b, int subItem0b)
{
    setEditingItem(itemInd0b, subItem0b);

    // get rect for the item
    RECT r = getItemRect(itemInd0b, subItem0b);

    edit = CreateWindow(L"Edit", editingItem.pszText, 
                        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL,
                        r.left, r.top, r.right-r.left, r.bottom-r.top, wnd, 0, avInst, 0);
    
    applyChanges = true;

    oldEditProc = (WNDPROC) SetWindowLong(edit, GWL_WNDPROC, LONG(editProc));
    SetWindowFont(edit, GetWindowFont(wnd), true);
    Edit_SetSel(edit, 0, -1);
    SetFocus(edit);
}

RECT ListView::getItemRect(int index, int iSubIndex) const
{
    RECT itemRect{0,0,0,0};
    ListView_GetSubItemRect(wnd, index, iSubIndex, LVIR_LABEL, &itemRect);

    return itemRect;
}

void ListView::createHeaders()
{
    LVCOLUMNW columns[COL_COUNT] = {0};

    for(int i=0; i < COL_COUNT; ++i)
    {
        LVCOLUMNW &col = columns[i];
        col = {0};
        col.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_MINWIDTH;
        col.iSubItem = i;
        col.cxMin = 5;
        
        switch(i)
        {
            case LV_IND_NAME:
                col.pszText = LPWSTR(L"Name");
                col.cx = 120;
                break;

            case LV_IND_FORMAT:
                col.pszText = LPWSTR(L"Format");
                col.cx = 150;
                break;

            case LV_IND_ARGS:
                col.pszText = LPWSTR(L"Args");
                col.cx = 40;
                break;
        }
    }

    for(int i=0; i < COL_COUNT; ++i)
        SendMessage(wnd, LVM_INSERTCOLUMNW, i, LPARAM(&columns[i]));
}

void ListView::setLVRequiredStyles()
{
    DWORD style = LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_CHECKBOXES;
    ListView_SetExtendedListViewStyleEx(wnd, style, style);
    SetClassLongPtr(wnd, GCL_STYLE, GetClassLongPtr(wnd, GCL_STYLE) & ~CS_DBLCLKS);
}

void ListView::addItem(cwstr itemText, cwstr* subTexts, int subTextsCount)
{
    LVITEMW item{ 0 };
    item.mask = LVIF_TEXT;
    item.pszText = LPWSTR(itemText);
    item.iItem = ListView_GetItemCount(wnd);
    ListView_InsertItem(wnd, &item);

    // subitem texts
    for (int i = 0; i < subTextsCount; ++i)
        ListView_SetItemText(wnd, item.iItem, i+1, LPWSTR(subTexts[i]));
}

void ListView::deleteItem()
{
    int index = getCurrIndex();
    if(index != -1)
        ListView_DeleteItem(wnd, index);

    applyChanges = false;
    destroyEdit();
}

HWND ListView::getEdit() const
{
    return edit;
}

HWND ListView::getHWND() const
{
    return wnd;
}

bool ListView::isEditingArgs() const
{
    return edit && editingItem.iSubItem == LV_IND_ARGS;
}

void ListView::setCheckedState(int itemInd, bool check)
{
    ListView_SetCheckState(wnd, itemInd, check);
}

void ListView::moveCurrItem(bool moveUp)
{
    int incr = moveUp ? -1 : +1;
    int currInd = getCurrIndex();

    // sanity check
    if (currInd < 0 || currInd +incr < 0 
     || currInd >= getItemCount() || currInd +incr >= getItemCount() )
        return;

    swapItems(currInd, currInd +incr);
    setCurrItem(currInd +incr);
}

bool ListView::isEditingFmt() const
{
    return edit && editingItem.iSubItem == LV_IND_FORMAT;
}

int ListView::getCurrIndex() const
{
    return ListView_GetNextItem(wnd, -1, LVNI_SELECTED);
}

bool ListView::isSelItemChecked() const
{
    int ind = getCurrIndex();
    if(ind == -1)
        return false;

    return ListView_GetCheckState(wnd, ind);
}

bool ListView::isItemChecked(int itemInd) const
{
    return ListView_GetCheckState(wnd, itemInd);
}

int ListView::getItemCount() const
{
    return ListView_GetItemCount(wnd);
}

void ListView::editCurrItem()
{
    int ind = getCurrIndex();
    if (ind != -1)
        createEditForItem(ind, 0);
}

void ListView::swapItems(int item1, int item2)
{
    // sanity check
    if(item1 < 0 || item2 < 0 || item1 >= getItemCount() || item2 >= getItemCount())
        return;

    // get all items
    std::wstring items1[COL_COUNT];
    std::wstring items2[COL_COUNT];
    for(int i=0; i < COL_COUNT; ++i)
    {
        items1[i] = getItem(item1, i);
        items2[i] = getItem(item2, i);
    }
    // swap labels
    for (int i = 0; i < COL_COUNT; ++i)
    {
        setItem(item2, i, items1[i].c_str());
        setItem(item1, i, items2[i].c_str());
    }


    // swap check states
    bool item1Checked = isItemChecked(item1);
    bool item2Checked = isItemChecked(item2);
    
    setCheckedState(item2, item1Checked);
    setCheckedState(item1, item2Checked);
}

std::wstring ListView::getItem(int itemInd, int subItemInd) const
{
    static WCHAR buff[1024]{ 0 };
    buff[0] = 0;

    ListView_GetItemText(wnd, itemInd, subItemInd, buff, sizeof(buff));
    return buff;
}

void ListView::setItem(int itemInd, int subItemInd, cwstr str)
{
    ListView_SetItemText(wnd, itemInd, subItemInd, (LPWSTR)str);
}

cwstr ListView::getCurrItemStr(int columnIndex) const
{
    static WCHAR fmt[1024]{};
    int index = getCurrIndex();

    ListView_GetItemText(wnd, index, columnIndex, fmt, 1024 );
    return index == -1 ? L"" : fmt;
}

std::vector<char> ListView::getArgs(cwstr argStr) const
{
    std::wstring args = argStr;

    std::vector<char> vec;
    vec.reserve(args.size());
    for(const WCHAR& c : args)
    {
        if(c == L'r' || c == L'g' || c == L'b' || c == L'x' || c == L'y')
            vec.emplace_back(char(c));
    }

    return vec;
}

LRESULT CALLBACK ListView::editProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    ListView* This = (ListView*)GetWindowLongPtr(GetParent(hwnd), GWLP_USERDATA);

    switch (message)
    {
        case WM_GETDLGCODE:
            return DLGC_WANTALLKEYS;
        case WM_KILLFOCUS:
            This->destroyEdit();
            break;
        case WM_KEYDOWN:
        {
            switch (wparam)
            {
                case VK_ESCAPE:
                    This->applyChanges = false;
            // fall through
                case VK_RETURN:
                    DestroyWindow(hwnd);
                    break;
                case VK_TAB:
                    This->onEditTab();
                    break;
            }
        } break;
        case WM_LBUTTONUP: // for triple click
            This->onLbuttonUp();
            break;
    }

    return CallWindowProc(This->oldEditProc, hwnd, message, wparam, lparam);
}

LRESULT CALLBACK ListView::listViewProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    // redirect edit messages
    ListView* This = (ListView*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    switch (message)
    {
        case WM_RBUTTONDOWN:
        {
            //return 0;
            break;
        }
        case WM_COMMAND:
        {
            if(HIWORD(wparam) == EN_CHANGE && This->editingItem.iSubItem != LV_IND_NAME)
            {
                static NMHDR nmhdr{0};
                nmhdr.code = EN_CHANGE;
                nmhdr.hwndFrom = HWND(lparam);

                PostMessage(GetParent(hwnd), WM_NOTIFY, 0, LPARAM(&nmhdr));
            }
        } break;
        case WM_KEYDOWN:
            if(wparam == VK_F2)
                This->editCurrItem();
            else if(wparam == VK_DELETE)
                This->deleteItem();
            break;
    }

    return CallWindowProc(oldListViewProc, hwnd, message, wparam, lparam);
}
