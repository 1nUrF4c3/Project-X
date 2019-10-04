#pragma once

#include "Control.hpp"

namespace ctrl
{
	class cStatic : public cControl
	{
	public:

		cStatic(HWND hwnd = NULL)
			: cControl(hwnd) {  }

		std::wstring Text()
		{
			wchar_t buf[512] = { NULL };
			Static_GetText(_hwnd, buf, ARRAYSIZE(buf));

			return buf;
		}

		BOOL Text(const std::wstring& text) 
		{ 
			return Static_SetText(_hwnd, text.c_str()); 
		}
	};
}