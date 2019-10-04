#pragma once

#include "Window.hpp"

namespace ctrl
{
	class cControl : public cWindow
	{
	public:

		cControl(HWND hwnd = NULL)
			: cWindow(hwnd) {  }
	};
}