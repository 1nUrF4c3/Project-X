#pragma once

#include "../../Resources/Resource.h"
#include "../WinAPI/Dialog.hpp"
#include "../WinAPI/Static.hpp"
#include "../WinAPI/ProgressBar.hpp"
#include "../../Core/InjectionCore.h"

#include <thread>
#include <future>

class cWaitDlg : public cDialog
{
public:

	cWaitDlg(cInjectionCore& core, sInjectContext& context);
	~cWaitDlg();

	inline DWORD status() const { return _status; }

private:

	DWORD WaitForInjection();

	DLG_HANDLER(OnInit);
	DLG_HANDLER(OnCancel);

protected:

	ctrl::cStatic _waitText;
	ctrl::cProgressBar _progressBar;

	cInjectionCore& _core;
	sInjectContext& _context;

	std::thread _waitThread;
	DWORD _status = STATUS_SUCCESS;
};