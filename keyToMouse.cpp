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
	 	static T &getInstance(){
			static T instance;
			return instance;
		};
	};
}
namespace library {
	class Timer {
	private:

	public:
		Timer(std::chrono::milliseconds interval, std::function<bool()> call){
			std::thread(
				[interval,call] {
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
	enum class MOD {
		ALT = MOD_ALT
	};
	enum class MESSAGE {//windowを想定していないのでdispatchmessageを呼んでいない等、loopはぬけがある
		HOTKEY
	};
	class Message {
	private:
		std::unordered_map<MESSAGE, std::function<void()> > exe;
	public:
		inline auto &regist(decltype(exe)::key_type msg,decltype(exe)::mapped_type proccess) {
			exe.emplace(msg,proccess);//後から変えたい場合はinsert_or_assign
			return *this;
		}
		inline auto loop() {
			const std::unordered_map<decltype(MSG::message), MESSAGE> convert{
				{WM_HOTKEY,MESSAGE::HOTKEY}
			};
			MSG msg;
			while (GetMessage(&msg,nullptr,0,0)) {
				if (!convert.count(msg.message)||!exe.count(convert.at(msg.message)))continue;
				exe.at(convert.at(msg.message))();
			}
		}
		inline auto quit() {
			PostQuitMessage(0);
		}
	};
	template <int ID ,MOD M, UINT VK >
	class HotKey {
	private:
	public:
		inline HotKey() {

		}
		inline bool regist() const noexcept(RegisterHotKey) {
			return RegisterHotKey(nullptr,ID,static_cast<UINT>(M), VK);
		}
		inline ~HotKey()noexcept(UnregisterHotKey) {
			UnregisterHotKey(nullptr,ID);
		}
	};
	class Cursor{
	private:
		struct Point {
			long x, y;
		};
		
	public:
		static constexpr auto DEFAULT_DELTA = WHEEL_DELTA;
		enum class EVENT {
			LEFT_UP = MOUSEEVENTF_LEFTUP,
			LEFT_DOWN = MOUSEEVENTF_LEFTDOWN,
			RIGHT_UP = MOUSEEVENTF_RIGHTUP,
			RIGHT_DOWN = MOUSEEVENTF_RIGHTDOWN,
			MIDDLE_UP = MOUSEEVENTF_MIDDLEUP,
			MIDDLE_DOWN = MOUSEEVENTF_MIDDLEDOWN,
			SCROLL_HORIZON = MOUSEEVENTF_HWHEEL,
			SCROLL_VERTICAL = MOUSEEVENTF_WHEEL
		};
		inline Point getPos() {
			POINT pos;
			GetCursorPos(&pos);
			return {pos.x,pos.y};
		}
		inline auto setPos(Point pos) {
			SetCursorPos(pos.x,pos.y);
		}
		inline auto occurrence(const EVENT e,const Point pos={0,0},const int data=0) {//オーバーロードした方がいいかも
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
	enum class VIRTUAL_KEY {
		SHIFT=VK_SHIFT,
		R_SHIFT=VK_RSHIFT,
		L_SHIFT=VK_LSHIFT,
		SPACE=VK_SPACE,
		ALT=VK_LMENU
	};
	enum class KEY_STATUS {
		UP=WM_KEYUP,
		DOWN= WM_KEYDOWN,
		SYSTEM_UP= WM_SYSKEYUP,
		SYSTEM_DOWN= WM_SYSKEYDOWN
	};
	class KeyHook:public design::Singleton<KeyHook>{
	private:
		HHOOK hook;
		std::function<bool(KEY_STATUS,int)> exe;
		static LRESULT CALLBACK hookProc(int nCode, WPARAM wp, LPARAM lp) {//複数のhookを処理する場合はIDを持たせて任意の処理を呼ぶ。ここでは、一度が確定しているので、無駄なことはしない。
			if (nCode < 0) {
				return CallNextHookEx(getInstance().hook, nCode, wp, lp);
			}
//#define CONVERT(ENUM,ITEM) {static_cast<std::underlying_type<ENUM>::type>(ENUM::ITEM),ENUM::ITEM}
			constexpr auto convert = [](auto target)constexpr {
				return std::pair(static_cast<std::underlying_type<decltype(target)>::type>(target),target);
			};
			const std::unordered_map<decltype(wp), KEY_STATUS> convertStatus{
				convert(KEY_STATUS::UP),
				convert(KEY_STATUS::DOWN),
				convert(KEY_STATUS::SYSTEM_UP),
				convert(KEY_STATUS::SYSTEM_DOWN),
			};
			const std::unordered_map<decltype(((KBDLLHOOKSTRUCT*)lp)->vkCode), VIRTUAL_KEY> convertKey{
				convert(VIRTUAL_KEY::SHIFT),
				convert(VIRTUAL_KEY::L_SHIFT),
				convert(VIRTUAL_KEY::R_SHIFT),
				convert(VIRTUAL_KEY::SPACE),
				convert(VIRTUAL_KEY::ALT),
			};
//#undef CONVERT
			auto keyCode=((KBDLLHOOKSTRUCT*)lp)->vkCode;
			if (convertKey.count(keyCode)) {
				keyCode = static_cast<decltype(keyCode)>(convertKey.at(keyCode));
			}
			if(getInstance().exe(convertStatus.at(wp), keyCode))return 1;
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
int WINAPI WinMain(HINSTANCE,HINSTANCE,PSTR,int) {
	os::HotKey < 4649, os::MOD::ALT, 0x4A /*virual key code J*/ > hotkey;
	os::Cursor cursor;
	auto horizon = 0, vertical = 0;
	os::Message msg;
	bool slow = false, space = false,active=false;
	library::Timer(std::chrono::milliseconds(1), [&horizon,&vertical,&cursor] {
		if (!horizon && !vertical)return true;
		auto pos=cursor.getPos();
		pos.x += horizon;
		pos.y += vertical;
		cursor.setPos(pos);
		return true;
		});
	if (!hotkey.regist()||!os::KeyHook::getInstance().regist([&cursor, &horizon, &vertical, &slow, &space,&active,&msg](os::KEY_STATUS status, int keyCode)->bool {
		if (!active)return false;
		constexpr auto DEFAULT = 10, BOOST = 50, SLOW = 1, BOOST_DELTA = os::Cursor::DEFAULT_DELTA * 2;
		const auto speed = status == os::KEY_STATUS::DOWN ? space ? BOOST : slow ? SLOW : DEFAULT : 0;
		const auto closure = [&speed](auto& key, const auto& changed) {
			return [newSpeed = std::exchange(key, speed) ? DEFAULT : changed](auto& number) {
				if (!number)return;
				number = 0 < number ? newSpeed : -newSpeed;
			};
		};
		switch (keyCode) {
		case static_cast<std::underlying_type<os::VIRTUAL_KEY>::type>(os::VIRTUAL_KEY::SPACE):
		{
			if (space && status == os::KEY_STATUS::DOWN)break;
			const auto tryAssign = closure(space, BOOST);
			tryAssign(horizon);
			tryAssign(vertical);
		}
		break;
		case static_cast<std::underlying_type<os::VIRTUAL_KEY>::type>(os::VIRTUAL_KEY::ALT):
			if (status!=os::KEY_STATUS::SYSTEM_DOWN)return false;
			active = false;
			break;
		case 'A':
		{
			if (slow && status == os::KEY_STATUS::DOWN)break;;
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
			cursor.occurrence(speed ? os::Cursor::EVENT::LEFT_DOWN : os::Cursor::EVENT::LEFT_UP, cursor.getPos());
			break;
		case 'D':
			cursor.occurrence(speed ? os::Cursor::EVENT::MIDDLE_DOWN : os::Cursor::EVENT::MIDDLE_UP, cursor.getPos());
			break;
		case 'F':
			cursor.occurrence(speed ? os::Cursor::EVENT::RIGHT_DOWN : os::Cursor::EVENT::RIGHT_UP, cursor.getPos());
			break;
		case 'E':
			cursor.occurrence(os::Cursor::EVENT::SCROLL_VERTICAL, cursor.getPos(), space ? BOOST_DELTA : slow ? SLOW : os::Cursor::DEFAULT_DELTA);
			break;
		case 'V':
			cursor.occurrence(os::Cursor::EVENT::SCROLL_VERTICAL, cursor.getPos(), -(space ? BOOST_DELTA : slow ? SLOW : os::Cursor::DEFAULT_DELTA));
			break;
		case 'G':
			cursor.occurrence(os::Cursor::EVENT::SCROLL_HORIZON, cursor.getPos(), -(space ? BOOST_DELTA : slow ? SLOW : os::Cursor::DEFAULT_DELTA));
			break;
		case 'H':
			cursor.occurrence(os::Cursor::EVENT::SCROLL_HORIZON, cursor.getPos(), space ? BOOST_DELTA : slow ? SLOW : os::Cursor::DEFAULT_DELTA);
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
		.regist(os::MESSAGE::HOTKEY, [&msg,&active] {
		active = true;
		})
		.loop();
	return EXIT_SUCCESS;
}