#pragma once

#include "button.h"
#include "container.h"
#include "scrollbar.h"
#include "simpletextfield.h"
#include "togglebutton.h"

struct SearchRank {
	lng index;
	double score;
	lng pos;
	lng subpos;
	lng text_pos;

	SearchRank(lng index, double score) : index(index), score(score), pos(-1), text_pos(-1) {}

	friend bool operator<(const SearchRank& l, const SearchRank& r) {
		return l.score < r.score;
	}
	friend bool operator> (const SearchRank& lhs, const SearchRank& rhs){ return rhs < lhs; }
	friend bool operator<=(const SearchRank& lhs, const SearchRank& rhs){ return !(lhs > rhs); }
	friend bool operator>=(const SearchRank& lhs, const SearchRank& rhs){ return !(lhs < rhs); }
	friend bool operator==(const SearchRank& lhs, const SearchRank& rhs) { return lhs.score == rhs.score; }
	friend bool operator!=(const SearchRank& lhs, const SearchRank& rhs){ return !(lhs == rhs); }
};

#define SEARCHBOX_MAX_ENTRIES 30

class SearchBox;

typedef void(*SearchBoxRenderFunction)(PGRendererHandle renderer, PGFontHandle font, SearchRank& rank, SearchEntry& entry, PGScalar& x, PGScalar& y, PGScalar button_height);
typedef void(*SearchBoxSelectionChangedFunction)(SearchBox* searchbox, SearchRank& rank, SearchEntry& entry, void* data);

class SearchBox : public PGContainer {
public:
	SearchBox(PGWindowHandle window, std::vector<SearchEntry> entries, bool render_subtitles = true);

	bool KeyboardButton(PGButton button, PGModifier modifier);

	void MouseWheel(int x, int y, double hdistance, double distance, PGModifier modifier);

	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	void Draw(PGRendererHandle renderer, PGIRect* rect);

	PGCursorType GetCursor(PGPoint mouse) { return PGCursorStandard; }

	void OnResize(PGSize old_size, PGSize new_size);

	void Close(bool success = false);

	void OnRender(SearchBoxRenderFunction func) { render_function = func; }
	void OnSelectionChanged(SearchBoxSelectionChangedFunction func, void* data) { selection_changed = func; selection_changed_data = data; }

	virtual PGControlType GetControlType() { return PGControlTypeSearchBox; }
private:
	std::vector<SearchEntry> entries;
	std::vector<SearchRank> displayed_entries;

	bool render_subtitles = false;

	PGFontHandle font;
	SimpleTextField* field;
	Scrollbar* scrollbar;

	PGScalar entry_height = 0;
	lng scroll_position = 0;
	lng selected_entry;
	lng filter_size;
	SearchBoxRenderFunction render_function = nullptr;
	SearchBoxSelectionChangedFunction selection_changed = nullptr;
	void* selection_changed_data = nullptr;

	PGScalar GetEntryHeight() { return entry_height; }
	lng GetRenderedEntries();
	void SetScrollPosition(lng entry);
	void SetSelectedEntry(lng entry);

	void Filter(std::string filter);
};