
#include "unicode.h"

lng utf8_strlen(std::string& utf8_str) {
	lng utf8_char_count = 0;
	lng i = 0;
	//we traverse the string and simply count the amount of utf8 characters in the string
	for (lng i = 0; i < (lng)utf8_str.size(); ) {
		int offset = utf8_character_length(utf8_str[i]);
		if (offset < 0 || i + offset > (lng)utf8_str.size()) {
			return -1; //invalid utf8 character
		}
		i += offset;
		utf8_char_count++;
	}
	return utf8_char_count;
}

int utf8_character_length(unsigned char utf8_char) {
	//the first byte tells us how many bytes the utf8 character uses
	if      (utf8_char < 0x80) return 1;
	else if (utf8_char < 0xe0) return 2;
	else if (utf8_char < 0xf0) return 3;
	else if (utf8_char < 0xf8) return 4;
	else return -1; //invalid utf8 character, the maximum value of the first byte is 0xf7
}

lng utf8_prev_character(char* text, lng current_character) {
	if (current_character == 0) return -1;
	do {
		current_character--;
	} while ((text[current_character] & 0xC0) == 0x80);
	return current_character;
}

lng utf8_character_number(char* text, lng position) {
	if (position == 0) return 0;
	lng character = 0;
	for (; position > 0; position--) {
		if ((text[position] & 0xC0) != 0x80) {
			character++;
		}
	}
	return character;
}