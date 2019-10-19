#pragma once

#include "../../Resources/Resource.h"
#include "../WinAPI/Dialog.hpp"
#include "../WinAPI/ComboBox.hpp"
#include "../WinAPI/Button.hpp"
#include "../WinAPI/EditBox.hpp"
#include "../../Profiler/Profiler.h"
#include "../../Core/InjectionCore.h"
#include "../../Core/DriverExtractor.hpp"

class cConfigDlg : public cDialog
{
public:

	cConfigDlg(HINSTANCE instance, cProfile& cfgMgr, const std::wstring& defConfig);
	~cConfigDlg() {};

	inline void SetExports(const blackbone::pe::vecExports& exports) { _exports = exports; }

private:

	void UpdateInterface();
	DWORD UpdateFromConfig();
	DWORD SaveConfig();
	blackbone::eLoadFlags MmapFlags();
	DWORD MmapFlags(blackbone::eLoadFlags flags);

	DLG_HANDLER(OnInit);
	DLG_HANDLER(OnOK);
	DLG_HANDLER(OnCancel);
	DLG_HANDLER(OnSelChange);

protected:

	cProfile& _profileMgr;
	std::wstring _defConfig;

	uint32_t _lastInjectionType;
	blackbone::pe::vecExports _exports;

	ctrl::cComboBox _injectionType; 
	ctrl::cComboBox _initFuncList;

	ctrl::cEditBox  _procCmdLine; 
	ctrl::cEditBox  _initArg;       
	ctrl::cEditBox  _delay;        
	ctrl::cEditBox  _period;        
	ctrl::cEditBox  _skipProc;     

	ctrl::cButton _injClose;        
	ctrl::cButton _unlink;            
	ctrl::cButton _erasePE;         
	ctrl::cButton _injIndef;
	ctrl::cButton _krnHandle;

	struct
	{
		ctrl::cButton addLdrRef;
		ctrl::cButton manualImport;
		ctrl::cButton noTls;
		ctrl::cButton noExceptions;
		ctrl::cButton wipeHeader;
		ctrl::cButton hideVad;

	} _mmapOptions;

	HINSTANCE _instance = NULL;
};