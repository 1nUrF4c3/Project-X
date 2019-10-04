#include "Profiler.h"

bool cProfile::Save(HWND hwnd, const std::wstring& path)
{
	try
	{
		auto filepath = path.empty() ? (blackbone::Utils::GetExeDirectory() + DEFAULT_PROFILE) : path;

		acut::XmlDoc<wchar_t> xml;
		xml.create_document();

		for (auto& imgpath : _config.images)
			xml.append(L"Project-X.Images.ImagePath").set(L"", imgpath.c_str());

		for (auto& imgcheck : _config.checks)
			xml.append(L"Project-X.Images.ImageChecked").set(L"", imgcheck);

		xml.set(L"Project-X.Process.ProcessName", _config.procName.c_str());
		xml.set(L"Project-X.Process.ProcessMode", _config.processMode);
		xml.set(L"Project-X.Injection.InjectMode", _config.injectMode);
		xml.set(L"Project-X.Injection.UnlinkModule", _config.unlink);
		xml.set(L"Project-X.Injection.EraseHeader", _config.erasePE);
		xml.set(L"Project-X.Injection.ManualMapFlags", _config.mmapFlags);
		xml.set(L"Project-X.General.AutoClose", _config.close);
		xml.set(L"Project-X.General.EscalateHandle", _config.krnHandle);
		xml.set(L"Project-X.General.DelayTime", _config.delay);
		xml.set(L"Project-X.General.IntervalTime", _config.period);
		xml.set(L"Project-X.Initialization.InitRoutine", _config.initRoutine.c_str());
		xml.set(L"Project-X.Initialization.InitParameters", _config.initArgs.c_str());
		xml.set(L"Project-X.Process.ProcessCommandLine", _config.procCmdLine.c_str());
		xml.set(L"Project-X.Process.InjectIndefinitely", _config.injIndef);
		xml.set(L"Project-X.Process.SkipCount", _config.skipProc);

		xml.write_document(filepath);

		return true;
	}
	catch (const std::runtime_error& error)
	{
		cMessage::ShowError(hwnd, blackbone::Utils::AnsiToWstring(error.what()));

		return false;
	}
}

bool cProfile::Load(HWND hwnd, const std::wstring& path)
{
	try
	{
		auto filepath = path.empty() ? (blackbone::Utils::GetExeDirectory() + DEFAULT_PROFILE) : path;

		if (!acut::file_exists(filepath))
			return false;

		acut::XmlDoc<wchar_t> xml;
		xml.read_from_file(filepath);

		if (xml.has(L"Project-X.Images.ImagePath"))
		{
			auto nodes = xml.all_nodes_named(L"Project-X.Images.ImagePath");
			for (auto& node : nodes)
				_config.images.emplace_back(node.value());
		}

		if (xml.has(L"Project-X.Images.ImageChecked"))
		{
			wchar_t* pEnd = nullptr;

			auto nodes = xml.all_nodes_named(L"Project-X.Images.ImageChecked");
			for (auto& node : nodes)
				_config.checks.emplace_back(std::wcstol(node.value().c_str(), nullptr, 2));
		}

		xml.get_if_present(L"Project-X.Process.ProcessName", _config.procName);
		xml.get_if_present(L"Project-X.Process.ProcessMode", _config.processMode);
		xml.get_if_present(L"Project-X.Injection.InjectMode", _config.injectMode);
		xml.get_if_present(L"Project-X.Injection.UnlinkModule", _config.unlink);
		xml.get_if_present(L"Project-X.Injection.EraseHeader", _config.erasePE);
		xml.get_if_present(L"Project-X.Injection.ManualMapFlags", _config.mmapFlags);
		xml.get_if_present(L"Project-X.General.AutoClose", _config.close);
		xml.get_if_present(L"Project-X.General.EscalateHandle", _config.krnHandle);
		xml.get_if_present(L"Project-X.General.DelayTime", _config.delay);
		xml.get_if_present(L"Project-X.General.IntervalTime", _config.period);
		xml.get_if_present(L"Project-X.Initialization.InitRoutine", _config.initRoutine);
		xml.get_if_present(L"Project-X.Initialization.InitParameters", _config.initArgs);
		xml.get_if_present(L"Project-X.Process.ProcessCommandLine", _config.procCmdLine);
		xml.get_if_present(L"Project-X.Process.InjectIndefinitely", _config.injIndef);
		xml.get_if_present(L"Project-X.Process.SkipCount", _config.skipProc);

		return true;
	}
	catch (const std::runtime_error& error)
	{
		cMessage::ShowError(hwnd, blackbone::Utils::AnsiToWstring(error.what()));

		return false;
	}
}
