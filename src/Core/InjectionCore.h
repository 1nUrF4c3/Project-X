#pragma once

#include "../Interface/WinAPI/Message.hpp"
#include "../Interface/WinAPI/StatusBar.hpp"
#include "../Profiler/Profiler.h"
#include "../Logger/Logger.hpp"

#include <Config.h>
#include <Process/Process.h>
#include <PE/PEImage.h>
#include <Misc/Utils.h>

typedef std::vector<std::shared_ptr<blackbone::pe::PEImage>> vecPEImages;
typedef std::vector<blackbone::pe::vecExports> vecImageExports;
typedef std::vector<bool> vecImageChecks;

enum eInjectMode
{
	Standard,
	ThreadHijack,
	ManualMap,
	KernelSTD,
	KernelAPC,
	KernelMMap,
	KernelDMap,
};

enum eProcMode
{
	Existing,
	Manual,
	Auto,
};

struct sInjectContext
{
	cProfile::sConfigData cfg;                     

	DWORD pid = 0;                                 
	vecPEImages images;
	std::wstring procPath;                          

	uint32_t skippedCount = 0;                         
	bool waitActive = false;                        
	std::vector<blackbone::ProcessInfo> procList;   
	std::vector<blackbone::ProcessInfo> procDiff;   
};

class cInjectionCore
{
	typedef int(__stdcall* fnInitRoutine)(const wchar_t*);

public:
	cInjectionCore(HWND& hMainDlg);
	~cInjectionCore();
	NTSTATUS GetTargetProcess(sInjectContext& context, PROCESS_INFORMATION& pi);
	NTSTATUS InjectMultiple(sInjectContext* pContext);
	inline blackbone::Process& process() { return _process; }

private:

	NTSTATUS ValidateInit(const std::string& init, uint32_t& initRVA, blackbone::pe::PEImage& img);
	NTSTATUS ValidateContext(sInjectContext& context, const blackbone::pe::PEImage& img);
	NTSTATUS InjectSingle(sInjectContext& context, blackbone::pe::PEImage& img);
	blackbone::call_result_t<blackbone::ModuleDataPtr> InjectDefault(sInjectContext& context, const blackbone::pe::PEImage& img, blackbone::ThreadPtr pThread = nullptr);
	NTSTATUS InjectKernel(sInjectContext& context, const blackbone::pe::PEImage& img, uint32_t initRVA);
	NTSTATUS CallInitRoutine(sInjectContext& context, const blackbone::pe::PEImage& img, const blackbone::ModuleDataPtr mod, uint64_t exportRVA, blackbone::ThreadPtr pThread = nullptr);

	HWND& _hMainDlg;

	blackbone::Process _process;
	std::vector<DWORD> _criticalProcList;
};