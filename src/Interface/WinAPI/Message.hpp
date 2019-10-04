#pragma once

#include "StdAfx.hpp"
#include "../../Logger/Logger.hpp"

class cMessage
{
public:

	enum eMsgType
	{
		Error,
		Warning,
		Info,
		Question,
	};

	static void ShowError(HWND parent, const std::wstring& msg, const std::wstring& title = L"Error")
	{
		Show(msg, title, Error, parent);
	}

	static void ShowWarning(HWND parent, const std::wstring& msg, const std::wstring& title = L"Warning")
	{
		Show(msg, title, Warning, parent);
	}

	static void ShowInfo(HWND parent, const std::wstring& msg, const std::wstring& title = L"Info")
	{
		Show(msg, title, Info, parent);
	}

	static bool ShowQuestion(HWND parent, const std::wstring& msg, const std::wstring& title = L"Question")
	{
		return Show(msg, title, Question, parent) == IDYES;
	}

private:

	static int Show(const std::wstring& msg, const std::wstring& title, eMsgType type, HWND parent = NULL)
	{
		UINT uType;
		xlog::LogLevel::e logLevel;

		switch (type)
		{
		case Error:
			uType = MB_OK | MB_ICONERROR;
			logLevel = xlog::LogLevel::error;
			break;

		case Warning:
			uType = MB_OK | MB_ICONWARNING;
			logLevel = xlog::LogLevel::warning;
			break;

		case Info:
			uType = MB_OK | MB_ICONINFORMATION;
			logLevel = xlog::LogLevel::normal;
			break;

		case Question:
			uType = MB_YESNO | MB_ICONQUESTION;
			logLevel = xlog::LogLevel::verbose;
			break;
		}

		if (logLevel < xlog::LogLevel::verbose)
			xlog::cLogger::Instance().DoLog(logLevel, "%ls", msg.c_str());

		return MessageBox(parent, msg.c_str(), title.c_str(), uType);
	}
};