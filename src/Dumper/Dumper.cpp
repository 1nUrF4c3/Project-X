#include "Dumper.h"

#pragma warning(disable : 4091)
#include <DbgHelp.h>
#pragma warning(default : 4091)

namespace dump
{

	cDumper::cDumper(void)
	{
		memset(&_ExceptionInfo, 0x00, sizeof(_ExceptionInfo));

		_hWatchThd = CreateThread(NULL, 0, &cDumper::WatchdogProc, this, 0, NULL);
	}

	cDumper::~cDumper(void)
	{
		DisableWatchdog();

		_active = false;
		if (_hWatchThd)
		{
			WaitForSingleObject(_hWatchThd, INFINITE);
			_hWatchThd = NULL;
		}
	}

	cDumper& cDumper::Instance()
	{
		static cDumper instance;
		return instance;
	}

	bool cDumper::CreateWatchdog(
		const std::wstring& dumpRoot,
		eDumpFlags flags,
		fnCallback pDump,
		fnCallback pFilter,
		void* pContext)
	{
		if (_pPrevFilter == nullptr)
		{
			_flags = flags;
			_pUserContext = pContext;
			_dumpRoot = dumpRoot;
			_pDumpProc = pDump;

			_pPrevFilter = SetUnhandledExceptionFilter(&cDumper::UnhandledExceptionFilter);

			return true;
		}

		return false;
	}

	void cDumper::DisableWatchdog()
	{
		if (_pPrevFilter)
		{
			SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)_pPrevFilter);
			_pPrevFilter = nullptr;
		}
	}

	LONG CALLBACK cDumper::UnhandledExceptionFilter(PEXCEPTION_POINTERS ExceptionInfo)
	{
		auto& inst = Instance();

		inst._ExceptionInfo = ExceptionInfo;
		inst._FailedThread = GetCurrentThreadId();

		if (Instance()._hWatchThd == NULL)
			ExitProcess(0x1338);

		WaitForSingleObject(inst._hWatchThd, 3 * 60 * 1000);

		return EXCEPTION_CONTINUE_SEARCH;
	}

	DWORD CALLBACK cDumper::WatchdogProc(LPVOID lpParam)
	{
		cDumper* pClass = reinterpret_cast<cDumper*>(lpParam);

		while (pClass && pClass->_FailedThread == 0)
		{
			if (pClass->_active)
				Sleep(5);
			else
				return ERROR_SUCCESS;
		}

		if (pClass)
			pClass->ExceptionHandler();

		return ERROR_SUCCESS;
	}

	void cDumper::ExceptionHandler()
	{
		std::wstring strFullDump, strMiniDump;

		int dumpFlags = MiniDumpWithIndirectlyReferencedMemory |
			MiniDumpWithDataSegs |
			MiniDumpWithHandleData |
			MiniDumpWithFullMemory;

		GenDumpFilenames(strFullDump, strMiniDump);

		if (_flags & CreateFullDump)
			CreateDump(strFullDump, dumpFlags);

		if (_flags & CreateMinidump)
			CreateDump(strMiniDump, MiniDumpNormal);

		if (_pDumpProc)
			_pDumpProc(strFullDump.c_str(), _pUserContext, _ExceptionInfo, true);

		_hWatchThd = NULL;
		ExitProcess(0x1337);
	}

	void cDumper::CreateDump(const std::wstring& strFilePath, int flags)
	{
		MINIDUMP_EXCEPTION_INFORMATION aMiniDumpInfo = { NULL };

		HANDLE hFile = CreateFile(strFilePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);

		aMiniDumpInfo.ThreadId = _FailedThread;
		aMiniDumpInfo.ExceptionPointers = _ExceptionInfo;
		aMiniDumpInfo.ClientPointers = TRUE;

		if (hFile != INVALID_HANDLE_VALUE)
		{
			MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, (MINIDUMP_TYPE)flags, &aMiniDumpInfo, NULL, NULL);

			FlushFileBuffers(hFile);
			CloseHandle(hFile);
		}
	}

	bool cDumper::GenDumpFilenames(std::wstring& strFullDump, std::wstring& strMiniDump)
	{
		wchar_t tFullName[MAX_PATH] = { NULL };
		wchar_t tMiniName[MAX_PATH] = { NULL };
		wchar_t tImageName[MAX_PATH] = { NULL };

		SYSTEMTIME time = { NULL };

		GetLocalTime(&time);

		GetModuleFileName(NULL, tImageName, ARRAYSIZE(tImageName));
		std::wstring modulename(tImageName);
		modulename = modulename.substr(modulename.rfind(L"\\") + 1);

#ifdef USE64
		swprintf_s(tFullName, sizeof(tFullName) / sizeof(wchar_t), L"%s\\Project-X64_%d.%.2d.%.2d.%.2d.%.2d.%.2d.dmp",
			_dumpRoot.c_str(), time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);

		swprintf_s(tMiniName, sizeof(tFullName) / sizeof(wchar_t), L"%s\\Project-X64_%d.%.2d.%.2d.%.2d.%.2d.%.2d.mdmp",
			_dumpRoot.c_str(), time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);
#else
		swprintf_s(tFullName, sizeof(tFullName) / sizeof(wchar_t), L"%s\\Project-X32_%d.%.2d.%.2d.%.2d.%.2d.%.2d.dmp",
			_dumpRoot.c_str(), time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);

		swprintf_s(tMiniName, sizeof(tFullName) / sizeof(wchar_t), L"%s\\Project-X32_%d.%.2d.%.2d.%.2d.%.2d.%.2d.mdmp",
			_dumpRoot.c_str(), time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);
#endif

		strFullDump = tFullName;
		strMiniDump = tMiniName;

		return true;
	}
}
