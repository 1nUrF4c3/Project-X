#pragma once

#include "../Interface/WinAPI/StdAfx.hpp"
#include <string>

#include <Config.h>

namespace dump
{
	enum eDumpFlags
	{
		NoFlags,
		CreateMinidump,
		CreateFullDump,
		CreateBoth,
	};

	class cDumper
	{
	public:
		typedef int(*fnCallback)(const wchar_t*, void*, EXCEPTION_POINTERS*, bool);

		~cDumper(void);

		static cDumper& Instance();

		bool CreateWatchdog(
			const std::wstring& dumpRoot,
			eDumpFlags flags,
			fnCallback pDump = nullptr,
			fnCallback pFilter = nullptr,
			void* pContext = nullptr
		);

		void DisableWatchdog();

	private:
		cDumper(void);

		static LONG CALLBACK UnhandledExceptionFilter(PEXCEPTION_POINTERS ExceptionInfo);

		void CreateDump(const std::wstring& strFilePath, int flags);

		static DWORD CALLBACK WatchdogProc(LPVOID lpParam);

		void ExceptionHandler();

		bool GenDumpFilenames(std::wstring& strFullDump, std::wstring& strMiniDump);

		void* _pPrevFilter = nullptr;                   
		bool _active = true;                           
		eDumpFlags _flags = NoFlags;                   
		DWORD _FailedThread = 0;                       
		HANDLE _hWatchThd = NULL;                     
		PEXCEPTION_POINTERS _ExceptionInfo = nullptr;
		void* _pUserContext = nullptr;              
		std::wstring _dumpRoot = L".";               
		fnCallback _pDumpProc = nullptr;
	};
}