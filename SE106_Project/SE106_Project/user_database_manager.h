#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <io.h>
#include <set>
#include <map>
#include <queue>
#include <functional>
#include <Windows.h>

#include "user.h"
#include "btree_node.h"

using namespace std;

//采用B+树作为索引
//索引与用户基本信息分为两个文件存储
//这样能使B+树每个节点内能容纳的关键字数量相同，且数量较大
//（如果合并成一个文件，用B树表示，则每个节点能容纳的关键字数量太少
//                  用B+树表示，则叶子节点能容纳的关键字数量太少）
//关注关系使用双向十字链表表示

static const int EDGE_LENGTH = 64;
static const int OUT_LIST_PRE = 32;
static const int OUT_LIST_NEXT = 40;
static const int IN_LIST_PRE = 48;
static const int IN_LIST_NEXT = 56;

static const int MESSAGE_LENGTH = 370;
static const int MESSAGE_TIME = 280;
static const int MESSAGE_AUTHOR = 294;
static const int ORIGIN_MESSAGE = 310;
static const int PARENT_MESSAGE = 318;
static const int CHILD_MESSAGE = 326;
static const int PREVIOUS_SIBLING = 334;
static const int NEXT_SIBLING = 342;
static const int PREVIOUS_MESSAGE = 350;
static const int NEXT_MESSAGE = 358;
static const int MESSAGE_FORWARD_COUNT = 366;

struct Time {
	Time() = default;
	Time(SYSTEMTIME &time);
	WORD year;
	WORD month;
	WORD day;
	WORD hour;
	WORD minute;
	WORD second;
	WORD milliseconds;
	void set_time(char *time);
	void get_time(char *time);
	bool operator <(const Time& other) const;
};

struct Message {
	wstring message;
	Time time;
	string author;
	int forward_count;
	streamoff offset;
	wstring origin_message;
	Time origin_time;
	string origin_author;
	int origin_forward_count;
	bool operator <(const Message &other) const;
	bool operator >(const Message &other) const;
};

struct Position {
	streamoff pre;
	streamoff current;
	streamoff next;
};

class UserDatabaseManager
{
public:
	UserDatabaseManager();
	void close_files();
	void recreate_files();
	void insert_user_record(User &user);
	int modify_user_record(User &user);
	int find_user_record(string &username, User &user);
	set<User> find_regioned_user(Query &query, string &current_username);

	void insert_user_relation(User &start, User &end);
	int delete_user_relation(User &start, User &end);
	void delete_user(User &user);
	set<string> get_followed(User &user);
	set<string> get_fans(User &user);

	void send_message(User &user, streamoff forwarded_msg, wstring &message, Time &time);
	void delete_message(User &user, Message &message);
	void user_message_list_init(User &user);
	int get_next_user_message(Message &message);
	int get_pre_user_message(Message &message);
	void fresh_message_init(User &user, set<string> &followed);
	int get_next_fresh_message(Message &message);
	int get_pre_fresh_message(Message &message);
private:
	void create_index_file();
	void create_data_file();
	void create_relation_file();
	void create_message_file();
	void recreate_files_ask(int opened_files);

	int insert_user_index(string &username, streamoff pointer);
	pair<streamoff, bool> find_user_index(string &username);
	void get_message(Message &message, char(&msg)[MESSAGE_LENGTH], streamoff offset);

	void insert_into_freelist(fstream &fs, streamoff offset);
	void delete_pos_node_usermsglist(Position &pos, streamoff prev, streamoff next);
	void delete_message_node(User &user, char(&msg)[MESSAGE_LENGTH], streamoff offset, bool delete_from_tree);
	void decrease_ancestor_forward_count(char(&msg)[MESSAGE_LENGTH], int decrease_count);

	bool using_message_list;
	bool fresh_message_forward;

	Position user_message_list_pos;
	priority_queue<Message, vector<Message>, less<Message>> fresh_message_buffer_forward;
	priority_queue<Message, vector<Message>, greater<Message>> fresh_message_buffer_backward;
	map<string, Position> fresh_message_position;

	fstream user_index;
	fstream user_data;
	fstream user_relation;
	fstream user_message;
};