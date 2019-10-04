#pragma once

#include "Control.hpp"

namespace ctrl
{
	class cComboBox : public cControl
	{
	public:

		cComboBox(HWND hwnd = NULL)
			: cControl(hwnd) {  }

		int Add(const std::wstring& text, int data = 0)
		{
			auto idx = ComboBox_AddString(_hwnd, text.c_str());
			ComboBox_SetItemData(_hwnd, idx, data);

			return idx;
		}

		int ItemData(int index)
		{
			return static_cast<int>(ComboBox_GetItemData(_hwnd, index));
		}

		int ItemData(int index, int data)
		{
			return ComboBox_SetItemData(_hwnd, index, data);
		}

		int ModifyItem(int index, const std::wstring& text, int data = 0)
		{
			auto oldData = ItemData(index);
			if (data == 0)
				data = oldData;

			ComboBox_DeleteString(_hwnd, index);
			index = ComboBox_InsertString(_hwnd, index, text.c_str());
			return ComboBox_SetItemData(_hwnd, index, data);
		}

		int Selection()
		{
			return ComboBox_GetCurSel(_hwnd);
		}

		int Selection(int index)
		{
			return ComboBox_SetCurSel(_hwnd, index);
		}

		std::wstring ItemText(int index)
		{
			wchar_t buf[512] = { NULL };
			ComboBox_GetLBText(_hwnd, index, buf);

			return buf;
		}

		std::wstring SelectedText()
		{
			wchar_t buf[512] = { NULL };
			ComboBox_GetText(_hwnd, buf, ARRAYSIZE(buf));

			return buf;
		}

		BOOL SelectedText(const std::wstring& text)
		{
			return ComboBox_SetText(_hwnd, text.c_str());
		}

		int Reset()
		{
			return ComboBox_ResetContent(_hwnd);
		}
	};
}
