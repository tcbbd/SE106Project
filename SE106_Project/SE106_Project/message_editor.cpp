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
	//message = L"a一二三四五六七八九十一二三四五六七八九十一二三四五六七八九十一二三四五六七八九十一二三四五六七八九十一二三四五六七八九十一二三四五六七八九十一二三四五六七八九a十一二三四五六七八九十一二三四五六七八九十一二三四五六七八九十一二三四五六七八a";
	//只有当with_frame为false时，即转发输入界面时，才会需要利用message中的内容进行初始化
	int pos_x = 0; //待写位置，最后一个字符后一格
	int pos_y = 0;
	line_end_pos = vector<int>(1, -1); //end_pos指最后一个字的位置(占两格的字居于前一格)，-1表示该行为空行
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
	//在该行字符串中的位置，若为line.size()表示该行最后一个字的后面一格的位置(只有在最后一行会出现此情况)
	line_str_pos = vector<int>(line_end_pos.size(), 0);
	line_cursor_pos = vector<int>(line_end_pos.size(), LEFT); //这两个vector，以及line_end_pos的size将始终保持相等
	blank_char.Char.UnicodeChar = L' ';
	blank_char.Attributes = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;

	SetConsoleCursorPosition(stdout_handle, input_cursor_pos);
}

void MessageEditor::make_align() { //只要求cursor_pos能与str_pos匹配即可调整至对齐
	int tmp_cursor_pos;
	for (int i = 0; i < line_str_pos.size(); i++) {
		if (i == current_line_num)
			continue;
		tmp_cursor_pos = line_cursor_pos[i];
		while (tmp_cursor_pos > line_cursor_pos[current_line_num]) { //处在右侧，尝试后退一个字
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
		while (tmp_cursor_pos < line_cursor_pos[current_line_num]) { //处在左侧，尝试前进一个字
			if (line_str_pos[i] == line[i].size()) //最后一行行尾后一格，无法再前进
				break;
			if ((line_str_pos[i] == line[i].size() - 1) //行尾
				&& (i != line_str_pos.size() - 1)) //但不是最后一行，无法再前进
				break;
			wch = line[i][line_str_pos[i]];
			if (!isascii(wch))
				tmp_cursor_pos += 2;
			else
				tmp_cursor_pos++;
			if (tmp_cursor_pos > line_cursor_pos[current_line_num]) //前进一个字后来到右侧，放弃该步移动，采用当前位置
				break;
			line_str_pos[i]++;
			line_cursor_pos[i] = tmp_cursor_pos;
		}
	}
}

void MessageEditor::left_work() {
	if (line_str_pos[current_line_num] == 0) { //位于行首
		if (current_line_num == 0)
			return;
		current_line_num--;
		input_cursor_pos.Y--;
		input_cursor_pos.X = line_end_pos[current_line_num];
		for (int i = 0; i < line_str_pos.size(); i++) {
			line_str_pos[i] = line[i].size() - 1;
			line_cursor_pos[i] = line_end_pos[i];
			if (i == line_str_pos.size() - 1) { //最后一行光标可以移至行尾后一格
				if (line[i].empty()) //最后一行是空行
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
	if (line_str_pos[current_line_num] == line[current_line_num].size()) //位于最后一行行尾后一格
		return;
	if ((line_str_pos[current_line_num] == line[current_line_num].size() - 1) && //来到该行结尾
		(current_line_num != line_str_pos.size() - 1)) { //且不是最后一行
		current_line_num++;
		input_cursor_pos.Y++;
		for (auto& i : line_str_pos)
			i = 0;
		for (auto& i : line_cursor_pos)
			i = LEFT;
		input_cursor_pos.X = LEFT;
	}
	else { //在一行中间，或者在最后一行的行尾
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
		if (line[current_line_num].empty()) //该行是空行
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
	if (current_line_num == line_str_pos.size() - 1) { //最后一行
		line_str_pos[current_line_num]++;
		if (line[current_line_num].empty()) //最后一行是空行
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

void MessageEditor::delete_work() { //str_pos和cursor_pos不变
	int tmp_post_end_pos;
	DWORD cCharsWritten;

	if (line_str_pos[current_line_num] == line[current_line_num].size()) //最后一行行尾后一格
		return;
	character_count--;
	wch = line[current_line_num][line_str_pos[current_line_num]];
	int deleted_ch_width;
	if (!isascii(wch))
		deleted_ch_width = 2;
	else
		deleted_ch_width = 1;
	//在该行删去当前光标位置的字符，并将后面的字符向前移动
	if (line_str_pos[current_line_num] == line[current_line_num].size() - 1) { //当前位置在行尾
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

	//将后面的行依次往前移动(每次操作某行时，从后一行提取若干字符，并将后一行左移)
	int operating_line_num = current_line_num;
	while (operating_line_num < line_str_pos.size()) {
		if (operating_line_num == line_str_pos.size() - 1) { //最后一行，只需设置line_end_pos
			if (line[operating_line_num].empty()) //最后一行被删成空行
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
			while (tmp_post_end_pos <= RIGHT) { //从下一行借取若干字接在此行最后
				if (line[operating_line_num + 1].empty()) { //下一行已经被取空，将下一行删去
					line_str_pos.pop_back();
					line_cursor_pos.pop_back();
					line_end_pos.pop_back();
					break;
				}
				else { //否则取出一个字，接在该行后
					wch = line[operating_line_num + 1].front();
					if (!isascii(wch))
						tmp_char_width = 2;
					else
						tmp_char_width = 1;
					tmp_post_end_pos += tmp_char_width;
					if (tmp_post_end_pos > RIGHT + 1) { //太长，此步操作取消
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

			//设置line_end_pos
			wch = line[operating_line_num].back();
			if (!isascii(wch))
				line_end_pos[operating_line_num] = tmp_post_end_pos - 2;
			else
				line_end_pos[operating_line_num] = tmp_post_end_pos - 1;

			if (deleted_chars_width == 0) { //未从下一行取走字符
				if ((operating_line_num == current_line_num) && //光标所在的行
					(line_cursor_pos[current_line_num] > line_end_pos[current_line_num]) && //光标位置上已不存在字符
					(current_line_num + 1 < line_str_pos.size())) { //下一行存在，光标应移动到下一行
					line_str_pos[current_line_num] = 0;
					line_cursor_pos[current_line_num] = LEFT;
					current_line_num++;
					line_str_pos[current_line_num] = 0;
					line_cursor_pos[current_line_num] = LEFT;
					input_cursor_pos.X = LEFT;
					input_cursor_pos.Y++;
				}
				break; //无需对后面的行再进行操作
			}
			else { //取出了下一行的字符，因此将下一行的内容左移
				if (operating_line_num + 1 < tmp_line_count) { //如果下一行还存在
					end_pos.Y++;
					end_pos.X = LEFT;
					if (line[operating_line_num + 1].empty()) //下一行为空
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

	//对齐虚拟光标位置
	for (int i = current_line_num + 1; i < line_str_pos.size(); i++) {
		line_str_pos[i] = 0;
		line_cursor_pos[i] = 0;
	}
	make_align();

	refresh_character_count();
	SetConsoleCursorPosition(stdout_handle, input_cursor_pos);
}

void MessageEditor::line_overflow(int operating_line_num, int &tmp_post_end_pos, int &overflow_chars_count, int &overflow_chars_width) {
	//将超过该行容纳范围的字，放到下一行
	overflow_chars_count = 0;
	overflow_chars_width = 0;
	while (tmp_post_end_pos > RIGHT + 1) {
		if (operating_line_num == line_str_pos.size() - 1) { //没有下一行
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

	//重设line_end_pos
	wch = line[operating_line_num].back();
	if (!isascii(wch))
		line_end_pos[operating_line_num] = tmp_post_end_pos - 2;
	else
		line_end_pos[operating_line_num] = tmp_post_end_pos - 1;
}

void MessageEditor::insert_work(wchar_t input) {
	//如果输入Ascii字符，新浪微博、人人将其处理为半个“字”，输入“aa”算作输入了一个字，此处为符合作业要求也将其视作一个“字”
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
	if ((line_str_pos[current_line_num] == 0) && (current_line_num > 0)) { //试图在上一行末尾插入该字
		wch = line[current_line_num - 1].back();
		if (!isascii(wch))
			tmp_post_end_pos = line_end_pos[current_line_num - 1] + 2 + insert_ch_width;
		else
			tmp_post_end_pos = line_end_pos[current_line_num - 1] + 1 + insert_ch_width;
		if (tmp_post_end_pos <= RIGHT + 1) { //能插入，则跳过后面步骤，在上一行末尾插入完后直接前往END
			COORD new_pos = { tmp_post_end_pos - insert_ch_width, input_cursor_pos.Y - 1 };
			line[current_line_num - 1].push_back(input);
			line_end_pos[current_line_num - 1] = tmp_post_end_pos - insert_ch_width;
			FillConsoleOutputCharacter(stdout_handle, input, insert_ch_width, new_pos, &cCharsWritten);
			move_right = false;
			goto END;
		}
	}

	//先在该行插入该字
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

	//重绘当前行
	int operating_line_num;
	if (line_cursor_pos[current_line_num] >= tmp_post_end_pos) { //原来光标所在位置也被顶到下一行
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
		if (scroll_rect.Left <= scroll_rect.Right) //如果在行尾后一格插入，则无需移动
			ScrollConsoleScreenBuffer(stdout_handle, &scroll_rect, NULL, new_pos, &blank_char);
		FillConsoleOutputCharacter(stdout_handle, input, insert_ch_width, input_cursor_pos, &cCharsWritten);
		operating_line_num = current_line_num + 1;
	}

	//将后面的行依次向后移动
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

		//重绘当前行
		COORD new_pos = { LEFT + overflow_chars_width, input_cursor_pos.Y + (operating_line_num - current_line_num) };
		SMALL_RECT scroll_rect;
		scroll_rect.Bottom = scroll_rect.Top = new_pos.Y;
		scroll_rect.Left = LEFT;
		scroll_rect.Right = tmp_post_end_pos - 1 - overflow_chars_width;
		if (scroll_rect.Left <= scroll_rect.Right) //如果在行尾后一格插入，则无需移动
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
	//对齐虚拟光标位置
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

		//上下移动时，如果字符没有对齐，优先将光标向左移动
		//即左右移动时其他行的虚拟光标位置，在没对齐时总是比当前行偏左
		//在上下移动时为保证上下移动可逆，因此可能体现为会向右
		//但一旦再次进行若干次左右移动，则再进行一次上下移动，又会重新体现为向左
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
			if (!isprint(int((unsigned char)(ch)))) { //不可打印字符
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
					if (message == wstring(message.size(), L' ')) //消息不能为空
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