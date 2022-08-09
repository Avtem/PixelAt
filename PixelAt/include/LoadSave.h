/* This file was created by Avtem using Visual Studio IDE.
 * 
 * It contains all logic for loading / saving operations
 *
 * Created: 12/9/2021 6:03:25 PM 
 * Version 1.0.0:	dd.mm.yyyy*/

#pragma once

#include <Windows.h>
#include <av.h>
#include "ListView.h"

void setLoadSaveVars(HWND wnd, ListView* listView); // must be called before any Load/Save operations
void loadAllSettings(AvTrayIcon& trayIcon);
void saveAllSettings();
// =========
SIZE getWndSize(HWND wnd);
void loadWndPos(AvTrayIcon& trayIcon, bool noSize = true);
void saveWndPos();
void loadZoomPercentage();
void saveZoomPercentage();
void loadLVColWidths();
void saveLVColWidths();
void loadLVItems();
void saveLVItems();
void loadLVSelectionInd();
void saveLVSelectionInd();

