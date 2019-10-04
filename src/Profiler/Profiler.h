#pragma once

#include "../Interface/WinAPI/Message.hpp"
#include "RapidXML/rapidxml_wrap.hpp"

#ifdef USE64
#define DEFAULT_PROFILE L"\\Default.pjx64"
#else
#define DEFAULT_PROFILE L"\\Default.pjx32"
#endif

class cProfile
{
public:

	typedef std::vector<std::wstring> vecPaths;
	typedef std::vector<bool> vecChecks;

	struct sConfigData
	{
		vecPaths images;
		vecChecks checks;

		std::wstring procName;          
		std::wstring procCmdLine;      
		std::wstring initRoutine;       
		std::wstring initArgs;          

		uint32_t pid = 0;
		uint32_t mmapFlags = 0;        
		uint32_t processMode = 0;      
		uint32_t injectMode = 0;     
		uint32_t delay = 0;   
		uint32_t period = 0;      
		uint32_t skipProc = 0;        
         
		bool unlink = false;          
		bool erasePE = false;          
		bool close = false;            
		bool injIndef = false;
		bool krnHandle = false;

	};

	bool Save(HWND hwnd, const std::wstring& path = L"");
	bool Load(HWND hwnd, const std::wstring& path = L"");

	inline sConfigData& config() { return _config; }

private:
	sConfigData _config;
};


