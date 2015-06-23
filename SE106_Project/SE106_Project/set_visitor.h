#pragma once

#include "special_iterator.h"

// ֻʵ����ҵ���߼��б���Ľӿڣ�����ӿ�������Ҫʱ����
class SetVisitor
{
public:
	enum KIND { STRING, USER };
	SetVisitor(set<string>& a) { m_str_set = &a; kind = STRING; }
	SetVisitor(set<User>& a) { m_usr_set = &a; kind = USER; }
	SpecialIterator begin() {
		if (kind == STRING)
			return m_str_set->begin();
		else
			return m_usr_set->begin();
	}
	SpecialIterator end() {
		if (kind == STRING)
			return m_str_set->end();
		else
			return m_usr_set->end();
	}
	size_t size() {
		if (kind == STRING)
			return m_str_set->size();
		else
			return m_usr_set->size();
	}
	KIND get_kind() const { return kind; }
private:
	set<string>* m_str_set;
	set<User>* m_usr_set;
	KIND kind;
};