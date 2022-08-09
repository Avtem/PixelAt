#pragma once

/* This file was created by Avtem using Visual Studio IDE.
 * 
 * Created: 9/30/2021 12:41:23 AM 
 * Here are all stuff needed for managing ListView operations*/
#include <Windows.h>
#include <CommCtrl.h>
#include <av.h>

#define LV_IND_NAME   0
#define LV_IND_FORMAT 1
#define LV_IND_ARGS   2

class ListView
{
public:
    ListView(HWND hwndExistingLV);
    bool operator==(HWND hwnd) const            { return wnd == hwnd;     }
    bool operator!=(HWND hwnd) const            { return wnd != hwnd;     }
    bool operator==(const ListView& rhs) const  { return *this == rhs;    }
    bool operator!=(const ListView& rhs) const  { return !(*this == rhs); }

    void addItem(cwstr itemText, cwstr* subTexts = nullptr, int subTextsCount = 0);
    void deleteItem();
    HWND getEdit() const;
    HWND getHWND() const;
    bool isEditingArgs() const;
    void setCheckedState(int itemInd, bool check);
    void moveCurrItem(bool moveUp);
    bool isEditingFmt() const;
    int getCurrIndex() const;
    int getColWidth(int col0b) const;
    void setCurrItem(int index);
    void setColWidth(int col0b, int width);
    bool isSelItemChecked() const;
    bool isItemChecked(int itemInd) const;
    int getItemCount() const;
    void editCurrItem(); // F2 or double click
    void swapItems(int item1, int item2);
    std::wstring getItem(int itemInd, int subItemInd) const;
    void setItem(int itemInd, int subItemInd, cwstr str);
    cwstr getCurrItemStr(int columnIndex) const;
    std::vector<char> getArgs(cwstr argStr) const;
    // you must call this func only if it IS related to the ListView!!!
    static BOOL onNotify(HWND hwndParent, int ctlID, NMHDR* nmhdr);

private:
    HWND wnd;
    static WNDPROC oldListViewProc;
    static constexpr int COL_COUNT = 3; // only for PixelAt app

    // for sub-item editing
    HWND edit;
    WNDPROC oldEditProc;
    LVITEM editingItem;
    bool applyChanges;

    //// methods
    static LRESULT CALLBACK editProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    static LRESULT CALLBACK listViewProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    void destroyEdit();
    void onEditTab();
    void onLbuttonUp();
    void setEditingItem(int itemInd0b, int subItem0b);
    void createEditForItem(int itemInd0b, int subItem0b);
    RECT getItemRect(int index, int iSubIndex) const;

    void createHeaders();
    void setLVRequiredStyles();
};