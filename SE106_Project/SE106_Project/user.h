#pragma once

#include <iostream>

using namespace std;

static const int RECORD_LENGTH = 144;
static const int PASSWORD = 16;
static const int NAME = 48;
static const int PHONE_GENDOR = 72;
static const int BIRTHDAY = 80;
static const int HOMETOWN = 84;
static const int IN_EDGES = 108;
static const int OUT_EDGES = 116;
static const int MESSAGE = 124;
static const int IN_COUNT = 132;
static const int OUT_COUNT = 136;
static const int MESSAGE_COUNT = 140;

struct date
{
	unsigned short year;
	unsigned char month;
	unsigned char day;
	bool operator ==(const date &other);
	bool operator !=(const date &other);
	bool operator <(const date &other);
	bool operator >(const date &other);
	bool operator <=(const date &other);
	bool operator >=(const date &other);
};

struct Query {
	pair<string, bool> username;
	pair<string, bool> name;
	pair<bool, bool> ismale;
	pair<date, bool> birthday_start;
	pair<date, bool> birthday_end;
	pair<long long, bool> phonenum;
	pair<string, bool> hometown;
};

struct User
{
public:
	void initialize(string un, string pw, string na, bool ism, date bir, long long pn, string ht);
	void initialize(char (&record)[RECORD_LENGTH]);
	string get_record();
	void print_info(bool followed, bool fans);
	bool fits(Query &query, string &current_username);

	string username;
	string password;
	string name;
	bool ismale;
	date birthday;
	long long phonenum;
	string hometown;

	bool isdeleted;

	streamoff in_edges;
	streamoff out_edges;
	streamoff message;

	int in_count;
	int out_count;
	int message_count;
};

bool operator <(const User &a, const User &b);

bool operator !=(const date &a, const date &b);

bool operator !=(const User &a, const User &b);