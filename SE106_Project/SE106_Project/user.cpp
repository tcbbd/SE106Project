#include "stdafx.h"
#include "user.h"

#include <string>

void User::initialize(string un, string pw, string na, bool ism, date bir, long long pn, string ht) {
	username = un;
	password = pw;
	name = na;
	ismale = ism;
	birthday = bir;
	phonenum = pn;
	hometown = ht;

	isdeleted = false;
	in_edges = NULL;
	out_edges = NULL;
	message = NULL;
	in_count = 0;
	out_count = 0;
	message_count = 0;
}

void User::initialize(char (&record)[RECORD_LENGTH]) {
	username.clear();
	for (int i = 0; i < 16; i++) {
		if (record[i] == '\0')
			break;
		else
			username.push_back(record[i]);
	}
	password.clear();
	for (int i = PASSWORD; i < PASSWORD + 32; i++) {
		if (record[i] == '\0')
			break;
		else
			password.push_back(record[i]);
	}
	name.clear();
	for (int i = NAME; i < NAME + 24; i++){
		if (record[i] == '\0')
			break;
		else
			name.push_back(record[i]);
	}
	memcpy(&phonenum, record + PHONE_GENDOR, 8);
	if (phonenum >> 56)
		ismale = true;
	else
		ismale = false;
	phonenum &= 0x00FFFFFFFFFFFFFF;
	if (phonenum >> 48)
		isdeleted = true;
	else
		isdeleted = false;
	phonenum &= 0x0000FFFFFFFFFFFF;
	memcpy(&birthday.year, record + BIRTHDAY, 2);
	birthday.month = record[BIRTHDAY + 2];
	birthday.day = record[BIRTHDAY + 3];
	hometown.clear();
	for (int i = HOMETOWN; i < HOMETOWN + 24; i++){
		if (record[i] == '\0')
			break;
		else
			hometown.push_back(record[i]);
	}
	memcpy(&in_edges, record + IN_EDGES, 8);
	memcpy(&out_edges, record + OUT_EDGES, 8);
	memcpy(&message, record + MESSAGE, 8);
	memcpy(&in_count, record + IN_COUNT, 4);
	memcpy(&out_count, record + OUT_COUNT, 4);
	memcpy(&message_count, record + MESSAGE_COUNT, 4);
}

string User::get_record() {
	char record[RECORD_LENGTH] = {};
	for (size_t i = 0; i < username.size(); i++)
		record[i] = username[i];
	for (size_t i = 0; i < password.size(); i++)
		record[i + PASSWORD] = password[i];
	for (size_t i = 0; i < name.size(); i++)
		record[i + NAME] = name[i];
	long long phone_gendor = phonenum;
	if (ismale)
		phone_gendor |= 0x0100000000000000; //最高字节标明性别
	if (isdeleted)
		phone_gendor |= 0x0001000000000000; //次高字节标明是否删除
	memcpy(record + PHONE_GENDOR, &phone_gendor, 8);
	memcpy(record + BIRTHDAY, &birthday.year, 2);
	record[BIRTHDAY + 2] = birthday.month;
	record[BIRTHDAY + 3] = birthday.day;
	for (size_t i = 0; i < hometown.size(); i++)
		record[i + HOMETOWN] = hometown[i];
	memcpy(record + IN_EDGES, &in_edges, 8);
	memcpy(record + OUT_EDGES, &out_edges, 8);
	memcpy(record + MESSAGE, &message, 8);
	memcpy(record + IN_COUNT, &in_count, 4);
	memcpy(record + OUT_COUNT, &out_count, 4);
	memcpy(record + MESSAGE_COUNT, &message_count, 4);
	return string(record, RECORD_LENGTH);
}

void User::print_info(bool followed, bool fans) {
	cout << username << "  关注：" << out_count << "  粉丝：" << in_count << "  微博：" << message_count << endl;
	if (followed)
		if (fans)
			cout << "互相关注  ";
		else
			cout << "已关注  ";
	else
		if (fans)
			cout << "我的粉丝  ";
	if (ismale)
		cout << "男  ";
	else
		cout << "女  ";
	cout << birthday.year << '-' << int(birthday.month) << '-' << int(birthday.day) << "  ";
	cout << "姓名： " << name << endl;
	cout << "联系电话： ";
	if (phonenum)
		cout << phonenum;
	else
		cout << "无";
	cout << "  来自： ";
	if (!hometown.empty())
		cout << hometown << endl;
	else
		cout << "无" << endl;
}

bool User::fits(Query &query, string &current_username) {
	if (isdeleted || username == current_username)
		return false;
	if (query.name.second && name != query.name.first)
		return false;
	if (query.ismale.second && ismale != query.ismale.first)
		return false;
	if (query.birthday_start.second) {
		if (query.birthday_end.second) {
			if (birthday < query.birthday_start.first ||
				birthday > query.birthday_end.first) //生日区间理解为闭区间
				return false;
		}
		else if (birthday != query.birthday_start.first)
			return false;
	}
	if (query.phonenum.second && phonenum != query.phonenum.first)
		return false;
	if (query.hometown.second && hometown != query.hometown.first)
		return false;
	return true;
}

bool date::operator==(const date &other) {
	if (year == other.year && month == other.month &&
		day == other.day)
		return true;
	else
		return false;
}

bool date::operator!=(const date &other) {
	if (year != other.year || month != other.month ||
		day != other.day)
		return true;
	else
		return false;
}

bool date::operator<(const date &other) {
	if (year < other.year)
		return true;
	else if (year == other.year) {
		if (month < other.month)
			return true;
		else if (month == other.month && day < other.day)
			return true;
	}
	return false;
}

bool date::operator>(const date &other) {
	if (year > other.year)
		return true;
	else if (year == other.year) {
		if (month > other.month)
			return true;
		else if (month == other.month && day > other.day)
			return true;
	}
	return false;
}

bool date::operator<=(const date &other) {
	return !(*this > other);
}

bool date::operator>=(const date &other) {
	return !(*this < other);
}

bool operator <(const User &a, const User &b) {
	return a.username < b.username;
}

bool operator !=(const date &a, const date &b) {
	if (a.year != b.year || a.month != b.month ||
		a.day != b.day)
		return true;
	else
		return false;
}

bool operator !=(const User &a, const User &b) {
	if (a.birthday != b.birthday)
		return true;
	if (a.hometown != b.hometown)
		return true;
	if (a.in_count != b.in_count)
		return true;
	if (a.in_edges != b.in_edges)
		return true;
	if (a.isdeleted != b.isdeleted)
		return true;
	if (a.ismale != b.ismale)
		return true;
	if (a.message != b.message)
		return true;
	if (a.message_count != b.message_count)
		return true;
	if (a.name != b.name)
		return true;
	if (a.out_count != b.out_count)
		return true;
	if (a.out_edges != b.out_edges)
		return true;
	if (a.password != b.password)
		return true;
	if (a.phonenum != b.phonenum)
		return true;
	if (a.username != b.username)
		return true;
	return false;
}