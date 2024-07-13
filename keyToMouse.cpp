#include <windows.h>
#include <thread>
#include <chrono>

namespace os::cursor {
	inline POINT getPos() {
		POINT pos;
		GetCursorPos(&pos);
		return pos;
	}
	inline auto setPos(POINT pos) {
		SetCursorPos(pos.x, pos.y);
	}
	inline auto occurrence(const DWORD e, const POINT pos = { 0,0 }, const int data = 0) {
		INPUT input;
		input.type = INPUT_MOUSE;
		input.mi.dx = pos.x;
		input.mi.dy = pos.y;
		input.mi.time = input.mi.dwExtraInfo = 0;
		input.mi.dwFlags = static_cast<int>(e);
		input.mi.mouseData = data;
		return  SendInput(1, &input, sizeof(input));
	}
}
HHOOK hook;
auto horizon = 0, vertical = 0;
bool slow = false, space = false, active = false;
auto callback(DWORD status, int keyCode) {
	if (!active)return false;
	constexpr auto DEFAULT = 10, BOOST = 50, SLOW = 1, BOOST_DELTA = WHEEL_DELTA * 2;
	const auto speed = status == WM_KEYDOWN ? space ? BOOST : slow ? SLOW : DEFAULT : 0;
	const auto closure = [&speed](auto& key, const auto& changed) {
		return [newSpeed = std::exchange(key, speed) ? DEFAULT : changed](auto& number) {
			if (!number)return;
			number = 0 < number ? newSpeed : -newSpeed;
		};
	};
	switch (keyCode) {
	case VK_SPACE:
	{
		if (space && status == WM_KEYDOWN)break;
		const auto tryAssign = closure(space, BOOST);
		tryAssign(horizon);
		tryAssign(vertical);
	}
	break;
	case VK_LMENU:
		if (status != WM_SYSKEYDOWN)return false;
		active = false;
		break;
	case 'A':
	{
		if (slow && status == WM_KEYDOWN)break;;
		const auto tryAssign = closure(slow, SLOW);
		tryAssign(horizon);
		tryAssign(vertical);
	}
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
	case 'S':
		os::cursor::occurrence(speed ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP, os::cursor::getPos());
		break;
	case 'D':
		os::cursor::occurrence(speed ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP, os::cursor::getPos());
		break;
	case 'F':
		os::cursor::occurrence(speed ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP, os::cursor::getPos());
		break;
	case 'E':
		os::cursor::occurrence(MOUSEEVENTF_WHEEL, os::cursor::getPos(), space ? BOOST_DELTA : slow ? SLOW : WHEEL_DELTA);
		break;
	case 'V':
		os::cursor::occurrence(MOUSEEVENTF_WHEEL, os::cursor::getPos(), -(space ? BOOST_DELTA : slow ? SLOW : WHEEL_DELTA));
		break;
	case 'G':
		os::cursor::occurrence(MOUSEEVENTF_HWHEEL, os::cursor::getPos(), -(space ? BOOST_DELTA : slow ? SLOW : WHEEL_DELTA));
		break;
	case 'H':
		os::cursor::occurrence(MOUSEEVENTF_HWHEEL, os::cursor::getPos(), space ? BOOST_DELTA : slow ? SLOW : WHEEL_DELTA);
		break;
	case 'C':
		PostQuitMessage(0);
		break;
	default:
		return false;
	}
	return true;
}

LRESULT CALLBACK hookProc(int nCode, WPARAM wp, LPARAM lp) {
			if (nCode < 0) {
				return CallNextHookEx(hook, nCode, wp, lp);
			}
			if (callback(wp, ((KBDLLHOOKSTRUCT*)lp)->vkCode))return 1;
			return CallNextHookEx(hook, nCode, wp, lp);
}
int WINAPI WinMain(HINSTANCE, HINSTANCE, PSTR, int) {
	constexpr auto ID = 4649;
	std::thread(
		[] {
			while (true) {
				auto start = std::chrono::system_clock::now();
				auto pos = os::cursor::getPos();
				pos.x += horizon;
				pos.y += vertical;
				os::cursor::setPos(pos);
				auto waste = std::chrono::system_clock::now() - start;
				constexpr auto interval = std::chrono::milliseconds(1);
				if (waste >= interval)continue;
				std::this_thread::sleep_for(interval - waste);
			}
		}
	).detach();
	if (RegisterHotKey(nullptr, ID, MOD_ALT, 0x4A) && (hook = SetWindowsHookEx(WH_KEYBOARD_LL, hookProc, GetModuleHandle(0), 0)) != nullptr) {
		MSG msg;
		while (GetMessage(&msg, nullptr, 0, 0)) {
			if (msg.message == WM_HOTKEY) {
				active = true;
			}
			
		}
	}
	UnregisterHotKey(nullptr, ID);
	UnhookWindowsHookEx(hook);
	return EXIT_SUCCESS;
}
