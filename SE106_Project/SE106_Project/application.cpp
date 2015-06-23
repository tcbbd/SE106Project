#include "stdafx.h"
#include "application.h"

#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <conio.h>

//TODO 请求输入时 按Esc返回 //或者用Ctrl-C实现

#define CURSOR_INIT	\
	CONSOLE_SCREEN_BUFFER_INFO csbi_start, csbi_end;\
	DWORD dwLength, cCharsWritten;\
	GetConsoleScreenBufferInfo(stdout_handle, &csbi_start);

#define CURSOR_RETRY do {\
	GetConsoleScreenBufferInfo(stdout_handle, &csbi_end);\
	dwLength = (csbi_end.dwCursorPosition.Y - csbi_start.dwCursorPosition.Y) * 80\
		+ (csbi_end.dwCursorPosition.X - csbi_start.dwCursorPosition.X);\
	FillConsoleOutputCharacter(stdout_handle, (TCHAR)' ', dwLength,\
		csbi_start.dwCursorPosition, &cCharsWritten);\
	SetConsoleCursorPosition(stdout_handle, csbi_start.dwCursorPosition); } while (0);

Application* Application::pInstance = nullptr;
HANDLE Application::stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
HANDLE Application::stdin_handle = GetStdHandle(STD_INPUT_HANDLE);

Application::Application() {
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);
	SetConsoleTitle(L"迷你社交系统");
	COORD size = {80, 25};
	SetConsoleScreenBufferSize(stdout_handle, size); //固定窗口大小
	other_user_return_to_search = false;
	return_from_message_list = true;
	state = MAIN_INTERFACE;
	pre_current_state_buffer = MAIN_INTERFACE;
}

Application* Application::getInstance() {
	if (pInstance == nullptr)
		pInstance = new Application;
	return pInstance;
}

//设置signal handler以屏蔽Ctrl-C和Ctrl-Break，并在意外退出时关闭打开的文件
BOOL Application::CtrlHandler(DWORD fdwCtrlType) {
	switch (fdwCtrlType) {
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
		return TRUE;
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT: {
		Application *app = Application::getInstance();
		app->quit(0);
		//delete app;
		//退出时操作系统会回收堆内存，无需delete也不会泄漏内存
		return TRUE;
	}
	default:
		return FALSE;
	}
}

//getline过程中如果被Ctrl-C或Ctrl-Break中断，则取消getline，返回上级菜单
basic_istream<char, char_traits<char>>& Application::getline(
	basic_istream<char, char_traits<char>>& _Istr,
	basic_string<char, char_traits<char>, allocator<char>>& _Str) {
	Application *app = Application::getInstance();
	std::getline(_Istr, _Str);
	while (_Istr.fail()) {
		_Istr.clear();
		app->state = app->pre_current_state_buffer;
		throw runtime_error("");
	}
	return _Istr;
}

KEY_EVENT_RECORD Application::_getkeyevent() {
	INPUT_RECORD input;
	DWORD num_read;
	DWORD oldmode;
	GetConsoleMode(stdin_handle, &oldmode);
	SetConsoleMode(stdin_handle, 0);
	while (true) {
		//经测试，KeyEvent.wRepeatCount始终为1，故以下不考虑其取值
		if (!ReadConsoleInput(stdin_handle, &input, 1, &num_read) || num_read == 0)
			continue;
		if (input.EventType != KEY_EVENT || !input.Event.KeyEvent.bKeyDown)
			continue;
		SetConsoleMode(stdin_handle, oldmode);
		if (input.Event.KeyEvent.uChar.AsciiChar == '\x3' && //Ctrl-C
			((input.Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED) ||
			(input.Event.KeyEvent.dwControlKeyState & RIGHT_CTRL_PRESSED))) {
			Application *app = Application::getInstance();
			app->state = app->pre_current_state_buffer;
			throw runtime_error("");
		}
		return input.Event.KeyEvent;
	}
}

void Application::set_cursor_invisible() {
	CONSOLE_CURSOR_INFO cci;
	cci.bVisible = FALSE;
	cci.dwSize = 25;
	SetConsoleCursorInfo(stdout_handle, &cci);
}

void Application::set_cursor_visible() {
	CONSOLE_CURSOR_INFO cci;
	cci.bVisible = TRUE;
	cci.dwSize = 25;
	SetConsoleCursorInfo(stdout_handle, &cci);
}

void Application::print_user_info() {
	stringstream ss;
	ss << "关注：" << current_user.out_count << "  粉丝：" << current_user.in_count << "  微博：" << current_user.message_count;
	cout << ss.str();
	cout << setiosflags(ios::right) << setw(80 - ss.str().length()) << "用户：" + current_user.username + '\n';
	cout << resetiosflags(ios::right) << endl;
}

void Application::print_hello_info(string info) {
	cout << "################################################################################";
	cout << "#                                                                              #";
	int tmp = (78 + info.size()) / 2;
	cout << '#' << setiosflags(ios::right) << setw(tmp) << info << setw(79 - tmp) << '#' << resetiosflags(ios::right);
	cout << "#                                                         Designed by dzc      #";
	cout << "################################################################################";
	cout << endl;
}

void Application::print_info() {
	switch (state) {
	case MAIN_INTERFACE:
		print_hello_info("欢迎使用迷你社交系统");
		cout << "提示： 您尚未登录，登录后才能使用本系统。" << endl << endl;
		cout << "请输入序号来选择： 1.我已经有用户,现在立刻登录 2.注册新用户 3.退出本系统" << "  ";
		break;
	case LOGIN:
		print_hello_info("用户登录");
		break;
	case REGIST:
		print_hello_info("注册新用户");
		break;
	case MAIN_MENU:
		print_hello_info("欢迎使用迷你社交系统");
		print_user_info();
		break;
	case USER_CENTER:
		print_hello_info("用户设置");
		print_user_info();
		break;
	case USER_INFO_MODIFY:
		print_hello_info("修改用户信息");
		print_user_info();
		break;
	case SEARCH_USER:
		print_hello_info("查找用户");
		print_user_info();
		break;
	case FOLLOWED_USER_LIST:
		print_hello_info("我关注的");
		print_user_info();
		break;
	case FANS_USER_LIST:
		print_hello_info("我的粉丝");
		print_user_info();
		break;
	case OTHER_USER_MAIN_MENU:
		print_hello_info("查看用户");
		print_user_info();
		break;
	case SEND_MESSAGE:
		print_hello_info("发布微博");
		print_user_info();
		break;
	case MY_MESSAGE:
		print_hello_info("我的微博");
		print_user_info();
		break;
	case OTHER_USER_MESSAGE:
		print_hello_info(other_user.username + "的微博");
		print_user_info();
		break;
	case FORWARD_MESSAGE:
		print_hello_info("转发微博");
		print_user_info();
		break;
	case FRESH_MESSAGE:
		print_hello_info("新鲜事");
		print_user_info();
		break;
	case DELETE_MESSAGE:
		print_hello_info("删除微博");
		print_user_info();
		break;
	default:
		break;
	}
}

int Application::quit(int i) {
	userdb_manager.close_files();
	return i;
}

void Application::get_username(string &username) {
	CURSOR_INIT;
	cout << "请输入用户名：  ";
	getline(cin, username);
	while (username.size() < 3 || username.size() > 16) {
		CURSOR_RETRY;
		cout << "用户名的长度必须为3-16个字符，请重新输入：  ";
		cin.clear();
		getline(cin, username);
	}
}

void Application::get_password_help(string &password) {
	char ch;
	size_t pos = 0;
	while (true) {
		ch = _getch();
		if (isprint(int((unsigned char)(ch)))) { //A Character
			password.insert(pos, 1, ch);
			cout << string(password.size() - pos, '*')
				<< string(password.size() - pos - 1, '\b');
			pos++;
		}
		else if (pos && (ch == '\b' || ch == '\x7f')) { //A Backspace or C-Backspace
			cout << string(password.size() - pos, '*') << "\b \b"
				<< string(password.size() - pos, '\b');
			password.erase(--pos, 1);
		}
		else if (ch == '\xe0') {
			ch = _getch();
			if (pos && ch == '\x4b') { //A Left Arrow
				pos--;
				cout << "\b";
			}
			else if (pos < password.size() && ch == '\x4d') { //A Right Arrow
				pos++;
				cout << "*";
			}
			else if (pos && ch == '\x47') { //A Home Key
				cout << string(pos, '\b');
				pos = 0;
			}
			else if (pos < password.size() && ch == '\x4f') { //An End Key
				cout << string(password.size() - pos, '*');
				pos = password.size();
			}
			else if (pos < password.size() && ch == '\x53') {//A Delete Key
				cout << string(password.size() - pos, '*') << "\b \b"
					<< string(password.size() - pos - 1, '\b');
				password.erase(pos, 1);
			}
		}
		else if (ch == '\x3') { //Ctrl-C
			state = pre_current_state_buffer;
			throw runtime_error("");
		}
		else if (!isascii(ch)) //可能为双字节编码的中文，需丢弃第二个字节
			ch = _getch();
		else if (!password.empty() && ch == '\r') {
			cout << endl;
			return;
		}
	}
}

void Application::get_password(string &password, string info) {
	CURSOR_INIT;
	cout << info;
	get_password_help(password);
	while (password.size() < 8 || password.size() > 32) {
		CURSOR_RETRY;
		password.clear();
		cout << "密码的长度必须为8-32个字符，请重新输入：  ";
		get_password_help(password);
	}
}

void Application::get_name(string &name) {
	CURSOR_INIT;
	cout << "请输入姓名：  ";
	getline(cin, name);
	while (name.size() < 3 || name.size() > 24) {
		CURSOR_RETRY;
		cout << "姓名的长度必须为3-24个字符，请重新输入：  ";
		getline(cin, name);
	}
}

bool Application::get_gender() {
	bool ismale;
	KEY_EVENT_RECORD key_event;
	wchar_t wch;
	bool inputted = false;
	while (true) {
		key_event = _getkeyevent();
		wch = key_event.uChar.UnicodeChar;
		if (wch == L'男' && !inputted) {
			inputted = true;
			ismale = true;
			cout << "男";
		}
		if (wch == L'女' && !inputted) {
			inputted = true;
			ismale = false;
			cout << "女";
		}
		if ((wch == L'\b' || wch == L'\x7f') && inputted) {
			inputted = false;
			cout << "\b\b  \b\b";
		}
		if ((wch == L'\r' || wch == L'\n') && inputted) {
			cout << endl;
			break;
		}
	}
	return ismale;
}

void Application::get_date(date &date) {
	char ch;
	const int INPUT_YEAR = 0;
	const int INPUT_MONTH = 1;
	const int INPUT_DAY = 2;
	int state = INPUT_YEAR;
	int year = 0, month = 0, day = 0;
	while (true) {
		ch = _getch();
		if (ch == '\x3') { //Ctrl-C
			this->state = pre_current_state_buffer;
			throw runtime_error("");
		}
		switch (state) {
		case INPUT_YEAR:
			if (isdigit(int((unsigned char)(ch)))) {
				if (year == 0 && ch == '0') //Disable leading zeros
					break;
				if (year * 10 + int(ch - '0') > 65535) //Should not overflow
					break;
				cout << ch;
				year = year * 10 + int(ch - '0');
			}
			else if (year && (ch == '\b' || ch == '\x7f')) {  //A Backspace or C-Backspace
				cout << "\b\x7f\b";
				year /= 10;
			}
			else if (year && ch == '-') {
				cout << ch;
				state = INPUT_MONTH;
			}
			break;
		case INPUT_MONTH:
			if (isdigit(int((unsigned char)(ch)))) {
				if (month == 0 && ch == '0') //Disable leading zeros
					break;
				if (month * 10 + int(ch - '0') > 12) //There are 12 months in a year
					break;
				cout << ch;
				month = month * 10 + int(ch - '0');
			}
			else if (ch == '\b' || ch == '\x7f') {  //A Backspace or C-Backspace
				cout << "\b\x7f\b";
				if (month)
					month /= 10;
				else
					state = INPUT_YEAR;
			}
			else if (month && ch == '-') {
				cout << ch;
				state = INPUT_DAY;
			}
			break;
		case INPUT_DAY:
			if (isdigit(int((unsigned char)(ch)))) {
				if (day == 0 && ch == '0') //Disable leading zeros
					break;
				int tmp_day = day * 10 + int(ch - '0');
				bool available = true;
				switch (month) {
				case 1: case 3: case 5: case 7: case 8: case 10: case 12:
					if (tmp_day > 31)
						available = false;
					break;
				case 4: case 6: case 9: case 11:
					if (tmp_day > 30)
						available = false;
					break;
				case 2:
					if (tmp_day > 29)
						available = false;
					else if (tmp_day == 29 && (year % 4 != 0
							|| (year % 100 == 0 && year % 400 != 0))) //Not a leap year
						available = false;
					break;
				default:
					break;
				}
				if (available) {
					cout << ch;
					day = tmp_day;
				}
				//else do nothing
			}
			else if (ch == '\b' || ch == '\x7f') {  //A Backspace or C-Backspace
				cout << "\b\x7f\b";
				if (day)
					day /= 10;
				else
					state = INPUT_MONTH;
			}
			else if (day && ch == '\r') {
				cout << endl;
				date.year = year;
				date.month = month;
				date.day = day;
				return;
			}
			break;
		default:
			break;
		}
	}
}

long long Application::get_phonenum() {
	char ch;
	long long phonenum = 0; //zero represents no phone number
	while (true) {
		ch = _getch();
		if (phonenum == 0 && ch == '0') //Disable leading zeros
			continue;
		if (isdigit(int((unsigned char)(ch)))) {
			if (phonenum * 10 + int(ch - '0') > 100000000000000)
				//14 zeros in RHS for something like +86-0571-66666666
				//where +86 represents China and 0571 represents HangZhou
				continue;
			cout << ch;
			phonenum = phonenum * 10 + int(ch - '0');
		}
		else if (phonenum && (ch == '\b' || ch == '\x7f')) {  //A Backspace or C-Backspace
			cout << "\b\x7f\b";
			phonenum /= 10;
		}
		else if (ch == '\r') {
			cout << endl;
			break;
		}
		else if(ch == '\x3') { //Ctrl-C
			state = pre_current_state_buffer;
			throw runtime_error("");
		}
	}
	return phonenum;
}

void Application::get_hometown(string &hometown) {
	CURSOR_INIT;
	cout << "请输入家乡（按回车跳过）：  ";
	getline(cin, hometown);
	while (hometown.size() > 24) {
		CURSOR_RETRY;
		cout << "家乡的长度不得大于24个字符，请重新输入：  ";
		getline(cin, hometown);
	}
}

bool Application::get_bool(string info) {
	CURSOR_INIT;
	cout << info << "（是/否）:  ";
	bool ret;
	KEY_EVENT_RECORD key_event;
	wchar_t wch;
	bool inputted = false;
	while (true) {
		key_event = _getkeyevent();
		wch = key_event.uChar.UnicodeChar;
		if (wch == L'是' && !inputted) {
			inputted = true;
			ret = true;
			cout << "是";
		}
		if (wch == L'否' && !inputted) {
			inputted = true;
			ret = false;
			cout << "否";
		}
		if ((wch == L'\b' || wch == L'\x7f') && inputted) {
			inputted = false;
			cout << "\b\b  \b\b";
		}
		if ((wch == L'\r' || wch == L'\n') && inputted) {
			cout << endl;
			break;
		}
	}
	CURSOR_RETRY;
	return ret;
}

void Application::regist_work() {
	string username;
	bool username_retry = false;
	set_cursor_visible();
	print_info();
	while (true) {
		get_username(username);
		int result = userdb_manager.find_user_record(username, current_user);
		if (result != -1) {
			system("cls");
			username_retry = true;
			print_info();
			if (result == 0)
				cout << "该用户已存在！" << endl;
			else if (result == -2)
				cout << "该用户已被删除，无法用于注册新用户！" << endl;
			continue;
		}
		else
			break;
	}

	string password, confirm_password;
	CURSOR_INIT;
	get_password(password, "请输入密码：  ");
	get_password(confirm_password, "请再次输入密码以确认：  ");
	while (confirm_password != password) {
		CURSOR_RETRY;
		password.clear();
		confirm_password.clear();
		get_password(password, "两次输入的密码不同，请再试一次：  ");
		get_password(confirm_password, "请再次输入密码以确认：  ");
	}

	string name;
	get_name(name);

	cout << "请输入性别（男/女）:  ";
	bool ismale = get_gender();

	date birthday;
	cout << "请输入生日（年-月-日）：  ";
	get_date(birthday);

	cout << "请输入联系电话（按回车跳过）：  ";
	long long phonenum = get_phonenum();

	string hometown;
	get_hometown(hometown);

	current_user.initialize(username, password, name, ismale, birthday, phonenum, hometown);
	userdb_manager.insert_user_record(current_user);
	current_followed.clear();
	current_fans.clear();
	set_cursor_invisible();
	cout << endl << endl << endl << "恭喜您注册完毕，正在为您跳转";
	Sleep(500);
	cout << "。";
	Sleep(500);
	cout << "。";
	Sleep(500);
	cout << "。";
	Sleep(500);
	system("cls");
	state = MAIN_MENU;
}

void Application::login_work() {
	string username, password;
	set_cursor_visible();
	print_info();
	while (true) {
		get_username(username);
		get_password(password, "请输入密码：  ");

		if (userdb_manager.find_user_record(username, current_user) < 0) {
			password.clear();
			system("cls");
			print_info();
			cout << "该用户不存在！" << endl;
			continue;
		}
		else if (password != current_user.password) {
			password.clear();
			system("cls");
			print_info();
			cout << "密码不正确，请重试!" << endl;
			continue;
		}
		else
			break;
	}
	current_followed = userdb_manager.get_followed(current_user);
	current_fans = userdb_manager.get_fans(current_user);
	set_cursor_invisible();
	cout << endl << endl << endl << "登录成功，正在为您跳转";
	Sleep(500);
	cout << "。";
	Sleep(500);
	cout << "。";
	Sleep(500);
	cout << "。";
	Sleep(500);
	system("cls");
	state = MAIN_MENU;
}

void Application::main_menu_work() {
	pre_current_state_buffer = MAIN_MENU;
	set_cursor_invisible();
	print_info();
	cout << "请输入序号来选择： 1.发布微博 2.新鲜事   3.我的微博 4.我关注的" << endl;
	cout << "                   5.我的粉丝 6.查找用户 7.用户设置 8.退出登录" << "  ";
	char ch;
	while (true) {
		ch = _getch();
		if ('1' <= ch  && ch <= '8')
			break;
	}
	if (ch == '8') {
		current_followed.clear();
		current_fans.clear();
		search_result.clear();
		pre_current_state_buffer = MAIN_INTERFACE;
	}
	system("cls");
	if (ch == '1')
		state = SEND_MESSAGE;
	else if (ch == '2')
		state = FRESH_MESSAGE;
	else if (ch == '3')
		state = MY_MESSAGE;
	else if (ch == '4')
		state = FOLLOWED_USER_LIST;
	else if (ch == '5')
		state = FANS_USER_LIST;
	else if (ch == '6')
		state = SEARCH_USER;
	else if (ch == '7')
		state = USER_CENTER;
	else if (ch == '8')
		state = MAIN_INTERFACE;
}

void Application::user_center_work() {
	pre_current_state_buffer = USER_CENTER;
	set_cursor_invisible();
	print_info();
	cout << "请输入序号来选择： 1.修改密码 2.修改个人信息 3.删除用户 4.返回上级菜单" << "  ";
	char ch;
	while (true) {
		ch = _getch();
		if ('1' <= ch  && ch <= '4')
			break;
	}
	system("cls");
	if (ch == '1') {
		set_cursor_visible();
		print_info();
		string old_password, new_password;
		CURSOR_INIT;
		get_password(old_password, "请输入原密码：  ");
		while (old_password != current_user.password) {
			CURSOR_RETRY;
			old_password.clear();
			get_password(old_password, "密码不正确，请重新输入：  ");
		}
		get_password(new_password, "请输入新密码：  ");
		current_user.password = new_password;
		userdb_manager.modify_user_record(current_user);
		set_cursor_invisible();
		cout << endl << endl << endl << "修改密码成功，按任意键继续";
		_getch();
		system("cls");
	}
	else if (ch == '2')
		state = USER_INFO_MODIFY;
	else if (ch == '3') {
		set_cursor_visible();
		print_info();
		string password;
		CURSOR_INIT;
		get_password(password, "请输入密码：  ");
		while (password != current_user.password) {
			CURSOR_RETRY;
			password.clear();
			get_password(password, "密码不正确，您无权删除用户，请重试：  ");
		}
		userdb_manager.delete_user(current_user);
		set_cursor_invisible();
		cout << endl << endl << endl << "删除用户成功，按任意键继续";
		_getch();
		state = MAIN_INTERFACE;
		system("cls");
	}
	else if (ch == '4')
		state = MAIN_MENU;
}

void Application::user_info_modify_work() {
	pre_current_state_buffer = USER_INFO_MODIFY;
	set_cursor_invisible();
	print_info();
	cout << "您的个人信息如下：" << endl << endl;
	current_user.print_info(false, false);
	cout << endl << "请输入序号来选择： 1.修改姓名     2.修改性别 3.修改生日" << endl;
	cout << "                   4.修改联系电话 5.修改家乡 6.返回上级菜单" << "  ";
	char ch;
	while (true) {
		ch = _getch();
		if ('1' <= ch  && ch <= '6')
			break;
	}
	system("cls");
	if (ch == '6') {
		state = USER_CENTER;
		return;
	}

	set_cursor_visible();
	print_info();
	switch (ch) {
	case '1': {
		string name;
		get_name(name);
		current_user.name = name;
		break;
	}
	case '2':
		cout << "请输入性别（男/女）:  ";
		current_user.ismale = get_gender();
		break;
	case '3': {
		date birthday;
		cout << "请输入生日（年-月-日）：  ";
		get_date(birthday);
		current_user.birthday = birthday;
		break;
	}
	case '4':
		cout << "请输入联系电话（按回车跳过）：  ";
		current_user.phonenum = get_phonenum();
		break;
	case '5': {
		string hometown;
		get_hometown(hometown);
		current_user.hometown = hometown;
		break;
	}
	default:
		break;
	}
	userdb_manager.modify_user_record(current_user);
	set_cursor_invisible();
	cout << endl << endl << endl << "修改用户信息成功，按任意键继续";
	_getch();
	system("cls");
}

//仅当退出到MAIN_MENU或进入OTHER_USER时返回
void Application::list_user_set(SetVisitor &user_set) {
	const int ENTRY_PER_PAGE = 30;
	int page_num = (user_set.size() - 1) / ENTRY_PER_PAGE + 1;
	int current_page = 1;
	stringstream ss;
	vector<string> usernames;
	vector<User> users;
	string username;
	int num = ENTRY_PER_PAGE;
	auto iter = user_set.begin();
	int input = -1; //-1 represents nothing inputed
	bool left_arrow = false;
	KEY_EVENT_RECORD key_event;
	wchar_t wch;
	char ch;

	CURSOR_INIT;
	while (true) {
		//列出用户
		if (left_arrow)
			for (int i = 0; i < 30 + num; i++)
				--iter;
		num = ENTRY_PER_PAGE;
		usernames.clear();
		users.clear();
		for (int i = 1; i <= ENTRY_PER_PAGE; i++) {
			//2 24 2 24 2 24 2 共80字符
			//24字符的一项，由4+12+8组成，见下
			username = iter.get_username();
			if (user_set.get_kind() == SetVisitor::STRING)
				usernames.push_back(username);
			else if (user_set.get_kind() == SetVisitor::USER)
				users.push_back(iter.get_user());
			if (i % 3 == 1)
				cout << "  ";
			//"XX. "，4字符
			ss << i << ". ";
			cout << setiosflags(ios::right) << setw(4) << ss.str();
			cout << resetiosflags(ios::right);
			//username,11字符+1空格
			ss.str("");
			if (username.size() <= 11)
				ss << username;
			else {
				//用户名太长显示不下，需要截断，截断时需注意不能将一个二字节字符从中截断
				int pos = 0;
				while (pos < 8) {
					if (isascii(username[pos]))
						pos++;
					else
						pos += 2;
				}
				if (pos == 8)
					ss << username.substr(0, 8) << "...";
				else
					ss << username.substr(0, 7) << "...";
			}
			cout << setiosflags(ios::left) << setw(12) << ss.str();
			cout << resetiosflags(ios::left);
			//关注状态，8字符
			ss.str("");
			bool followed, fans;
			if (current_followed.find(username) != current_followed.end())
				followed = true;
			else
				followed = false;
			if (current_fans.find(username) != current_fans.end())
				fans = true;
			else
				fans = false;
			if (followed) {
				if (fans)
					cout << setiosflags(ios::right) << setw(8) << "互相关注";
				else
					cout << setiosflags(ios::right) << setw(8) << "已关注";
			}
			else {
				if (fans)
					cout << setiosflags(ios::right) << setw(8) << "我的粉丝";
				else
					cout << setiosflags(ios::right) << setw(8) << "  ";
			}
			cout << resetiosflags(ios::right);
			cout << "  ";
			++iter;
			if (iter == user_set.end()) {
				num = i;
				break;
			}
		}

		//显示页码
		cout << endl;
		ss << "第" << current_page << '/' << page_num << "页";
		cout << setiosflags(ios::right) << setw(80) << ss.str();
		cout << resetiosflags(ios::right) << endl;
		ss.str("");

		//用户输入
		cout << "请输入序号选择一名用户，按Ctrl-C返回主菜单，左右键翻页：" << "  ";
		if (input != -1)
			cout << input;
		while (true) {
			key_event = _getkeyevent();
			if (key_event.wVirtualKeyCode == VK_LEFT) {
				if (current_page == 1)
					continue;
				left_arrow = true;
				current_page--;
				CURSOR_RETRY;
				break;
			}
			else if (key_event.wVirtualKeyCode == VK_RIGHT) {
				if (current_page == page_num)
					continue;
				left_arrow = false;
				current_page++;
				CURSOR_RETRY;
				break;
			}
			wch = key_event.uChar.UnicodeChar;
			if (!isascii(wch))
				continue;
			ch = key_event.uChar.AsciiChar;
			if (isdigit(int((unsigned char)(ch)))) {
				if (input == -1) {
					if (ch == '0') //Disable leading zeros
						continue;
					input = int(ch - '0');
					cout << ch;
				}
				else {
					int tmp = input * 10 + int(ch - '0');
					if (tmp > num)
						continue;
					input = tmp;
					cout << ch;
				}
			}
			else if (input != -1 && (ch == '\b' || ch == '\x7f')) {  //A Backspace or C-Backspace
				cout << "\b \b";
				if (input < 10)
					input = -1;
				else
					input /= 10;
			}
			else if (input != -1 && ch == '\r') {
				system("cls");
				if (user_set.get_kind() == SetVisitor::STRING)
					userdb_manager.find_user_record(usernames[input - 1], other_user);
				else if (user_set.get_kind() == SetVisitor::USER)
					other_user = users[input - 1];
				pre_other_user_state_buffer = state;
				state = OTHER_USER_MAIN_MENU;
				return;
			}
		}
	}
}

void Application::search_user_work() {
	set_cursor_visible();
	print_info();

	//上一次查找到了若干用户，查看了某个用户的信息后，又返回查看整个查找到的用户列表
	if (other_user_return_to_search) {
		other_user_return_to_search = false;
		cout << "找到的用户如下：" << endl << endl;
		list_user_set(SetVisitor(search_result));
		return;
	}

	string username;
	if (get_bool("是否要按用户名精确查找")) {
		CURSOR_INIT;
		get_username(username);
		while (username == current_user.username) {
			cout << endl << "您查找的是自己，请重试";
			_getch();
			CURSOR_RETRY;
			get_username(username);
		}
		int ret = userdb_manager.find_user_record(username, other_user);
		system("cls");
		if (ret != 0) {
			set_cursor_invisible();
			print_info();
			if (ret == -1)
				cout << "该用户不存在，按任意键继续";
			else if (ret == -2)
				cout << "该用户已被删除，按任意键继续";
			_getch();
			system("cls");
			state = MAIN_MENU;
			return;
		}
		pre_other_user_state_buffer = MAIN_MENU;
		state = OTHER_USER_MAIN_MENU;
		return;
	}

	cout << "以下将根据用户选择的某一项或若干选项的组合进行查找" << endl << endl;
	bool use_username = get_bool("是否要按用户名模糊查找");
	if (use_username) {
		CURSOR_INIT;
		cout << "请输入用户名（或其一部分）：  ";
		getline(cin, username);
		while (username.empty() || username.size() > 16) {
			CURSOR_RETRY;
			cout << "用户名的长度不能为空或超过16个字符，请重新输入：  ";
			cin.clear();
			getline(cin, username);
		}
	}

	string name;
	bool use_name = get_bool("是否要按用户的姓名查找");
	if (use_name)
		get_name(name);

	bool ismale;
	bool use_gender = get_bool("是否要按用户的性别查找");
	if (use_gender) {
		cout << "请输入性别（男/女）:  ";
		ismale = get_gender();
	}

	date birthday_start, birthday_end;
	bool use_birthday_start = get_bool("是否要按用户的生日查找");
	bool use_birthday_end = false;
	if (use_birthday_start) {
		CURSOR_INIT;
		cout << "请输入序号来选择： 1.精确查找生日 2.查找生日区段  ";
		char ch;
		while (true) {
			ch = _getch();
			if (ch == '1' || ch == '2')
				break;
		}
		CURSOR_RETRY;
		if (ch == '1') {
			cout << "请输入生日（年-月-日）：  ";
			get_date(birthday_start);
		}
		else if (ch == '2') {
			use_birthday_end = true;
			cout << "请输入起始日期（年-月-日）：  ";
			get_date(birthday_start);
			CURSOR_INIT;
			cout << "请输入结束日期（年-月-日）：  ";
			get_date(birthday_end);
			while (birthday_end <= birthday_start) {
				CURSOR_RETRY;
				cout << "结束日期应晚于起始日期，请重新输入：  ";
				get_date(birthday_end);
			}
		}
	}

	long long phonenum;
	bool use_phonenum = get_bool("是否要按用户的电话号码查找");
	if (use_phonenum) {
		CURSOR_INIT;
		do {
			CURSOR_RETRY;
			cout << "请输入联系电话：  ";
			phonenum = get_phonenum();
		} while (phonenum == 0);
	}

	string hometown;
	bool use_hometown = get_bool("是否要按用户的家乡查找");
	if (use_hometown) {
		CURSOR_INIT;
		do {
			CURSOR_RETRY;
			get_hometown(hometown);
		} while (hometown.empty());
	}

	if (!use_username && !use_name && !use_gender && !use_birthday_start &&
		!use_birthday_end && !use_phonenum && !use_hometown) {
		cout << endl << "您未指定任何搜索选项，按任意键返回主菜单";
		_getch();
		system("cls");
		state = MAIN_MENU;
		return;
	}

	Query query;
	query.username = make_pair(username, use_username);
	query.name = make_pair(name, use_name);
	query.ismale = make_pair(ismale, use_gender);
	query.birthday_start = make_pair(birthday_start, use_birthday_start);
	query.birthday_end = make_pair(birthday_end, use_birthday_end);
	query.phonenum = make_pair(phonenum, use_phonenum);
	query.hometown = make_pair(hometown, use_hometown);

	cout << endl << "正在查找，请稍候";
	search_result = userdb_manager.find_regioned_user(query, current_user.username);

	system("cls");
	print_info();
	if (search_result.empty()) {
		set_cursor_invisible();
		cout << "未找到任何符合的用户，按任意键继续";
		_getch();
		system("cls");
		state = MAIN_MENU;
		return;
	}

	cout << "找到的用户如下：" << endl << endl;
	list_user_set(SetVisitor(search_result));
}

void Application::followed_fans_user_list_work(bool followed) {
	set_cursor_visible();
	print_info();
	if (followed) {
		if (current_followed.empty()) {
			set_cursor_invisible();
			cout << "您还没有关注过别人，按任意键返回主菜单";
			_getch();
			system("cls");
			state = MAIN_MENU;
			return;
		}
		list_user_set(SetVisitor(current_followed));
	}
	else {
		if (current_fans.empty()) {
			set_cursor_invisible();
			cout << "您还没有粉丝，按任意键返回主菜单";
			_getch();
			system("cls");
			state = MAIN_MENU;
			return;
		}
		list_user_set(SetVisitor(current_fans));
	}
}

void Application::other_user_main_menu_work() {
	pre_current_state_buffer = MAIN_MENU;
	set_cursor_invisible();
	bool followed, fans;

	if (current_followed.find(other_user.username) != current_followed.end())
		followed = true;
	else
		followed = false;
	if (current_fans.find(other_user.username) != current_fans.end())
		fans = true;
	else
		fans = false;

	print_info();
	cout << "您要查看的用户如下：" << endl << endl;
	other_user.print_info(followed, fans);

	cout << endl << "请输入序号来选择： ";
	if (other_user.ismale)
		cout << "1.他的微博 ";
	else
		cout << "1.她的微博 ";
	if (followed)
		cout << "2.取消关注 ";
	else
		cout << "2.关注 ";
	if (fans)
		cout << "3.移除粉丝 4.返回上级菜单" << "  ";
	else
		cout << "3.返回上级菜单" << "  ";

	char ch;
	while (true) {
		ch = _getch();
		if ('1' <= ch  && ch <= '3')
			break;
		if (fans && ch == '4')
			break;
	}
	system("cls");
	if (ch == '1')
		state = OTHER_USER_MESSAGE;
	else if (ch == '2') {
		if (followed) {
			userdb_manager.delete_user_relation(current_user, other_user);
			current_followed.erase(other_user.username);
		}
		else {
			userdb_manager.insert_user_relation(current_user, other_user);
			current_followed.insert(other_user.username);
		}
	}
	else if (ch == '3') {
		if (fans) {
			userdb_manager.delete_user_relation(other_user, current_user);
			current_fans.erase(other_user.username);
		}
		else {
			state = pre_other_user_state_buffer;
			if (state == SEARCH_USER)
				other_user_return_to_search = true;
		}
	}
	else if (ch == '4') {
		state = pre_other_user_state_buffer;
		if (state == SEARCH_USER)
			other_user_return_to_search = true;
	}
}

void Application::send_message_work() {
	set_cursor_visible();
	print_info();
	cout << "输入您要发布的微博，按Ctrl-回车发布，按Ctrl-C返回：" << endl;

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	COORD input_num_pos, input_cursor_pos;

	cout << "                                                                       ";
	GetConsoleScreenBufferInfo(stdout_handle, &csbi);
	input_num_pos = csbi.dwCursorPosition;
	cout << "  0/140字";
	cout << "################################################################################";
	GetConsoleScreenBufferInfo(stdout_handle, &csbi);
	input_cursor_pos.Y = input_num_pos.Y + 2;
	input_cursor_pos.X = 1;
	for (int i = 0; i < 4; i++)
		cout << "#                                                                              #";
	cout << "################################################################################";

	wstring message;
	MessageEditor editor(message, true, input_num_pos, input_cursor_pos);
	message = editor.get_message();

	system("cls");
	print_info();
	SYSTEMTIME systemtime;
	GetLocalTime(&systemtime);
	Time time(systemtime);
	userdb_manager.send_message(current_user, NULL, message, time);
	state = MAIN_MENU;
	cout << "发送微博成功,按任意键继续";
	_getch();
	system("cls");
}

void Application::forward_message_work() {
	set_cursor_visible();
	print_info();

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	COORD input_num_pos, input_cursor_pos, end_pos;
	cout << "输入您要转发的内容，按Ctrl-回车发布，按Ctrl-C返回：" << endl;
	cout << "                                                                       ";
	GetConsoleScreenBufferInfo(stdout_handle, &csbi);
	input_num_pos = csbi.dwCursorPosition;
	cout << "  0/140字";
	input_cursor_pos.X = 0;
	input_cursor_pos.Y = input_num_pos.Y + 1;
	cout << endl << endl << endl << endl;
	print_origin_message();
	cout << endl;
	GetConsoleScreenBufferInfo(stdout_handle, &csbi);
	end_pos = csbi.dwCursorPosition;

	//初始化消息
	wstring message;
	if (!current_message.origin_message.empty()) {
		message = current_message.message;
		const char *author_str = current_message.author.c_str();
		size_t author_size = current_message.author.size();
		int w_author_size = MultiByteToWideChar(CP_ACP, 0, author_str, author_size, NULL, 0);
		wchar_t *w_author_str = new wchar_t[w_author_size + 1];
		MultiByteToWideChar(CP_ACP, 0, author_str, author_size, w_author_str, w_author_size);
		w_author_str[w_author_size] = L'\0';
		message = L"//@" + wstring(w_author_str) + L":" + message;
		delete[] w_author_str;
	}
	int character_count = message.size();
	SetConsoleCursorPosition(stdout_handle, input_num_pos);
	if (character_count > 140) {
		SetConsoleTextAttribute(stdout_handle, FOREGROUND_RED | FOREGROUND_INTENSITY);
		cout << setiosflags(ios::right) << setw(3) << character_count << resetiosflags(ios::right);
		SetConsoleTextAttribute(stdout_handle, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
	}
	else
		cout << setiosflags(ios::right) << setw(3) << character_count << resetiosflags(ios::right);

	MessageEditor editor(message, false, input_num_pos, input_cursor_pos);
	message = editor.get_message();
	SYSTEMTIME systemtime;
	GetLocalTime(&systemtime);
	Time time(systemtime);
	userdb_manager.send_message(current_user, current_message.offset, message, time);
	current_message.forward_count++;
	if (!current_message.origin_message.empty())
		current_message.origin_forward_count++;
	set_cursor_invisible();
	SetConsoleCursorPosition(stdout_handle, end_pos);
	state = pre_forward_delete_message_state_buffer;
	cout << "转发微博成功,按任意键继续";
	_getch();
	system("cls");
}

void Application::delete_message_work() {
	set_cursor_invisible();
	print_info();
	if (get_bool("你确定要删除这条微博吗？")) {
		userdb_manager.delete_message(current_user, current_message);
		cout << "成功删除微博，按任意键返回";
	}
	else
		cout<<"您取消了删除，按任意键返回";
	state = pre_forward_delete_message_state_buffer;
	_getch();
	system("cls");
}

void Application::my_message_work() {
	pre_current_state_buffer = MY_MESSAGE; //进入转发界面后，Ctrl-C返回
	pre_forward_delete_message_state_buffer = MY_MESSAGE;
	set_cursor_invisible();
	print_info();
	if (return_from_message_list) {
		return_from_message_list = false;
		userdb_manager.user_message_list_init(current_user);
		if (userdb_manager.get_next_user_message(current_message) == -1) {
			cout << "您尚未发布过任何微博，快去发布一条吧！按任意键继续";
			_getch();
			system("cls");
			state = MAIN_MENU;
			return_from_message_list = true;
			return;
		}
	}
	int ret = list_user_message(false);
	if (ret == -1)
		return;
	if (ret == -2) {
		cout << "您尚未发布过任何微博，快去发布一条吧！按任意键继续";
		_getch();
	}
	system("cls");
	state = MAIN_MENU;
	return_from_message_list = true;
}

void Application::other_user_message_work() {
	pre_current_state_buffer = OTHER_USER_MESSAGE; //进入转发界面后，Ctrl-C返回
	pre_forward_delete_message_state_buffer = OTHER_USER_MESSAGE;
	set_cursor_invisible();
	print_info();
	if (return_from_message_list) {
		return_from_message_list = false;
		userdb_manager.user_message_list_init(other_user);
		if (userdb_manager.get_next_user_message(current_message) == -1) {
			if (other_user.ismale)
				cout << "他";
			else
				cout << "她";
			cout << "还未发布过微博，快去看看其他用户吧！按任意键继续";
			_getch();
			system("cls");
			state = OTHER_USER_MAIN_MENU;
			return_from_message_list = true;
			return;
		}
	}
	if (list_user_message(false) == -1)
		return;
	system("cls");
	state = OTHER_USER_MAIN_MENU;
	return_from_message_list = true;
}

//返回上级菜单返回0，转发返回-1，删除后无可显示的信息返回-2
int Application::list_user_message(bool fresh_message) {
	if (current_message.message.empty())
		return -2;
	char ch;
	int tmp;
	CURSOR_INIT;
	while (true) {
		print_message(fresh_message);
		bool my_message = (current_message.author == current_user.username);
		cout << "请输入序号来选择： ";
		if (fresh_message)
			cout << "1.上一条新鲜事 2.下一条新鲜事 3.转发";
		else
			cout << "1.上一条微博 2.下一条微博 3.转发";
		if (current_message.forward_count)
			cout << '(' << current_message.forward_count << ')';
		if (my_message)
			cout << " 4.删除 5.返回上级菜单  ";
		else
			cout << " 4.返回上级菜单  ";
		while (true) {
			ch = _getch();
			if (ch == '1') {
				if (fresh_message)
					tmp = userdb_manager.get_pre_fresh_message(current_message);
				else
					tmp = userdb_manager.get_pre_user_message(current_message);
				if (tmp == -1) {
					CURSOR_INIT;
					if (fresh_message)
						cout << endl << endl << "抱歉，没有上一条新鲜事了，按任意键继续";
					else
						cout << endl << endl << "抱歉，没有上一条微博了，按任意键继续";
					_getch();
					CURSOR_RETRY;
					continue;
				}
				CURSOR_RETRY;
				break;
			}
			else if (ch == '2') {
				if (fresh_message)
					tmp = userdb_manager.get_next_fresh_message(current_message);
				else
					tmp = userdb_manager.get_next_user_message(current_message);
				if (tmp == -1) {
					CURSOR_INIT;
					if (fresh_message)
						cout << endl << endl << "抱歉，没有下一条新鲜事了，按任意键继续";
					else
						cout << endl << endl << "抱歉，没有下一条微博了，按任意键继续";
					_getch();
					CURSOR_RETRY;
					continue;
				}
				CURSOR_RETRY;
				break;
			}
			else if (ch == '3') {
				system("cls");
				state = FORWARD_MESSAGE;
				return -1;
			}
			else if (ch == '4' && my_message) {
				system("cls");
				state = DELETE_MESSAGE;
				return -1;
			}
			else if ((ch == '4' && !my_message) || (ch == '5' && my_message))
				return 0;
		}
	}
}

void Application::fresh_message_work() {
	pre_current_state_buffer = FRESH_MESSAGE; //进入转发界面后，Ctrl-C返回
	pre_forward_delete_message_state_buffer = FRESH_MESSAGE;
	set_cursor_invisible();
	print_info();
	if (return_from_message_list) {
		return_from_message_list = false;
		userdb_manager.fresh_message_init(current_user, current_followed);
		if (userdb_manager.get_next_fresh_message(current_message) == -1) {
			cout << "抱歉，目前没有任何新鲜事！按任意键继续";
			_getch();
			system("cls");
			state = MAIN_MENU;
			return_from_message_list = true;
			return;
		}
	}
	int ret = list_user_message(true);
	if (ret == -1)
		return;
	if (ret == -2) {
		cout << "抱歉，目前没有任何新鲜事！按任意键继续";
		_getch();
	}
	system("cls");
	state = MAIN_MENU;
	return_from_message_list = true;
}

void Application::print_message(bool fresh_message) {
	if (fresh_message)
		cout << current_message.author << ':' << endl;
	wchar_t wch;
	int i = 0;
	while (i < current_message.message.size()) {
		wch = current_message.message[i];
		if (wch == L' ') {
			bool print_blank;
			if (i == 0) //开头的空格串忽略
				print_blank = false;
			else
				print_blank = true;
			i++;
			while (i < current_message.message.size()) {
				wch = current_message.message[i];
				if (wch != L' ')
					break;
				i++;
			}
			if (i == current_message.message.size()) //延续到结尾的空格串忽略
				break;
			wch = current_message.message[i];
			if (print_blank)
				cout << ' ';
		}
		WriteConsoleW(stdout_handle, &wch, 1, NULL, NULL);
		i++;
	}
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(stdout_handle, &csbi);
	if (csbi.dwCursorPosition.X != 0)
		cout << endl;
	print_origin_message();
	print_time(current_message.time);
	cout << endl << endl;
}

void Application::print_origin_message() {
	if (current_message.origin_message.empty())
		return;
	cout << "     #                                                                          ";
	cout << "##### ##########################################################################";
	cout << "#@";
	cout << setiosflags(ios::left) << setw(77) << current_message.origin_author;
	cout << resetiosflags(ios::left) << '#';

	cout << '#';
	int pos = 1;
	wchar_t wch;
	int i = 0;
	while (i < current_message.origin_message.size()) {
		wch = current_message.origin_message[i];
		if (wch == L' ') {
			bool print_blank;
			if (i == 0) //开头的空格串忽略
				print_blank = false;
			else
				print_blank = true;
			i++;
			while (i < current_message.origin_message.size()) {
				wch = current_message.origin_message[i];
				if (wch != L' ')
					break;
				i++;
			}
			if (i == current_message.origin_message.size()) //延续到结尾的空格串忽略
				break;
			wch = current_message.origin_message[i];
			wchar_t tmp = L' ';
			if (print_blank) {
				if (pos == 79) {
					cout << "##";
					pos = 2;
				}
				else
					pos++;
				WriteConsoleW(stdout_handle, &tmp, 1, NULL, NULL);
			}
		}
		if (isascii(wch)) {
			if (pos == 79) {
				cout << "##";
				pos = 2;
			}
			else
				pos++;
		}
		else {
			if (pos == 78) {
				cout << " ##";
				pos = 3;
			}
			else if (pos == 79) {
				cout << "##";
				pos = 3;
			}
			else
				pos += 2;
		}
		WriteConsoleW(stdout_handle, &wch, 1, NULL, NULL);
		i++;
	}
	cout << setiosflags(ios::right) << setw(80 - pos) << '#' << resetiosflags(ios::right);
	cout << '#';
	print_time(current_message.origin_time);
	stringstream ss;
	ss << "转发";
	if (current_message.origin_forward_count != 0)
		ss << '(' << current_message.origin_forward_count << ')';
	ss << '#';
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(stdout_handle, &csbi);
	cout << setiosflags(ios::right) << setw(80 - csbi.dwCursorPosition.X) << ss.str() << resetiosflags(ios::right);
	cout << "################################################################################";
}

void Application::print_time(Time &time) {
	SYSTEMTIME systemtime;
	GetLocalTime(&systemtime);
	SetConsoleTextAttribute(stdout_handle, FOREGROUND_INTENSITY);
	if (systemtime.wYear < time.year)
		cout << time.year << '-'
		<< time.month << '-'
		<< time.day << ' ';
	else {
		if (systemtime.wMonth == time.month &&
			systemtime.wDay == time.day)
			cout << "今天 ";
		else
			cout << time.month << "月"
			<< time.day << "日 ";
	}
	cout << setfill('0') << setiosflags(ios::right) << setw(2)
		<< time.hour << ':' << setw(2) << time.minute << ':' << setw(2) << time.second
		<< setfill(' ') << resetiosflags(ios::right);
	SetConsoleTextAttribute(stdout_handle, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
}

int Application::exec() {
#ifndef TEST
	while (true) {
		try {
			switch (state) {
			case MAIN_INTERFACE: {
				set_cursor_invisible();
				print_info();
				char ch;
				while (true) {
					ch = _getch();
					if ('1' <= ch  && ch <= '3')
						break;
				}
				system("cls");
				if (ch == '1')
					state = LOGIN;
				else if (ch == '2')
					state = REGIST;
				else if (ch == '3')
					return quit(0);
				break;
			}
			case LOGIN:
				login_work();
				break;
			case REGIST:
				regist_work();
				break;
			case MAIN_MENU:
				main_menu_work();
				break;
			case USER_CENTER:
				user_center_work();
				break;
			case USER_INFO_MODIFY:
				user_info_modify_work();
				break;
			case SEARCH_USER:
				search_user_work();
				break;
			case FOLLOWED_USER_LIST:
				followed_fans_user_list_work(true);
				break;
			case FANS_USER_LIST:
				followed_fans_user_list_work(false);
				break;
			case OTHER_USER_MAIN_MENU:
				other_user_main_menu_work();
				break;
			case SEND_MESSAGE:
				send_message_work();
				break;
			case MY_MESSAGE:
				my_message_work();
				break;
			case OTHER_USER_MESSAGE:
				other_user_message_work();
				break;
			case FORWARD_MESSAGE:
				forward_message_work();
				break;
			case FRESH_MESSAGE:
				fresh_message_work();
				break;
			case DELETE_MESSAGE:
				delete_message_work();
				break;
			default:
				break;
			}
		}
		catch (runtime_error &e) { system("cls"); }
	}
#else
#ifdef SPEED
	//先把已存在的文件删除
	userdb_manager.close_files();
	userdb_manager.recreate_files();

	//test1 注册用户
	//speed_test1();

	//test2 精确查找用户
	//speed_test2();

	//test3 关注、取消关注
	speed_test3();
#endif
#ifdef CORRECTNESS
	//先把已存在的文件删除
	userdb_manager.close_files();
	userdb_manager.recreate_files();

	//test1 注册用户以及精确查找用户
	//correct_test1();

	//test2 范围查找用户
	//correct_test2();

	//test3 关注、取消关注、移除粉丝以及取出关注/粉丝列表
	correct_test3();
#endif
#endif
	return quit(0);
}

#ifdef TEST
void Application::speed_test1() {
	User user;
	date birthday;
	birthday.year = 1994;
	birthday.month = 4;
	birthday.day = 1;
	user.initialize("", "password", "", true, birthday, 10086, "东川路800号");

	const int NUM = 1000000;
	User *users = new User[NUM];
	stringstream ss;
	DWORD start, end;
	start = GetTickCount();
	for (int i = 0; i < NUM; i++) {
		ss << "user" << i;
		user.username = user.name = ss.str();
		ss.str("");
		users[i] = user;
		userdb_manager.insert_user_record(users[i]);
		if (i % (NUM / 20) == 0) { //取大约20个点
			end = GetTickCount();
			cout << "注册" << i << "个用户耗时:" << end - start << "ms" << endl;
		}
	}
	end = GetTickCount();
	cout << "注册" << NUM << "个用户耗时:" << end - start << "ms" << endl;

	while (true) {}
}

void Application::speed_test2() {
	User user;
	date birthday;
	birthday.year = 1994;
	birthday.month = 4;
	birthday.day = 1;
	user.initialize("", "password", "", true, birthday, 10086, "东川路800号");

	const int NUM = 1000000;
	const int SEARCH_TIME = 1000;
	User *users = new User[NUM];
	stringstream ss;
	DWORD start, end;
	User user2;
	srand(unsigned(time(0)));
	for (int i = 0; i < NUM; i++) {
		ss << "user" << i;
		user.username = user.name = ss.str();
		ss.str("");
		users[i] = user;
		userdb_manager.insert_user_record(users[i]);
		if (i % (NUM / 20) == 0 && i != 0) { //取大约20个点
			start = GetTickCount();
			for (int j = 0; j < SEARCH_TIME; j++) {
				ss << "user" << ((rand() << 16) + rand()) % i;
				userdb_manager.find_user_record(ss.str(), user2);
				ss.str("");
			}
			end = GetTickCount();
			cout << "注册" << i << "个用户时，进行" << SEARCH_TIME << "次搜索共耗时:" << end - start << "ms" << endl;
		}
	}

	start = GetTickCount();
	for (int j = 0; j < SEARCH_TIME; j++) {
		ss << "user" << ((rand() << 16) + rand()) % NUM;
		userdb_manager.find_user_record(ss.str(), user2);
		ss.str("");
	}
	end = GetTickCount();
	cout << "注册" << NUM << "个用户时，进行" << SEARCH_TIME << "次搜索共耗时:" << end - start << "ms" << endl;

	start = GetTickCount();
	for (int i = 0; i < NUM; i++) {
		ss << "user" << ((rand() << 16) + rand()) % NUM;
		userdb_manager.find_user_record(ss.str(), user2);
		ss.str("");
		if (i % (NUM / 20) == 0) { //取大约20个点
			end = GetTickCount();
			cout << "注册" << NUM << "个用户时，进行" << i << "次搜索共耗时:" << end - start << "ms" << endl;
		}
	}
	end = GetTickCount();
	cout << "注册" << NUM << "个用户时，进行" << NUM << "次搜索共耗时:" << end - start << "ms" << endl;

	while (true) {}
}

void Application::speed_test3() {
	User user;
	date birthday;
	birthday.year = 1994;
	birthday.month = 4;
	birthday.day = 1;
	user.initialize("", "password", "", true, birthday, 10086, "东川路800号");

	const int NUM = 1000000;
	User *users = new User[NUM];
	stringstream ss;
	CURSOR_INIT;
	for (int i = 0; i < NUM; i++) {
		ss << "user" << i;
		user.username = user.name = ss.str();
		ss.str("");
		users[i] = user;
		userdb_manager.insert_user_record(users[i]);
		if (i % (NUM / 1000) == 0) {
			CURSOR_RETRY;
			cout << "注册用户:" << i / double(NUM / 100) << "%";
		}
	}

	srand(unsigned(time(0)));
	int tmp1, tmp2;
	set<pair<int, int>> nums;
	for (int i = 0; i < NUM; i++) {
		tmp1 = ((rand() << 16) + rand()) % NUM;
		tmp2 = ((rand() << 16) + rand()) % NUM;
		while (nums.find(make_pair(tmp1, tmp2)) != nums.end()) {
			tmp1 = ((rand() << 16) + rand()) % NUM;
			tmp2 = ((rand() << 16) + rand()) % NUM;
		}
		nums.insert(make_pair(tmp1, tmp2));
	}

	DWORD start, end;
	start = GetTickCount();
	auto iter = nums.begin();
	for (int i = 0; i < NUM; i++) {
		userdb_manager.insert_user_relation(users[iter->first], users[iter->second]);
		iter++;
		if (i % (NUM / 20) == 0) { //取大约20个点
			end = GetTickCount();
			cout << "关注" << i << "个用户耗时:" << end - start << "ms" << endl;
		}
	}
	end = GetTickCount();
	cout << "关注" << NUM << "个用户耗时:" << end - start << "ms" << endl;

	while (true) {}
}

void Application::correct_test1() {
	User user;
	date birthday;
	birthday.year = 1994;
	birthday.month = 4;
	birthday.day = 1;
	user.initialize("", "password", "user", true, birthday, 10086, "东川路800号");

	srand(unsigned(time(0)));
	const int NUM = 1000000;
	User *users = new User[NUM];
	stringstream ss;
	set<int> nums;
	int tmp;
	CURSOR_INIT;
	for (int i = 0; i < NUM; i++) {
		tmp = (rand() << 16) + rand();
		while (nums.find(tmp) != nums.end())
			tmp = (rand() << 16) + rand();
		ss << tmp;
		nums.insert(tmp);
		user.username = ss.str();
		ss.str("");
		users[i] = user;
		userdb_manager.insert_user_record(users[i]);
		if (i % (NUM / 1000) == 0) {
			CURSOR_RETRY;
			cout << "注册用户:" << i / double(NUM / 100) << "%";
		}
	}

	User user2;
	bool correct = true;
	for (int i = 0; i < NUM; i++) {
		userdb_manager.find_user_record(users[i].username, user2);
		if (users[i] != user2) {
			correct = false;
			break;
		}
		if (i % (NUM / 1000) == 0) {
			CURSOR_RETRY;
			cout << "检查用户:" << i / double(NUM / 100) << "%";
		}
	}
	if (correct)
		cout << "测试成功，注册功能正确无误！";
	else
		cout << "测试失败，注册功能有误，B树的插入或查找有问题！";

	while (true) {}
}

void Application::correct_test2() {
	User user;
	date birthday;
	birthday.year = 1994;
	birthday.month = 4;
	birthday.day = 1;
	user.initialize("", "password", "user", true, birthday, 10086, "东川路800号");

	srand(unsigned(time(0)));
	const int NUM1 = 900000;
	User *users1 = new User[NUM1];
	stringstream ss;
	set<int> nums1;
	int tmp;
	CURSOR_INIT;
	for (int i = 0; i < NUM1; i++) {
		tmp = (rand() << 16) + rand();
		while (nums1.find(tmp) != nums1.end())
			tmp = (rand() << 16) + rand();
		ss << tmp;
		nums1.insert(tmp);
		user.username = ss.str();
		ss.str("");
		users1[i] = user;
		userdb_manager.insert_user_record(users1[i]);
		if (i % (NUM1 / 1000) == 0) {
			CURSOR_RETRY;
			cout << "注册用户组1:" << i / double(NUM1 / 100) << "%";
		}
	}

	const int NUM2 = 100000;
	User *users2 = new User[NUM2];
	set<int> nums2;
	for (int i = 0; i < NUM2; i++) {
		tmp = (rand() << 16) + rand();
		while (nums2.find(tmp) != nums2.end())
			tmp = (rand() << 16) + rand();
		ss << "user" << tmp;
		nums2.insert(tmp);
		user.username = ss.str();
		ss.str("");
		users2[i] = user;
		userdb_manager.insert_user_record(users2[i]);
		if (i % (NUM2 / 1000) == 0) {
			CURSOR_RETRY;
			cout << "注册用户组2:" << i / double(NUM2 / 100) << "%";
		}
	}
	sort(users2, users2 + NUM2);
	
	Query query;
	query.username = make_pair("user", true);
	query.phonenum = make_pair(0, false);
	query.name = make_pair("", false);
	query.ismale = make_pair(false, false);
	query.hometown = make_pair("", false);
	query.birthday_start = make_pair(date(), false);
	query.birthday_end = make_pair(date(), false);
	string a = "aaaaaaaa";
	bool correct = true;
	auto result = userdb_manager.find_regioned_user(query, a); //第二个参数用于保证不查找到当前用户
	if (result.size() != NUM2) {
		cout << "测试失败，模糊查找功能有误，B树的范围查找有问题！";
		while (true) {}
	}
	auto iter = result.begin();
	for (int i = 0; i < NUM2; i++) {
		if (*iter != users2[i]) {
			correct = false;
			break;
		}
		iter++;
		if (i % (NUM2 / 1000) == 0) {
			CURSOR_RETRY;
			cout << "检查用户:" << i / double(NUM2 / 100) << "%";
		}
	}
	if (correct)
		cout << "测试成功，模糊查找功能正确无误！";
	else
		cout << "测试失败，模糊查找功能有误，B树的范围查找有问题！";

	while (true) {}
}

void Application::correct_test3() {
	User user;
	date birthday;
	birthday.year = 1994;
	birthday.month = 4;
	birthday.day = 1;
	user.initialize("", "password", "", true, birthday, 10086, "东川路800号");

	const int NUM = 10000; //由于删除关注图中的边是线性复杂度，设置100万将无法跑完测试
	User *users = new User[NUM];
	stringstream ss;
	CURSOR_INIT;
	for (int i = 0; i < NUM; i++) {
		ss << "user" << i;
		user.username = user.name = ss.str();
		ss.str("");
		users[i] = user;
		userdb_manager.insert_user_record(users[i]);
		if (i % (NUM / 1000) == 0) {
			CURSOR_RETRY;
			cout << "注册用户:" << i / double(NUM / 100) << "%";
		}
	}

	srand(unsigned(time(0)));
	int user_start = ((rand() << 16) + rand()) % NUM;
	for (int i = 0; i < NUM; i++) {
		if (i != user_start)
			userdb_manager.insert_user_relation(users[user_start], users[i]);
		if (i % (NUM / 1000) == 0) {
			CURSOR_RETRY;
			cout << "关注用户:" << i / double(NUM / 100) << "%";
		}
	}

	auto result = userdb_manager.get_followed(users[user_start]);
	if (result.size() != NUM - 1) {
		cout << "测试失败，关注图的插入或关注链表的读取有问题！";
		while (true) {}
	}
	bool correct = true;
	for (int i = 0; i < NUM; i++) {
		if (i == user_start)
			continue;
		if (result.find(users[i].username) == result.end()) {
			correct = false;
			break;
		}
		if (i % (NUM / 1000) == 0) {
			CURSOR_RETRY;
			cout << "检查用户关注列表:" << i / double(NUM / 100) << "%";
		}
	}

	if (!correct) {
		cout << "测试失败，关注图的插入或关注链表的读取有问题！";
		while (true) {}
	}
		
	for (int i = 0; i < NUM; i++) {
		if (i != user_start)
			userdb_manager.delete_user_relation(users[user_start], users[i]);
		if (i % (NUM / 1000) == 0) {
			CURSOR_RETRY;
			cout << "取消关注用户:" << i / double(NUM / 100) << "%";
		}
	}

	result = userdb_manager.get_followed(users[user_start]);
	if (!result.empty()) {
		cout << "测试失败，关注图的删除或关注链表的读取有问题！";
		while (true) {}
	}

	int user_end = ((rand() << 16) + rand()) % NUM;
	for (int i = 0; i < NUM; i++) {
		if (i != user_end)
			userdb_manager.insert_user_relation(users[i], users[user_end]);
		if (i % (NUM / 1000) == 0) {
			CURSOR_RETRY;
			cout << "关注用户:" << i / double(NUM / 100) << "%";
		}
	}

	result = userdb_manager.get_fans(users[user_end]);
	if (result.size() != NUM - 1) {
		cout << "测试失败，关注图的插入或粉丝链表的读取有问题！";
		while (true) {}
	}
	for (int i = 0; i < NUM; i++) {
		if (i == user_end)
			continue;
		if (result.find(users[i].username) == result.end()) {
			correct = false;
			break;
		}
		if (i % (NUM / 1000) == 0) {
			CURSOR_RETRY;
			cout << "检查用户粉丝列表:" << i / double(NUM / 100) << "%";
		}
	}

	if (!correct) {
		cout << "测试失败，关注图的插入或粉丝链表的读取有问题！";
		while (true) {}
	}

	for (int i = 0; i < NUM; i++) {
		if (i != user_end)
			userdb_manager.delete_user_relation(users[i], users[user_end]);
		if (i % (NUM / 1000) == 0) {
			CURSOR_RETRY;
			cout << "取消关注用户:" << i / double(NUM / 100) << "%";
		}
	}

	result = userdb_manager.get_fans(users[user_end]);
	if (!result.empty()) {
		cout << "测试失败，关注图的删除或粉丝链表的读取有问题！";
		while (true) {}
	}

	cout << "测试成功，关注、取消关注、移除粉丝以及取出关注/粉丝列表的功能都正确无误";

	while (true) {}
}
#endif