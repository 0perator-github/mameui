// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

#ifndef MAMEUI_WINAPP_MUI_AUDIT_H
#define MAMEUI_WINAPP_MUI_AUDIT_H

#pragma once

// standard windows headers
#include <windef.h>

void AuditDialog(HWND hParent, int choice);

// For property sheet Game Audit tab
void InitGameAudit(int gameIndex);
INT_PTR CALLBACK GameAuditDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);

int MameUIVerifyRomSet(int game, bool choice);
int MameUIVerifySampleSet(int game);

const char * GetAuditString(int audit_result);
bool IsAuditResultKnown(int audit_result);
bool IsAuditResultYes(int audit_result);
bool IsAuditResultNo(int audit_result);

#endif // MAMEUI_WINAPP_MUI_AUDIT_H
