#include "InjectionCore.h"
#include <Process/RPC/RemoteFunction.hpp>
#include <iterator>

cInjectionCore::cInjectionCore(HWND& hMainDlg) : _hMainDlg(hMainDlg)
{
}

cInjectionCore::~cInjectionCore()
{
	std::vector<DWORD> existing = blackbone::Process::EnumByName(L""), mutual;
	std::sort(existing.begin(), existing.end());
	std::sort(_criticalProcList.begin(), _criticalProcList.end());
	std::set_intersection(existing.begin(), existing.end(), _criticalProcList.begin(), _criticalProcList.end(), std::back_inserter(mutual));
	if (mutual.empty())
		blackbone::Driver().Unload();
}

NTSTATUS cInjectionCore::GetTargetProcess(sInjectContext& context, PROCESS_INFORMATION& pi)
{
	NTSTATUS status = STATUS_SUCCESS;
	
	if (context.cfg.injectMode == KernelDMap)
		return status;

	if (context.cfg.processMode == Manual)
	{
		xlog::Normal("'%ls' - Waiting on program to launch.", blackbone::Utils::StripPath(context.procPath).c_str());

		auto procName = blackbone::Utils::StripPath(context.procPath);

		std::vector<blackbone::ProcessInfo> newList;

		if (context.procList.empty())
			context.procList = blackbone::Process::EnumByNameOrPID(0, procName).result(std::vector<blackbone::ProcessInfo>());

		for (context.waitActive = true;; Sleep(10))
		{
			if (!context.waitActive)
			{
				xlog::Warning("Process detection canceled by user.");
				return STATUS_REQUEST_ABORTED;
			}

			if (!context.procDiff.empty())
			{
				context.procList = newList;

				if (context.skippedCount < context.cfg.skipProc)
				{
					context.skippedCount++;
					context.procDiff.erase(context.procDiff.begin());

					continue;
				}

				context.pid = context.procDiff.front().pid;
				context.procDiff.erase(context.procDiff.begin());

				xlog::Verbose("Got process: 0x%X.", context.pid);
				break;
			}
			else
			{
				newList = blackbone::Process::EnumByNameOrPID(0, procName).result(std::vector<blackbone::ProcessInfo>());
				std::set_difference(
					newList.begin(), newList.end(),
					context.procList.begin(), context.procList.end(),
					std::inserter(context.procDiff, context.procDiff.begin())
				);
			}
		}
	}

	else if (context.cfg.processMode == Auto)
	{
		STARTUPINFOW si = { NULL };
		si.cb = sizeof(si);

		xlog::Normal("'%ls' - Auto-launching with parameters '%ls'.", blackbone::Utils::StripPath(context.procPath).c_str(), context.cfg.procCmdLine.c_str());

		if (!CreateProcess(context.procPath.c_str(), (LPWSTR)context.cfg.procCmdLine.c_str(),
			NULL, NULL, FALSE, NULL, NULL, blackbone::Utils::GetParent(context.procPath).c_str(), &si, &pi))
		{
			wchar_t errBuf[1024] = { NULL };
			wsprintf(errBuf, L"Failed to create new process.\n%ls\nStatus code: 0x%X.", blackbone::Utils::GetErrorDescription(LastNtStatus()).c_str(), LastNtStatus());
			cMessage::ShowError(_hMainDlg, errBuf);
			return LastNtStatus();
		}

		context.pid = pi.dwProcessId;

		if (context.cfg.krnHandle)
		{
			xlog::Normal("Escalating process handle access.");

			status = _process.Attach(pi.dwProcessId, PROCESS_QUERY_LIMITED_INFORMATION);
			if (NT_SUCCESS(status))
			{
				status = blackbone::Driver().PromoteHandle(
					GetCurrentProcessId(),
					_process.core().handle(),
					DEFAULT_ACCESS_P | PROCESS_QUERY_LIMITED_INFORMATION
				);
			}
		}

		else
		{
			status = _process.Attach(pi.dwProcessId);
		}

		if (NT_SUCCESS(status))
			_process.EnsureInit();

		if (!NT_SUCCESS(status))
		{
			xlog::Error("Failed to attach to process, status: 0x%X.", status);
			return status;
		}

		if (context.cfg.injectMode >= KernelSTD)
			_process.Detach();
	}

	if (context.cfg.injectMode < KernelSTD && context.cfg.processMode != Auto)
	{
		if (context.cfg.krnHandle)
		{
			xlog::Normal("Escalating process handle access.");

			status = _process.Attach(context.pid, PROCESS_QUERY_LIMITED_INFORMATION);
			if (NT_SUCCESS(status))
			{
				status = blackbone::Driver().PromoteHandle(
					GetCurrentProcessId(),
					_process.core().handle(),
					DEFAULT_ACCESS_P | PROCESS_QUERY_LIMITED_INFORMATION
				);
			}

			if (!NT_SUCCESS(status))
				xlog::Error("Failed to escalate process handle access, status 0x%X.", status);
		}

		else
			status = _process.Attach(context.pid);

		if (NT_SUCCESS(status) && !_process.valid())
			status = STATUS_PROCESS_IS_TERMINATING;

		if (status != STATUS_SUCCESS)
		{
			wchar_t errBuf[1024] = { NULL };
			wsprintf(errBuf, L"Cannot attach to the process.\n%ls\nStatus code: 0x%X.", blackbone::Utils::GetErrorDescription(status).c_str(), status);
			cMessage::ShowError(_hMainDlg, errBuf);

			return status;
		}

		bool ldrInit = false;
		if (_process.core().isWow64())
		{
			blackbone::_PEB32 peb32 = { NULL };
			_process.core().peb32(&peb32);
			ldrInit = peb32.Ldr != 0;
		}
		else
		{
			blackbone::_PEB64 peb64 = { NULL };
			_process.core().peb64(&peb64);
			ldrInit = peb64.Ldr != 0;
		}

		if (!ldrInit)
			_process.EnsureInit();
	}

	return STATUS_SUCCESS;
}

NTSTATUS cInjectionCore::ValidateContext(sInjectContext& context, const blackbone::pe::PEImage& img)
{
	if (context.images.empty())
	{
		cMessage::ShowError(_hMainDlg, L"Please add at least one module to inject.");
		return STATUS_INVALID_PARAMETER_1;
	}

	if (context.cfg.injectMode == KernelDMap)
	{
		if (img.mType() != blackbone::mt_mod64)
		{
			cMessage::ShowError(_hMainDlg, L"'" + img.name() + L" - Cannot map 32-bit drivers.");
			return STATUS_INVALID_IMAGE_WIN_32;
		}

		if (img.subsystem() != IMAGE_SUBSYSTEM_NATIVE)
		{
			cMessage::ShowError(_hMainDlg, L"'" + img.name() + L" - Cannot map image with non-native subsystem.");
			return STATUS_INVALID_IMAGE_HASH;
		}

		return STATUS_SUCCESS;
	}

	if (!_process.valid())
	{
		cMessage::ShowError(_hMainDlg, L"Please select valid process before injection.");
		return STATUS_INVALID_HANDLE;
	}

	auto& barrier = _process.barrier();

	if (!img.pureIL() && img.mType() == blackbone::mt_mod32 && barrier.targetWow64 == false)
	{
		cMessage::ShowError(_hMainDlg, L"'" + img.name() + L"' - Cannot inject 32-bit module into 64-bit process.");
		return STATUS_INVALID_IMAGE_WIN_32;
	}

	if (!img.pureIL() && img.mType() == blackbone::mt_mod64 && barrier.targetWow64 == true)
	{
		cMessage::ShowError(_hMainDlg, L"'" + img.name() + L"' - Cannot inject 64-bit module into 32-bit process.");
		return STATUS_INVALID_IMAGE_WIN_64;
	}

	if (context.cfg.injectMode == ManualMap || context.cfg.injectMode == KernelMMap)
	{
		if (img.pureIL() && (context.cfg.injectMode == KernelMMap || !img.isExe()))
		{
			cMessage::ShowError(_hMainDlg, L"'" + img.name() + L"' - Pure managed class library cannot be manually mapped.");
			return STATUS_INVALID_IMAGE_FORMAT;
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS cInjectionCore::ValidateInit(const std::string& init, uint32_t& initRVA, blackbone::pe::PEImage& img)
{
	if (img.pureIL())
	{
		if (img.isExe())
		{
			initRVA = 0;
			return STATUS_SUCCESS;
		};

		blackbone::ImageNET::mapMethodRVA methods;
		img.net().Parse(&methods);
		bool found = false;

		if (!methods.empty() && !init.empty())
		{
			auto winit = blackbone::Utils::AnsiToWstring(init);

			for (auto& val : methods)
				if (winit == (val.first.first + L"." + val.first.second))
				{
					found = true;
					break;
				}
		}

		if (!found)
		{
			if (init.empty())
				cMessage::ShowError(_hMainDlg, L"'" + img.name() + L"' - Please select entry point.");

			else
				cMessage::ShowError(_hMainDlg, L"'" + img.name() + L"' - Module does not contain function '" + blackbone::Utils::AnsiToWstring(init) + L"'.");

			return STATUS_NOT_FOUND;
		}
	}

	else if (!init.empty())
	{
		blackbone::pe::vecExports exports;
		img.GetExports(exports);

		auto iter = std::find_if(
			exports.begin(), exports.end(),
			[&init](const blackbone::pe::ExportData& val) { return val.name == init; });

		if (iter == exports.end())
		{
			initRVA = 0;
			cMessage::ShowError(_hMainDlg, L"'" + img.name() + L"' - Module does not contain export '" + blackbone::Utils::AnsiToWstring(init) + L"'.");
			return STATUS_NOT_FOUND;
		}

		else
		{
			initRVA = iter->RVA;
		}
	}

	return STATUS_SUCCESS;
}

NTSTATUS cInjectionCore::InjectMultiple(sInjectContext* pContext)
{
	DWORD status = STATUS_SUCCESS;
	PROCESS_INFORMATION pi = { NULL };

	if (pContext->cfg.processMode == Existing && pContext->pid == 0)
		pContext->pid = pContext->cfg.pid;

	xlog::Critical(
		"Injection initiated. Method: %d, process type: %d, process id: 0x%X, manual map flags: 0x%X, erase header: %d, unlink: %d, init routine: '%s', init param: '%ls'.",
		pContext->cfg.injectMode,
		pContext->cfg.processMode,
		pContext->pid,
		pContext->cfg.mmapFlags,
		pContext->cfg.erasePE,
		pContext->cfg.unlink,
		pContext->cfg.initRoutine.c_str(),
		pContext->cfg.initArgs.c_str()
	);

	if (pContext->cfg.processMode != Manual)
		status = GetTargetProcess(*pContext, pi);

	if (status != STATUS_SUCCESS)
		return status;

	if (pContext->cfg.delay)
		Sleep(pContext->cfg.delay);

	for (auto& img : pContext->images)
	{
		status |= InjectSingle(*pContext, *img);
		if (pContext->cfg.period)
			Sleep(pContext->cfg.period);
	}

	if (pi.hThread)
		CloseHandle(pi.hThread);

	if (pi.hProcess)
		CloseHandle(pi.hProcess);

	if (status == STATUS_SUCCESS && (pContext->cfg.mmapFlags & blackbone::HideVAD) &&
		(pContext->cfg.injectMode == Manual || pContext->cfg.injectMode == KernelMMap))
	{
		_criticalProcList.emplace_back(_process.pid());
	}

	if (_process.core().handle())
		_process.Detach();

	return status;
}

NTSTATUS cInjectionCore::InjectSingle(sInjectContext& context, blackbone::pe::PEImage& img)
{
	NTSTATUS status = STATUS_SUCCESS;
	blackbone::ThreadPtr pThread = nullptr;
	blackbone::ModuleDataPtr mod = nullptr;
	uint32_t exportRVA = 0;

	xlog::Critical("'%ls' - Injecting module.", img.name().c_str());

	status = ValidateInit(blackbone::Utils::WstringToUTF8(context.cfg.initRoutine), exportRVA, img);
	if (!NT_SUCCESS(status))
	{
		xlog::Error("Module init routine check failed, status: 0x%X.", status);
		return status;
	}

	if (context.cfg.injectMode < KernelSTD || context.cfg.injectMode == KernelDMap)
	{
		status = ValidateContext(context, img);
		if (!NT_SUCCESS(status))
		{
			xlog::Error("Context validation failed, status: 0x%X.", status);
			return status;
		}
	}

	if (context.cfg.injectMode == ThreadHijack)
	{
		xlog::Normal("Searching for thread to hijack.");
		pThread = _process.threads().getMostExecuted();
		if (pThread == nullptr)
		{
			cMessage::ShowError(_hMainDlg, L"Failed to get suitable thread for execution.");
			return status = STATUS_NOT_FOUND;
		}
	}

	auto modCallback = [](blackbone::CallbackType type, void*, blackbone::Process&, const blackbone::ModuleData& modInfo)
	{
		if (type == blackbone::PreCallback)
		{
			if (modInfo.name == L"user32.dll")
				return blackbone::LoadData(blackbone::MT_Native, blackbone::Ldr_Ignore);
		}

		return blackbone::LoadData(blackbone::MT_Default, blackbone::Ldr_Ignore);
	};

	if (NT_SUCCESS(status))
	{
		switch (context.cfg.injectMode)
		{
		case Standard:
		case ThreadHijack:
		{
			auto injectedMod = InjectDefault(context, img, pThread);
			if (!injectedMod)
				status = injectedMod.status;
			else
				mod = injectedMod.result();
		}
		break;

		case ManualMap:
		{
			auto flags = static_cast<blackbone::eLoadFlags>(context.cfg.mmapFlags);
			if (img.isExe())
				flags |= blackbone::RebaseProcess;

			auto injectedMod = _process.mmap().MapImage(img.path(), flags, modCallback);
			if (!injectedMod)
			{
				status = injectedMod.status;
				xlog::Error("'%ls' - Failed to inject image using manual map injection, status: 0x%X.", img.name().c_str(), injectedMod.status);
			}
			else
				mod = injectedMod.result();
		}
		break;

		case KernelSTD:
		case KernelAPC:
		case KernelMMap:
			if (!NT_SUCCESS(status = InjectKernel(context, img, exportRVA)))
				xlog::Error("'%ls' - Failed to inject image using kernel injection, status: 0x%X.", img.name().c_str(), status);
			break;

		case KernelDMap:
			status = blackbone::Driver().MMapDriver(img.path());
			break;

		default:
			break;
		}
	}

	if (!img.pureIL() && mod == nullptr && context.cfg.injectMode < KernelSTD && NT_SUCCESS(status))
	{
		xlog::Error("'%ls' - Invalid failure status: 0x%X.", img.name().c_str(), status);
		status = STATUS_DLL_NOT_FOUND;
	}

	if (NT_SUCCESS(status) && context.cfg.injectMode < KernelSTD)
	{
		status = CallInitRoutine(context, img, mod, exportRVA, pThread);
	}

	else if (!NT_SUCCESS(status))
	{
		wchar_t errBuf[1024] = { NULL };
		wsprintf(errBuf, L"'%ls' - Failed to inject module.\n%ls\nStatus code: 0x%X.", img.name().c_str(), blackbone::Utils::GetErrorDescription(status).c_str(), status);
		cMessage::ShowError(_hMainDlg, errBuf);
	}

	if (NT_SUCCESS(status) && mod && context.cfg.injectMode == Standard && context.cfg.erasePE)
	{
		auto size = img.headersSize();
		DWORD oldProt = 0;
		std::unique_ptr<uint8_t> zeroBuf(new uint8_t[size]());

		_process.memory().Protect(mod->baseAddress, size, PAGE_EXECUTE_READWRITE, &oldProt);
		_process.memory().Write(mod->baseAddress, size, zeroBuf.get());
		_process.memory().Protect(mod->baseAddress, size, oldProt);
	}

	if (NT_SUCCESS(status) && mod && context.cfg.injectMode == Standard && context.cfg.unlink)
		if (_process.modules().Unlink(mod) == false)
		{
			status = STATUS_UNSUCCESSFUL;
			cMessage::ShowError(_hMainDlg, L"'" + img.name() + L"' - Failed to unlink module.");
		}

	return status;
}

blackbone::call_result_t<blackbone::ModuleDataPtr> cInjectionCore::InjectDefault(
	sInjectContext& context,
	const blackbone::pe::PEImage& img,
	blackbone::ThreadPtr pThread
)
{
	NTSTATUS status = STATUS_SUCCESS;

	if (img.pureIL())
	{
		DWORD code = 0;

		xlog::Normal("'%ls' - Module is pure IL.", img.name().c_str());

		if (!_process.modules().InjectPureIL(
			blackbone::ImageNET::GetImageRuntimeVer(img.path().c_str()),
			img.path(),
			context.cfg.initRoutine,
			context.cfg.initArgs,
			code))
		{
			xlog::Error("'%ls' - Failed to inject pure IL module, status: 0x%X.", img.name().c_str(), code);
			if (NT_SUCCESS(code))
				code = STATUS_DLL_NOT_FOUND;

			return code;
		}

		auto mod = _process.modules().GetModule(img.name(), blackbone::Sections);
		return mod ? blackbone::call_result_t<blackbone::ModuleDataPtr>(mod)
			: blackbone::call_result_t<blackbone::ModuleDataPtr>(STATUS_NOT_FOUND);
	}

	else
	{
		auto injectedMod = _process.modules().Inject(img.path(), pThread);
		if (!injectedMod)
			xlog::Error("'%ls' - Failed to inject image using default injection, status: 0x%X.", img.name().c_str(), injectedMod.status);

		return injectedMod;
	}
}

NTSTATUS cInjectionCore::InjectKernel(
	sInjectContext& context,
	const blackbone::pe::PEImage& img,
	uint32_t initRVA
)
{
	if (context.cfg.injectMode == KernelMMap)
	{
		return blackbone::Driver().MmapDll(
			context.pid,
			img.path(),
			(KMmapFlags)context.cfg.mmapFlags,
			initRVA,
			context.cfg.initArgs
		);
	}
	else
	{
		return blackbone::Driver().InjectDll(
			context.pid,
			img.path(),
			(context.cfg.injectMode == KernelSTD ? IT_Thread : IT_Apc),
			initRVA,
			context.cfg.initArgs,
			context.cfg.unlink,
			context.cfg.erasePE
		);
	}
}

NTSTATUS cInjectionCore::CallInitRoutine(
	sInjectContext& context,
	const blackbone::pe::PEImage& img,
	const blackbone::ModuleDataPtr mod,
	uint64_t exportRVA,
	blackbone::ThreadPtr pThread
)
{
	if (!context.cfg.initRoutine.empty() && !img.pureIL() && context.cfg.injectMode < KernelSTD)
	{
		auto fnPtr = mod->baseAddress + exportRVA;

		xlog::Normal("'%ls' - Calling initialization routine.", img.name().c_str());

		if (pThread == nullptr)
		{
			auto argMem = _process.memory().Allocate(0x1000, PAGE_READWRITE);
			argMem->Write(0, context.cfg.initArgs.length() * sizeof(wchar_t) + 2, context.cfg.initArgs.c_str());

			xlog::Normal("'%ls' - Initialization routine returned 0x%X.", img.name().c_str(), _process.remote().ExecDirect(fnPtr, argMem->ptr()));
		}

		else
		{
			blackbone::RemoteFunction<fnInitRoutine> pfn(_process, fnPtr);

			auto status = pfn.Call(context.cfg.initArgs.c_str(), pThread);

			xlog::Normal("'%ls' - Initialization routine returned 0x%X.", img.name().c_str(), status);
		}
	}

	return STATUS_SUCCESS;
}

