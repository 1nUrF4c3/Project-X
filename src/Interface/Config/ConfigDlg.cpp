#include "ConfigDlg.h"

cConfigDlg::cConfigDlg(HINSTANCE instance, cProfile& cfgMgr, const std::wstring& defConfig) : cDialog(IDD_CONFIG), _instance(instance), _profileMgr(cfgMgr), _defConfig(defConfig)
{
	_messages[WM_INITDIALOG] = static_cast<cDialog::fnDlgProc>(&cConfigDlg::OnInit);
	_messages[WM_CLOSE] = static_cast<cDialog::fnDlgProc>(&cConfigDlg::OnCancel);

	_events[IDC_CONFIGOK] = static_cast<cDialog::fnDlgProc>(&cConfigDlg::OnOK);
	_events[IDCANCEL] = static_cast<cDialog::fnDlgProc>(&cConfigDlg::OnCancel);
	_events[CBN_SELCHANGE] = static_cast<cDialog::fnDlgProc>(&cConfigDlg::OnSelChange);
}

INT_PTR cConfigDlg::OnInit(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	cDialog::OnInit(hDlg, message, wParam, lParam);

	HICON hIcon = LoadIcon(_instance, MAKEINTRESOURCE(IDI_ICON));
	SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	DeleteObject(hIcon);

	_injectionType.Attach(_hwnd, IDC_INJMETHOD);
	_initFuncList.Attach(_hwnd, IDC_INITEXPORT);

	_procCmdLine.Attach(_hwnd, IDC_CMDLINE);
	_initArg.Attach(_hwnd, IDC_INITPARAMETER);
	_injClose.Attach(_hwnd, IDC_INJCLOSE);
	_unlink.Attach(_hwnd, IDC_UNLINK);
	_erasePE.Attach(_hwnd, IDC_NATIVEWPHDR);
	_injIndef.Attach(_hwnd, IDC_INJEACH);
	_krnHandle.Attach(_hwnd, IDC_KRNHANDLE);

	_delay.Attach(_hwnd, IDC_DELAY);
	_period.Attach(_hwnd, IDC_INTERVAL);

	_skipProc.Attach(_hwnd, IDC_SKIP);

	_mmapOptions.addLdrRef.Attach(GetDlgItem(_hwnd, IDC_LDRREF));
	_mmapOptions.manualImport.Attach(GetDlgItem(_hwnd, IDC_MMAPIMPORTS));
	_mmapOptions.noTls.Attach(GetDlgItem(_hwnd, IDC_IGNORETLS));
	_mmapOptions.noExceptions.Attach(GetDlgItem(_hwnd, IDC_NOEXCEPT));
	_mmapOptions.wipeHeader.Attach(GetDlgItem(_hwnd, IDC_MMAPWPHDR));
	_mmapOptions.hideVad.Attach(GetDlgItem(_hwnd, IDC_HIDEVAD));

	_injectionType.Add(L"Standard", Standard);
	_injectionType.Add(L"Thread Hijack", ThreadHijack);
	_injectionType.Add(L"Manual Map", ManualMap);

	if (blackbone::Driver().loaded())
	{
		_injectionType.Add(L"Kernel (Standard)", KernelSTD);
		_injectionType.Add(L"Kernel (APC)", KernelAPC);
		_injectionType.Add(L"Kernel (Manual Map)", KernelMMap);
		_injectionType.Add(L"Kernel (Driver Map)", KernelDMap);
		_mmapOptions.hideVad.Enable();
	}

	_initFuncList.Reset();
	for (auto& exp : _exports)
		_initFuncList.Add(blackbone::Utils::AnsiToWstring(exp.name));

	UpdateFromConfig();

	return TRUE;
}

INT_PTR cConfigDlg::OnOK(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	SaveConfig();
	return cDialog::OnClose(hDlg, message, wParam, lParam);
}

INT_PTR cConfigDlg::OnCancel(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	return cDialog::OnClose(hDlg, message, wParam, lParam);
}

INT_PTR cConfigDlg::OnSelChange(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (LOWORD(wParam) == IDC_INJMETHOD)
	{
		_lastInjectionType = _injectionType.Selection();
		UpdateInterface();
	}

	return TRUE;
}

void cConfigDlg::UpdateInterface()
{
	auto& cfg = _profileMgr.config();

	_initFuncList.Enable();
	_initArg.Enable();
	_erasePE.Enable();
	_unlink.Enable();

	if (blackbone::Driver().loaded())
		_krnHandle.Enable();

	switch (cfg.processMode)
	{
	case Existing:
		_procCmdLine.Disable();
		_skipProc.Disable();
		_injIndef.Disable();
		break;

	case Manual:
		_procCmdLine.Disable();
		_skipProc.Enable();
		_injIndef.Enable();
		break;

	case Auto:
		_procCmdLine.Enable();
		_skipProc.Disable();
		_injIndef.Disable();
		break;
	};

	switch (_lastInjectionType)
	{
	case Standard:
	case ThreadHijack:
		_mmapOptions.manualImport.Disable();
		_mmapOptions.addLdrRef.Disable();
		_mmapOptions.wipeHeader.Disable();
		_mmapOptions.noTls.Disable();
		_mmapOptions.noExceptions.Disable();
		_mmapOptions.hideVad.Disable();
		break;

	case ManualMap:
		_mmapOptions.manualImport.Enable();
		_mmapOptions.addLdrRef.Enable();
		_mmapOptions.wipeHeader.Enable();
		_mmapOptions.noTls.Enable();
		_mmapOptions.noExceptions.Enable();

		if (blackbone::Driver().loaded())
			_mmapOptions.hideVad.Enable();

		_erasePE.Disable();
		_unlink.Disable();
		break;

	case KernelSTD:
	case KernelAPC:
	case KernelMMap:
	case KernelDMap:
		_krnHandle.Disable();

		_mmapOptions.addLdrRef.Disable();

		if (_lastInjectionType == KernelMMap)
		{
			_mmapOptions.manualImport.Enable();
			_mmapOptions.wipeHeader.Enable();
			_mmapOptions.noTls.Enable();
			_mmapOptions.noExceptions.Enable();
			_mmapOptions.hideVad.Enable();

			_unlink.Disable();
			_erasePE.Disable();
		}

		else
		{
			_mmapOptions.manualImport.Disable();
			_mmapOptions.wipeHeader.Disable();
			_mmapOptions.noTls.Disable();
			_mmapOptions.noExceptions.Disable();
			_mmapOptions.hideVad.Disable();
		}

		if (_lastInjectionType == KernelDMap)
		{
			_unlink.Disable();
			_erasePE.Disable();
			_initFuncList.SelectedText(L"");
			_initFuncList.Disable();
			_initArg.Reset();
			_initArg.Disable();
			_procCmdLine.Disable();
		}

		break;
	}
}

DWORD cConfigDlg::UpdateFromConfig()
{
	auto& cfg = _profileMgr.config();

	_procCmdLine.Text(cfg.procCmdLine);
	_initArg.Text(cfg.initArgs);
	_initFuncList.Text(cfg.initRoutine);

	_delay.Text(std::to_wstring(cfg.delay));
	_period.Text(std::to_wstring(cfg.period));
	_skipProc.Text(std::to_wstring(cfg.skipProc));

	_unlink.Checked(cfg.unlink);
	_erasePE.Checked(cfg.erasePE);
	_injClose.Checked(cfg.close);
	_injIndef.Checked(cfg.injIndef);

	_injectionType.Selection(cfg.injectMode);

	if (blackbone::Driver().loaded())
		_krnHandle.Checked(cfg.krnHandle);

	MmapFlags((blackbone::eLoadFlags)cfg.mmapFlags);
	_lastInjectionType = _injectionType.Selection();

	UpdateInterface();

	return ERROR_SUCCESS;
}

DWORD cConfigDlg::SaveConfig()
{
	auto& cfg = _profileMgr.config();

	cfg.procCmdLine = _procCmdLine.Text();
	cfg.initRoutine = _initFuncList.SelectedText();
	cfg.initArgs = _initArg.Text();
	cfg.injectMode = _injectionType.Selection();
	cfg.mmapFlags = MmapFlags();
	cfg.unlink = _unlink.Checked();
	cfg.erasePE = _erasePE.Checked();
	cfg.close = _injClose.Checked();
	cfg.krnHandle = _krnHandle.Checked();
	cfg.delay = _delay.Integer();
	cfg.period = _period.Integer();
	cfg.skipProc = _skipProc.Integer();
	cfg.injIndef = _injIndef.Checked();

	_profileMgr.Save(_hwnd, _defConfig);

	return ERROR_SUCCESS;
}

blackbone::eLoadFlags cConfigDlg::MmapFlags()
{
	blackbone::eLoadFlags flags = blackbone::NoFlags;

	if (_mmapOptions.manualImport.Checked())
		flags |= blackbone::ManualImports;

	if (_mmapOptions.addLdrRef.Checked() && _injectionType.Selection() != KernelMMap)
		flags |= blackbone::CreateLdrRef;

	if (_mmapOptions.wipeHeader.Checked())
		flags |= blackbone::WipeHeader;

	if (_mmapOptions.noTls.Checked())
		flags |= blackbone::NoTLS;

	if (_mmapOptions.noExceptions.Checked())
		flags |= blackbone::NoExceptions;

	if (_mmapOptions.hideVad.Checked() && blackbone::Driver().loaded())
		flags |= blackbone::HideVAD;

	return flags;
}

DWORD cConfigDlg::MmapFlags(blackbone::eLoadFlags flags)
{
	if (!blackbone::Driver().loaded())
		flags &= ~blackbone::HideVAD;

	if (_injectionType.Selection() == KernelMMap)
		flags &= ~blackbone::CreateLdrRef;

	_mmapOptions.manualImport.Checked(flags & blackbone::ManualImports ? true : false);
	_mmapOptions.addLdrRef.Checked(flags & blackbone::CreateLdrRef ? true : false);
	_mmapOptions.wipeHeader.Checked(flags & blackbone::WipeHeader ? true : false);
	_mmapOptions.noTls.Checked(flags & blackbone::NoTLS ? true : false);
	_mmapOptions.noExceptions.Checked(flags & blackbone::NoExceptions ? true : false);
	_mmapOptions.hideVad.Checked(flags & blackbone::HideVAD ? true : false);

	return flags;
}