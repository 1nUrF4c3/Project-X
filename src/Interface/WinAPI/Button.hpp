#pragma once

#include "Control.hpp"

namespace ctrl
{
	class cButton : public cControl
	{
	public:

		cButton(HWND hwnd = NULL)
			: cControl(hwnd) {  }

		bool Checked()
		{
			return Button_GetCheck(_hwnd) != BST_UNCHECKED;
		}

		void Checked(bool state)
		{
			Button_SetCheck(_hwnd, state);
		}

		std::wstring Text()
		{
			wchar_t buf[512] = { NULL };
			Button_GetText(_hwnd, buf, ARRAYSIZE(buf));

			return buf;
		}

		BOOL Text(const std::wstring& text)
		{
			return Button_SetText(_hwnd, text.c_str());
		}
	};
}