#pragma once

#include <iostream>
#include <vector>
#include <iomanip>
#include <Windows.h>

#include "application.h"

using namespace std;

class MessageEditor
{
public:
	MessageEditor(wstring &message, bool with_frame, COORD num_pos, COORD cursor_pos);
	wstring get_message();
private:
	void make_align();
	void refresh_character_count();

	void left_work();
	void right_work();
	void up_work();
	void down_work();
	void home_work();
	void end_work();
	void delete_work();

	void line_overflow(int operating_line_num, int &tmp_post_end_pos, int &overflow_chars_count, int &overflow_chars_width);
	void insert_work(wchar_t input);

	HANDLE stdout_handle;

	int LEFT;
	int RIGHT;
	COORD input_num_pos;
	COORD input_cursor_pos;

	int character_count;
	int current_line_num;

	wstring line[4];
	vector<int> line_end_pos; //end_posָ���һ���ֵ�λ��(ռ������־���ǰһ��)��-1��ʾ����Ϊ����
	vector<int> line_str_pos;
	vector<int> line_cursor_pos;
	//������vector��sizeʼ�ձ������

	wchar_t wch;
	CHAR_INFO blank_char;
};