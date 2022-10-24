// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

#ifndef MAMEUI_WINAPP_MESSUI_H
#define MAMEUI_WINAPP_MESSUI_H

#pragma once

extern std::string g_szSelectedItem;

void InitMessPicker(void);
void MessUpdateSoftwareList(void);
bool MyFillSoftwareList(int drvindex, bool bForce);
BOOL MessCommand(HWND hwnd,int id, HWND hwndCtl, UINT codeNotify);
void MessReadMountedSoftware(int nGame);
void SoftwareTabView_OnSelectionChanged(void);
bool CreateMessIcons(void);
void MySoftwareListClose(void);
void MView_RegisterClass(void);
void MView_Refresh(HWND hwndDevView);

#endif // MAMEUI_WINAPP_MESSUI_H
