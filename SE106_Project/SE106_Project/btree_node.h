#pragma once

#include <iostream>
#include <vector>
#include <tuple>

using namespace std;

static const int BLOCK_SIZE = 4096;

class BTreeNode
{
public:
	enum state { FAILED, SUCCESS, SPLITTED };

	BTreeNode() {}
	BTreeNode(char(&nd)[BLOCK_SIZE], char(&nd_sp)[BLOCK_SIZE]): node(nd), node_split(nd_sp) {}
	inline bool is_internal() { return (node[0] == 0); }
	inline void set_internal(bool inter) { node[0] = inter ? 0 : 1; }

	state leaf_insert(string &username, streamoff pointer, char(&split_key)[16]);
	state internal_insert(char(&insert_key)[16], streamoff child, int i);
	pair<streamoff, bool> leaf_find(string &username);
	pair<streamoff, int> internal_find(string &username);
	tuple<vector<streamoff>, streamoff, bool> leaf_region_find(string &start, string &end);

	void set_sibling(streamoff sibling);	
	void make_root(char(&split_key)[16], streamoff left_child, streamoff right_child);
private:
	inline int key_num();
	inline void set_key_num(int i);

	state leaf_insert_notfull(char(&insert_key)[16], streamoff insert_data);

	char *node;
	char *node_split; //��Ϊ���Ǵ����ڴ��е����ݽṹ������new���½ڵ㣬���ѽڵ����ڻ������У���UserDataBaseManager��������ļ�
	static const int MAX_CHILD_NUM = 171; //ע���෽����ʵ��ֻ�ܴ���MAX_CHILD_NUMΪ���������Σ���Ӧ��Ϊż��
	static const int MIN_CHILD_NUM = 86;
};