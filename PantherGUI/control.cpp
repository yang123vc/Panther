
#include "control.h"

const PGAnchor PGAnchorNone = 0x00;
const PGAnchor PGAnchorLeft = 0x01;
const PGAnchor PGAnchorRight = 0x02;
const PGAnchor PGAnchorTop = 0x04;
const PGAnchor PGAnchorBottom = 0x08;

Control::Control(PGWindowHandle handle) :
	percentage_width(-1), percentage_height(-1), fixed_height(-1), fixed_width(-1),
	dirty(false) {
	this->window = handle;
	this->x = 0;
	this->y = 0;
	this->anchor = PGAnchorNone;
	this->parent = nullptr;
	this->visible = true;
}

Control::~Control() {
	if (this->destroy_data.function) {
		this->destroy_data.function(this, destroy_data.data);
	}
}

void Control::Draw(PGRendererHandle handle) {
	this->dirty = false;
}

void Control::PeriodicRender(void) {
}

void Control::MouseWheel(int x, int y, double hdistance, double distance, PGModifier modifier) {

}

void Control::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count) {

}

void Control::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {

}


void Control::MouseMove(int x, int y, PGMouseButton buttons) {

}

bool Control::AcceptsDragDrop(PGDragDropType type) {
	return false;
}

void Control::DragDrop(PGDragDropType type, int x, int y, void* data) {

}

void Control::PerformDragDrop(PGDragDropType type, int x, int y, void* data) {

}

void Control::ClearDragDrop(PGDragDropType type) {

}

void Control::LosesFocus(void) {

}

void Control::GainsFocus(void) {

}

bool Control::KeyboardButton(PGButton button, PGModifier modifier) {
	return false;
}

bool Control::KeyboardCharacter(char character, PGModifier modifier) {
	return false;
}

bool Control::KeyboardUnicode(PGUTF8Character character, PGModifier modifier) {
	return false;
}

void Control::Invalidate(bool initial_invalidate) {
	if (initial_invalidate) {
		RefreshWindow(this->window, PGIRect((int)X() - 1, (int)Y() - 1, (int)this->width + 2, (int)this->height + 2), false);
	}
	this->dirty = true;
	if (this->parent) {
		this->parent->Invalidate(false);
	}
}

bool Control::HasFocus() {
	return GetFocusedControl(this->window) == this;
}

void Control::OnResize(PGSize old_size, PGSize new_size) {

}

void Control::LoadWorkspace(nlohmann::json& j) {

}

void Control::WriteWorkspace(nlohmann::json& j) {

}

void Control::TriggerResize() {
	this->OnResize(PGSize(this->width, this->height), PGSize(this->width, this->height));
}

void Control::SetSize(PGSize size) {
	PGSize oldsize(this->width, this->height);
	this->width = size.width;
	this->height = size.height;
	this->OnResize(oldsize, PGSize(this->width, this->height));
}

bool Control::IsDragging() {
	return false;
}

PGScalar Control::X() {
	return Position().x;
}

PGScalar Control::Y() {
	return Position().y;
}

PGPoint Control::Position() {
	PGPoint point = PGPoint(x, y);
	Control* p = parent;
	while (p) {
		point.x += p->x;
		point.y += p->y;
		p = p->parent;
	}
	return point;
}

void Control::MouseEnter() {

}

void Control::MouseLeave() {

}

void Control::ResolveSize(PGSize new_size) {
	if (size_resolved) return;
	PGSize current_size = PGSize(this->width, this->height);
	if (fixed_height >= 0 || percentage_height >= 0) {
		if (fixed_height > 0) {
			this->height = fixed_height;
		}
		if (top_anchor != nullptr && anchor & PGAnchorTop) {
			top_anchor->ResolveSize(new_size);
			this->y = top_anchor->y + top_anchor->height;
			if (percentage_height > 0) {
				PGScalar remaining_height = new_size.height - this->y;
				if (bottom_anchor != nullptr) {
					bottom_anchor->ResolveSize(new_size);
					remaining_height = bottom_anchor->y - this->y;
				}
				this->height = percentage_height * remaining_height;
			}
		} else if (bottom_anchor != nullptr && anchor & PGAnchorBottom) {
			bottom_anchor->ResolveSize(new_size);
			if (percentage_height > 0) {
				PGScalar remaining_height = bottom_anchor->y;
				if (top_anchor != nullptr) {
					top_anchor->ResolveSize(new_size);
					remaining_height -= top_anchor->y + top_anchor->height;
				}
				this->height = percentage_height * remaining_height;
			}
			this->y = bottom_anchor->y - this->height;
		} else {
			if (anchor & PGAnchorTop) {
				this->y = 0;
			} else if (anchor & PGAnchorBottom) {
				assert(this->fixed_height > 0);
				this->y = new_size.height - this->fixed_height;
			} else {
				assert(0);
			}
		}
	}
	if (fixed_width >= 0 || percentage_width >= 0) {
		if (horizontal_anchor == nullptr) {
			if (fixed_width >= 0) {
				this->width = fixed_width;
			} else {
				assert(percentage_width > 0);
				this->width = percentage_width * new_size.width;
			}
			if (this->anchor & PGAnchorLeft) {
				this->x = 0;
			}
			if (this->anchor & PGAnchorRight) {
				// right anchor only makes sense with a fixed width
				assert(this->fixed_width > 0);
				this->x = new_size.width - this->width;
			}
		} else {
			horizontal_anchor->ResolveSize(new_size);
			PGScalar remaining_width = 0;
			if (fixed_width > 0) {
				this->width = fixed_width;
				assert(percentage_width < 0);
			}
			if (this->anchor & PGAnchorLeft) {
				this->x = horizontal_anchor->x + horizontal_anchor->width;
				remaining_width = new_size.width - this->x;
			}
			if (this->anchor & PGAnchorRight) {
				remaining_width = horizontal_anchor->x;
			}
			if (percentage_width > 0) {
				this->width = percentage_width * remaining_width;
			}
			if (this->anchor & PGAnchorRight) {
				this->x = horizontal_anchor->x - this->width;
			}
			this->size_resolved = true;
		}
	}
	this->size_resolved = true;
	this->OnResize(current_size, PGSize(this->width, this->height));
}

bool Control::PressKey(std::map<PGKeyPress, PGKeyFunctionCall>& keybindings, PGButton button, PGModifier modifier) {
	PGKeyPress press;
	press.button = button;
	press.modifier = modifier;
	if (keybindings.count(press) > 0) {
		keybindings[press].Call(this);
		return true;
	}
	return false;
}

bool Control::PressCharacter(std::map<PGKeyPress, PGKeyFunctionCall>& keybindings, char character, PGModifier modifier) {
	PGKeyPress press;
	press.character = character;
	press.modifier = modifier;
	if (keybindings.find(press) != keybindings.end()) {
		keybindings[press].Call(this);
		return true;
	}
	return false;
}

bool Control::PressMouseButton(std::map<PGMousePress, PGMouseFunctionCall>& mousebindings, PGMouseButton button, PGPoint mouse, PGModifier modifier, int clicks, lng line, lng character) {
	PGMousePress press;
	press.button = button;
	press.modifier = modifier;
	press.clicks = clicks;
	if (mousebindings.find(press) != mousebindings.end()) {
		mousebindings[press].Call(this, button, mouse, line, character);
		return true;
	}
	return false;
}
