#pragma once

#include "../../Resources/Resource.h"
#include "../WinAPI/Dialog.hpp"
#include "../WinAPI/ListView.hpp"
#include "../WinAPI/Button.hpp"
#include "../WinAPI/Message.hpp"
#include "../../Core/InjectionCore.h"

#include <Process/Process.h>
#include <Misc/Utils.h>

class cEjectDlg : public cDialog
{
public:

	enum eColumnID
	{
		Name = 1,
		ImageBase,
		Platform,
		LoadType
	};

	cEjectDlg(blackbone::Process& proc, const std::wstring& procName);
	~cEjectDlg();

private:

	void RefreshList();
	void UpdateInterface();

	DLG_HANDLER(OnInit);
	DLG_HANDLER(OnClose);
	DLG_HANDLER(OnNotify);
	DLG_HANDLER(OnEject);
	DLG_HANDLER(OnRefresh);

protected:

	std::wstring _processName;
	blackbone::Process& _process;

	vecImageChecks _checks;

	ctrl::cListView _modList;
	ctrl::cButton _unload;
};