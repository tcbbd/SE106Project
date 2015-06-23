#pragma once

#include <iostream>
#include <Windows.h>

#include "user.h"
#include "user_database_manager.h"
#include "set_visitor.h"
#include "message_editor.h"

using namespace std;

//#define TEST
//#define CORRECTNESS
//#define SPEED

//用于处理前台业务逻辑的类，通过状态转移驱动，每个状态有对应的处理方法
class Application
{
public:
	friend class MessageEditor;
	static Application* getInstance();
	int exec();
private:
	Application();
	static BOOL CtrlHandler(DWORD fdwCtrlType);
	static basic_istream<char, char_traits<char>>&
		getline(basic_istream<char, char_traits<char>>& _Istr,
		basic_string<char, char_traits<char>, allocator<char>>& _Str);
	static KEY_EVENT_RECORD _getkeyevent();

	void print_user_info();
	void print_hello_info(string info);
	void print_info();
	int quit(int i);

	void get_username(string &username);
	void get_password_help(string &password); //help routine
	void get_password(string &password, string info);
	void get_name(string &name);
	bool get_gender();
	void get_date(date &date);
	long long get_phonenum();
	void get_hometown(string &hometown);
	bool get_bool(string info);

	void list_user_set(SetVisitor &user_set);
	int list_user_message(bool fresh_message);

	void print_message(bool fresh_message);
	void print_time(Time &time);
	void print_origin_message();

	void regist_work();
	void login_work();
	void main_menu_work();
	void user_center_work();
	void user_info_modify_work();
	void search_user_work();
	void followed_fans_user_list_work(bool followed);
	void other_user_main_menu_work();
	void send_message_work();
	void my_message_work();
	void other_user_message_work();
	void forward_message_work();
	void fresh_message_work();
	void delete_message_work();

	enum { MAIN_INTERFACE, LOGIN, REGIST, MAIN_MENU, USER_CENTER,
		USER_INFO_MODIFY, SEARCH_USER, FOLLOWED_USER_LIST,
		FANS_USER_LIST, OTHER_USER_MAIN_MENU, SEND_MESSAGE,
		MY_MESSAGE, OTHER_USER_MESSAGE, FORWARD_MESSAGE,
		FRESH_MESSAGE, DELETE_MESSAGE}
	state, pre_current_state_buffer, pre_other_user_state_buffer,
		pre_forward_delete_message_state_buffer;
	bool other_user_return_to_search;
	bool return_from_message_list;

	User current_user;
	User other_user;
	set<string> current_followed;
	set<string> current_fans;
	set<User> search_result;
	Message current_message;
	UserDatabaseManager userdb_manager;

	static void set_cursor_invisible();
	static void set_cursor_visible();

	static HANDLE stdout_handle;
	static HANDLE stdin_handle;
	static Application* pInstance;

#ifdef TEST
	void speed_test1();
	void speed_test2();
	void speed_test3();

	void correct_test1();
	void correct_test2();
	void correct_test3();
#endif
};