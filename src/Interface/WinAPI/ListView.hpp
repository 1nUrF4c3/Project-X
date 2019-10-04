#pragma once

#include "Control.hpp"
#include <initializer_list>

namespace ctrl
{
	class cListView : public cControl
	{
	public:

		cListView(HWND hwnd = NULL)
			: cControl(hwnd) {  }

		int AddColumn(const std::wstring& name, int width, int iSubItem = 0)
		{
			LVCOLUMN lvc = { NULL };

			lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
			lvc.pszText = const_cast<LPWSTR>(name.c_str());
			lvc.iSubItem = iSubItem;
			lvc.cx = width;

			return ListView_InsertColumn(_hwnd, iSubItem, &lvc);
		}

		int AddItem(const std::wstring& text, LPARAM lParam, const std::initializer_list<std::wstring>& args = {})
		{
			LVITEM lvi = { NULL };

			lvi.mask = LVIF_TEXT | LVIF_PARAM;

			lvi.pszText = const_cast<LPWSTR>(text.c_str());
			lvi.cchTextMax = static_cast<int>(text.length()) + 1;
			lvi.lParam = lParam;
			lvi.iItem = ListView_GetItemCount(_hwnd);

			int pos = ListView_InsertItem(_hwnd, &lvi);

			for (size_t i = 0; i < args.size(); i++)
				ListView_SetItemText(_hwnd, pos, static_cast<int>(i + 1), const_cast<LPWSTR>((args.begin() + i)->c_str()));

			return pos;
		}

		BOOL RemoveItem(int idx)
		{
			return ListView_DeleteItem(_hwnd, idx);
		}

		int GetCount()
		{
			return ListView_GetItemCount(_hwnd);
		}

		LPARAM GetParam(int idx)
		{
			LVITEM LVI = { NULL };
			LVI.mask = LVIF_PARAM;
			LVI.iItem = idx;
			ListView_GetItem(_hwnd, &LVI);
			return LVI.lParam;
		}

		int Selection()
		{
			return ListView_GetNextItem(_hwnd, -1, LVNI_SELECTED);
		}

		std::wstring ItemText(int idx, int iSubItem = 0)
		{
			wchar_t buf[256] = { NULL };
			ListView_GetItemText(_hwnd, idx, iSubItem, buf, ARRAYSIZE(buf));

			return buf;
		}

		BOOL Reset()
		{
			return ListView_DeleteAllItems(_hwnd);
		}
		
		UINT Checked(int idx)
		{ 
			return ListView_GetCheckState(_hwnd, idx); 
		}

		void Checked(int idx, bool state) 
		{ 
			ListView_SetCheckState(_hwnd, idx, state); 
		}
	};
}
