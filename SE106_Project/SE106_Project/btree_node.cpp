#include "stdafx.h"
#include "btree_node.h"

inline int BTreeNode::key_num() {
	return int((unsigned char)(node[1]));
}

inline void BTreeNode::set_key_num(int i) {
	node[1] = i;
}

BTreeNode::state BTreeNode::leaf_insert_notfull(char(&insert_key)[16], streamoff insert_data) {
	char *key = node + 8;
	char *data = node + 8 + 16 * (MAX_CHILD_NUM - 1);
	int i;
	for (i = key_num(); i > 0; i--) {
		int tmp = strncmp(insert_key, key + 16 * (i - 1), 16);
		if (tmp == 0)
			return FAILED;
		if (tmp < 0) { //insert_key < key[i-1]
			memcpy(key + 16 * i, key + 16 * (i - 1), 16); //key[i] = key[i-1]
			memcpy(data + 8 * i, data + 8 * (i - 1), 8); //data[i] = data[i-1]
		}
		else //insert_key > key[i-1]
			break;
	}
	memcpy(key + 16 * i, insert_key, 16); //key[i] = insert_key
	memcpy(data + 8 * i, &insert_data, 8); //data[i] = insert_data
	set_key_num(key_num() + 1);
	return SUCCESS;
}

//如果分裂，左节点的兄弟节点不会进行设置，需由UserDataBaseManager设置（因为需要知道文件中的位置）
BTreeNode::state BTreeNode::leaf_insert(string &username, streamoff pointer, char(&split_key)[16]) {
	char insert_key[16] = {};
	for (size_t i = 0; i < username.size(); i++)
		insert_key[i] = username[i];
	if (key_num() < MAX_CHILD_NUM - 1) //能插入
		return leaf_insert_notfull(insert_key, pointer);
	else { //不能插入，需分裂
		BTreeNode new_node;
		new_node.node = node_split;
		char *key_left = node + 8;
		char *data_left = node + 8 + 16 * (MAX_CHILD_NUM - 1);
		char *key_right = node_split + 8;
		char *data_right = node_split + 8 + 16 * (MAX_CHILD_NUM - 1);

		int tmp = strncmp(insert_key, key_left + 16 * (MIN_CHILD_NUM - 1), 16);
		if (tmp == 0) //insert_key == right.key[0]
			return FAILED;

		memcpy(key_right, key_left + 16 * (MIN_CHILD_NUM - 1), 16 * (MIN_CHILD_NUM - 1));
		memcpy(data_right, data_left + 8 * (MIN_CHILD_NUM - 1), 8 * (MIN_CHILD_NUM - 1));
		new_node.set_internal(false);
		set_key_num(MIN_CHILD_NUM - 1);
		new_node.set_key_num(MIN_CHILD_NUM - 1);
		memcpy(data_right + 8 * (MAX_CHILD_NUM - 1), data_left + 8 * (MAX_CHILD_NUM - 1), 8); //right.sibling = left.sibling

		if (tmp < 0) { //insert_key < right.key[0]
			if (leaf_insert_notfull(insert_key, pointer) == FAILED)
				return FAILED;
		}
		else { //insert_key > right.key[0]
			if (new_node.leaf_insert_notfull(insert_key, pointer) == FAILED)
				return FAILED;
		}
		memcpy(split_key, key_right, 16);
		return SPLITTED;
	}
}

BTreeNode::state BTreeNode::internal_insert(char(&insert_key)[16], streamoff child, int i) {
	if (key_num() < MAX_CHILD_NUM - 1) { //能插入
		char *key = node + 8;
		char *data = node + 8 + 16 * (MAX_CHILD_NUM - 1);
		memmove(key + 16 * (i + 1), key + 16 * i, 16 * (key_num() - i));
		memmove(data + 8 * (i + 2), data + 8 * (i + 1), 8 * (key_num() - i));
		memcpy(key + 16 * i, insert_key, 16);
		memcpy(data + 8 * (i + 1), &child, 8);
		set_key_num(key_num() + 1);
		return SUCCESS;
	}
	else { //不能插入，需分裂
		BTreeNode new_node;
		new_node.node = node_split;
		char *key_left = node + 8;
		char *data_left = node + 8 + 16 * (MAX_CHILD_NUM - 1);
		char *key_right = node_split + 8;
		char *data_right = node_split + 8 + 16 * (MAX_CHILD_NUM - 1);

		new_node.set_internal(true);
		set_key_num(MIN_CHILD_NUM - 1);
		new_node.set_key_num(MIN_CHILD_NUM - 1);

		if (i == MIN_CHILD_NUM - 1) { //i为分割点
			memcpy(key_right, key_left + 16 * (MIN_CHILD_NUM - 1), 16 * (MIN_CHILD_NUM - 1));
			memcpy(data_right + 8, data_left + 8 * MIN_CHILD_NUM, 8 * (MIN_CHILD_NUM - 1));
			memcpy(data_right, &child, 8);
		}
		else if (i < MIN_CHILD_NUM - 1) { //i位于分割点左侧
			memcpy(key_right, key_left + 16 * (MIN_CHILD_NUM - 1), 16 * (MIN_CHILD_NUM - 1));
			memcpy(data_right, data_left + 8 * (MIN_CHILD_NUM - 1), 8 * MIN_CHILD_NUM);
			internal_insert(insert_key, child, i);
			set_key_num(MIN_CHILD_NUM - 1);
			memcpy(insert_key, key_left + 16 * (MIN_CHILD_NUM - 1), 16);
		}
		else { //i位于分割点右侧
			memcpy(key_right, key_left + 16 * MIN_CHILD_NUM, 16 * (MIN_CHILD_NUM - 2));
			memcpy(data_right, data_left + 8 * MIN_CHILD_NUM, 8 * (MIN_CHILD_NUM - 1));
			new_node.set_key_num(MIN_CHILD_NUM - 2);
			new_node.internal_insert(insert_key, child, i - MIN_CHILD_NUM);
			memcpy(insert_key, key_left + 16 * (MIN_CHILD_NUM - 1), 16);
		}

		return SPLITTED;
	}
}

pair<streamoff, bool> BTreeNode::leaf_find(string &username) {
	char insert_key[16] = {};
	for (size_t i = 0; i < username.size(); i++)
		insert_key[i] = username[i];
	char *key = node + 8;
	char *data = node + 8 + 16 * (MAX_CHILD_NUM - 1);
	streamoff ret;
	for (int i = 0; i < key_num(); i++) {
		if (strncmp(insert_key, key + 16 * i, 16) == 0) { //insert_key == key[i]
			memcpy(&ret, data + 8 * i, 8);
			return make_pair(ret, true);
		}
	}
	return make_pair(0, false);
}

pair<streamoff, int> BTreeNode::internal_find(string &username) {
	char insert_key[16] = {};
	for (size_t i = 0; i < username.size(); i++)
		insert_key[i] = username[i];
	char *key = node + 8;
	char *data = node + 8 + 16 * (MAX_CHILD_NUM - 1);
	streamoff ret;
	int i;
	for (i = 0; i < key_num(); i++) {
		if (strncmp(insert_key, key + 16 * i, 16) < 0) { //insert_key < key[i]
			memcpy(&ret, data + 8 * i, 8);
			return make_pair(ret, i);
		}
	}
	memcpy(&ret, data + 8 * key_num(), 8);
	return make_pair(ret, i);
}

tuple<vector<streamoff>, streamoff, bool> BTreeNode::leaf_region_find(string &start, string &end) {
	char start_key[16] = {};
	for (size_t i = 0; i < start.size(); i++)
		start_key[i] = start[i];
	char end_key[16] = {};
	for (size_t i = 0; i < end.size(); i++)
		end_key[i] = end[i];
	char *key = node + 8;
	char *data = node + 8 + 16 * (MAX_CHILD_NUM - 1);
	vector<streamoff> result;
	int array, count = 0;
	bool first_time_inregion = true;
	for (int i = 0; i < key_num(); i++) {
		if ((strncmp(start_key, key + 16 * i, 16) <= 0) &&
			(strncmp(end_key, key + 16 * i, 16) >= 0)) { //start_key <= key[i] <= end_key
			if (first_time_inregion) {
				first_time_inregion = false;
				array = i;
			}
			count++;
			continue;
		}
		if (!first_time_inregion)
			break;
	}
	if (count)
		result.assign((streamoff*)(data + 8 * array), (streamoff*)(data + 8 * array + 8 * count));
	streamoff sibling;
	bool has_sibling;
	if (array + count == key_num()) {
		memcpy(&sibling, data + 8 * (MAX_CHILD_NUM - 1), 8);
		if (sibling != NULL)
			has_sibling = true;
		else
			has_sibling = false;
	}
	else
		has_sibling = false;
	return make_tuple(result, sibling, has_sibling);
}

void BTreeNode::set_sibling(streamoff sibling) {
	memcpy(node + 8 + 24 * (MAX_CHILD_NUM - 1), &sibling, 8);
}

void BTreeNode::make_root(char(&split_key)[16], streamoff left_child, streamoff right_child) {
	memset(node, 0, BLOCK_SIZE);
	set_key_num(1);

	char *key = node + 8;
	char *data = node + 8 + 16 * (MAX_CHILD_NUM - 1);
	memcpy(key, split_key, 16);
	memcpy(data, &left_child, 8);
	memcpy(data + 8, &right_child, 8);
}