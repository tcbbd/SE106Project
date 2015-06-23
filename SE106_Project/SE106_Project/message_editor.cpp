#include "stdafx.h"
#include "message_editor.h"

MessageEditor::MessageEditor(wstring &message, bool with_frame, COORD num_pos, COORD cursor_pos)
{
	stdout_handle = Application::stdout_handle;
	input_cursor_pos = cursor_pos;
	input_num_pos = num_pos;
	SetConsoleCursorPosition(stdout_handle, input_cursor_pos);
	if (with_frame) {
		LEFT = 1;
		RIGHT = 78;
	}
	else {
		LEFT = 0;
		RIGHT = 79;
	}
	//message = L"aһ�����������߰˾�ʮһ�����������߰˾�ʮһ�����������߰˾�ʮһ�����������߰˾�ʮһ�����������߰˾�ʮһ�����������߰˾�ʮһ�����������߰˾�ʮһ�����������߰˾�aʮһ�����������߰˾�ʮһ�����������߰˾�ʮһ�����������߰˾�ʮһ�����������߰�a";
	//ֻ�е�with_frameΪfalseʱ����ת���������ʱ���Ż���Ҫ����message�е����ݽ��г�ʼ��
	int pos_x = 0; //��дλ�ã����һ���ַ���һ��
	int pos_y = 0;
	line_end_pos = vector<int>(1, -1); //end_posָ���һ���ֵ�λ��(ռ������־���ǰһ��)��-1��ʾ����Ϊ����
	wchar_t wch;
	for (int i = 0; i < message.size(); i++) {
		wch = message[i];
		WriteConsoleW(stdout_handle, &wch, 1, NULL, NULL);
		if (!isascii(wch)) {
			if (pos_x == 78) {
				line_end_pos[pos_y] = pos_x;
				line[pos_y].push_back(wch);
				pos_y++;
				pos_x = 0;
				line_end_pos.push_back(-1);
			}
			else if (pos_x == 79) {
				pos_y++;
				line[pos_y].push_back(wch);
				pos_x = 2;
				line_end_pos.push_back(0);
			}
			else {
				line_end_pos[pos_y] = pos_x;
				line[pos_y].push_back(wch);
				pos_x += 2;
			}
		}
		else {
			line[pos_y].push_back(wch);
			if (pos_x == 79) {
				line_end_pos[pos_y] = pos_x;
				pos_y++;
				pos_x = 0;
				line_end_pos.push_back(-1);
			}
			else {
				line_end_pos[pos_y] = pos_x;
				pos_x++;
			}
		}
	}

	character_count = message.size();
	current_line_num = 0;
	//�ڸ����ַ����е�λ�ã���Ϊline.size()��ʾ�������һ���ֵĺ���һ���λ��(ֻ�������һ�л���ִ����)
	line_str_pos = vector<int>(line_end_pos.size(), 0);
	line_cursor_pos = vector<int>(line_end_pos.size(), LEFT); //������vector���Լ�line_end_pos��size��ʼ�ձ������
	blank_char.Char.UnicodeChar = L' ';
	blank_char.Attributes = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;

	SetConsoleCursorPosition(stdout_handle, input_cursor_pos);
}

void MessageEditor::make_align() { //ֻҪ��cursor_pos����str_posƥ�伴�ɵ���������
	int tmp_cursor_pos;
	for (int i = 0; i < line_str_pos.size(); i++) {
		if (i == current_line_num)
			continue;
		tmp_cursor_pos = line_cursor_pos[i];
		while (tmp_cursor_pos > line_cursor_pos[current_line_num]) { //�����Ҳ࣬���Ժ���һ����
			if (line_str_pos[i] == 0)
				break;
			wch = line[i][line_str_pos[i] - 1];
			if (!isascii(wch))
				tmp_cursor_pos -= 2;
			else
				tmp_cursor_pos--;
			line_str_pos[i]--;
			line_cursor_pos[i] = tmp_cursor_pos;
		}
		while (tmp_cursor_pos < line_cursor_pos[current_line_num]) { //������࣬����ǰ��һ����
			if (line_str_pos[i] == line[i].size()) //���һ����β��һ���޷���ǰ��
				break;
			if ((line_str_pos[i] == line[i].size() - 1) //��β
				&& (i != line_str_pos.size() - 1)) //���������һ�У��޷���ǰ��
				break;
			wch = line[i][line_str_pos[i]];
			if (!isascii(wch))
				tmp_cursor_pos += 2;
			else
				tmp_cursor_pos++;
			if (tmp_cursor_pos > line_cursor_pos[current_line_num]) //ǰ��һ���ֺ������Ҳ࣬�����ò��ƶ������õ�ǰλ��
				break;
			line_str_pos[i]++;
			line_cursor_pos[i] = tmp_cursor_pos;
		}
	}
}

void MessageEditor::left_work() {
	if (line_str_pos[current_line_num] == 0) { //λ������
		if (current_line_num == 0)
			return;
		current_line_num--;
		input_cursor_pos.Y--;
		input_cursor_pos.X = line_end_pos[current_line_num];
		for (int i = 0; i < line_str_pos.size(); i++) {
			line_str_pos[i] = line[i].size() - 1;
			line_cursor_pos[i] = line_end_pos[i];
			if (i == line_str_pos.size() - 1) { //���һ�й�����������β��һ��
				if (line[i].empty()) //���һ���ǿ���
					line_cursor_pos[i] = 0;
				else {
					wch = line[i].back();
					if (!isascii(wch))
						line_cursor_pos[i] += 2;
					else
						line_cursor_pos[i]++;
				}
				line_str_pos[i]++;
			}
		}
	}
	else {
		wch = line[current_line_num][line_str_pos[current_line_num] - 1];
		line_str_pos[current_line_num]--;
		if (!isascii(wch)) {
			line_cursor_pos[current_line_num] -= 2;
			input_cursor_pos.X -= 2;
		}
		else {
			line_cursor_pos[current_line_num]--;
			input_cursor_pos.X--;
		}
	}
	make_align();
	SetConsoleCursorPosition(stdout_handle, input_cursor_pos);
}

void MessageEditor::right_work() {
	if (line_str_pos[current_line_num] == line[current_line_num].size()) //λ�����һ����β��һ��
		return;
	if ((line_str_pos[current_line_num] == line[current_line_num].size() - 1) && //�������н�β
		(current_line_num != line_str_pos.size() - 1)) { //�Ҳ������һ��
		current_line_num++;
		input_cursor_pos.Y++;
		for (auto& i : line_str_pos)
			i = 0;
		for (auto& i : line_cursor_pos)
			i = LEFT;
		input_cursor_pos.X = LEFT;
	}
	else { //��һ���м䣬���������һ�е���β
		wch = line[current_line_num][line_str_pos[current_line_num]];
		line_str_pos[current_line_num]++;
		if (!isascii(wch)) {
			line_cursor_pos[current_line_num] += 2;
			input_cursor_pos.X += 2;
		}
		else {
			line_cursor_pos[current_line_num]++;
			input_cursor_pos.X++;
		}
		make_align();
	}
	SetConsoleCursorPosition(stdout_handle, input_cursor_pos);
}

void MessageEditor::up_work() {
	if (current_line_num == 0) {
		for (auto& i : line_str_pos)
			i = 0;
		for (auto& i : line_cursor_pos)
			i = LEFT;
		input_cursor_pos.X = LEFT;
	}
	else {
		current_line_num--;
		input_cursor_pos.Y--;
		input_cursor_pos.X = line_cursor_pos[current_line_num];
	}
	SetConsoleCursorPosition(stdout_handle, input_cursor_pos);
}

void MessageEditor::down_work() {
	if (current_line_num == line_str_pos.size() - 1) {
		line_str_pos[current_line_num] = line[current_line_num].size();
		input_cursor_pos.X = line_end_pos[current_line_num];
		if (line[current_line_num].empty()) //�����ǿ���
			input_cursor_pos.X = LEFT;
		else {
			wch = line[current_line_num].back();
			if (!isascii(wch))
				input_cursor_pos.X += 2;
			else
				input_cursor_pos.X++;
		}
		line_cursor_pos[current_line_num] = input_cursor_pos.X;
		make_align();
	}
	else {
		current_line_num++;
		input_cursor_pos.Y++;
		input_cursor_pos.X = line_cursor_pos[current_line_num];
	}
	SetConsoleCursorPosition(stdout_handle, input_cursor_pos);
}

void MessageEditor::home_work() {
	for (auto& i : line_str_pos)
		i = 0;
	for (auto& i : line_cursor_pos)
		i = LEFT;
	input_cursor_pos.X = LEFT;
	SetConsoleCursorPosition(stdout_handle, input_cursor_pos);
}

void MessageEditor::end_work() {
	line_str_pos[current_line_num] = line[current_line_num].size() - 1;
	input_cursor_pos.X = line_end_pos[current_line_num];
	line_cursor_pos[current_line_num] = input_cursor_pos.X;
	if (current_line_num == line_str_pos.size() - 1) { //���һ��
		line_str_pos[current_line_num]++;
		if (line[current_line_num].empty()) //���һ���ǿ���
			input_cursor_pos.X = 0;
		else {
			wch = line[current_line_num].back();
			if (!isascii(wch))
				input_cursor_pos.X += 2;
			else
				input_cursor_pos.X++;
		}
		line_cursor_pos[current_line_num] = input_cursor_pos.X;
	}
	make_align();
	SetConsoleCursorPosition(stdout_handle, input_cursor_pos);
}

void MessageEditor::refresh_character_count() {
	SetConsoleCursorPosition(stdout_handle, input_num_pos);
	if (character_count > 140) {
		SetConsoleTextAttribute(stdout_handle, FOREGROUND_RED | FOREGROUND_INTENSITY);
		cout << setiosflags(ios::right) << setw(3) << character_count << resetiosflags(ios::right);
		SetConsoleTextAttribute(stdout_handle, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
	}
	else
		cout << setiosflags(ios::right) << setw(3) << character_count << resetiosflags(ios::right);
}

void MessageEditor::delete_work() { //str_pos��cursor_pos����
	int tmp_post_end_pos;
	DWORD cCharsWritten;

	if (line_str_pos[current_line_num] == line[current_line_num].size()) //���һ����β��һ��
		return;
	character_count--;
	wch = line[current_line_num][line_str_pos[current_line_num]];
	int deleted_ch_width;
	if (!isascii(wch))
		deleted_ch_width = 2;
	else
		deleted_ch_width = 1;
	//�ڸ���ɾȥ��ǰ���λ�õ��ַ�������������ַ���ǰ�ƶ�
	if (line_str_pos[current_line_num] == line[current_line_num].size() - 1) { //��ǰλ������β
		tmp_post_end_pos = line_cursor_pos[current_line_num];
		FillConsoleOutputCharacter(stdout_handle, (TCHAR)' ', deleted_ch_width, input_cursor_pos, &cCharsWritten);
	}
	else {
		SMALL_RECT scroll_rect;
		scroll_rect.Bottom = scroll_rect.Top = input_cursor_pos.Y;
		scroll_rect.Left = line_cursor_pos[current_line_num] + deleted_ch_width;
		wch = line[current_line_num].back();
		if (!isascii(wch))
			scroll_rect.Right = line_end_pos[current_line_num] + 1;
		else
			scroll_rect.Right = line_end_pos[current_line_num];
		tmp_post_end_pos = scroll_rect.Right + 1 - deleted_ch_width;
		ScrollConsoleScreenBuffer(stdout_handle, &scroll_rect, NULL, input_cursor_pos, &blank_char);
	}
	line[current_line_num].erase(line_str_pos[current_line_num], 1);

	//���������������ǰ�ƶ�(ÿ�β���ĳ��ʱ���Ӻ�һ����ȡ�����ַ���������һ������)
	int operating_line_num = current_line_num;
	while (operating_line_num < line_str_pos.size()) {
		if (operating_line_num == line_str_pos.size() - 1) { //���һ�У�ֻ������line_end_pos
			if (line[operating_line_num].empty()) //���һ�б�ɾ�ɿ���
				line_end_pos[operating_line_num] = -1;
			else {
				wch = line[operating_line_num].back();
				if (!isascii(wch))
					line_end_pos[operating_line_num] = tmp_post_end_pos - 2;
				else
					line_end_pos[operating_line_num] = tmp_post_end_pos - 1;
			}
		}
		else {
			COORD end_pos = { tmp_post_end_pos, input_cursor_pos.Y + (operating_line_num - current_line_num) };
			int deleted_chars_width = 0;
			int tmp_char_width;
			int tmp_line_count = line_str_pos.size();
			while (tmp_post_end_pos <= RIGHT) { //����һ�н�ȡ�����ֽ��ڴ������
				if (line[operating_line_num + 1].empty()) { //��һ���Ѿ���ȡ�գ�����һ��ɾȥ
					line_str_pos.pop_back();
					line_cursor_pos.pop_back();
					line_end_pos.pop_back();
					break;
				}
				else { //����ȡ��һ���֣����ڸ��к�
					wch = line[operating_line_num + 1].front();
					if (!isascii(wch))
						tmp_char_width = 2;
					else
						tmp_char_width = 1;
					tmp_post_end_pos += tmp_char_width;
					if (tmp_post_end_pos > RIGHT + 1) { //̫�����˲�����ȡ��
						tmp_post_end_pos -= tmp_char_width;
						break;
					}
					deleted_chars_width += tmp_char_width;
					line[operating_line_num + 1].erase(0, 1);
					line[operating_line_num].push_back(wch);
					FillConsoleOutputCharacter(stdout_handle, wch, tmp_char_width, end_pos, &cCharsWritten);
					end_pos.X += tmp_char_width;
				}
			}

			//����line_end_pos
			wch = line[operating_line_num].back();
			if (!isascii(wch))
				line_end_pos[operating_line_num] = tmp_post_end_pos - 2;
			else
				line_end_pos[operating_line_num] = tmp_post_end_pos - 1;

			if (deleted_chars_width == 0) { //δ����һ��ȡ���ַ�
				if ((operating_line_num == current_line_num) && //������ڵ���
					(line_cursor_pos[current_line_num] > line_end_pos[current_line_num]) && //���λ�����Ѳ������ַ�
					(current_line_num + 1 < line_str_pos.size())) { //��һ�д��ڣ����Ӧ�ƶ�����һ��
					line_str_pos[current_line_num] = 0;
					line_cursor_pos[current_line_num] = LEFT;
					current_line_num++;
					line_str_pos[current_line_num] = 0;
					line_cursor_pos[current_line_num] = LEFT;
					input_cursor_pos.X = LEFT;
					input_cursor_pos.Y++;
				}
				break; //����Ժ�������ٽ��в���
			}
			else { //ȡ������һ�е��ַ�����˽���һ�е���������
				if (operating_line_num + 1 < tmp_line_count) { //�����һ�л�����
					end_pos.Y++;
					end_pos.X = LEFT;
					if (line[operating_line_num + 1].empty()) //��һ��Ϊ��
						FillConsoleOutputCharacter(stdout_handle, (TCHAR)' ', deleted_chars_width, end_pos, &cCharsWritten);
					else {
						SMALL_RECT scroll_rect;
						scroll_rect.Bottom = scroll_rect.Top = end_pos.Y;
						scroll_rect.Left = LEFT + deleted_chars_width;
						wch = line[operating_line_num + 1].back();
						if (!isascii(wch))
							scroll_rect.Right = line_end_pos[operating_line_num + 1] + 1;
						else
							scroll_rect.Right = line_end_pos[operating_line_num + 1];
						tmp_post_end_pos = scroll_rect.Right + 1 - deleted_chars_width;
						ScrollConsoleScreenBuffer(stdout_handle, &scroll_rect, NULL, end_pos, &blank_char);
					}
				}
			}
		}
		operating_line_num++;
	}

	//����������λ��
	for (int i = current_line_num + 1; i < line_str_pos.size(); i++) {
		line_str_pos[i] = 0;
		line_cursor_pos[i] = 0;
	}
	make_align();

	refresh_character_count();
	SetConsoleCursorPosition(stdout_handle, input_cursor_pos);
}

void MessageEditor::line_overflow(int operating_line_num, int &tmp_post_end_pos, int &overflow_chars_count, int &overflow_chars_width) {
	//�������������ɷ�Χ���֣��ŵ���һ��
	overflow_chars_count = 0;
	overflow_chars_width = 0;
	while (tmp_post_end_pos > RIGHT + 1) {
		if (operating_line_num == line_str_pos.size() - 1) { //û����һ��
			line_str_pos.push_back(0);
			line_cursor_pos.push_back(LEFT);
			line_end_pos.push_back(-1);
		}
		overflow_chars_count++;
		wch = line[operating_line_num].back();
		line[operating_line_num].pop_back();
		if (!isascii(wch)) {
			tmp_post_end_pos -= 2;
			overflow_chars_width += 2;
		}
		else {
			tmp_post_end_pos--;
			overflow_chars_width++;
		}
		line[operating_line_num + 1].insert(0, 1, wch);
	}
	if ((tmp_post_end_pos == RIGHT + 1) && (operating_line_num == line_str_pos.size() - 1)) {
		line_str_pos.push_back(0);
		line_cursor_pos.push_back(LEFT);
		line_end_pos.push_back(-1);
	}

	//����line_end_pos
	wch = line[operating_line_num].back();
	if (!isascii(wch))
		line_end_pos[operating_line_num] = tmp_post_end_pos - 2;
	else
		line_end_pos[operating_line_num] = tmp_post_end_pos - 1;
}

void MessageEditor::insert_work(wchar_t input) {
	//�������Ascii�ַ�������΢�������˽��䴦��Ϊ������֡������롰aa������������һ���֣��˴�Ϊ������ҵҪ��Ҳ��������һ�����֡�
	if (character_count >= 140)
		return;
	wch = input;
	character_count++;

	int tmp_post_end_pos;
	DWORD cCharsWritten;
	int insert_ch_width;
	if (!isascii(wch))
		insert_ch_width = 2;
	else
		insert_ch_width = 1;

	bool move_right = true;
	if ((line_str_pos[current_line_num] == 0) && (current_line_num > 0)) { //��ͼ����һ��ĩβ�������
		wch = line[current_line_num - 1].back();
		if (!isascii(wch))
			tmp_post_end_pos = line_end_pos[current_line_num - 1] + 2 + insert_ch_width;
		else
			tmp_post_end_pos = line_end_pos[current_line_num - 1] + 1 + insert_ch_width;
		if (tmp_post_end_pos <= RIGHT + 1) { //�ܲ��룬���������沽�裬����һ��ĩβ�������ֱ��ǰ��END
			COORD new_pos = { tmp_post_end_pos - insert_ch_width, input_cursor_pos.Y - 1 };
			line[current_line_num - 1].push_back(input);
			line_end_pos[current_line_num - 1] = tmp_post_end_pos - insert_ch_width;
			FillConsoleOutputCharacter(stdout_handle, input, insert_ch_width, new_pos, &cCharsWritten);
			move_right = false;
			goto END;
		}
	}

	//���ڸ��в������
	if (line[current_line_num].empty())
		tmp_post_end_pos = LEFT + insert_ch_width;
	else {
		wch = line[current_line_num].back();
		if (!isascii(wch))
			tmp_post_end_pos = line_end_pos[current_line_num] + 2 + insert_ch_width;
		else
			tmp_post_end_pos = line_end_pos[current_line_num] + 1 + insert_ch_width;
	}
	line[current_line_num].insert(line_str_pos[current_line_num], 1, input);

	int overflow_chars_count, overflow_chars_width;
	line_overflow(current_line_num, tmp_post_end_pos, overflow_chars_count, overflow_chars_width);

	//�ػ浱ǰ��
	int operating_line_num;
	if (line_cursor_pos[current_line_num] >= tmp_post_end_pos) { //ԭ���������λ��Ҳ��������һ��
		FillConsoleOutputCharacter(stdout_handle, (TCHAR)' ', RIGHT + 1 - input_cursor_pos.X, input_cursor_pos, &cCharsWritten);
		line_cursor_pos[current_line_num] = LEFT;
		line_str_pos[current_line_num] = 0;
		current_line_num++;
		line_cursor_pos[current_line_num] = LEFT;
		line_str_pos[current_line_num] = 0;
		input_cursor_pos.X = LEFT;
		input_cursor_pos.Y++;
		operating_line_num = current_line_num;
	}
	else {
		COORD new_pos = { input_cursor_pos.X + insert_ch_width, input_cursor_pos.Y };
		SMALL_RECT scroll_rect;
		scroll_rect.Bottom = scroll_rect.Top = input_cursor_pos.Y;
		scroll_rect.Left = line_cursor_pos[current_line_num];
		scroll_rect.Right = tmp_post_end_pos - 1 - insert_ch_width;
		if (scroll_rect.Left <= scroll_rect.Right) //�������β��һ����룬�������ƶ�
			ScrollConsoleScreenBuffer(stdout_handle, &scroll_rect, NULL, new_pos, &blank_char);
		FillConsoleOutputCharacter(stdout_handle, input, insert_ch_width, input_cursor_pos, &cCharsWritten);
		operating_line_num = current_line_num + 1;
	}

	//�����������������ƶ�
	while (operating_line_num < line_str_pos.size()) {
		if (overflow_chars_width == 0)
			break;
		wch = line[operating_line_num].back();
		if (!isascii(wch))
			tmp_post_end_pos = line_end_pos[operating_line_num] + 2 + overflow_chars_width;
		else
			tmp_post_end_pos = line_end_pos[operating_line_num] + 1 + overflow_chars_width;

		int tmp_overflow_chars_count, tmp_overflow_chars_width;
		line_overflow(operating_line_num, tmp_post_end_pos, tmp_overflow_chars_count, tmp_overflow_chars_width);

		//�ػ浱ǰ��
		COORD new_pos = { LEFT + overflow_chars_width, input_cursor_pos.Y + (operating_line_num - current_line_num) };
		SMALL_RECT scroll_rect;
		scroll_rect.Bottom = scroll_rect.Top = new_pos.Y;
		scroll_rect.Left = LEFT;
		scroll_rect.Right = tmp_post_end_pos - 1 - overflow_chars_width;
		if (scroll_rect.Left <= scroll_rect.Right) //�������β��һ����룬�������ƶ�
			ScrollConsoleScreenBuffer(stdout_handle, &scroll_rect, NULL, new_pos, &blank_char);
		new_pos.X = tmp_post_end_pos;
		FillConsoleOutputCharacter(stdout_handle, (TCHAR)' ', RIGHT + 1 - new_pos.X, new_pos, &cCharsWritten);

		new_pos.X = LEFT;
		int processing_ch_width;
		for (int i = 0; i < overflow_chars_count; i++) {
			wch = line[operating_line_num][i];
			if (!isascii(wch))
				processing_ch_width = 2;
			else
				processing_ch_width = 1;
			FillConsoleOutputCharacter(stdout_handle, wch, processing_ch_width, new_pos, &cCharsWritten);
			new_pos.X += processing_ch_width;
		}
		overflow_chars_count = tmp_overflow_chars_count;
		overflow_chars_width = tmp_overflow_chars_width;
		operating_line_num++;
	}

END:
	//����������λ��
	for (int i = current_line_num + 1; i < line_str_pos.size(); i++) {
		line_str_pos[i] = 0;
		line_cursor_pos[i] = 0;
	}
	make_align();

	if (move_right)
		right_work();
	refresh_character_count();
	SetConsoleCursorPosition(stdout_handle, input_cursor_pos);
}

wstring MessageEditor::get_message() {
	char ch;
	KEY_EVENT_RECORD key_event;
	while (true) {
		key_event = Application::_getkeyevent();

		//�����ƶ�ʱ������ַ�û�ж��룬���Ƚ���������ƶ�
		//�������ƶ�ʱ�����е�������λ�ã���û����ʱ���Ǳȵ�ǰ��ƫ��
		//�������ƶ�ʱΪ��֤�����ƶ����棬��˿�������Ϊ������
		//��һ���ٴν������ɴ������ƶ������ٽ���һ�������ƶ����ֻ���������Ϊ����
		switch (key_event.wVirtualKeyCode) {
		case VK_LEFT: {
			left_work();
			continue;
		}
		case VK_RIGHT: {
			right_work();
			continue;
		}
		case VK_UP: {
			up_work();
			continue;
		}
		case VK_DOWN: {
			down_work();
			continue;
		}
		case VK_HOME: {
			home_work();
			continue;
		}
		case VK_END: {
			end_work();
			continue;
		}
		case VK_DELETE: {
			delete_work();
			continue;
		}
		default:
			break;
		}

		wch = key_event.uChar.UnicodeChar;
		if (isascii(wch)) {
			ch = key_event.uChar.AsciiChar;
			if (!isprint(int((unsigned char)(ch)))) { //���ɴ�ӡ�ַ�
				if (ch == '\b' || ch == '\x7f') { //Backspace or Ctrl-Backspace
					if (current_line_num == 0 && line_str_pos[current_line_num] == 0)
						continue;
					left_work();
					delete_work();
					continue;
				}
				else if (ch == '\n') { //Ctrl-Enter
					if (character_count == 0 || character_count > 140)
						continue;
					wstring message = line[0] + line[1] + line[2] + line[3];
					if (message == wstring(message.size(), L' ')) //��Ϣ����Ϊ��
						continue;
					return message;
				}
				else
					continue;
			}
		}

		insert_work(wch);
	}
}