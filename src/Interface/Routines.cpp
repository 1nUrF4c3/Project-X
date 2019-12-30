#include "Main/MainDlg.h"
#include "../Core/DriverExtractor.hpp"

DWORD cMainDlg::LoadConfig(const std::wstring& path)
{
	auto& cfg = _profileMgr.config();
	
	if (_profileMgr.Load(_hwnd, path))
	{
		for (size_t idx = 0; idx < cfg.images.size(); idx++)
			LoadImageFile(cfg.images[idx], cfg.checks[idx]);

		if (!cfg.procName.empty())
		{
			if (cfg.processMode == Existing)
			{
				std::vector<DWORD> pidList = blackbone::Process::EnumByName(cfg.procName);

				if (!pidList.empty())
				{
					cfg.pid = pidList.front();

					wchar_t text[255] = { NULL };
					swprintf_s(text, L"%ls (0x%X)", cfg.procName.c_str(), pidList.front());

					auto idx = _procList.Add(text, pidList.front());
					_procList.Selection(idx);

					SetActiveProcess(pidList.front(), cfg.procName);
				}
			}

			else
			{
				SetActiveProcess(0, cfg.procName);
			}
		}

		_existing.Checked(false);
		_manual.Checked(false);
		_auto.Checked(false);

		switch (cfg.processMode)
		{
		case Existing:
			_existing.Checked(true);
			break;

		case Manual:
			_manual.Checked(true);
			break;

		case Auto:
			_auto.Checked(true);
			break;
		}

		if (cfg.injectMode >= KernelSTD)
		{
			cDriverExtractor::Instance().Extract();
			if (!NT_SUCCESS(blackbone::Driver().EnsureLoaded()))
			{
				cDriverExtractor::Instance().Cleanup();
				cfg.injectMode = Standard;
			}
		}

		if (!_defConfig.empty())
			_info.SetText(2, L"Profile: " + blackbone::Utils::StripPath(_defConfig));
	}

	else
		_existing.Checked(true);

	UpdateInterface();

	return ERROR_SUCCESS;
}

DWORD cMainDlg::SaveConfig(const std::wstring& path)
{
	auto& cfg = _profileMgr.config();

	cfg.procName = _processPath;
	cfg.images.clear();
	cfg.checks.clear();

	for (auto& img : _images)
		cfg.images.emplace_back(img->path());

	for (auto& check : _checks)
		cfg.checks.emplace_back(check);
	
	cfg.processMode = (_existing.Checked() ? Existing : (_manual.Checked() ? Manual : Auto));

	_profileMgr.Save(_hwnd, path);

	UpdateInterface();

	return ERROR_SUCCESS;
}

DWORD cMainDlg::FillProcessList()
{
	_procList.Reset();

	auto found = blackbone::Process::EnumByNameOrPID(0, L"").result(std::vector<blackbone::ProcessInfo>());

	for (auto& proc : found)
	{
		wchar_t text[255] = { NULL };
		swprintf_s(text, L"%ls (0x%X)", proc.imageName.c_str(), proc.pid);

		_procList.Add(text, proc.pid);
	}

	return ERROR_SUCCESS;
}

DWORD cMainDlg::SetActiveProcess(DWORD pid, const std::wstring& path)
{
	_processPath = path;

	if (pid == 0)
	{
		_procList.Reset();
		_procList.Add(blackbone::Utils::StripPath(path), 0);
		_procList.Selection(0);
	}

	return ERROR_SUCCESS;
}

void cMainDlg::Inject()
{
	_inject.Disable();

	sInjectContext context;
	auto& cfg = _profileMgr.config();
	NTSTATUS status = STATUS_SUCCESS;

	for (size_t idx = 0; idx < _images.size(); idx++)
		if (_checks[idx])
			context.images.emplace_back(_images[idx]);

	context.cfg = cfg;
	context.pid = _procList.ItemData(_procList.Selection());
	context.procPath = _processPath;
	context.skippedCount = 0;
	context.procList.clear();
	context.procDiff.clear();

	_info.SetText(0, L"Injecting...");

	if (context.cfg.processMode == Manual)
	{
		cWaitDlg WaitDlg(_instance, _core, context);
		WaitDlg.RunModal(_hwnd);
		status = WaitDlg.status();
	}

	if (NT_SUCCESS(status))
		status = _core.InjectMultiple(&context);

	if (NT_SUCCESS(status) && _action != RunProfile)
	{
		if (cfg.close)
		{
			SaveConfig(_defConfig);
			CloseDialog();
			return;
		}

		cMessage::ShowInfo(_hwnd, L"Injection completed successfully.");
	}

	_inject.Enable();
	_info.SetText(0, L"Idle");
}

DWORD cMainDlg::UpdateInterface()
{
	if (_existing.Checked())
	{
		_procList.Enable();
		_select.Disable();
	}

	else
	{
		_procList.Disable();
		_select.Enable();
	}
	
	if (_profileMgr.config().injectMode == KernelDMap)
	{
		_existing.Disable();
		_manual.Disable();
		_auto.Disable();
		_procList.Disable();
		_select.Disable();
	}
	
	else
	{
		_existing.Enable();
		_manual.Enable();
		_auto.Enable();
	}

	bool checked = false;

	for (auto& check : _checks)
		if (check)
			checked = true;

	if (!_images.empty() && checked)
		_remove.Enable();

	else
		_remove.Disable();

	if (!_images.empty())
		_clear.Enable();

	else
		_clear.Disable();

	if ((_profileMgr.config().injectMode == KernelDMap || _procList.Selection() > -1) && !_images.empty() && checked)
		_inject.Enable();

	else
		_inject.Disable();

	if (_procList.Selection() > -1 && _existing.Checked())
		_eject.Enable();

	else
		_eject.Disable();

	return ERROR_SUCCESS;
}

BOOL cMainDlg::OpenFileDialog(const wchar_t* Filter, int Index, std::wstring& Path, bool Save, const std::wstring& Ext, HWND hWnd)
{
	wchar_t FilePath[MAX_PATH];
	OPENFILENAME OFN;

	ZeroMemory(&OFN, sizeof(OPENFILENAME));

	OFN.lStructSize = sizeof(OPENFILENAME);
	OFN.hwndOwner = hWnd;
	OFN.lpstrFile = FilePath;
	OFN.lpstrFile[0] = '\0';
	OFN.nMaxFile = sizeof(FilePath);
	OFN.lpstrFilter = Filter;
	OFN.nFilterIndex = Index;
	OFN.lpstrFileTitle = NULL;
	OFN.nMaxFileTitle = NULL;
	OFN.lpstrInitialDir = NULL;

	if (!Save)
		OFN.Flags = OFN_PATHMUSTEXIST;

	else
	{
		OFN.Flags = OFN_OVERWRITEPROMPT;
		OFN.lpstrDefExt = Ext.c_str();

	}

	BOOL Result = Save ? GetSaveFileName(&OFN) : GetOpenFileName(&OFN);

	if (Result)
		Path = FilePath;

	return Result;
}

DWORD cMainDlg::LoadImageFile(const std::wstring& path, bool checked)
{
	std::shared_ptr<blackbone::pe::PEImage> img(new blackbone::pe::PEImage);
	blackbone::pe::vecExports exports;

	if (std::find_if(_images.begin(), _images.end(),
		[&path](std::shared_ptr<blackbone::pe::PEImage>& img) { return path == img->path(); }) != _images.end())
	{
		cMessage::ShowInfo(_hwnd, L"'" + blackbone::Utils::StripPath(path) + L"' - Module is already loaded.");
		return ERROR_ALREADY_EXISTS;
	}

	if (!NT_SUCCESS(img->Load(path)))
	{
		cMessage::ShowError(_hwnd, L"'" + blackbone::Utils::StripPath(path) + L"' - File is not a valid PE image.");
		
		img->Release();
		return ERROR_INVALID_IMAGE_HASH;
	}

	if (img->pureIL() && img->net().Init(path))
	{
		blackbone::ImageNET::mapMethodRVA methods;
		img->net().Parse(&methods);

		for (auto& entry : methods)
		{
			std::wstring name = entry.first.first + L"." + entry.first.second;
			exports.push_back(blackbone::pe::ExportData(blackbone::Utils::WstringToAnsi(name), 0));
		}
	}
	
	else
		img->GetExports(exports);

	AddToModuleList(img, checked);
	_exports.emplace_back(exports);

	img->Release(true);
	return ERROR_SUCCESS;
}

void cMainDlg::AddToModuleList(std::shared_ptr<blackbone::pe::PEImage>& img, bool checked)
{
	wchar_t* platform = nullptr;
	wchar_t* managed = nullptr;

	if (img->mType() == blackbone::mt_mod64)
		platform = L"64-bit";
	else if (img->mType() == blackbone::mt_mod32)
		platform = L"32-bit";
	else
		platform = L"Unknown";

	if (!img->pureIL())
		managed = L"Yes";
	else
		managed = L"No";

	_images.emplace_back(img);
	_checks.emplace_back(checked);
	_modules.Checked(_modules.AddItem(blackbone::Utils::StripPath(img->path()), 0, { blackbone::Utils::StripPath(img->path()), platform, managed }), checked);
}

void cMainDlg::TestSigningMode(bool enable)
{
	if (cMessage::ShowQuestion(_hwnd, enable ? L"Enable test signing mode to allow driver usage?" : L"Disable test signing mode to disallow driver usage?"))
	{
		STARTUPINFOW si = { NULL };
		PROCESS_INFORMATION pi = { NULL };
		wchar_t windir[255] = { NULL };
		PVOID oldVal = nullptr;

		GetWindowsDirectory(windir, sizeof(windir));
		std::wstring bcdpath = std::wstring(windir) + L"\\system32\\cmd.exe";

		Wow64DisableWow64FsRedirection(&oldVal);

		if (CreateProcess(bcdpath.c_str(), enable ? L"/C Bcdedit.exe -set TESTSIGNING ON" : L"/C Bcdedit.exe -set TESTSIGNING OFF", NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
		{
			cMessage::ShowWarning(_hwnd, enable ? L"Test signing mode enabled, please reboot your computer." : L"Test signing mode disabled, please reboot your computer.");

			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}

		else
			cMessage::ShowError(_hwnd, enable ? L"Failed to enable test signing mode." : L"Failed to disable test signing mode.");

		Wow64RevertWow64FsRedirection(oldVal);
	}
}