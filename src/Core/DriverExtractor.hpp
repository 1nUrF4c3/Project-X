#pragma once

#include "../Resources/Resource.h"
#include <VersionApi.h>
#include <DriverControl/DriverControl.h>

class cDriverExtractor
{
public:

	static cDriverExtractor& Instance()
	{
		static cDriverExtractor inst;
		return inst;
	}

	void Extract()
	{
		int resID = IDR_DRV7;
		const wchar_t* filename = L"BlackBone.sys";

		if (IsWindows10OrGreater())
		{
			resID = IDR_DRV10;
		}
		else if (IsWindows8Point1OrGreater())
		{
			resID = IDR_DRV81;
		}
		else if (IsWindows8OrGreater())
		{
			resID = IDR_DRV8;
		}

		HRSRC resInfo = FindResource(NULL, MAKEINTRESOURCEW(resID), L"Driver");
		if (resInfo)
		{
			HGLOBAL hRes = LoadResource(NULL, resInfo);
			PVOID pDriverData = LockResource(hRes);
			HANDLE hFile = CreateFile(
				(blackbone::Utils::GetExeDirectory() + L"\\" + filename).c_str(),
				FILE_GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL
			);

			if (hFile != INVALID_HANDLE_VALUE)
			{
				DWORD bytes = 0;
				WriteFile(hFile, pDriverData, SizeofResource(NULL, resInfo), &bytes, NULL);
				CloseHandle(hFile);
			}
		}
	}

	~cDriverExtractor()
	{
		Cleanup();
	}

	void Cleanup()
	{
		const wchar_t* filename = L"BlackBone.sys";

		DeleteFile((blackbone::Utils::GetExeDirectory() + L"\\" + filename).c_str());
	}

private:

	cDriverExtractor() = default;
	cDriverExtractor(const cDriverExtractor&) = default;
};
