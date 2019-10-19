#pragma once

#include "../../Resources/Resource.h"
#include "../WinAPI/Dialog.hpp"
#include "../WinAPI/EditBox.hpp"

class cProcessDlg : public cDialog
{
public:

	cProcessDlg(HINSTANCE instance, const std::wstring& procName);
	~cProcessDlg() {};

	std::wstring _processName;

private:

	DLG_HANDLER(OnInit);
	DLG_HANDLER(OnOK);
	DLG_HANDLER(OnCancel);

protected:

	ctrl::cEditBox _process;

	HINSTANCE _instance = NULL;
};