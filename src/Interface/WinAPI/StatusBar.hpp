#pragma once

#include "Control.hpp"
#include <memory>

namespace ctrl
{
	class cStatusBar : public cControl
	{
	public:

		cStatusBar(HWND hwnd = NULL)
			: cControl(hwnd) {  }

		LRESULT SetParts(const std::initializer_list<int>& coords)
		{
			int i = 0;
			std::unique_ptr<int[]> coordArray(new int[coords.size()]());
			for (auto& item : coords)
			{
				coordArray[i] = item;
				i++;
			}

			return SendMessage(_hwnd, SB_SETPARTS, i, reinterpret_cast<LPARAM>(coordArray.get()));
		}

		LRESULT SetText(int index, const std::wstring& text)
		{
			return SendMessage(_hwnd, SB_SETTEXT, MAKEWORD(index, SBT_NOBORDERS), reinterpret_cast<LPARAM>(text.c_str()));
		}
	};
}