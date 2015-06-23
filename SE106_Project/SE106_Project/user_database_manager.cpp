#include "stdafx.h"
#include "user_database_manager.h"
#include <Windows.h>
#include <stack>

UserDatabaseManager::UserDatabaseManager() {
	int access = _access("user.idx", 0) + _access("user.dat", 0) +
		_access("user.rel", 0) + _access("user.msg", 0);
	if (access == -4) { //文件都不存在，创建文件
		recreate_files();
		return;
	}
	if (access < 0) { //存在一部分文件，属于错误
		recreate_files_ask(0);
		return;
	}

	//文件都存在，只做最基本的签名与长度的检查
	//user index
	user_index.open("user.idx", ios::in | ios::out | ios::binary);

	char head[9] = {};
	user_index.read(head, 8);
	if (!user_index || strcmp(head, "\x89IDX\xd\xa\x1a\xa") != 0) {
		recreate_files_ask(1);
		return;
	}
	else {
		user_index.seekg(0, ios::end);
		auto length = user_index.tellg();
		if ((length - streamoff(8)) % BLOCK_SIZE != 0) {
			recreate_files_ask(1);
			return;
		}
	}

	//user data
	user_data.open("user.dat", ios::in | ios::out | ios::binary);

	user_data.read(head, 8);
	if (!user_data || strcmp(head, "\x89""DAT\xd\xa\x1a\xa") != 0) {
		recreate_files_ask(2);
		return;
	}
	else {
		user_data.seekg(0, ios::end);
		auto length = user_data.tellg();
		if ((length - streamoff(8)) % RECORD_LENGTH != 0) {
			recreate_files_ask(2);
			return;
		}
	}

	//user relation
	user_relation.open("user.rel", ios::in | ios::out | ios::binary);

	user_relation.read(head, 8);
	if (!user_relation || strcmp(head, "\x89REL\xd\xa\x1a\xa") != 0) { //签名错误
		recreate_files_ask(3);
		return;
	}
	else {
		user_relation.seekg(0, ios::end);
		auto length = user_relation.tellg();
		if ((length - streamoff(16)) % EDGE_LENGTH != 0) {
			recreate_files_ask(3);
			return;
		}
	}

	//user message
	user_message.open("user.msg", ios::in | ios::out | ios::binary);

	user_message.read(head, 8);
	if (!user_message || strcmp(head, "\x89MSG\xd\xa\x1a\xa") != 0) { //签名错误
		recreate_files_ask(4);
		return;
	}
	else {
		user_message.seekg(0, ios::end);
		auto length = user_message.tellg();
		if ((length - streamoff(16)) % MESSAGE_LENGTH != 0) {
			recreate_files_ask(4);
			return;
		}
	}
}

void UserDatabaseManager::close_files() {
	user_index.close();
	user_data.close();
	user_relation.close();
	user_message.close();
}

void UserDatabaseManager::recreate_files() {
	DeleteFileA("user.idx");
	DeleteFileA("user.dat");
	DeleteFileA("user.rel");
	DeleteFileA("user.msg");
	create_index_file();
	create_data_file();
	create_relation_file();
	create_message_file();
	user_index.open("user.idx", ios::in | ios::out | ios::binary);
	user_data.open("user.dat", ios::in | ios::out | ios::binary);
	user_relation.open("user.rel", ios::in | ios::out | ios::binary);
	user_message.open("user.msg", ios::in | ios::out | ios::binary);
}

void UserDatabaseManager::recreate_files_ask(int opened_files) {
	if (opened_files > 0)
		user_index.close();
	if (opened_files > 1)
		user_data.close();
	if (opened_files > 2)
		user_relation.close();
	if (opened_files > 3)
		user_message.close();
	int msgboxID = MessageBoxA(NULL,
		"数据库已受损，无法正常读取。您可以清空整个数据库，以恢复本应用正常运作，或者联系作者为您修复，是否清空？",
		"数据库受损！", MB_YESNO | MB_ICONWARNING);
	switch (msgboxID) {
	case IDYES:
		recreate_files();
		break;
	case IDNO:
		exit(0);
		break;
	default:
		break;
	}
}

void UserDatabaseManager::create_message_file() {
	ofstream fs("user.msg", ios::out | ios::binary);
	fs.write("\x89MSG\xd\xa\x1a\xa", 8); //文件头签名模仿PNG格式
	fs.write("\0\0\0\0\0\0\0\0", 8); //空闲链表头
	fs.close();
}

void UserDatabaseManager::create_relation_file() {
	ofstream fs("user.rel", ios::out | ios::binary);
	fs.write("\x89REL\xd\xa\x1a\xa", 8); //文件头签名模仿PNG格式
	fs.write("\0\0\0\0\0\0\0\0", 8); //空闲链表头
	fs.close();
}

void UserDatabaseManager::create_data_file() {
	ofstream fs("user.dat", ios::out | ios::binary);
	fs.write("\x89""DAT\xd\xa\x1a\xa", 8); //文件头签名模仿PNG格式
	fs.close();
}

void UserDatabaseManager::create_index_file() {
	char padding[BLOCK_SIZE] = {};
	padding[0] = '\1'; //Leaf Node
	ofstream fs("user.idx", ios::out | ios::binary);
	fs.write("\x89IDX\xd\xa\x1a\xa", 8); //文件头签名模仿PNG格式
	fs.write(padding, BLOCK_SIZE);
	fs.close();
}

//return 0 if success, else return -1
int UserDatabaseManager::insert_user_index(string &username, streamoff pointer) {
	char node_buf[BLOCK_SIZE];
	char node_split_buf[BLOCK_SIZE] = {};
	BTreeNode node(node_buf, node_split_buf);
	stack<pair<streamoff, int>> path;
	streamoff current = 8;
	while (true) { //找到叶子节点
		user_index.seekg(current, ios::beg);
		user_index.read(node_buf, BLOCK_SIZE);
		if (node.is_internal()) {
			auto tmp = node.internal_find(username);
			path.push(make_pair(current, tmp.second));
			current = tmp.first;
		}
		else
			break;
	}
	char split_key[16];
	auto ret = node.leaf_insert(username, pointer, split_key);
	if (ret == BTreeNode::FAILED) //已存在该用户名
		return -1;
	if (ret == BTreeNode::SUCCESS) { //未分裂，直接返回
		user_index.seekp(current);
		user_index.write(node_buf, BLOCK_SIZE);
		user_index.flush(); //保证数据写入了磁盘文件
		return 0;
	}
	if (ret == BTreeNode::SPLITTED) { //已分裂
		//写入分裂后的两个节点
		user_index.seekp(0, ios::end);
		streamoff pos = user_index.tellp();
		user_index.write(node_split_buf, BLOCK_SIZE);
		node.set_sibling(pos);
		user_index.seekp(current, ios::beg);
		user_index.write(node_buf, BLOCK_SIZE);

		//递归地向内部节点插入
		while (!path.empty()) {
			auto tmp = path.top();
			path.pop();
			user_index.seekg(tmp.first, ios::beg);
			user_index.read(node_buf, BLOCK_SIZE);
			ret = node.internal_insert(split_key, pos, tmp.second);
			user_index.seekp(tmp.first, ios::beg);
			user_index.write(node_buf, BLOCK_SIZE);
			if (ret == BTreeNode::SUCCESS) { //未分裂，插入结束
				user_index.flush(); //保证数据写入了磁盘文件
				return 0;
			}
			else { //已分裂，继续插入
				user_index.seekp(0, ios::end);
				pos = user_index.tellp();
				user_index.write(node_split_buf, BLOCK_SIZE);
			}
		}

		//根节点分裂
		user_index.seekp(0, ios::end);
		streamoff pos_left = user_index.tellp();
		user_index.write(node_buf, BLOCK_SIZE); //move left child to end of file
		node.make_root(split_key, pos_left, pos);
		user_index.seekp(8, ios::beg);
		user_index.write(node_buf, BLOCK_SIZE); //new root node
		user_index.flush(); //保证数据写入了磁盘文件
		return 0;
	}
	return -1;
}

void UserDatabaseManager::insert_user_record(User &user) {
	user_data.seekp(0, ios::end);
	auto pos = user_data.tellp();
	insert_user_index(user.username, pos);
	user_data.write(user.get_record().c_str(), RECORD_LENGTH);
	user_data.flush(); //保证数据写入了磁盘文件
}

pair<streamoff, bool> UserDatabaseManager::find_user_index(string &username) {
	char node_buf[BLOCK_SIZE];
	BTreeNode node(node_buf, node_buf);
	streamoff current = 8;
	while (true) { //找到叶子节点
		user_index.seekg(current, ios::beg);
		user_index.read(node_buf, BLOCK_SIZE);
		if (node.is_internal()) {
			auto tmp = node.internal_find(username);
			current = tmp.first;
		}
		else
			break;
	}
	return node.leaf_find(username);
}

//return 0 if success, else return -1
int UserDatabaseManager::modify_user_record(User &user) {
	auto ret = find_user_index(user.username);
	if (!ret.second)
		return -1;
	string record = user.get_record();
	user_data.seekp(ret.first, ios::beg);
	user_data.write(record.c_str(), RECORD_LENGTH);
	user_data.flush(); //保证数据写入了磁盘文件
	return 0;
}

//return 0 if success, return -1 if didn't find, return -2 if deleted
int UserDatabaseManager::find_user_record(string &username, User &user) {
	auto ret = find_user_index(username);
	if (!ret.second)
		return -1;
	char record[RECORD_LENGTH];
	user_data.seekg(ret.first, ios::beg);
	user_data.read(record, RECORD_LENGTH);
	if (record[78] != '\0') //已删除
		return -2;
	user.initialize(record);
	return 0;
}

set<User> UserDatabaseManager::find_regioned_user(Query &query, string &current_username) {
	char record[RECORD_LENGTH];
	User user;
	set<User> ret;
	if (query.username.second == true) { //使用对用户名的部分匹配搜索，这会导致使用B树进行范围查找
		char node_buf[BLOCK_SIZE];
		BTreeNode node(node_buf, node_buf);
		streamoff current = 8;
		string &start = query.username.first;
		string end = start;
		end.append(16 - start.size(), '\xff');
		while (true) { //找到叶子节点
			user_index.seekg(current, ios::beg);
			user_index.read(node_buf, BLOCK_SIZE);
			if (node.is_internal()) {
				auto tmp = node.internal_find(start);
				current = tmp.first;
			}
			else
				break;
		}
		tuple<vector<streamoff>, streamoff, bool> result;
		while (true) {
			result = node.leaf_region_find(start, end);
			for (streamoff off : get<0>(result)) {
				user_data.seekg(off, ios::beg);
				user_data.read(record, RECORD_LENGTH);
				user.initialize(record);
				if (user.fits(query, current_username))
					ret.insert(user);
			}
			if (get<2>(result)) {
				user_index.seekg(get<1>(result), ios::beg);
				user_index.read(node_buf, BLOCK_SIZE);
			}
			else
				break;
		}
	}
	else { //否则将对整个文件进行顺序查找
		user_data.seekg(8, ios::beg);
		while (user_data) {
			user_data.read(record, RECORD_LENGTH);
			user.initialize(record);
			if (user.fits(query, current_username))
				ret.insert(user);
		}
		user_data.clear();
	}
	return ret;
}

//假设start->end这条边还不存在
void UserDatabaseManager::insert_user_relation(User &start, User &end) {
	char edge[EDGE_LENGTH] = {};
	for (size_t i = 0; i < start.username.size(); i++)
		edge[i] = start.username[i];
	for (size_t i = 0; i < end.username.size(); i++)
		edge[i + 16] = end.username[i];
	//OutListPre = NULL
	memcpy(edge + OUT_LIST_NEXT, &start.out_edges, 8);
	//InListPre = NULL
	memcpy(edge + IN_LIST_NEXT, &end.in_edges, 8);

	streamoff freelist;
	user_relation.seekg(8, ios::beg);
	user_relation.read((char *)&freelist, 8);

	streamoff newnode;
	if (freelist == NULL) {
		user_relation.seekp(0, ios::end);
		newnode = user_relation.tellp();
		user_relation.write(edge, EDGE_LENGTH);
	}
	else {
		newnode = freelist;
		//delete node from free list
		user_relation.seekg(newnode, ios::beg);
		user_relation.read((char *)&freelist, 8);
		user_relation.seekp(8, ios::beg);
		user_relation.write((char *)&freelist, 8);

		user_relation.seekp(newnode, ios::beg);
		user_relation.write(edge, EDGE_LENGTH);
	}

	if (start.out_edges != NULL) {
		user_relation.seekp(start.out_edges + OUT_LIST_PRE, ios::beg);
		user_relation.write((char *)&newnode, 8);
	}
	if (end.in_edges != NULL) {
		user_relation.seekp(end.in_edges + IN_LIST_PRE, ios::beg);
		user_relation.write((char *)&newnode, 8);
	}

	user_relation.flush(); //保证数据写入了磁盘文件
	start.out_edges = newnode;
	end.in_edges = newnode;
	start.out_count++;
	end.in_count++;
	modify_user_record(start);
	modify_user_record(end);
}

//return 0 if success, else return -1
int UserDatabaseManager::delete_user_relation(User &start, User &end) {
	char edge[EDGE_LENGTH];
	string username;
	streamoff next_node = start.out_edges;

	while (true) {
		if (next_node == NULL)
			return -1;
		user_relation.seekg(next_node, ios::beg);
		user_relation.read(edge, EDGE_LENGTH);
		for (int i = 16; i < 32; i++)
			if (edge[i] == '\0')
				break;
			else
				username.push_back(edge[i]);
		if (username != end.username) {
			memcpy(&next_node, edge + OUT_LIST_NEXT, 8);
			username.clear();
		}
		else {
			insert_into_freelist(user_relation, next_node);

			//delete node from out list
			streamoff pre, next;
			memcpy(&pre, edge + OUT_LIST_PRE, 8);
			memcpy(&next, edge + OUT_LIST_NEXT, 8);
			if (pre != NULL) {
				user_relation.seekp(pre + OUT_LIST_NEXT, ios::beg);
				user_relation.write((char *)&next, 8);
			}
			else
				start.out_edges = next;
			if (next != NULL) {
				user_relation.seekp(next + OUT_LIST_PRE, ios::beg);
				user_relation.write((char *)&pre, 8);
			}

			//delete node from in list
			memcpy(&pre, edge + IN_LIST_PRE, 8);
			memcpy(&next, edge + IN_LIST_NEXT, 8);
			if (pre != NULL) {
				user_relation.seekp(pre + IN_LIST_NEXT, ios::beg);
				user_relation.write((char *)&next, 8);
			}
			else
				end.in_edges = next;
			if (next != NULL) {
				user_relation.seekp(next + IN_LIST_PRE, ios::beg);
				user_relation.write((char *)&pre, 8);
			}

			start.out_count--;
			end.in_count--;
			modify_user_record(start);
			modify_user_record(end);
			user_relation.flush(); //保证数据写入了磁盘文件
			return 0;
		}
	}
}

void UserDatabaseManager::delete_user(User &user) {
	user.isdeleted = true;
	modify_user_record(user);
	char edge[EDGE_LENGTH];
	User tmp_user;
	//delete out list
	streamoff next_node = user.out_edges;
	streamoff pre, next;
	string end_username;
	while (true) {
		if (next_node == NULL)
			break;
		//delete node from in list
		user_relation.seekg(next_node, ios::beg);
		user_relation.read(edge, EDGE_LENGTH);
		memcpy(&pre, edge + IN_LIST_PRE, 8);
		memcpy(&next, edge + IN_LIST_NEXT, 8);

		end_username.clear();
		for (int i = 16; i < 32; i++)
			if (edge[i] == '\0')
				break;
			else
				end_username.push_back(edge[i]);
		find_user_record(end_username, tmp_user);
		tmp_user.in_count--;
		
		if (pre != NULL) {
			user_relation.seekp(pre + IN_LIST_NEXT, ios::beg);
			user_relation.write((char *)&next, 8);
		}
		else
			tmp_user.in_edges = next;
		modify_user_record(tmp_user);
		if (next != NULL) {
			user_relation.seekp(next + IN_LIST_PRE, ios::beg);
			user_relation.write((char *)&pre, 8);
		}

		insert_into_freelist(user_relation, next_node);
		memcpy(&next_node, edge + OUT_LIST_NEXT, 8);
	}

	//delete in list
	next_node = user.in_edges;
	string start_username;
	while (true) {
		if (next_node == NULL)
			break;
		//delete node from out list
		user_relation.seekg(next_node, ios::beg);
		user_relation.read(edge, EDGE_LENGTH);
		memcpy(&pre, edge + OUT_LIST_PRE, 8);
		memcpy(&next, edge + OUT_LIST_NEXT, 8);

		start_username.clear();
		for (int i = 0; i < 16; i++)
			if (edge[i] == '\0')
				break;
			else
				start_username.push_back(edge[i]);
		find_user_record(start_username, tmp_user);
		tmp_user.out_count--;

		if (pre != NULL) {
			user_relation.seekp(pre + OUT_LIST_NEXT, ios::beg);
			user_relation.write((char *)&next, 8);
		}
		else
			tmp_user.out_edges = next;
		modify_user_record(tmp_user);
		if (next != NULL) {
			user_relation.seekp(next + OUT_LIST_PRE, ios::beg);
			user_relation.write((char *)&pre, 8);
		}

		insert_into_freelist(user_relation, next_node);
		memcpy(&next_node, edge + IN_LIST_NEXT, 8);
	}

	char msg[MESSAGE_LENGTH];
	char descendant_msg[MESSAGE_LENGTH];
	streamoff child, sibling, current_node;
	stack<streamoff> nodes;
	string author;
	int decreased_forward_count = 1;
	while (user.message != NULL) {
		user_message.seekg(user.message, ios::beg);
		user_message.read(msg, MESSAGE_LENGTH);
		delete_message_node(user, msg, user.message, true);

		//清除所有由此消息转发的消息
		memcpy(&child, msg + CHILD_MESSAGE, 8);
		if (child != NULL)
			nodes.push(child);
		while (!nodes.empty()) {
			current_node = nodes.top();
			nodes.pop();
			decreased_forward_count++;

			user_message.seekg(current_node, ios::beg);
			user_message.read(descendant_msg, MESSAGE_LENGTH);
			memcpy(&child, descendant_msg + CHILD_MESSAGE, 8);
			memcpy(&sibling, descendant_msg + NEXT_SIBLING, 8);
			if (child != NULL)
				nodes.push(child);
			if (sibling != NULL)
				nodes.push(sibling);

			author.clear();
			for (int i = MESSAGE_AUTHOR; i < MESSAGE_AUTHOR + 16; i++)
				if (descendant_msg[i] == '\0')
					break;
				else
					author.push_back(descendant_msg[i]);
			find_user_record(author, tmp_user);
			delete_message_node(tmp_user, descendant_msg, current_node, false);
		}

		decrease_ancestor_forward_count(msg, decreased_forward_count);
		find_user_record(user.username, user); //重新载入user，以防止删除转发过程中删除了自己转发自己的消息而使消息数不正确
	}

	user_relation.flush(); //保证数据写入了磁盘文件
	user_message.flush(); //保证数据写入了磁盘文件
}

set<string> UserDatabaseManager::get_followed(User &user) {
	char edge[EDGE_LENGTH];
	streamoff next = user.out_edges;
	string username;
	set<string> list;
	while (next != NULL) {
		user_relation.seekg(next, ios::beg);
		user_relation.read(edge, EDGE_LENGTH);
		for (int i = 16; i < 32; i++)
			if (edge[i] == '\0')
				break;
			else
				username.push_back(edge[i]);
		list.insert(username);
		username.clear();
		memcpy(&next, edge + OUT_LIST_NEXT, 8);
	}
	return list;
}

set<string> UserDatabaseManager::get_fans(User &user) {
	char edge[EDGE_LENGTH];
	streamoff next = user.in_edges;
	string username;
	set<string> list;
	while (next != NULL) {
		user_relation.seekg(next, ios::beg);
		user_relation.read(edge, EDGE_LENGTH);
		for (int i = 0; i < 16; i++)
			if (edge[i] == '\0')
				break;
			else
				username.push_back(edge[i]);
		list.insert(username);
		username.clear();
		memcpy(&next, edge + IN_LIST_NEXT, 8);
	}
	return list;
}

void UserDatabaseManager::send_message(User &user, streamoff forwarded_msg, wstring &message, Time &time) {
	char msg[MESSAGE_LENGTH] = {};
	wchar_t *msg_content = (wchar_t *)msg;
	for (size_t i = 0; i < message.size(); i++)
		msg_content[i] = message[i];
	time.set_time(msg + MESSAGE_TIME);
	for (size_t i = 0; i < user.username.size(); i++)
		msg[MESSAGE_AUTHOR + i] = user.username[i];
	//message list pre = NULL
	memcpy(msg + NEXT_MESSAGE, &user.message, 8);

	char parent_msg[MESSAGE_LENGTH];
	char ancestor_msg[MESSAGE_LENGTH];
	streamoff parent;
	streamoff sibling;
	int forward_count;
	if (forwarded_msg != NULL) { //转发
		user_message.seekg(forwarded_msg, ios::beg);
		user_message.read(parent_msg, MESSAGE_LENGTH);
		memcpy(&parent, parent_msg + PARENT_MESSAGE, 8);
		if (parent != NULL)
			memcpy(msg + ORIGIN_MESSAGE, parent_msg + ORIGIN_MESSAGE, 8);
		else
			memcpy(msg + ORIGIN_MESSAGE, &forwarded_msg, 8);

		while (parent != NULL) { //将所有祖先的转发计数+1
			user_message.seekg(parent, ios::beg);
			user_message.read(ancestor_msg, MESSAGE_LENGTH);
			memcpy(&forward_count, ancestor_msg + MESSAGE_FORWARD_COUNT, 4);
			forward_count++;
			memcpy(ancestor_msg + MESSAGE_FORWARD_COUNT, &forward_count, 4);
			user_message.seekp(parent, ios::beg);
			user_message.write(ancestor_msg, MESSAGE_LENGTH);
			memcpy(&parent, ancestor_msg + PARENT_MESSAGE, 8);
		}
		memcpy(&forward_count, parent_msg + MESSAGE_FORWARD_COUNT, 4);
		forward_count++;
		memcpy(parent_msg + MESSAGE_FORWARD_COUNT, &forward_count, 4);

		memcpy(msg + PARENT_MESSAGE, &forwarded_msg, 8);
		memcpy(&sibling, parent_msg + CHILD_MESSAGE, 8);
		memcpy(msg + NEXT_SIBLING, &sibling, 8);
	}

	streamoff freelist;
	user_message.seekg(8, ios::beg);
	user_message.read((char *)&freelist, 8);

	streamoff newnode;
	if (freelist == NULL) {
		user_message.seekp(0, ios::end);
		newnode = user_message.tellp();
		user_message.write(msg, MESSAGE_LENGTH);
	}
	else {
		newnode = freelist;
		//delete node from free list
		user_message.seekg(newnode, ios::beg);
		user_message.read((char *)&freelist, 8);
		user_message.seekp(8, ios::beg);
		user_message.write((char *)&freelist, 8);

		user_message.seekp(newnode, ios::beg);
		user_message.write(msg, MESSAGE_LENGTH);
	}

	if (forwarded_msg != NULL) {
		memcpy(parent_msg + CHILD_MESSAGE, &newnode, 8);
		user_message.seekp(forwarded_msg, ios::beg);
		user_message.write(parent_msg, MESSAGE_LENGTH);
		if (sibling != NULL) {
			user_message.seekp(sibling + PREVIOUS_SIBLING, ios::beg);
			user_message.write((char *)&newnode, 8);
		}
		//转发时的特殊情况处理（必须在查看微博时才能转发，因此using_message_list总有意义）
		if (using_message_list) { //在查看某人（包括自己）微博时转发
			//转发的恰是用户自己的最新微博
			if (forwarded_msg == user.message)
				user_message_list_pos.pre = newnode;
		}
		else { //在查看新鲜事时转发
			if (user.message == NULL) { //用户此前还未发过微博
				Position pos;
				if (fresh_message_forward) { //正向，无需加入buffer，待转换方向时会加入
					pos.pre = newnode;
					pos.current = NULL;
					pos.next = NULL;
					fresh_message_position[user.username] = pos;
				}
				else { //反向，需将转发的消息加入buffer，从而可在将来取出
					pos.pre = NULL;
					pos.current = newnode;
					pos.next = NULL;
					fresh_message_position[user.username] = pos;
					Message tmp_message;
					get_message(tmp_message, msg, newnode);
					fresh_message_buffer_backward.push(tmp_message);
				}
			}
			else { //用户此前已经发过微博
				Position &pos = fresh_message_position.at(user.username);
				if (pos.current == user.message) //包括正向与反向
					pos.pre = newnode;
				if (pos.next == user.message) { //反向且pos已行进到链表头部的NULL
					pos.current = newnode;
					Message tmp_message;
					get_message(tmp_message, msg, newnode);
					fresh_message_buffer_backward.push(tmp_message);
				}
			}
		}
	}
	if (user.message != NULL) { //next->pre
		user_message.seekp(user.message + PREVIOUS_MESSAGE, ios::beg);
		user_message.write((char *)&newnode, 8);
	}

	user_message.flush(); //保证数据写入了磁盘文件
	user.message = newnode;
	user.message_count++;
	modify_user_record(user);
}

//若不在浏览新鲜事，或在浏览新鲜事且正处在正向（下一条）查看过程中，则
//删除消息后，原本的下一条消息顶上，如没有下一条则改为上一条
//否则，删除消息后，原本的上一条消息顶上，如没有上一条则改为下一条
//如都没有则设置message为空，由Application类负责提示用户没有可读消息
void UserDatabaseManager::delete_message(User &user, Message &message) {
	char msg[MESSAGE_LENGTH];
	user_message.seekg(message.offset, ios::beg);
	user_message.read(msg, MESSAGE_LENGTH);
	delete_message_node(user, msg, message.offset, true);

	streamoff prev_msg, next_msg;
	//显示微博/新鲜事的状态需变更
	memcpy(&prev_msg, msg + PREVIOUS_MESSAGE, 8);
	memcpy(&next_msg, msg + NEXT_MESSAGE, 8);
	if (using_message_list)
		delete_pos_node_usermsglist(user_message_list_pos, prev_msg, next_msg);
	else {
		Position &pos = fresh_message_position.at(user.username);
		if (fresh_message_forward) {
			if (pos.current == NULL && prev_msg == NULL)
				fresh_message_position.erase(user.username);
			else
				pos.pre = prev_msg;
		}
		else {
			if (pos.current == NULL && next_msg == NULL)
				fresh_message_position.erase(user.username);
			else
				pos.next = next_msg;
		}
	}

	char descendant_msg[MESSAGE_LENGTH];
	streamoff child, sibling, current_node;
	stack<streamoff> nodes;
	string author;
	int decreased_forward_count = 1;
	User tmp_user;
	Message tmp_msg;
	//清除所有由此消息转发的消息
	memcpy(&child, msg + CHILD_MESSAGE, 8);
	if (child != NULL)
		nodes.push(child);
	while (!nodes.empty()) {
		current_node = nodes.top();
		nodes.pop();
		decreased_forward_count++;

		user_message.seekg(current_node, ios::beg);
		user_message.read(descendant_msg, MESSAGE_LENGTH);
		memcpy(&child, descendant_msg + CHILD_MESSAGE, 8);
		memcpy(&sibling, descendant_msg + NEXT_SIBLING, 8);
		if (child != NULL)
			nodes.push(child);
		if (sibling != NULL)
			nodes.push(sibling);

		author.clear();
		for (int i = MESSAGE_AUTHOR; i < MESSAGE_AUTHOR + 16; i++)
			if (descendant_msg[i] == '\0')
				break;
			else
				author.push_back(descendant_msg[i]);
		find_user_record(author, tmp_user);
		delete_message_node(tmp_user, descendant_msg, current_node, false);

		//显示微博/新鲜事的状态需变更
		memcpy(&prev_msg, descendant_msg + PREVIOUS_MESSAGE, 8);
		memcpy(&next_msg, descendant_msg + NEXT_MESSAGE, 8);
		if (using_message_list) {
			if (user_message_list_pos.pre == current_node)
				user_message_list_pos.pre = prev_msg;
			else if (user_message_list_pos.current == current_node)
				delete_pos_node_usermsglist(user_message_list_pos, prev_msg, next_msg);
			else if (user_message_list_pos.next == current_node)
				user_message_list_pos.next = next_msg;
		}
		else {
			auto iter = fresh_message_position.find(author);
			if (iter != fresh_message_position.end()) {
				Position &pos = iter->second;
				if (fresh_message_forward) {
					//此时pre为往前若干条已显示过的消息
					//current为处在buffer中待取出显示的往后若干条的消息
					//next为还未加入buffer中待加入buffer的往后若干条的消息
					if (pos.pre == current_node) {
						if (pos.current == NULL && prev_msg == NULL)
							fresh_message_position.erase(author);
						else
							pos.pre = prev_msg;
					}
					else if (pos.current == current_node) {
						pos.current = pos.next;
						if (pos.next != NULL) {
							//修改了当前在buffer中的Message，但由于buffer为优先队列，无法随机存取
							//故只push进去，待取出时由于Message带有offset，可以进行验证是否已经在此处被删除
							user_message.seekg(pos.next, ios::beg);
							user_message.read(descendant_msg, MESSAGE_LENGTH);
							get_message(tmp_msg, descendant_msg, pos.next);
							fresh_message_buffer_forward.push(tmp_msg);
							memcpy(&pos.next, descendant_msg + NEXT_MESSAGE, 8);
						}
						else
							if (pos.pre == NULL)
								fresh_message_position.erase(author);
					}
					else if (pos.next == current_node)
						pos.next = next_msg;
				}
				else {
					//此时next为往后若干条已显示过的消息
					//current为处在buffer中待取出显示的往前若干条的消息
					//pre为还未加入buffer中待加入buffer的往前若干条的消息
					if (pos.next == current_node) {
						if (pos.current == NULL && next_msg == NULL)
							fresh_message_position.erase(author);
						else
							pos.next = next_msg;
					}
					else if (pos.current == current_node) {
						pos.current = pos.pre;
						if (pos.pre != NULL) {
							//修改了当前在buffer中的Message，但由于buffer为优先队列，无法随机存取
							//故只push进去，待取出时由于Message带有offset，可以进行验证是否已经在此处被删除
							user_message.seekg(pos.pre, ios::beg);
							user_message.read(descendant_msg, MESSAGE_LENGTH);
							get_message(tmp_msg, descendant_msg, pos.pre);
							fresh_message_buffer_backward.push(tmp_msg);
							memcpy(&pos.pre, descendant_msg + PREVIOUS_MESSAGE, 8);
						}
						else
							if (pos.next == NULL)
								fresh_message_position.erase(author);
					}
					else if (pos.pre == current_node)
						pos.pre = prev_msg;
				}
			}
		}
	}
	
	decrease_ancestor_forward_count(msg, decreased_forward_count);

	user_message.flush(); //保证数据写入了磁盘文件

	find_user_record(user.username, user);//重新载入user，以防止删除转发过程中删除了自己转发自己的消息而使消息数不正确

	//设置message
	if (using_message_list) {
		if (user_message_list_pos.current != NULL) {
			user_message.seekg(user_message_list_pos.current, ios::beg);
			user_message.read(msg, MESSAGE_LENGTH);
			get_message(message, msg, user_message_list_pos.current);
		}
		else
			message.message.clear();
	}
	else {
		if (fresh_message_position.empty())
			message.message.clear();
		else {
			if (fresh_message_forward) {
				if (get_next_fresh_message(message) == -1)
					get_pre_fresh_message(message);
			}
			else {
				if (get_pre_fresh_message(message) == -1)
					get_next_fresh_message(message);
			}
		}
	}
}

void UserDatabaseManager::user_message_list_init(User &user) {
	using_message_list = true;
	user_message_list_pos.pre = NULL;
	user_message_list_pos.current = NULL;
	user_message_list_pos.next = user.message;
}

//return 0 if success, else return -1
int UserDatabaseManager::get_next_user_message(Message &message) {
	if (user_message_list_pos.next == NULL)
		return -1;
	char msg[MESSAGE_LENGTH];
	user_message.seekg(user_message_list_pos.next, ios::beg);
	user_message.read(msg, MESSAGE_LENGTH);
	get_message(message, msg, user_message_list_pos.next);

	user_message_list_pos.pre = user_message_list_pos.current;
	user_message_list_pos.current = user_message_list_pos.next;
	memcpy(&user_message_list_pos.next, msg + NEXT_MESSAGE, 8);
	return 0;
}

//return 0 if success, else return -1
int UserDatabaseManager::get_pre_user_message(Message &message) {
	if (user_message_list_pos.pre == NULL)
		return -1;
	char msg[MESSAGE_LENGTH];
	user_message.seekg(user_message_list_pos.pre, ios::beg);
	user_message.read(msg, MESSAGE_LENGTH);
	get_message(message, msg, user_message_list_pos.pre);

	user_message_list_pos.next = user_message_list_pos.current;
	user_message_list_pos.current = user_message_list_pos.pre;
	memcpy(&user_message_list_pos.pre, msg + PREVIOUS_MESSAGE, 8);
	return 0;
}

//初始化时默认是forward（查看下一条即时间更早的一条）
void UserDatabaseManager::fresh_message_init(User &user, set<string> &followed) {
	using_message_list = false;
	fresh_message_forward = true;
	fresh_message_position.clear();
	User followed_user;
	Position followed_user_pos;
	Message message;
	vector<Message> temp_message_buffer;
	char msg[MESSAGE_LENGTH];
	followed_user_pos.pre = NULL;
	if (user.message != NULL) {
		followed_user_pos.current = user.message;
		user_message.seekg(user.message, ios::beg);
		user_message.read(msg, MESSAGE_LENGTH);
		memcpy(&followed_user_pos.next, msg + NEXT_MESSAGE, 8);
		get_message(message, msg, user.message);
		fresh_message_position[user.username] = followed_user_pos;
		temp_message_buffer.push_back(message);
	}
	for (auto i : followed) {
		find_user_record(i, followed_user);
		if (followed_user.message != NULL) {
			followed_user_pos.current = followed_user.message;
			user_message.seekg(followed_user.message, ios::beg);
			user_message.read(msg, MESSAGE_LENGTH);
			memcpy(&followed_user_pos.next, msg + NEXT_MESSAGE, 8);
			get_message(message, msg, followed_user.message);
			fresh_message_position[i] = followed_user_pos;
			temp_message_buffer.push_back(message);
		}
	}
	//先用vector<Message>存储，然后移动构造优先队列，从而使用make_heap算法建堆而不是重复push_heap，达到线性时间复杂度
	//priority_queue为max heap，因此时间最大即最新的消息在堆顶
	priority_queue<Message, vector<Message>, less<Message>>(
		less<Message>(), std::move(temp_message_buffer)).swap(fresh_message_buffer_forward);
}

//return 0 if success, else return -1
int UserDatabaseManager::get_next_fresh_message(Message &message) {
	Message tmp_message;
	char msg[MESSAGE_LENGTH];
	if (!fresh_message_forward) {
		vector<Message> temp_message_buffer;
		for (auto& i : fresh_message_position) {
			i.second.pre = i.second.current;
			i.second.current = i.second.next;
			if (i.second.next != NULL) {
				user_message.seekg(i.second.next, ios::beg);
				user_message.read(msg, MESSAGE_LENGTH);
				memcpy(&i.second.next, msg + NEXT_MESSAGE, 8);
				get_message(tmp_message, msg, i.second.current);
				temp_message_buffer.push_back(tmp_message);
			}
		}
		//先用vector<Message>存储，然后移动构造优先队列，从而使用make_heap算法建堆而不是重复push_heap，达到线性时间复杂度
		//priority_queue为max heap，因此时间最大即最新的消息在堆顶
		priority_queue<Message, vector<Message>, less<Message>>(
			less<Message>(), std::move(temp_message_buffer)).swap(fresh_message_buffer_forward);
	}

	int forward_count = 0;
	while (true) {
		if (fresh_message_buffer_forward.empty())
			return -1;
		message = fresh_message_buffer_forward.top();
		fresh_message_buffer_forward.pop();
		auto iter = fresh_message_position.find(message.author);
		if (iter == fresh_message_position.end()) //已被删除而仍处在buffer中的伪Message
			continue;
		Position &pos = iter->second;
		if (pos.current != message.offset) //已被删除而仍处在buffer中的伪Message
			continue;
		user_message.seekg(message.offset + MESSAGE_FORWARD_COUNT, ios::beg); //更新转发计数(可能由于转发消息或删除消息与文件不同步)
		user_message.read((char *)&forward_count, 4);
		message.forward_count = forward_count;
		pos.pre = pos.current;
		pos.current = pos.next;
		if (pos.next != NULL) {
			user_message.seekg(pos.next, ios::beg);
			user_message.read(msg, MESSAGE_LENGTH);
			memcpy(&pos.next, msg + NEXT_MESSAGE, 8);
			get_message(tmp_message, msg, pos.current);
			fresh_message_buffer_forward.push(tmp_message);
		}
		if (!fresh_message_forward)
			fresh_message_forward = true;
		else
			break;
	}
	return 0;
}

//return 0 if success, else return -1
int UserDatabaseManager::get_pre_fresh_message(Message &message) {
	Message tmp_message;
	char msg[MESSAGE_LENGTH];
	if (fresh_message_forward) {
		vector<Message> temp_message_buffer;
		for (auto& i : fresh_message_position) {
			i.second.next = i.second.current;
			i.second.current = i.second.pre;
			if (i.second.pre != NULL) {
				user_message.seekg(i.second.pre, ios::beg);
				user_message.read(msg, MESSAGE_LENGTH);
				memcpy(&i.second.pre, msg + PREVIOUS_MESSAGE, 8);
				get_message(tmp_message, msg, i.second.current);
				temp_message_buffer.push_back(tmp_message);
			}
		}
		//先用vector<Message>存储，然后移动构造优先队列，从而使用make_heap算法建堆而不是重复push_heap，达到线性时间复杂度
		//priority_queue为max heap，采用与默认的less相反的greater进行比较，由此时间最小即最老的消息在堆顶
		priority_queue<Message, vector<Message>, greater<Message>>(
			greater<Message>(), std::move(temp_message_buffer)).swap(fresh_message_buffer_backward);
	}

	int forward_count = 0;
	while (true) {
		if (fresh_message_buffer_backward.empty())
			return -1;
		message = fresh_message_buffer_backward.top();
		fresh_message_buffer_backward.pop();
		auto iter = fresh_message_position.find(message.author);
		if (iter == fresh_message_position.end()) //已被删除而仍处在buffer中的伪Message
			continue;
		Position &pos = iter->second;
		if (pos.current != message.offset) //已被删除而仍处在buffer中的伪Message
			continue;
		user_message.seekg(message.offset + MESSAGE_FORWARD_COUNT, ios::beg); //更新转发计数(可能由于转发消息或删除消息与文件不同步)
		user_message.read((char *)&forward_count, 4);
		message.forward_count = forward_count;
		pos.next = pos.current;
		pos.current = pos.pre;
		if (pos.pre != NULL) {
			user_message.seekg(pos.pre, ios::beg);
			user_message.read(msg, MESSAGE_LENGTH);
			memcpy(&pos.pre, msg + PREVIOUS_MESSAGE, 8);
			get_message(tmp_message, msg, pos.current);
			fresh_message_buffer_backward.push(tmp_message);
		}
		if (fresh_message_forward)
			fresh_message_forward = false;
		else
			break;
	}
	return 0;
}

void UserDatabaseManager::get_message(Message &message, char(&msg)[MESSAGE_LENGTH], streamoff offset) {
	message.message.clear();
	message.author.clear();
	message.origin_message.clear();
	message.origin_author.clear();
	wchar_t *msg_content = (wchar_t *)msg;
	for (int i = 0; i < 140; i++)
		if (msg_content[i] == L'\0')
			break;
		else
			message.message.push_back(msg_content[i]);
	message.time.get_time(msg + MESSAGE_TIME);
	for (int i = 0; i < 16; i++)
		if (msg[i + MESSAGE_AUTHOR] == '\0')
			break;
		else
			message.author.push_back(msg[i + MESSAGE_AUTHOR]);
	memcpy(&message.forward_count, msg + MESSAGE_FORWARD_COUNT, 4);
	message.offset = offset;

	char org_msg[MESSAGE_LENGTH];
	streamoff origin_message;
	memcpy(&origin_message, msg + ORIGIN_MESSAGE, 8);
	if (origin_message != NULL) {
		user_message.seekg(origin_message, ios::beg);
		user_message.read(org_msg, MESSAGE_LENGTH);
		wchar_t *org_msg_content = (wchar_t *)org_msg;
		for (int i = 0; i < 140; i++)
			if (org_msg_content[i] == L'\0')
				break;
			else
				message.origin_message.push_back(org_msg_content[i]);
		message.origin_time.get_time(org_msg + MESSAGE_TIME);
		for (int i = 0; i < 16; i++)
			if (org_msg[i + MESSAGE_AUTHOR] == '\0')
				break;
			else
				message.origin_author.push_back(org_msg[i + MESSAGE_AUTHOR]);
		memcpy(&message.origin_forward_count, org_msg + MESSAGE_FORWARD_COUNT, 4);
	}
}

void UserDatabaseManager::insert_into_freelist(fstream &fs, streamoff offset) {
	char buf[8];
	fs.seekg(8, ios::beg);
	fs.read(buf, 8);
	fs.seekp(offset, ios::beg);
	fs.write(buf, 8);
	fs.seekp(8, ios::beg);
	fs.write((char *)&offset, 8);
}

void UserDatabaseManager::delete_pos_node_usermsglist(Position &pos, streamoff prev, streamoff next) {
	if (next != NULL) {
		pos.current = next;
		user_message.seekg(next + NEXT_MESSAGE, ios::beg);
		user_message.read((char *)&pos.next, 8);
	}
	else if (prev != NULL) {
		pos.current = prev;
		user_message.seekg(prev + PREVIOUS_MESSAGE, ios::beg);
		user_message.read((char *)&pos.pre, 8);
	}
	else
		pos.current = NULL;
}

void UserDatabaseManager::delete_message_node(User &user, char(&msg)[MESSAGE_LENGTH], streamoff offset, bool delete_from_tree) {
	user.message_count--;

	streamoff prev_msg, next_msg;
	//delete from message list
	memcpy(&prev_msg, msg + PREVIOUS_MESSAGE, 8);
	memcpy(&next_msg, msg + NEXT_MESSAGE, 8);
	if (prev_msg != NULL) {
		user_message.seekp(prev_msg + NEXT_MESSAGE, ios::beg);
		user_message.write((char *)&next_msg, 8);
	}
	else
		user.message = next_msg;
	if (next_msg != NULL) {
		user_message.seekp(next_msg + PREVIOUS_MESSAGE, ios::beg);
		user_message.write((char *)&prev_msg, 8);
	}

	if (delete_from_tree) {
		streamoff parent, prev_sibling, next_sibling;
		//delete from message forward tree
		memcpy(&parent, msg + PARENT_MESSAGE, 8);
		memcpy(&prev_sibling, msg + PREVIOUS_SIBLING, 8);
		memcpy(&next_sibling, msg + NEXT_SIBLING, 8);
		if (prev_sibling != NULL) {
			user_message.seekp(prev_sibling + NEXT_SIBLING, ios::beg);
			user_message.write((char *)&next_sibling, 8);
		}
		else if (parent != NULL) {
			user_message.seekp(parent + CHILD_MESSAGE, ios::beg);
			user_message.write((char *)&next_sibling, 8);
		}
		if (next_sibling != NULL) {
			user_message.seekp(next_sibling + PREVIOUS_SIBLING, ios::beg);
			user_message.write((char *)&prev_sibling, 8);
		}
	}

	insert_into_freelist(user_message, offset);
	modify_user_record(user);
}

void UserDatabaseManager::decrease_ancestor_forward_count(char(&msg)[MESSAGE_LENGTH], int decrease_count) {
	streamoff parent;
	int forward_count;
	char ancestor_msg[MESSAGE_LENGTH];
	memcpy(&parent, msg + PARENT_MESSAGE, 8);
	while (parent != NULL) {
		user_message.seekg(parent, ios::beg);
		user_message.read(ancestor_msg, MESSAGE_LENGTH);
		memcpy(&forward_count, ancestor_msg + MESSAGE_FORWARD_COUNT, 4);
		forward_count -= decrease_count;
		memcpy(ancestor_msg + MESSAGE_FORWARD_COUNT, &forward_count, 4);
		user_message.seekp(parent, ios::beg);
		user_message.write(ancestor_msg, MESSAGE_LENGTH);
		memcpy(&parent, ancestor_msg + PARENT_MESSAGE, 8);
	}
}

Time::Time(SYSTEMTIME &time) {
	year = time.wYear;
	month = time.wMonth;
	day = time.wDay;
	hour = time.wHour;
	minute = time.wMinute;
	second = time.wSecond;
	milliseconds = time.wMilliseconds;
}

void Time::set_time(char *time) {
	memcpy(time, &year, 2);
	memcpy(time + 2, &month, 2);
	memcpy(time + 4, &day, 2);
	memcpy(time + 6, &hour, 2);
	memcpy(time + 8, &minute, 2);
	memcpy(time + 10, &second, 2);
	memcpy(time + 12, &milliseconds, 2);
}

void Time::get_time(char *time) {
	memcpy(&year, time, 2);
	memcpy(&month, time + 2, 2);
	memcpy(&day, time + 4, 2);
	memcpy(&hour, time + 6, 2);
	memcpy(&minute, time + 8, 2);
	memcpy(&second, time + 10, 2);
	memcpy(&milliseconds, time + 12, 2);
}

bool Time::operator <(const Time& other) const {
	if (year < other.year)
		return true;
	else if (year == other.year) {
		if (month < other.month)
			return true;
		else if (month == other.month) {
			if (day < other.day)
				return true;
			else if (day == other.day) {
				if (hour < other.hour)
					return true;
				else if (hour == other.hour) {
					if (minute < other.minute)
						return true;
					else if (minute == other.minute) {
						if (second < other.second)
							return true;
						else if (second == other.second && milliseconds < other.milliseconds)
							return true;
					}
				}
			}
		}
	}
	return false;
}

bool Message::operator <(const Message &other) const {
	return time < other.time;
}

bool Message::operator >(const Message &other) const {
	return other.time < time;
}