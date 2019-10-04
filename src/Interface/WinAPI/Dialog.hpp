#pragma once

#include "Window.hpp"

#define DLG_HANDLER(n) INT_PTR n(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)

class cDialog : public cWindow
{
public:

	typedef INT_PTR(cDialog::*fnDlgProc)(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

	typedef std::map<UINT, fnDlgProc> mapMsgProc;
	typedef std::map<WORD, fnDlgProc> mapCtrlProc;

	cDialog(int dialogID)
		: _dialogID(dialogID)
	{
		_messages[WM_INITDIALOG] = &cDialog::OnInit;
		_messages[WM_COMMAND] = &cDialog::OnCommand;
		_messages[WM_CLOSE] = &cDialog::OnClose;
	}

	INT_PTR RunModal(HWND parent = NULL)
	{
		Win32Thunk<DLGPROC, cDialog> dlgProc(&cDialog::DlgProc, this);
		return DialogBox(NULL, MAKEINTRESOURCE(_dialogID), parent, dlgProc.GetThunk());
	}

	INT_PTR RunModeless(HWND parent = NULL, int accelID = NULL)
	{
		MSG msg = { NULL };
		HACCEL hAccel = LoadAccelerators(NULL, MAKEINTRESOURCE(accelID));
		_modeless = true;

		Win32Thunk<DLGPROC, cDialog> dlgProc(&cDialog::DlgProc, this);
		_hwnd = CreateDialog(NULL, MAKEINTRESOURCE(_dialogID), parent, dlgProc.GetThunk());
		ShowWindow(_hwnd, SW_SHOW);

		while (IsWindow(_hwnd) && GetMessage(&msg, NULL, NULL, NULL) > 0)
		{
			if (TranslateAccelerator(_hwnd, hAccel, &msg))
				continue;

			if (!IsDialogMessage(_hwnd, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		return msg.wParam;
	}

protected:

	INT_PTR CloseDialog()
	{
		BOOL bRes;

		if (_modeless)
			bRes = DestroyWindow(_hwnd);

		else
			bRes = EndDialog(_hwnd, EXIT_SUCCESS);

		_hwnd = NULL;

		return bRes;
	}

	INT_PTR DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (_messages.count(message))
			return (this->*_messages[message])(hDlg, message, wParam, lParam);

		return FALSE;
	}

	INT_PTR OnInit(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
	{
		_hwnd = hDlg;
		return TRUE;
	}

	INT_PTR OnCommand(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (_events.count(LOWORD(wParam)))
			return (this->*_events[LOWORD(wParam)])(hDlg, message, wParam, lParam);

		if (_events.count(HIWORD(wParam)))
			return (this->*_events[HIWORD(wParam)])(hDlg, message, wParam, lParam);

		return FALSE;
	}

	INT_PTR OnClose(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
	{
		return CloseDialog();
	}

	mapMsgProc _messages;
	mapCtrlProc _events;
	int _dialogID;
	bool _modeless = false;
};
