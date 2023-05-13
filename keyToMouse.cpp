#include <windows.h>
#include <functional>
#include <unordered_map>
#include <array>
#include <thread>
#include <chrono>
namespace design {
	template <class T>
	class Singleton {
	protected://getInstanceを実装しなければprivateにする
		Singleton() = default;
		~Singleton() = default;
	public:
		Singleton(const Singleton&) = delete;
		Singleton(const Singleton&&) = delete;
		Singleton& operator=(const Singleton&) = delete;
		Singleton& operator=(const Singleton&&) = delete;
		static T& getInstance() {
			static T instance;
			return instance;
		};
	};
}
namespace library {
	class Timer {
	private:

	public:
		Timer(std::chrono::milliseconds interval, std::function<bool()> call) {
			std::thread(
				[interval, call] {
					while (true) {
						auto start = std::chrono::system_clock::now();
						if (!call())break;
						auto waste = std::chrono::system_clock::now() - start;
						if (waste >= interval)continue;
						std::this_thread::sleep_for(interval - waste);
					}
				}
			).detach();
		}
	};
}
namespace os {
	class Message {
	private:
		std::unordered_map<DWORD, std::function<void()> > exe;
	public:
		inline auto& regist(decltype(exe)::key_type msg, decltype(exe)::mapped_type proccess) {
			exe.emplace(msg, proccess);//後から変えたい場合はinsert_or_assign
			return *this;
		}
		inline auto loop() {
			MSG msg;
			while (GetMessage(&msg, nullptr, 0, 0)) {
				if (!exe.count(msg.message))continue;
				exe.at(msg.message)();
			}
		}
		inline auto quit() {
			PostQuitMessage(0);
		}
	};
	template <int ID, DWORD M, UINT VK >
	class HotKey {
	private:
	public:
		inline HotKey() {

		}
		inline bool regist() const noexcept(RegisterHotKey) {
			return RegisterHotKey(nullptr, ID, static_cast<UINT>(M), VK);
		}
		inline ~HotKey()noexcept(UnregisterHotKey) {
			UnregisterHotKey(nullptr, ID);
		}
	};
	class Cursor {
	private:
		struct Point {
			long x, y;
		};

	public:
		inline Point getPos() {
			POINT pos;
			GetCursorPos(&pos);
			return { pos.x,pos.y };
		}
		inline auto setPos(Point pos) {
			SetCursorPos(pos.x, pos.y);
		}
		inline auto occurrence(const DWORD e, const Point pos = { 0,0 }, const int data = 0) {//オーバーロードした方がいいかも
			INPUT input;
			input.type = INPUT_MOUSE;
			input.mi.dx = pos.x;
			input.mi.dy = pos.y;
			input.mi.time = input.mi.dwExtraInfo = 0;
			input.mi.dwFlags = static_cast<int>(e);
			input.mi.mouseData = data;
			return  SendInput(1, &input, sizeof(input));
		}
	};
	class KeyHook :public design::Singleton<KeyHook> {
	private:
		HHOOK hook;
		std::function<bool(DWORD, int) > exe;
		static LRESULT CALLBACK hookProc(int nCode, WPARAM wp, LPARAM lp) {//複数のhookを処理する場合はIDを持たせて任意の処理を呼ぶ。ここでは、一度が確定しているので、無駄なことはしない。
			if (nCode < 0) {
				return CallNextHookEx(getInstance().hook, nCode, wp, lp);
			}
			if (getInstance().exe(wp,((KBDLLHOOKSTRUCT*)lp)->vkCode))return 1;
			return CallNextHookEx(getInstance().hook, nCode, wp, lp);
		}
	public:
		auto regist(decltype(exe) exe) {
			this->exe = exe;
			return (hook = SetWindowsHookEx(WH_KEYBOARD_LL, hookProc, GetModuleHandle(0), 0)) != nullptr;
		}
		~KeyHook() {
			UnhookWindowsHookEx(hook);
		}
	};

}
int WINAPI WinMain(HINSTANCE, HINSTANCE, PSTR, int) {
	os::HotKey < 4649, MOD_ALT, 0x4A /*virual key code J*/ > hotkey;
	os::Cursor cursor;
	auto horizon = 0, vertical = 0;
	os::Message msg;
	bool slow = false, space = false, active = false;
	library::Timer(std::chrono::milliseconds(1), [&horizon, &vertical, &cursor] {
		if (!horizon && !vertical)return true;
		auto pos = cursor.getPos();
		pos.x += horizon;
		pos.y += vertical;
		cursor.setPos(pos);
		return true;
		});
	if (!hotkey.regist() || !os::KeyHook::getInstance().regist([&cursor, &horizon, &vertical, &slow, &space, &active, &msg](DWORD status, int keyCode)->bool {
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
			cursor.occurrence(speed ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP, cursor.getPos());
			break;
		case 'D':
			cursor.occurrence(speed ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP, cursor.getPos());
			break;
		case 'F':
			cursor.occurrence(speed ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP, cursor.getPos());
			break;
		case 'E':
			cursor.occurrence(MOUSEEVENTF_WHEEL, cursor.getPos(), space ? BOOST_DELTA : slow ? SLOW : WHEEL_DELTA);
			break;
		case 'V':
			cursor.occurrence(MOUSEEVENTF_WHEEL, cursor.getPos(), -(space ? BOOST_DELTA : slow ? SLOW : WHEEL_DELTA));
			break;
		case 'G':
			cursor.occurrence(MOUSEEVENTF_HWHEEL, cursor.getPos(), -(space ? BOOST_DELTA : slow ? SLOW : WHEEL_DELTA));
			break;
		case 'H':
			cursor.occurrence(MOUSEEVENTF_HWHEEL, cursor.getPos(), space ? BOOST_DELTA : slow ? SLOW : WHEEL_DELTA);
			break;
		case 'C':
			msg.quit();
			break;
		default:
			return false;
		}
		return true;
		}))return EXIT_SUCCESS;
	msg
		.regist(WM_HOTKEY, [&msg, &active] {
		active = true;
			})
		.loop();
			return EXIT_SUCCESS;
}