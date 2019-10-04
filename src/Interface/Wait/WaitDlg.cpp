#include "WaitDlg.h"

cWaitDlg::cWaitDlg(cInjectionCore& core, sInjectContext& context) : cDialog(IDD_WAIT), _core(core) , _context(context), _waitThread(&cWaitDlg::WaitForInjection, this)
{
	_messages[WM_INITDIALOG] = static_cast<cDialog::fnDlgProc>(&cWaitDlg::OnInit);
	_messages[WM_CLOSE] = static_cast<cDialog::fnDlgProc>(&cWaitDlg::OnCancel);

	_events[IDCANCEL] = static_cast<cDialog::fnDlgProc>(&cWaitDlg::OnCancel);
}

cWaitDlg::~cWaitDlg()
{
	if (_waitThread.joinable())
		_waitThread.join();
}

INT_PTR cWaitDlg::OnInit(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	cDialog::OnInit(hDlg, message, wParam, lParam);

	HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON));
	SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	DeleteObject(hIcon);

	_waitText.Attach(_hwnd, IDC_WAITTEXT);
	_waitText.Text(L"Awaiting detection of " + blackbone::Utils::StripPath(_context.procPath));

	_progressBar.Attach(_hwnd, IDC_WAITBAR);
	_progressBar.SetMarque(true);

	return TRUE;
}

INT_PTR cWaitDlg::OnCancel(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	_context.waitActive = false;
	return TRUE;
}

DWORD cWaitDlg::WaitForInjection()
{
	PROCESS_INFORMATION pi = { NULL };

	for (bool inject = true; inject && _status != STATUS_REQUEST_ABORTED; inject = _context.cfg.injIndef)
		_status = _core.GetTargetProcess(_context, pi);

	CloseDialog();
	return _status;
}