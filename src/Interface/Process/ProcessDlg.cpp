#include "ProcessDlg.h"

cProcessDlg::cProcessDlg(HINSTANCE instance, const std::wstring& procName) : cDialog(IDD_PROCESS), _instance(instance), _processName(procName)
{
	_messages[WM_INITDIALOG] = static_cast<cDialog::fnDlgProc>(&cProcessDlg::OnInit);
	_messages[WM_CLOSE] = static_cast<cDialog::fnDlgProc>(&cProcessDlg::OnCancel);

	_events[IDC_PROCESSOK] = static_cast<cDialog::fnDlgProc>(&cProcessDlg::OnOK);
	_events[IDCANCEL] = static_cast<cDialog::fnDlgProc>(&cProcessDlg::OnCancel);
}

INT_PTR cProcessDlg::OnInit(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	cDialog::OnInit(hDlg, message, wParam, lParam);

	HICON hIcon = LoadIcon(_instance, MAKEINTRESOURCE(IDI_ICON));
	SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	DeleteObject(hIcon);

	_process.Attach(_hwnd, IDC_PROCESSTEXT);
	_process.Text(_processName);

	return TRUE;
}

INT_PTR cProcessDlg::OnOK(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	_processName = _process.Text();
	return cDialog::OnClose(hDlg, message, wParam, lParam);
}

INT_PTR cProcessDlg::OnCancel(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	return cDialog::OnClose(hDlg, message, wParam, lParam);
}