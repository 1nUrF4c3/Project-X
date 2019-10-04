﻿#include "EjectDlg.h"

cEjectDlg::cEjectDlg(blackbone::Process& proc, const std::wstring& procName) : cDialog(IDD_EJECT), _process(proc), _processName(procName)
{
	_messages[WM_INITDIALOG] = static_cast<cDialog::fnDlgProc>(&cEjectDlg::OnInit);
	_messages[WM_CLOSE] = static_cast<cDialog::fnDlgProc>(&cEjectDlg::OnClose);
	_messages[WM_NOTIFY] = static_cast<cDialog::fnDlgProc>(&cEjectDlg::OnNotify);

	_events[IDCANCEL] = static_cast<cDialog::fnDlgProc>(&cEjectDlg::OnClose);
	_events[IDC_UNLOAD] = static_cast<cDialog::fnDlgProc>(&cEjectDlg::OnEject);
	_events[IDC_REFRESH] = static_cast<cDialog::fnDlgProc>(&cEjectDlg::OnRefresh);
}

cEjectDlg::~cEjectDlg()
{
}

INT_PTR cEjectDlg::OnInit(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	cDialog::OnInit(hDlg, message, wParam, lParam);

	HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON));
	SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	DeleteObject(hIcon);

	_unload.Attach(_hwnd, IDC_UNLOAD);

	LVCOLUMNW lvc = { NULL };
	_modList.Attach(GetDlgItem(hDlg, IDC_EJECTMODULES));

	ListView_SetExtendedListViewStyle(_modList.GetHwnd(), LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_CHECKBOXES);

	SetWindowText(_hwnd, (L"Eject Modules - " + _processName).c_str());

	RECT Rect;

	GetClientRect(_modList.GetHwnd(), &Rect);

	_modList.AddColumn(L"X", 20);
	_modList.AddColumn(L"Name", (int)((Rect.right - Rect.left) / 5 * 1.5f), Name);
	_modList.AddColumn(L"Image base", (int)((Rect.right - Rect.left) / 5 * 1.5f), ImageBase);
	_modList.AddColumn(L"Platform", (Rect.right - Rect.left) / 5 * 1 - 20, Platform);
	_modList.AddColumn(L"Load type", (Rect.right - Rect.left) / 5 * 1, LoadType);

	RefreshList();

	return TRUE;
}

INT_PTR cEjectDlg::OnClose(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	return cDialog::OnClose(hDlg, message, wParam, lParam);
}

INT_PTR cEjectDlg::OnNotify(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
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
				for (int idx = 0; idx < _modList.GetCount(); idx++)
					_checks[idx] = _modList.Checked(idx);

				UpdateInterface();
				break;
			}
		}

		break;
	}

	return TRUE;
}

INT_PTR cEjectDlg::OnEject(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (_process.valid())
	{
		for (int idx = _modList.GetCount() - 1; idx >= 0; idx--)
		{
			if (_modList.Checked(idx))
			{
				wchar_t* pEnd = nullptr;
				blackbone::module_t modBase = wcstoull(_modList.ItemText(idx, ImageBase).c_str(), &pEnd, 0x10);
				auto mod = _process.modules().GetModule(modBase);

				if (mod != nullptr)
					auto result = _process.modules().Unload(mod);

				else
					cMessage::ShowError(_hwnd, L"'" + mod->name + L"' - Module not found.");
			}
		}

		RefreshList();
	}

	return TRUE;
}

INT_PTR cEjectDlg::OnRefresh(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	RefreshList();
	return TRUE;
}

void cEjectDlg::RefreshList()
{
	_checks.clear();
	_modList.Reset();
	if (!_process.valid())
		return;

	auto modsLdr = _process.modules().GetAllModules(blackbone::LdrList);
	auto modsSec = _process.modules().GetAllModules(blackbone::Sections);
	auto modsPE = _process.modules().GetAllModules(blackbone::PEHeaders);

	decltype(modsLdr) modsAll;
	auto modsManual = _process.modules().GetManualModules();

	modsAll.insert(modsLdr.begin(), modsLdr.end());
	modsAll.insert(modsSec.begin(), modsSec.end());
	modsAll.insert(modsPE.begin(), modsPE.end());
	modsAll.insert(modsManual.begin(), modsManual.end());

	for (auto& mod : modsAll)
	{
		wchar_t address[64];
		wchar_t* platform = nullptr;
		wchar_t* detected = nullptr;

		wsprintf(address, L"0x%08I64x", mod.second->baseAddress);

		if (mod.second->type == blackbone::mt_mod32)
			platform = L"32-bit";
		else if (mod.second->type == blackbone::mt_mod64)
			platform = L"64-bit";
		else
			platform = L"Unknown";

		if (modsManual.count(mod.first))
			detected = L"Manual map";
		else if (modsLdr.count(mod.first))
			detected = L"Normal";
		else if (modsSec.count(mod.first))
			detected = L"Section only";
		else if (modsPE.count(mod.first))
			detected = L"PE header";
		else
			detected = L"Unknown";

		_checks.emplace_back(false);
		_modList.AddItem(mod.second->name, static_cast<LPARAM>(mod.second->baseAddress), { mod.second->name, address, platform, detected });
	}

	UpdateInterface();
}

void cEjectDlg::UpdateInterface()
{
	bool checked = false;

	for (auto& check : _checks)
		if (check)
			checked = true;

	if (checked)
		_unload.Enable();

	else
		_unload.Disable();
}