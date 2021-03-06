
#include "basictextfield.h"
#include "style.h"

#include "controlmanager.h"
#include "container.h"

PG_CONTROL_INITIALIZE_KEYBINDINGS(BasicTextField);

BasicTextField::BasicTextField(PGWindowHandle window, std::shared_ptr<TextView> view) :
	Control(window), view(view), prev_loaded(false), support_multiple_lines(false) {
	if (view) {
		view->SetTextField(this);
	}

	drag_type = PGDragNone;

	textfield_font = PGCreateFont(PGFontTypeTextField);
	SetTextFontSize(textfield_font, 15);
}

BasicTextField::~BasicTextField() {
}

void BasicTextField::Update(void) {
	bool loaded = view->file->IsLoaded();
	if (loaded != prev_loaded || !loaded) {
		this->Invalidate();
		prev_loaded = loaded;
	}
	prev_loaded = loaded;
	if (!WindowHasFocus(window) || !ControlHasFocus()) {
		display_carets = false;
		display_carets_count = 0;
		if (!current_focus) {
			current_focus = true;
			this->InvalidateTextField();
		}
		return;
	} else if (current_focus) {
		if (current_focus) {
			current_focus = false;
			this->InvalidateTextField();
		}
	}

	display_carets_count++;
	if (display_carets_count % FLICKER_CARET_INTERVAL == 0) {
		display_carets_count = 0;
		display_carets = !display_carets;
		this->InvalidateTextField();
	}
}

bool BasicTextField::KeyboardButton(PGButton button, PGModifier modifier) {
	if (this->PressKey(BasicTextField::keybindings, button, modifier)) {
		return true;
	}
	return false;
}

bool BasicTextField::KeyboardCharacter(char character, PGModifier modifier) {
	if (!view->file->IsLoaded()) return false;
	if (modifier == PGModifierNone) {
		view->InsertText(character);
		return true;
	} else if (this->PressCharacter(BasicTextField::keybindings, character, modifier)) {
		return true;
	}
	return false;
}

bool BasicTextField::KeyboardUnicode(PGUTF8Character character, PGModifier modifier) {
	if (!view->file->IsLoaded()) return false;

	if (modifier == PGModifierNone) {
		view->InsertText(character);
		return true;
	}
	return false;
}

PGCursorType BasicTextField::GetCursor(PGPoint mouse) {
	return PGCursorIBeam;
}

void BasicTextField::_GetCharacterFromPosition(PGScalar x, TextLine line, lng& character) {
	if (!line.IsValid()) {
		character = 0;
		return;
	}
	x -= text_offset;
	x += view->GetXOffset();
	char* text = line.GetLine();
	lng length = line.GetLength();
	character = GetPositionInLine(textfield_font, x, text, length);
}

void BasicTextField::GetLineCharacterFromPosition(PGScalar x, PGScalar y, lng& line, lng& character) {
	GetLineFromPosition(y, line);
	_GetCharacterFromPosition(x, view->file->GetLine(line), character);
}

void BasicTextField::GetLineFromPosition(PGScalar y, lng& line) {
	// find the line position of the mouse
	auto offset = view->GetLineOffset();
	lng lineoffset_y = offset.linenumber;
	PGScalar line_height = GetTextHeight(textfield_font);
	y += line_height * offset.line_fraction;
	lng line_offset = std::max(std::min((lng)(y / line_height), view->file->GetLineCount() - lineoffset_y - 1), -lineoffset_y);
	line = lineoffset_y + line_offset;
}

void BasicTextField::GetPositionFromLineCharacter(lng line, lng pos, PGScalar& x, PGScalar& y) {
	GetPositionFromLine(line, y);
	_GetPositionFromCharacter(pos, view->file->GetLine(line), x);
}

void BasicTextField::GetPositionFromLine(lng line, PGScalar& y) {
	lng lineoffset_y = view->GetLineOffset().linenumber;
	y = (line - lineoffset_y) * GetTextHeight(textfield_font);
}

void BasicTextField::_GetPositionFromCharacter(lng pos, TextLine line, PGScalar& x) {
	if (!line.IsValid()) {
		x = 0;
		return;
	}
	x = MeasureTextWidth(textfield_font, line.GetLine(), pos);
	x += text_offset;
	x -= view->GetXOffset();
}

void BasicTextField::RefreshCursors() {
	display_carets_count = 0;
	display_carets = true;
}

int BasicTextField::GetLineHeight() {
	return (int)(GetTextfieldHeight() / GetTextHeight(textfield_font)) - 1;
}

PGScalar BasicTextField::GetMaxXOffset() {
	if (view->GetWordWrap()) return 0;
	PGScalar max_textsize = view->file->GetMaxLineWidth(textfield_font);
	return std::max(max_textsize - GetTextfieldWidth() + text_offset, 0.0f);
}

void BasicTextField::SelectionChanged() {
	this->RefreshCursors();
	this->Invalidate();
}

void BasicTextField::TextChanged() {
	this->Invalidate();
}

void BasicTextField::PasteHistory() {
	std::vector<std::string> history = GetClipboardTextHistory();
	PGPopupMenuHandle menu = PGCreatePopupMenu(this->window, this);
	for (auto it = history.begin(); it != history.end(); it++) {
		PGPopupInformation info(menu);
		info.text = *it;
		if (info.text.size() > 20) {
			info.text.substr(0, 20);
		}
		panther::replace(info.text, "\n", "\\n");
		info.data = *it;
		PGPopupMenuInsertEntry(menu, info, [](Control* c, PGPopupInformation* info) {
			dynamic_cast<BasicTextField*>(c)->view->PasteText(info->data);
		});
	}
	Cursor& c = view->GetActiveCursor();
	PGScalar x, y;
	auto position = c.BeginPosition();
	GetPositionFromLineCharacter(position.line, position.position + 1, x, y);
	PGDisplayPopupMenu(menu, ConvertWindowToScreen(window,
		PGPoint(this->X() + x, this->Y() + y + GetTextHeight(textfield_font))),
		PGTextAlignLeft | PGTextAlignTop);
}


void BasicTextField::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	if (drag_type != PGDragNone && button == this->drag_button) {
		this->ClearDragging();
	}
}

void BasicTextField::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count) {
	PGPoint mouse(x - this->x, y - this->y);
	lng line, character;
	GetLineCharacterFromPosition(mouse.x, mouse.y, line, character);
	if (this->PressMouseButton(BasicTextField::mousebindings, button, mouse, modifier, click_count, line, character)) {
		return;
	}
	return;
}

void BasicTextField::MouseMove(int x, int y, PGMouseButton buttons) {
	PGPoint mouse(x - this->x, y - this->y);
	if (this->drag_type != PGDragNone) {
		if (IsDragging(mouse, buttons)) {
			if (drag_type == PGDragSelection) {
				if (!(drag_point.x == -1 && drag_point.y == -1) &&
					mouse.GetDistance(drag_point) <= 1) {
					return;
				} else {
					drag_point.x = -1;
					drag_point.y = -1;
				}
				// FIXME: when having multiple cursors and we are altering the active cursor,
				// the active cursor can never "consume" the other selections (they should always stay)
				// FIXME: this should be done in the textview
				lng line, character;
				GetLineCharacterFromPosition(mouse.x, mouse.y, line, character);
				Cursor& active_cursor = view->GetActiveCursor();
				auto selected_pos = active_cursor.SelectedCharacterPosition();
				if (selected_pos.line != line || selected_pos.character != character) {
					lng old_line = selected_pos.line;
					active_cursor.SetCursorStartLocation(line, character);
					if (minimal_selections.count(view->GetActiveCursorIndex()) > 0) {
						active_cursor.ApplyMinimalSelection(minimal_selections[view->GetActiveCursorIndex()]);
					}
					Cursor::NormalizeCursors(view.get(), view->GetCursors());
				}
			}
		} else {
			this->ClearDragging();
			this->Invalidate();
		}
	}
}

void BasicTextField::StartDragging(PGPoint initial_point, PGMouseButton button, PGDragType drag_type) {
	if (this->drag_type == PGDragNone) {
		this->drag_point = initial_point;
		this->drag_button = button;
		this->drag_type = drag_type;
		auto cursors = view->GetCursors();
		for (lng i = 0; i < cursors.size(); i++) {
			minimal_selections[i] = cursors[i].GetCursorSelection();
		}
		this->Invalidate();
	}
}

void BasicTextField::ClearDragging() {
	minimal_selections.clear();
	drag_type = PGDragNone;
}

bool BasicTextField::IsDragging(PGPoint point, PGMouseButton buttons) {
	return buttons & drag_button;
}

void BasicTextField::InitializeKeybindings() {
	std::map<std::string, PGBitmapHandle>& images = BasicTextField::keybindings_images;
	std::map<std::string, PGKeyFunction>& noargs = BasicTextField::keybindings_noargs;
	images["undo"] = PGStyleManager::GetImage("data/icons/undo.png");
	noargs["undo"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->view->Undo();
	};
	images["redo"] = PGStyleManager::GetImage("data/icons/redo.png");
	noargs["redo"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->view->Redo();
	};
	noargs["select_all"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->view->SelectEverything();
	};
	images["copy"] = PGStyleManager::GetImage("data/icons/newproject.png");
	noargs["copy"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		std::string text = t->view->CopyText();
		SetClipboardText(t->window, text);
	};
	noargs["paste"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		std::string text = GetClipboardText(t->window);
		t->view->PasteText(text);
	};
	noargs["paste_from_history"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->PasteHistory();
	};
	noargs["cut"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		std::string text = t->view->CutText();
		SetClipboardText(t->window, text);
	};
	noargs["left_delete"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->view->DeleteCharacter(PGDirectionLeft);
	};
	noargs["left_delete_word"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->view->DeleteWord(PGDirectionLeft);
	};
	noargs["left_delete_line"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->view->DeleteLine(PGDirectionLeft);
	};
	noargs["right_delete"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->view->DeleteCharacter(PGDirectionRight);
	};
	noargs["right_delete_word"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->view->DeleteWord(PGDirectionRight);
	};
	noargs["right_delete_line"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->view->DeleteLine(PGDirectionRight);
	};
	noargs["delete_selected_lines"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->view->DeleteLines();
	};
	noargs["offset_start_of_line"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->view->OffsetStartOfLine();
	};
	noargs["select_start_of_line"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->view->SelectStartOfLine();
	};
	noargs["offset_end_of_line"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->view->OffsetEndOfLine();
	};
	noargs["select_end_of_line"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->view->SelectEndOfLine();
	};
	noargs["offset_start_of_file"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->view->OffsetStartOfFile();
	};
	noargs["select_start_of_file"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->view->SelectStartOfFile();
	};
	noargs["offset_end_of_file"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->view->OffsetEndOfFile();
	};
	noargs["select_end_of_file"] = [](Control* c) {
		BasicTextField* t = (BasicTextField*)c;
		t->view->SelectEndOfFile();
	};
	std::map<std::string, PGKeyFunctionArgs>& args = BasicTextField::keybindings_varargs;
	args["insert"] = [](Control* c, std::map<std::string, std::string> args) {
		BasicTextField* tf = (BasicTextField*)c;
		if (args.count("characters") == 0) {
			return;
		}
		tf->view->PasteText(args["characters"]);
	};
	args["offset_character"] = [](Control* c, std::map<std::string, std::string> args) {
		BasicTextField* tf = (BasicTextField*)c;
		if (args.count("direction") == 0) {
			return;
		}
		bool word = args.count("word") != 0;
		bool selection = args.count("selection") != 0;
		PGDirection direction = args["direction"] == "left" ? PGDirectionLeft : PGDirectionRight;
		if (word) {
			if (selection) {
				tf->view->OffsetSelectionWord(direction);
			} else {
				tf->view->OffsetWord(direction);
			}
		} else {
			if (selection) {
				tf->view->OffsetSelectionCharacter(direction);
			} else {
				tf->view->OffsetCharacter(direction);
			}
		}
	};
	args["scroll_lines"] = [](Control* c, std::map<std::string, std::string> args) {
		BasicTextField* tf = (BasicTextField*)c;
		if (!tf->support_multiple_lines) return;
		if (args.count("amount") == 0) {
			return;
		}
		double offset = atof(args["amount"].c_str());
		if (offset != 0.0) {
			tf->view->OffsetLineOffset(offset);
			tf->Invalidate();
		}
	};
	args["offset_line"] = [](Control* c, std::map<std::string, std::string> args) {
		BasicTextField* tf = (BasicTextField*)c;
		if (!tf->support_multiple_lines) return;
		if (args.count("amount") == 0) {
			return;
		}
		bool selection = args.count("selection") != 0;
		int offset = atol(args["amount"].c_str());
		if (args.count("unit")) {
			if (args["unit"] == "page") {
				offset = offset * tf->GetLineHeight();
			}
		}
		if (offset != 0.0) {
			if (selection) {
				tf->view->OffsetSelectionLine((int)offset);
			} else {
				tf->view->OffsetLine((int)offset);
			}
		}
	};
	std::map<std::string, PGMouseFunction>& mouse_bindings = BasicTextField::mousebindings_noargs;
	mouse_bindings["add_cursor"] = [](Control* c, PGMouseButton button, PGPoint mouse, lng line, lng character) {
		BasicTextField* tf = (BasicTextField*)c;
		if (tf->drag_type != PGDragNone) return;
		tf->view->AddNewCursor(line, character);
		tf->StartDragging(mouse, button, PGDragSelection);
	};
	mouse_bindings["set_cursor_location"] = [](Control* c, PGMouseButton button, PGPoint mouse, lng line, lng character) {
		BasicTextField* tf = (BasicTextField*)c;
		if (tf->drag_type != PGDragNone) return;
		tf->view->SetCursorLocation(line, character);
		tf->StartDragging(mouse, button, PGDragSelection);
	};
	mouse_bindings["set_cursor_selection"] = [](Control* c, PGMouseButton button, PGPoint mouse, lng line, lng character) {
		BasicTextField* tf = (BasicTextField*)c;
		if (tf->drag_type != PGDragNone) return;
		tf->view->GetActiveCursor().SetCursorStartLocation(line, character);
		tf->StartDragging(mouse, button, PGDragSelection);
	};
	mouse_bindings["select_word"] = [](Control* c, PGMouseButton button, PGPoint mouse, lng line, lng character) {
		BasicTextField* tf = (BasicTextField*)c;
		if (tf->drag_type != PGDragNone) return;
		tf->view->SetCursorLocation(line, character);
		tf->view->GetActiveCursor().SelectWord();
		tf->StartDragging(mouse, button, PGDragSelection);
	};
	mouse_bindings["select_line"] = [](Control* c, PGMouseButton button, PGPoint mouse, lng line, lng character) {
		BasicTextField* tf = (BasicTextField*)c;
		if (tf->drag_type != PGDragNone) return;
		tf->view->SetCursorLocation(line, character);
		tf->view->GetActiveCursor().SelectLine();
		tf->StartDragging(mouse, button, PGDragSelection);
	};
}

void BasicTextField::InvalidateTextField() {
	Control::Invalidate(true);
}
