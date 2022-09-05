
#include <windows.h>
namespace ano {
	inline auto getCursorPos() {
		POINT pos;
		GetCursorPos(&pos);
		return pos;
	}
	inline auto setCursorRelativePos(int x, int y) noexcept(true) {
		auto pos = getCursorPos();
		SetCursorPos(pos.x + x, pos.y + y);
		return pos;
	}
	inline auto mouseEvent(DWORD flag, DWORD data) {
		auto pos = ano::getCursorPos();
		INPUT input;
		input.type = INPUT_MOUSE;
		input.mi.dx = pos.x;
		input.mi.dy = pos.y;
		input.mi.time = input.mi.dwExtraInfo = 0;
		input.mi.dwFlags = flag;
		input.mi.mouseData = data;
		return  SendInput(1, &input, sizeof(input));
	}
}
HHOOK hook;
bool active;
auto horizon = 0, vertical = 0;
VOID CALLBACK TimerProc(HWND hwnd, UINT msg, UINT id, DWORD) {
	if (horizon | vertical) {
		ano::setCursorRelativePos(horizon, vertical);
	}
}
LRESULT CALLBACK hookProc(int nCode, WPARAM wp, LPARAM lp) {
	if (!active || nCode < 0) {
		return CallNextHookEx(hook, nCode, wp, lp);
	}
	//#define speed wp == WM_KEYDOWN ? 1 : 0;
	static auto space = false, shift = false;
	const auto speed = wp == WM_KEYDOWN ? shift ? 1 : space ? 50 : 10 : 0;
	switch (((KBDLLHOOKSTRUCT*)lp)->vkCode) {
	case VK_SHIFT:
	case VK_RSHIFT:
	case VK_LSHIFT:
		if (shift = speed) {
			horizon /= 10;
			vertical /= 10;
		}
		else {
			horizon *= 10;
			vertical *= 10;
		}
		break;
	case VK_SPACE:
		if (space = speed) {
			horizon *= 5;
			vertical *= 5;
		}
		else {
			horizon /= 5;
			vertical /= 5;
		}
		break;
	case 'G':
		ano::mouseEvent(MOUSEEVENTF_HWHEEL, speed ? -WHEEL_DELTA : 0);
		break;
	case 'H':
		ano::mouseEvent(MOUSEEVENTF_HWHEEL, speed ? WHEEL_DELTA : 0);
		break;
	case 'M':
		ano::mouseEvent(MOUSEEVENTF_WHEEL, speed ? -WHEEL_DELTA : 0);
		break;
	case 'E':
		ano::mouseEvent(MOUSEEVENTF_WHEEL, speed ? WHEEL_DELTA : 0);
		break;
	case 'S':
		ano::mouseEvent(speed ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP, 0);//こっちにshiftでscrollでもいい
		break;
	case 'F':
		ano::mouseEvent(speed ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP, 0);
		break;
	case 'D':
		ano::mouseEvent(speed ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP, 0);
		break;
	case 'Q':
		PostQuitMessage(0);
		break;
	case 'C':
		active = false;
		break;
	case 'J':
		horizon = -speed;
		break;
	case 'L':
		horizon = speed;
		break;
	case 'I':
		vertical = -speed;
		break;
	case 'K':
		vertical = speed;
		break;
	default:
		return CallNextHookEx(hook, nCode, wp, lp);
	}
	return 1;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow) {
	constexpr auto REGISTER_HOT_KEY_ID = 4649;
	if (!RegisterHotKey(nullptr, REGISTER_HOT_KEY_ID, MOD_CONTROL, 'M') || (hook = SetWindowsHookEx(WH_KEYBOARD_LL, hookProc, hInstance, 0)) == nullptr)return 0;
	MSG msg;
	SetTimer(nullptr, 4649, 1, (TIMERPROC)TimerProc);
	while (GetMessage(&msg, nullptr, 0, 0)) {
		if (msg.message == WM_HOTKEY) {
			//active = !active;//起動と終了
			active = true;//setwindowhookex//必要な時だけ
		}
		DispatchMessage(&msg);
	}
	UnhookWindowsHookEx(hook);
	UnregisterHotKey(nullptr, REGISTER_HOT_KEY_ID);

	return 0;
}