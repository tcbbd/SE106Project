#pragma once

#include <iostream>
#include <set>

using namespace std;

//ֻʵ����ҵ���߼��б���Ľӿڣ�����ӿ�������Ҫʱ����
class SpecialIterator
{
public:
	SpecialIterator(set<string>::iterator &iter) { m_str_iter = iter; kind = STRING; }
	SpecialIterator(set<User>::iterator &iter) { m_usr_iter = iter; kind = USER; }
	SpecialIterator& operator++() {
		if (kind == STRING)
			++m_str_iter;
		else if (kind == USER)
			++m_usr_iter;
		return *this;
	}
	SpecialIterator& operator--() {
		if (kind == STRING)
			--m_str_iter;
		else if (kind == USER)
			--m_usr_iter;
		return *this;
	}
	string get_username() const {
		if (kind == STRING)
			return *m_str_iter;
		else
			return (*m_usr_iter).username;
	}
	set<User>::iterator::reference get_user() const { return *m_usr_iter; }
	bool operator==(const SpecialIterator& right) const {
		if (kind != right.kind)
			return false;
		if (kind == STRING)
			return m_str_iter == right.m_str_iter;
		else
			return m_usr_iter == right.m_usr_iter;
	}
private:
	set<string>::iterator m_str_iter;
	set<User>::iterator m_usr_iter;
	enum { STRING, USER } kind;
};