#pragma once

#include "../../Resources/Resource.h"
#include "../Config/ConfigDlg.h"
#include "../Process/ProcessDlg.h"
#include "../Wait/WaitDlg.h"
#include "../Eject/EjectDlg.h"
#include "../WinAPI/Dialog.hpp"
#include "../WinAPI/Button.hpp"
#include "../WinAPI/ComboBox.hpp"
#include "../WinAPI/ListView.hpp"
#include "../WinAPI/StatusBar.hpp"
#include "../WinAPI/Message.hpp"
#include "../../Profiler/Profiler.h"
#include "../../Core/InjectionCore.h"

class cMainDlg : public cDialog
{
public:

	enum eStartAction
	{
		Nothing,
		LoadProfile,
		RunProfile
	};

	enum eColumnID
	{
		Name,
		Architecture,
		Native
	};

	cMainDlg(HINSTANCE instance, eStartAction action, const std::wstring& defConfig = L"");
	~cMainDlg() {};

	INT LoadAndInject()
	{
		LoadConfig(_defConfig);
		Inject();
		return 0;
	}

private:

	DWORD LoadConfig(const std::wstring& path = L"");
	DWORD SaveConfig(const std::wstring& path = L"");
	DWORD FillProcessList();
	DWORD SetActiveProcess(DWORD pid, const std::wstring& path);
	void Inject();
	DWORD UpdateInterface();
	BOOL OpenFileDialog(const wchar_t* Filter, int Index, std::wstring& Path, bool Save = false, const std::wstring& Ext = L"", HWND hWnd = NULL);
	DWORD LoadImageFile(const std::wstring& path, bool checked);
	void AddToModuleList(std::shared_ptr<blackbone::pe::PEImage>& img, bool checked);
	void TestSigningMode(bool enable);

	DLG_HANDLER(OnInit);
	DLG_HANDLER(OnClose);
	DLG_HANDLER(OnNotify);
	DLG_HANDLER(OnDragDrop);
	DLG_HANDLER(OnOpen);
	DLG_HANDLER(OnSave);
	DLG_HANDLER(OnProtect);
	DLG_HANDLER(OnConfig);
	DLG_HANDLER(OnEnable);
	DLG_HANDLER(OnDisable);
	DLG_HANDLER(OnExisting);
	DLG_HANDLER(OnManual);
	DLG_HANDLER(OnAuto);
	DLG_HANDLER(OnDropDown);
	DLG_HANDLER(OnCloseUp);
	DLG_HANDLER(OnSelect);
	DLG_HANDLER(OnAdd);
	DLG_HANDLER(OnRemove);
	DLG_HANDLER(OnClear);
	DLG_HANDLER(OnInject);
	DLG_HANDLER(OnEject);

protected:
	
	eStartAction _action;
	std::wstring _defConfig;
	cInjectionCore _core;
	cProfile _profileMgr;
	std::wstring _processPath;
	vecPEImages _images;       
	vecImageExports _exports;
	vecImageChecks _checks;
	std::mutex _lock;

	ctrl::cButton _existing;
	ctrl::cButton _manual;
	ctrl::cButton _auto;
	ctrl::cButton _select;
	ctrl::cButton _remove;
	ctrl::cButton _clear;
	ctrl::cButton _inject;
	ctrl::cButton _eject;

	ctrl::cComboBox _procList;
	ctrl::cListView _modules;
	ctrl::cStatusBar _info;

	HINSTANCE _instance = NULL;
};
