#include "Interface/Main/MainDlg.h"
#include "Dumper/Dumper.h"
#include "Core/DriverExtractor.hpp"

#include <shellapi.h>

int DumpNotifier(const wchar_t* path, void* context, EXCEPTION_POINTERS* expt, bool success)
{
	cMessage::ShowError(NULL, L"Project-X has crashed. Dump file saved at '" + blackbone::Utils::StripPath(std::wstring(path)) + L"'.");
	return 0;
}

void AssociateExtension()
{
	wchar_t tmp[255] = { NULL };
	GetModuleFileName(NULL, tmp, sizeof(tmp));

#ifdef USE64
	std::wstring ext = L".pjx64";
	std::wstring alias = L"ProfilePJX64";
	std::wstring desc = L"Project-X 64-bit profile";
#else
	std::wstring ext = L".pjx32";
	std::wstring alias = L"ProfilePJX32";
	std::wstring desc = L"Project-X 32-bit profile";
#endif

	std::wstring editWith = L"\"" + std::wstring(tmp) + L"\" --load \"%1\"";
	std::wstring runWith = L"\"" + std::wstring(tmp) + L"\" --run \"%1\"";
	std::wstring icon = L"\"" + std::wstring(tmp) + L"\",-" + std::to_wstring(IDI_ICON);

	auto AddKey = [](const std::wstring& subkey, const std::wstring& value, const wchar_t* regValue)
	{
		SHSetValue(HKEY_CLASSES_ROOT, subkey.c_str(), regValue, REG_SZ, value.c_str(), (DWORD)(value.size() * sizeof(wchar_t)));
	};

	SHDeleteKey(HKEY_CLASSES_ROOT, alias.c_str());

	AddKey(ext, alias, nullptr);
	AddKey(ext, L"Application/xml", L"Content Type");
	AddKey(alias, desc, nullptr);
	AddKey(alias + L"\\shell", L"Run", nullptr);
	AddKey(alias + L"\\shell\\Edit\\command", editWith, nullptr);
	AddKey(alias + L"\\shell\\Run\\command", runWith, nullptr);
	AddKey(alias + L"\\DefaultIcon", icon, nullptr);
}

void LogOSInfo()
{
	SYSTEM_INFO info = { NULL };
	char* osArch = "x64";

	auto pPeb = (blackbone::PEB_T*)NtCurrentTeb()->ProcessEnvironmentBlock;
	GetNativeSystemInfo(&info);

	if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
		osArch = "x86";

	xlog::Normal("Started on Windows %d.%d.%d.%d %s. Driver status: 0x%X.", pPeb->OSMajorVersion, pPeb->OSMinorVersion, (pPeb->OSCSDVersion >> 8) & 0xFF, pPeb->OSBuildNumber, osArch, blackbone::Driver().status());
}

cMainDlg::eStartAction ParseCmdLine(std::wstring& param)
{
	int argc = 0;
	auto pCmdLine = GetCommandLine();
	auto argv = CommandLineToArgvW(pCmdLine, &argc);

	for (int i = 1; i < argc; i++)
	{
		if (_wcsicmp(argv[i], L"--load") == 0 && i + 1 < argc)
		{
			param = argv[i + 1];
			return cMainDlg::LoadProfile;
		}
		if (_wcsicmp(argv[i], L"--run") == 0 && i + 1 < argc)
		{
			param = argv[i + 1];
			return cMainDlg::RunProfile;
		}
	}

	return cMainDlg::Nothing;
}

INT APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	InitCommonControls();

	dump::cDumper::Instance().CreateWatchdog(blackbone::Utils::GetExeDirectory(), dump::CreateFullDump, &DumpNotifier);
	AssociateExtension();

	cDriverExtractor::Instance().Extract();
	auto status = blackbone::Driver().EnsureLoaded();

	if (!NT_SUCCESS(status))
		cDriverExtractor::Instance().Cleanup();

	LogOSInfo();

	std::wstring Param;
	auto Action = ParseCmdLine(Param);

	cMainDlg mainDLG(Action, Param);

	if (Action == cMainDlg::RunProfile)
		return mainDLG.LoadAndInject();

	else
		return (INT)mainDLG.RunModeless(NULL, IDR_ACCELERATOR);
}