#pragma once

char* PANTHER_DEFAULT_KEYBINDINGS = R"DEFAULTSETTINGS(
{
	"global": [
		{ "key": "ctrl+shift+n", "command": "new_window" },
		{ "key": "ctrl+shift+w", "command": "close_window" },

		{ "key": "ctrl+f", "command": "show_find", "args": {"type": "find"} },
		{ "key": "ctrl+h", "command": "show_find", "args": {"type": "findreplace"} },
		{ "key": "ctrl+shift+f", "command": "show_find", "args": {"type": "findinfiles"} },

		{ "key": "ctrl+shift+1", "command": "set_textfield_layout", "args": {"columns": "1", "rows": "1"} },
		{ "key": "ctrl+shift+2", "command": "set_textfield_layout", "args": {"columns": "2", "rows": "1"} },
		{ "key": "ctrl+shift+3", "command": "set_textfield_layout", "args": {"columns": "1", "rows": "2"} },
		{ "key": "ctrl+shift+4", "command": "set_textfield_layout", "args": {"columns": "2", "rows": "2"} }
	],
	"basictextfield": [
		{ "key": "backspace", "command": "left_delete" },
		{ "key": "shift+backspace", "command": "left_delete" },
		{ "key": "ctrl+backspace", "command": "left_delete_word" },
		{ "key": "ctrl+shift+backspace", "command": "left_delete_line" },
		{ "key": "delete", "command": "right_delete" },
		{ "key": "ctrl+delete", "command": "right_delete_word" },
		{ "key": "ctrl+shift+delete", "command": "right_delete_line" },
		{ "key": "shift+delete", "command": "delete_selected_lines" },


		{ "key": "tab", "command": "insert", "args": { "characters": "\t" } },

		{ "key": "ctrl+z", "command": "undo" },
		{ "key": "ctrl+shift+z", "command": "redo" },
		{ "key": "ctrl+y", "command": "redo" },

		{ "key": "ctrl+[", "command": "undo_selection" },
		{ "key": "ctrl+]", "command": "redo_selection" },

		{ "key": "ctrl+x", "command": "cut" },
		{ "key": "ctrl+c", "command": "copy" },
		{ "key": "ctrl+v", "command": "paste" },
		{ "key": "ctrl+shift+v", "command": "paste_from_history" },

		{ "key": "ctrl+a", "command": "select_all" },

		{ "key": "insert", "command": "toggle_overwrite" },

		{ "key": "ctrl+backspace", "command": "delete_word", "args": { "forward": false } },
		{ "key": "ctrl+shift+backspace", "command": "delete_line", "args": { "forward": false } },
		{ "key": "ctrl+delete", "command": "delete_word", "args": { "forward": true } },
		{ "key": "ctrl+shift+delete", "command": "delete_line", "args": { "forward": true } },

		{ "key": "left", "command": "offset_character", "args": {"direction": "left"}},
		{ "key": "right", "command": "offset_character", "args": {"direction": "right"}},
		{ "key": "shift+left", "command": "offset_character", "args": {"direction": "left", "selection": true}},
		{ "key": "shift+right", "command": "offset_character", "args": {"direction": "right", "selection": true}},

		{ "key": "ctrl+left", "command": "offset_character", "args": {"direction": "left", "word": true}},
		{ "key": "ctrl+right", "command": "offset_character", "args": {"direction": "right", "word": true}},
		{ "key": "ctrl+shift+left", "command": "offset_character", "args": {"direction": "left", "selection": true, "word": true}},
		{ "key": "ctrl+shift+right", "command": "offset_character", "args": {"direction": "right", "selection": true, "word": true}},

		{ "key": "home", "command": "offset_start_of_line"},
		{ "key": "shift+home", "command": "select_start_of_line"},
		{ "key": "ctrl+home", "command": "offset_start_of_file"},
		{ "key": "ctrl+shift+home", "command": "select_start_of_file"},

		{ "key": "end", "command": "offset_end_of_line"},
		{ "key": "shift+end", "command": "select_end_of_line"},
		{ "key": "ctrl+end", "command": "offset_end_of_file"},
		{ "key": "ctrl+shift+end", "command": "select_end_of_file"},

		{ "key": "ctrl+up", "command": "scroll_lines", "args": {"amount": -1.0 } },
		{ "key": "ctrl+down", "command": "scroll_lines", "args": {"amount": 1.0 } },
		{ "key": "up", "command": "offset_line", "args": {"amount": -1.0 } },
		{ "key": "shift+up", "command": "offset_line", "args": {"amount": -1.0, "selection": true } },
		{ "key": "down", "command": "offset_line", "args": {"amount": 1.0 } },
		{ "key": "shift+down", "command": "offset_line", "args": {"amount": 1.0, "selection": true } },

		{ "key": "pageup", "command": "offset_line", "args": {"amount": -1.0, "unit": "page" } },
		{ "key": "shift+pageup", "command": "offset_line", "args": {"amount": -1.0, "unit": "page", "selection": true } },
		{ "key": "pagedown", "command": "offset_line", "args": {"amount": 1.0, "unit": "page" } },
		{ "key": "shift+pagedown", "command": "offset_line", "args": {"amount": 1.0, "unit": "page", "selection": true } },


		{ "mouse": "ctrl+left", "command": "select_word" },
		{ "mouse": "shift+left", "command": "set_cursor_selection" },
		{ "mouse": "left", "clicks": 1, "command": "set_cursor_location" },
		{ "mouse": "left", "clicks": 2, "command": "select_word" },
		{ "mouse": "left", "clicks": 3, "command": "select_line" }
			
	],
	"simpletextfield": [
		{ "key": "ctrl+enter", "command": "insert", "args": { "characters": "\n" } },
		{ "key": "up", "command": "prev_entry"},
		{ "key": "down", "command": "next_entry"}
	],
	"textfield": [
		{ "key": "ctrl+s", "command": "save" },
		{ "key": "ctrl+shift+s", "command": "save_as" },
		{ "key": "ctrl+g", "command": "show_goto", "args": {"type": "line"} },
		{ "key": "ctrl+p", "command": "show_goto", "args": {"type": "file"} },

		{ "key": "enter", "command": "insert", "args": { "characters": "\n" } },
		{ "key": "shift+enter", "command": "insert", "args": { "characters": "\n" } },
		{ "key": "ctrl+enter", "command": "insert_newline_before" },
		{ "key": "ctrl+shift+enter", "command": "insert_newline_after" },

		{ "key": "ctrl+shift+up", "command": "swap_line_up" },
		{ "key": "ctrl+shift+down", "command": "swap_line_down" },
		{ "key": "ctrl+/", "command": "toggle_comment", "args": { "block": false } },
		{ "key": "ctrl+shift+/", "command": "toggle_comment", "args": { "block": true } },
		{ "key": "ctrl++", "command": "increase_font_size" },
		{ "key": "ctrl+=", "command": "increase_font_size" },
		{ "key": "ctrl+keypad_plus", "command": "increase_font_size" },
		{ "key": "ctrl+-", "command": "decrease_font_size" },
		{ "key": "ctrl+keypad_minus", "command": "decrease_font_size" },
		{ "key": "ctrl+equals", "command": "increase_font_size" },
		{ "key": "ctrl+shift+equals", "command": "decrease_font_size" },
		{ "key": "ctrl+shift+keypad_plus", "command": "decrease_font_size" },

		{ "key": "tab", "command": "increase_indent"},
		{ "key": "shift+tab", "command": "decrease_indent"},

		{ "mouse": "middle", "command": "drag_region" }
	],
	"tabcontrol": [
		{ "key": "ctrl+o", "command": "open_file" },
		
		{ "key": "ctrl+w", "command": "close_tab" },
		{ "key": "ctrl+n", "command": "new_tab" },
		{ "key": "ctrl+shift+t", "command": "reopen_last_file" },

		{ "key": "ctrl+pagedown", "command": "next_tab" },
		{ "key": "ctrl+pageup", "command": "prev_tab" },
		{ "key": "ctrl+tab", "command": "next_tab" },
		{ "key": "ctrl+shift+tab", "command": "prev_tab" }
	],
	"findtext": [
		{"key": "enter", "command": "find_next"},
		{"key": "shift+enter", "command": "find_prev"},
		{"key": "tab", "command": "shift_focus_forward"},
		{"key": "shift+tab", "command": "shift_focus_backward"},


		{"key": "ctrl+r", "command": "toggle_regex"},
		{"key": "ctrl+c", "command": "toggle_matchcase"},
		{"key": "ctrl+w", "command": "toggle_wholeword"},
		{"key": "ctrl+z", "command": "toggle_wrap"},
		{"key": "ctrl+h", "command": "toggle_highlight"},

		{"key": "escape", "command": "close"}
	],
	"goto": [
		{ "key": "enter", "command": "confirm" },
		{ "key": "escape", "command": "cancel" }
	],
	"searchbox": [
		{ "key": "enter", "command": "confirm" },
		{ "key": "escape", "command": "cancel" }
	]
}
)DEFAULTSETTINGS";


