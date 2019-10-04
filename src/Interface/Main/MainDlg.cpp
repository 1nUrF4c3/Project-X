#include "MainDlg.h"

#include <thread>
#include <future>
#include <shellapi.h>

cMainDlg::cMainDlg(eStartAction action, const std::wstring& defConfig) : cDialog(IDD_MAIN), _core(_hwnd), _action(action), _defConfig(defConfig)
{
	_messages[WM_INITDIALOG] = static_cast<cDialog::fnDlgProc>(&cMainDlg::OnInit);
	_messages[WM_COMMAND] = static_cast<cDialog::fnDlgProc>(&cMainDlg::OnCommand);
	_messages[WM_CLOSE] = static_cast<cDialog::fnDlgProc>(&cMainDlg::OnClose);
	_messages[WM_NOTIFY] = static_cast<cDialog::fnDlgProc>(&cMainDlg::OnNotify);
	_messages[WM_DROPFILES] = static_cast<cDialog::fnDlgProc>(&cMainDlg::OnDragDrop);

	_events[ID_CTRL_O] = static_cast<cDialog::fnDlgProc>(&cMainDlg::OnOpen);
	_events[ID_CTRL_S] = static_cast<cDialog::fnDlgProc>(&cMainDlg::OnSave);
	_events[ID_PROFILE_OPEN] = static_cast<cDialog::fnDlgProc>(&cMainDlg::OnOpen);
	_events[ID_PROFILE_SAVE] = static_cast<cDialog::fnDlgProc>(&cMainDlg::OnSave);
	_events[ID_ADVANCED_PROTECT] = static_cast<cDialog::fnDlgProc>(&cMainDlg::OnProtect);
	_events[ID_ADVANCED_CONFIG] = static_cast<cDialog::fnDlgProc>(&cMainDlg::OnConfig);
	_events[ID_DRIVER_ENABLE] = static_cast<cDialog::fnDlgProc>(&cMainDlg::OnEnable);
	_events[ID_DRIVER_DISABLE] = static_cast<cDialog::fnDlgProc>(&cMainDlg::OnDisable);
	_events[IDC_EXISTING] = static_cast<cDialog::fnDlgProc>(&cMainDlg::OnExisting);
	_events[IDC_MANUAL] = static_cast<cDialog::fnDlgProc>(&cMainDlg::OnManual);
	_events[IDC_AUTO] = static_cast<cDialog::fnDlgProc>(&cMainDlg::OnAuto);
	_events[CBN_DROPDOWN] = static_cast<cDialog::fnDlgProc>(&cMainDlg::OnDropDown);
	_events[CBN_CLOSEUP] = static_cast<cDialog::fnDlgProc>(&cMainDlg::OnCloseUp);
	_events[IDC_SELECT] = static_cast<cDialog::fnDlgProc>(&cMainDlg::OnSelect);
	_events[IDC_ADD] = static_cast<cDialog::fnDlgProc>(&cMainDlg::OnAdd);
	_events[IDC_REMOVE] = static_cast<cDialog::fnDlgProc>(&cMainDlg::OnRemove);
	_events[IDC_CLEAR] = static_cast<cDialog::fnDlgProc>(&cMainDlg::OnClear);
	_events[IDC_INJECT] = static_cast<cDialog::fnDlgProc>(&cMainDlg::OnInject);
	_events[IDC_EJECT] = static_cast<cDialog::fnDlgProc>(&cMainDlg::OnEject);
}

cMainDlg::~cMainDlg()
{
}

INT_PTR cMainDlg::OnInit(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	cDialog::OnInit(hDlg, message, wParam, lParam);

	HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON));
	SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	DeleteObject(hIcon);

	_procList.Attach(_hwnd, IDC_PROCESS);
	_select.Attach(_hwnd, IDC_SELECT);
	_existing.Attach(_hwnd, IDC_EXISTING);
	_manual.Attach(_hwnd, IDC_MANUAL);
	_auto.Attach(_hwnd, IDC_AUTO);
	_modules.Attach(_hwnd, IDC_MODULES);
	_remove.Attach(_hwnd, IDC_REMOVE);
	_clear.Attach(_hwnd, IDC_CLEAR);
	_inject.Attach(_hwnd, IDC_INJECT);
	_eject.Attach(_hwnd, IDC_EJECT);

	ListView_SetExtendedListViewStyle(_modules.GetHwnd(), LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_CHECKBOXES);

	ChangeWindowMessageFilterEx(hDlg, WM_DROPFILES, MSGFLT_ADD, nullptr);
	ChangeWindowMessageFilterEx(hDlg, 0x49, MSGFLT_ADD, nullptr);

	if (blackbone::Driver().loaded())
		EnableMenuItem(GetMenu(_hwnd), ID_ADVANCED_PROTECT, MF_ENABLED);

	RECT ModRect, ClientRect;

	GetClientRect(_modules.GetHwnd(), &ModRect);
	GetClientRect(_hwnd, &ClientRect);

	_modules.AddColumn(L"X", 20);
	_modules.AddColumn(L"Name", (ModRect.right - ModRect.left) / 2, 1);
	_modules.AddColumn(L"Architecture", (ModRect.right - ModRect.left) / 4, 2);
	_modules.AddColumn(L"Native", (ModRect.right - ModRect.left) / 4 - 20, 3);

	_info.Attach(CreateStatusWindow(WS_CHILD | WS_VISIBLE, L"", _hwnd, IDR_STATUS));
	_info.SetParts({ (ClientRect.right - ClientRect.left) / 2 - 20, (ClientRect.right - ClientRect.left) / 2 + 20, -1 });
	
	_info.SetText(0, L"Idle");

#ifdef USE64
	_info.SetText(1, L"64-bit");
#else
	_info.SetText(1, L"32-bit");
#endif

	_info.SetText(2, L"Profile: default");

	LoadConfig(_defConfig);

	return TRUE;
}

INT_PTR cMainDlg::OnClose(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	SaveConfig(_defConfig);

	return cDialog::OnClose(hDlg, message, wParam, lParam);
}

INT_PTR cMainDlg::OnNotify(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (((LPNMHDR)lParam)->code)
	{
#ifdef USE64
	case HDN_BEGINTRACK:
		SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
		break;
#else
	case HDN_BEGINTRACK:
		SetWindowLong(hDlg, DWL_MSGRESULT, TRUE);
		break;
#endif
	case LVN_ITEMCHANGED:
		if (((LPNMLISTVIEW)lParam)->uChanged & LVIF_STATE)
		{
			switch (((LPNMLISTVIEW)lParam)->uNewState & LVIS_STATEIMAGEMASK)
			{
			case INDEXTOSTATEIMAGEMASK(1):
			case INDEXTOSTATEIMAGEMASK(2):
				for (int idx = 0; idx < _modules.GetCount(); idx++)
					_checks[idx] = _modules.Checked(idx);

				UpdateInterface();
				break;
			}
		}

		break;
	}

	return TRUE;
}

INT_PTR cMainDlg::OnDragDrop(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	wchar_t path[MAX_PATH] = { NULL };
	HDROP hDrop = (HDROP)wParam;

	if (DragQueryFile(hDrop, 0, path, ARRAYSIZE(path)) != 0)
	{
		if (LoadImageFile(path, false) == ERROR_SUCCESS)
			_profileMgr.config().initRoutine = L"";

		UpdateInterface();
	}

	return TRUE;
}

INT_PTR cMainDlg::OnOpen(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	std::wstring path;

#ifdef USE64
	if (OpenFileDialog(L"Project-X 64-bit profile (*.pjx64)\0*.pjx64\0", 1, path, false, L"", _hwnd))
#else
	if (OpenFileDialog(L"Project-X 32-bit profile (*.pjx32)\0*.pjx32\0", 1, path, false, L"", _hwnd))
#endif
	{
		_procList.Reset();
		_modules.Reset();
		_profileMgr.config().images.clear();
		_images.clear();
		_exports.clear();
		_checks.clear();

		LoadConfig(path);
		_info.SetText(2, L"Profile: " + blackbone::Utils::StripPath(path));
	}

	return TRUE;
}

INT_PTR cMainDlg::OnSave(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	std::wstring path;

#ifdef USE64
	if (OpenFileDialog(L"Project-X 64-bit profile (*.pjx64)\0*.pjx64\0", 1, path, true, L"pjx64", _hwnd))
#else
	if (OpenFileDialog(L"Project-X 32-bit profile (*.pjx32)\0*.pjx32\0", 1, path, true, L"pjx32", _hwnd))
#endif
	{
		SaveConfig(path);
		_info.SetText(2, L"Profile: " + blackbone::Utils::StripPath(path));
	}

	return TRUE;
}

INT_PTR cMainDlg::OnProtect(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	NTSTATUS status = STATUS_SUCCESS;

	if (blackbone::Driver().loaded())
	{
		status = blackbone::Driver().ProtectProcess(GetCurrentProcessId(), Policy_Enable);
		status |= blackbone::Driver().UnlinkHandleTable(GetCurrentProcessId());
	}

	if (!NT_SUCCESS(status))
	{
		wchar_t errBuf[1024] = { NULL };
		wsprintf(errBuf, L"Failed to protect Project-X.\n%ls\nStatus code: 0x%X.", blackbone::Utils::GetErrorDescription(status).c_str(), status);
		cMessage::ShowError(_hwnd, errBuf);
	}

	else
		cMessage::ShowInfo(hDlg, L"Project-X successfully protected.");

	return TRUE;
}

INT_PTR cMainDlg::OnConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	blackbone::pe::vecExports diff, tmp;

	if (!_exports.empty())
	{
		diff = tmp = _exports[0];
		for (size_t i = 1; i < _exports.size(); i++)
		{
			diff.clear();
			std::set_intersection(_exports[i].begin(), _exports[i].end(), tmp.begin(), tmp.end(), std::inserter(diff, diff.begin()));
			tmp = diff;
		}
	}

	cConfigDlg ConfigDlg(_profileMgr, _defConfig);
	ConfigDlg.SetExports(diff);

	ConfigDlg.RunModal(_hwnd);
	UpdateInterface();

	return TRUE;
}

INT_PTR cMainDlg::OnEnable(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	TestSigningMode(true);

	return TRUE;
}

INT_PTR cMainDlg::OnDisable(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	TestSigningMode(false);

	return TRUE;
}

INT_PTR cMainDlg::OnExisting(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	_profileMgr.config().processMode = Existing;
	_processPath.clear();
	_procList.Reset();
	UpdateInterface();

	return TRUE;
}

INT_PTR cMainDlg::OnManual(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	_profileMgr.config().processMode = Manual;
	_processPath.clear();
	_procList.Reset();
	UpdateInterface();

	return TRUE;
}

INT_PTR cMainDlg::OnAuto(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	_profileMgr.config().processMode = Auto;
	_processPath.clear();
	_procList.Reset();
	UpdateInterface();

	return TRUE;
}

INT_PTR cMainDlg::OnDropDown(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (LOWORD(wParam) == IDC_PROCESS)
		FillProcessList();

	return TRUE;
}

INT_PTR cMainDlg::OnCloseUp(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (LOWORD(wParam) == IDC_PROCESS)
	{
		SetActiveProcess((DWORD)_procList.ItemData(_procList.Selection()), _procList.SelectedText().substr(0, _procList.SelectedText().find(L'(') - 1));
		UpdateInterface();
	}

	return TRUE;
}

INT_PTR cMainDlg::OnSelect(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	std::wstring path;
	cProcessDlg ProcessDlg(_processPath);

	switch (_profileMgr.config().processMode)
	{
	case Manual:
		ProcessDlg.RunModal(_hwnd);
		if (!ProcessDlg._processName.empty())
			SetActiveProcess(0, ProcessDlg._processName);
		else
			_procList.Reset();
		break;

	case Auto:
		OpenFileDialog(L"All (*.*)\0*.*\0Executable image (*.exe)\0*.exe\0", 2, path, false, L"", _hwnd);
		if (!path.empty())
			SetActiveProcess(0, path);
		else
			_procList.Reset();
		break;
	}

	UpdateInterface();

	return TRUE;
}

INT_PTR cMainDlg::OnAdd(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	std::wstring path;

	OpenFileDialog(L"All (*.*)\0*.*\0Dynamic link library (*.dll)\0*.dll\0System driver (*.sys)\0*.sys\0", (_profileMgr.config().injectMode == KernelDMap) ? 3 : 2, path, false, L"", _hwnd);

	if (!path.empty() && LoadImageFile(path, false) == ERROR_SUCCESS)
		_profileMgr.config().initRoutine = L"";

	UpdateInterface();

	return TRUE;
}

INT_PTR cMainDlg::OnRemove(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	for (int idx = _modules.GetCount() - 1; idx >= 0; idx--)
	{
		if (_modules.Checked(idx))
		{
			_modules.RemoveItem(idx);
			_images.erase(_images.begin() + idx);
			_exports.erase(_exports.begin() + idx);
			_checks.erase(_checks.begin() + idx);
		}
	}

	UpdateInterface();

	return TRUE;
}

INT_PTR cMainDlg::OnClear(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	_profileMgr.config().images.clear();
	_images.clear();
	_exports.clear();
	_checks.clear();
	_modules.Reset();

	UpdateInterface();

	return TRUE;
}

INT_PTR cMainDlg::OnInject(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	std::lock_guard<std::mutex> lg(_lock);
	std::thread(&cMainDlg::Inject, this).detach();

	return TRUE;
}

INT_PTR cMainDlg::OnEject(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto pid = _procList.ItemData(_procList.Selection());

	if (pid > 0 && _core.process().pid() != pid)
	{
		auto status = _core.process().Attach(pid);

		if (status != STATUS_SUCCESS)
		{
			wchar_t errBuf[1024] = { NULL };
			wsprintf(errBuf, L"Cannot attach to process.\n%ls\nStatus code: 0x%X.", blackbone::Utils::GetErrorDescription(status).c_str(), status);	
			cMessage::ShowError(_hwnd, errBuf);
			return TRUE;
		}
	}

	if (!_core.process().valid())
	{
		cMessage::ShowWarning(_hwnd, L"Please select a valid process before ejection.");
		return TRUE;
	}

	cEjectDlg EjectDlg(_core.process(), _processPath);

	EjectDlg.RunModal(_hwnd);

	return TRUE;
}

