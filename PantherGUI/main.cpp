

#include "main.h"
#include "windowfunctions.h"
#include "control.h"
#include "scheduler.h"
#include "filemanager.h"
#include "container.h"
#include "droptarget.h"
#include "encoding.h"
#include "unicode.h"
#include "language.h"
#include "logger.h"
#include "style.h"

#include "statusbar.h"
#include "textfield.h"
#include "tabcontrol.h"
#include "projectexplorer.h"

#include "c.h"
#include "findresults.h"
#include "xml.h"

#include <malloc.h>

#include <shobjidl.h>
#include <windowsx.h>
#include <vector>
#include <map>

#include "renderer.h"
#include "windows_structs.h"

#include "keybindings.h"
#include "settings.h"
#include "globalsettings.h"

#include "dirent.h"

#include "splitter.h"

void DestroyWindow(PGWindowHandle window);

std::vector<PGWorkspace*> open_workspaces;

std::map<HWND, PGWindowHandle> handle_map = {};
WNDCLASSEX wcex;
int cmdshow;

HCURSOR cursor_standard = nullptr;
HCURSOR cursor_crosshair = nullptr;
HCURSOR cursor_hand = nullptr;
HCURSOR cursor_ibeam = nullptr;
HCURSOR cursor_wait = nullptr;
HCURSOR cursor_horizontal_resize = nullptr;
HCURSOR cursor_vertical_resize = nullptr;

static TCHAR szWindowClass[] = _T("Panther");
static TCHAR szTitle[] = _T("Panther");

#define MAX_REFRESH_FREQUENCY 1000/30

#ifndef PANTHER_TESTS
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	OleInitialize(nullptr);
	cmdshow = nCmdShow;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = (HICON)LoadImage( // returns a HANDLE so we have to cast to HICON
		nullptr,             // hInstance must be NULL when loading from a file
		"logo.ico",       // the icon file name
		IMAGE_ICON,       // specifies that the file is an icon
		0,                // width of the image (we'll specify default later on)
		0,                // height of the image
		LR_LOADFROMFILE |  // we want to load a file (as opposed to a resource)
		LR_DEFAULTSIZE |   // default metrics based on the type (IMAGE_ICON, 32x32)
		LR_SHARED         // let the system release the handle when it's no longer used
	);
	wcex.hCursor = NULL;
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

	if (!RegisterClassEx(&wcex)) {
		MessageBox(nullptr,
			_T("Call to RegisterClassEx failed!"),
			_T("Win32 Guided Tour"),
			0);

		return 1;
	}

	cursor_standard = LoadCursor(nullptr, IDC_ARROW);
	cursor_crosshair = LoadCursor(nullptr, IDC_CROSS);
	cursor_hand = LoadCursor(nullptr, IDC_HAND);
	cursor_ibeam = LoadCursor(nullptr, IDC_IBEAM);
	cursor_wait = LoadCursor(nullptr, IDC_WAIT);
	cursor_horizontal_resize = LoadCursor(nullptr, IDC_SIZEWE);
	cursor_vertical_resize = LoadCursor(nullptr, IDC_SIZENS);

	PGLanguageManager::AddLanguage(new CLanguage());
	PGLanguageManager::AddLanguage(new XMLLanguage());
	PGLanguageManager::AddLanguage(new FindResultsLanguage());

	PGSettingsManager::Initialize();
	PGKeyBindingsManager::Initialize();
	PGGlobalSettings::Initialize("globalsettings.json");

	Scheduler::Initialize();
	Scheduler::SetThreadCount(2);

	// load a workspace
	auto settings = PGGlobalSettings::GetSettings();
	if (settings.count("workspaces") == 0 || !settings["workspaces"].is_array()) {
		// no known workspaces in the settings, initialize the settings
		settings["workspaces"] = nlohmann::json::array();
	}
	lng active_workspace = 0;
	if (settings.count("active_workspace") != 0 && settings["active_workspace"].is_number()) {
		active_workspace = min(max(0LL, settings["active_workspace"].get<lng>()), settings["workspaces"].array().size() - 1);
	} else {
		settings["active_workspace"] = 0;
		active_workspace = 0;
	}
	if (settings["workspaces"].array().size() == 0) {
		// no known workspaces, add one
		settings["workspaces"][0] = "workspace.json";
		settings["active_workspace"] = 0;
		active_workspace = 0;
	}

	std::string workspace_path = settings["workspaces"][active_workspace];
	// load the active workspace
	PGWorkspace* workspace = new PGWorkspace();
	workspace->LoadWorkspace(workspace_path);
	open_workspaces.push_back(workspace);
	/*
	auto textfile = std::shared_ptr<TextFile>(new TextFile(nullptr));
	std::vector<std::shared_ptr<TextFile>> files;
	files.push_back(textfile);

	PGWindowHandle handle = PGCreateWindow(files);
	handle->workspace.LoadWorkspace("workspace.json");*/
	auto windows = workspace->GetWindows();
	for (auto it = windows.begin(); it != windows.end(); it++) {
		ShowWindow((*it)->hwnd, cmdshow);
		UpdateWindow((*it)->hwnd);
	}

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}
#endif

static void LogLastError() {
	DWORD dw = GetLastError();
	LPVOID lpMsgBuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	Logger::WriteLogMessage(std::string((char*)lpMsgBuf));
}

size_t UCS2Length(char* data) {
	int i = 0;
	while (true) {
		if (!data[i] && !data[i + 1]) {
			return i;
		}
		i += 2;
	}
}

std::string UCS2toUTF8(PWSTR data) {
	std::string text = std::string(((char*)data), UCS2Length((char*)data));
	char* result;
	// on Windows we assume the text on the clipboard is encoded in UTF16: convert to UTF8
	size_t length = PGConvertText(text, &result, PGEncodingUTF16Platform, PGEncodingUTF8);
	assert(length > 0);
	text = std::string(result, length);
	free(result);
	return text;
}

std::string UTF8toUCS2(std::string text) {
	char* result = nullptr;
	size_t length = PGConvertText(text, &result, PGEncodingUTF8, PGEncodingUTF16Platform);
	assert(length > 0);
	// UCS 2 strings have to be terminated by two null-terminators
	// but the string we get is only terminated by one
	// so we add an extra null-terminator
	char* temporary_buffer = (char*)calloc(length + 2, 1);
	memcpy(temporary_buffer, result, length);
	free(result);
	std::string res = std::string(temporary_buffer, length + 2);
	free(temporary_buffer);
	return res;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	PGWindowHandle handle = GetHWNDHandle(hWnd);
	if (!handle) {
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	switch (message) {
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			if (ps.rcPaint.left == ps.rcPaint.right || ps.rcPaint.top == ps.rcPaint.bottom) {
				EndPaint(hWnd, &ps);
				break;
			}

			RenderControlsToBitmap(handle->renderer, handle->bitmap, PGIRect(0, 0, handle->manager->width, handle->manager->height), handle->manager, 1);

			SkBitmap* bitmap = &handle->bitmap;
			SkBitmap subset_bitmap;
			if (ps.rcPaint.left != 0 || ps.rcPaint.right != bitmap->width() || ps.rcPaint.bottom != bitmap->height() || ps.rcPaint.top != 0) {
				SkIRect r;
				r.fLeft = ps.rcPaint.left;
				r.fTop = ps.rcPaint.top;
				r.fRight = ps.rcPaint.right;
				r.fBottom = ps.rcPaint.bottom;

				SkBitmap subset;
				if (!handle->bitmap.extractSubset(&subset, r)) {
					EndPaint(hWnd, &ps);
					break;
				}
				if (!subset.deepCopyTo(&subset_bitmap)) {
					EndPaint(hWnd, &ps);
					break;
				}
				bitmap = &subset_bitmap;
			}
			BITMAPINFO bmi;
			memset(&bmi, 0, sizeof(bmi));
			bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmi.bmiHeader.biWidth = bitmap->width();
			bmi.bmiHeader.biHeight = -bitmap->height(); // top-down image
			bmi.bmiHeader.biPlanes = 1;
			bmi.bmiHeader.biBitCount = 32;
			bmi.bmiHeader.biCompression = BI_RGB;
			bmi.bmiHeader.biSizeImage = 0;

			bitmap->lockPixels();
			int ret = SetDIBitsToDevice(hdc,
				ps.rcPaint.left, ps.rcPaint.top,
				bitmap->width(), bitmap->height(),
				0, 0,
				0, bitmap->height(),
				bitmap->getPixels(),
				&bmi,
				DIB_RGB_COLORS);
			(void)ret; // we're ignoring potential failures for now.
			bitmap->unlockPixels();
			EndPaint(hWnd, &ps);
			break;
		}
		case WM_MEASUREITEM:
		{
			auto measure_item = (LPMEASUREITEMSTRUCT)lParam;
			PGPopupInformation* info = (PGPopupInformation*)measure_item->itemData;
			PGSize size;
			size = PGMeasurePopupItem(info->handle->font, info, info->handle->text_size, info->handle->hotkey_size, info->type);

			measure_item->itemWidth = size.width;
			measure_item->itemHeight = size.height;
			break;
		}
		case WM_DRAWITEM:
		{
			auto render_item = (LPDRAWITEMSTRUCT)lParam;
			PGPopupInformation* info = (PGPopupInformation*)render_item->itemData;
			PGIRect rect(
				render_item->rcItem.left,
				render_item->rcItem.top,
				render_item->rcItem.right - render_item->rcItem.left,
				render_item->rcItem.bottom - render_item->rcItem.top);
			PGBitmapHandle bmp = CreateBitmapFromSize(rect.width, rect.height);
			PGRendererHandle renderer = CreateRendererForBitmap(bmp);

			PGPopupMenuFlags flags = 0;
			if (render_item->itemState & ODS_DISABLED || render_item->itemState & ODS_GRAYED) {
				flags |= PGPopupMenuGrayed;
			}
			if (render_item->itemState & ODS_HOTLIGHT) {
				flags |= PGPopupMenuHighlighted;
			}
			if (render_item->itemState & ODS_SELECTED) {
				flags |= PGPopupMenuSelected;
			}

			PGRenderPopupItem(renderer, PGPoint(0, 0), info->handle->font, info, PGSize(rect.width, rect.height), flags, info->handle->text_size, info->handle->hotkey_size, info->type);

			SkBitmap& bitmap = *(bmp->bitmap);
			BITMAPINFO bmi;
			memset(&bmi, 0, sizeof(bmi));
			bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmi.bmiHeader.biWidth = bitmap.width();
			bmi.bmiHeader.biHeight = -bitmap.height(); // top-down image
			bmi.bmiHeader.biPlanes = 1;
			bmi.bmiHeader.biBitCount = 32;
			bmi.bmiHeader.biCompression = BI_RGB;
			bmi.bmiHeader.biSizeImage = 0;

			assert(bitmap.width() * bitmap.bytesPerPixel() == bitmap.rowBytes());
			bitmap.lockPixels();
			int ret = SetDIBitsToDevice(render_item->hDC,
				rect.x, rect.y,
				bitmap.width(), bitmap.height(),
				0, 0,
				0, bitmap.height(),
				bitmap.getPixels(),
				&bmi,
				DIB_RGB_COLORS);
			(void)ret; // we're ignoring potential failures for now.
			bitmap.unlockPixels();
			break;
		}
		case WM_SETFOCUS:
			handle->manager->GainsFocus();
			break;
		case WM_KILLFOCUS:
			handle->manager->LosesFocus();
			break;
		case WM_KEYDOWN:
		{
			char character = '\0';
			PGButton button = PGButtonNone;
			switch (wParam) {
				case VK_SHIFT:
					handle->modifier |= PGModifierShift;
					break;
				case VK_CONTROL:
					handle->modifier |= PGModifierCtrl;
					break;
				case VK_MENU:
					handle->modifier |= PGModifierAlt;
					break;
				case VK_TAB:
					button = PGButtonTab;
					break;
				case VK_LEFT:
					button = PGButtonLeft;
					break;
				case VK_RIGHT:
					button = PGButtonRight;
					break;
				case VK_UP:
					button = PGButtonUp;
					break;
				case VK_DOWN:
					button = PGButtonDown;
					break;
				case VK_HOME:
					button = PGButtonHome;
					break;
				case VK_END:
					button = PGButtonEnd;
					break;
				case VK_INSERT:
					button = PGButtonInsert;
					break;
				case VK_DELETE:
					button = PGButtonDelete;
					break;
				case VK_F1:
					button = PGButtonF1;
					break;
				case VK_F2:
					button = PGButtonF2;
					break;
				case VK_F3:
					button = PGButtonF3;
					break;
				case VK_F4:
					button = PGButtonF4;
					break;
				case VK_F5:
					button = PGButtonF5;
					break;
				case VK_F6:
					button = PGButtonF6;
					break;
				case VK_F7:
					button = PGButtonF7;
					break;
				case VK_F8:
					button = PGButtonF8;
					break;
				case VK_F9:
					button = PGButtonF9;
					break;
				case VK_F10:
					button = PGButtonF10;
					break;
				case VK_F11:
					button = PGButtonF11;
					break;
				case VK_F12:
					button = PGButtonF12;
					break;
				case VK_BACK:
					button = PGButtonBackspace;
					break;
				case VK_RETURN:
					button = PGButtonEnter;
					break;
				case VK_PRIOR:
					button = PGButtonPageUp;
					break;
				case VK_NEXT:
					button = PGButtonPageDown;
					break;
				case VK_OEM_PLUS:
					character = '+';
					break;
				case VK_OEM_COMMA:
					character = ',';
					break;
				case VK_OEM_MINUS:
					character = '-';
					break;
				case VK_OEM_PERIOD:
					character = '.';
					break;
				case VK_ESCAPE:
					button = PGButtonEscape;
					break;
				default:
					break;
			}
			if (button != PGButtonNone) {
				handle->manager->KeyboardButton(button, handle->modifier);
				return 0;
			} else if ((wParam >= 0x20 && wParam <= 0x7E) || character != '\0') {
				if (character == '\0')
					character = (char)wParam;
				if (handle->modifier != PGModifierNone) {
					handle->manager->KeyboardCharacter(character, handle->modifier);
					return 0;
				}
			}
			break;
		}
		case WM_KEYUP:
			switch (wParam) {
				case VK_SHIFT:
					handle->modifier &= ~PGModifierShift;
					break;
				case VK_CONTROL:
					handle->modifier &= ~PGModifierCtrl;
					break;
				case VK_MENU:
					handle->modifier &= ~PGModifierAlt;
					break;
			}
			break;
		case WM_CHAR:
		{
			// FIXME: more efficient UTF16 -> UTF8 conversion
			wchar_t w = (wchar_t)wParam;
			std::string str = std::string((char*)&w, 2);
			char* output;
			lng size = PGConvertText(str, &output, PGEncodingUTF16Platform, PGEncodingUTF8);

			if (size == 1) {
				if (wParam < 0x20 || wParam >= 0x7F)
					break;
				handle->manager->KeyboardCharacter(output[0], PGModifierNone);
			} else {
				PGUTF8Character u;
				u.length = size;
				memcpy(u.character, output, size);

				handle->manager->KeyboardUnicode(u, PGModifierNone);
			}
			free(output);
			break;
		}
		case WM_UNICHAR:
			// we handle WM_CHAR (UTF16), not WM_UNICHAR (UTF32)
			return false;
		case WM_MOUSEWHEEL:
		{
			POINT point;
			point.x = GET_X_LPARAM(lParam);
			point.y = GET_Y_LPARAM(lParam);
			ScreenToClient(handle->hwnd, &point);
			PGModifier modifier = 0;
			if (wParam & MK_CONTROL) modifier |= PGModifierCtrl;
			if (wParam & MK_SHIFT) modifier |= PGModifierShift;
			int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			handle->manager->MouseWheel(point.x, point.y, 0, (double)zDelta, modifier);
			break;
		}
		case WM_LBUTTONDOWN:
		{
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
			PGModifier modifier = 0;
			if (wParam & MK_CONTROL) modifier |= PGModifierCtrl;
			if (wParam & MK_SHIFT) modifier |= PGModifierShift;
			handle->manager->MouseDown(x, y, PGLeftMouseButton, modifier, 0);
			break;
		}
		case WM_LBUTTONUP:
		{
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
			PGModifier modifier = 0;
			if (wParam & MK_CONTROL) modifier |= PGModifierCtrl;
			if (wParam & MK_SHIFT) modifier |= PGModifierShift;
			handle->manager->MouseUp(x, y, PGLeftMouseButton, modifier);
			break;
		}
		case WM_MBUTTONDOWN:
		{
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
			PGModifier modifier = 0;
			if (wParam & MK_CONTROL) modifier |= PGModifierCtrl;
			if (wParam & MK_SHIFT) modifier |= PGModifierShift;
			handle->manager->MouseDown(x, y, PGMiddleMouseButton, modifier, 0);
			break;
		}
		case WM_MBUTTONUP:
		{
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
			PGModifier modifier = 0;
			if (wParam & MK_CONTROL) modifier |= PGModifierCtrl;
			if (wParam & MK_SHIFT) modifier |= PGModifierShift;
			handle->manager->MouseUp(x, y, PGMiddleMouseButton, modifier);
			break;
		}

		case WM_RBUTTONDOWN:
		{
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
			PGModifier modifier = 0;
			if (wParam & MK_CONTROL) modifier |= PGModifierCtrl;
			if (wParam & MK_SHIFT) modifier |= PGModifierShift;
			handle->manager->MouseDown(x, y, PGRightMouseButton, modifier, 0);
			break;
		}
		case WM_RBUTTONUP:
		{
			int x = GET_X_LPARAM(lParam);
			int y = GET_Y_LPARAM(lParam);
			PGModifier modifier = 0;
			if (wParam & MK_CONTROL) modifier |= PGModifierCtrl;
			if (wParam & MK_SHIFT) modifier |= PGModifierShift;
			handle->manager->MouseUp(x, y, PGRightMouseButton, modifier);
			break;
		}
		case WM_SETCURSOR:
			if (handle->cursor) {
				SetCursor(handle->cursor);
				return true;
			}
			return DefWindowProc(hWnd, message, wParam, lParam);
		case WM_ERASEBKGND:
			return 1;
		case WM_DESTROY:
			DestroyWindow(handle);
			if (handle_map.size() == 0) {
				// no windows left, exit the program
				PostQuitMessage(0);
			}
			break;
		case WM_SIZE:
		{
			if (wParam != SIZE_MINIMIZED) {
				int width = LOWORD(lParam);
				int height = HIWORD(lParam);
				PGSize old_size = GetWindowSize(handle);
				handle->manager->SetSize(PGSize(width, height));
				RedrawWindow(handle);
			}
			break;
		}
		case WM_COMMAND:
		{
			if (wParam == 0 && lParam == 0) {
				handle->manager->PeriodicRender();
				if (handle->pending_destroy) {
					SendMessage(handle->hwnd, WM_CLOSE, 0, 0);
					break;
				}
				if (handle->pending_drag_drop) {
					handle->pending_drag_drop = false;
					PGPerformDragDrop(handle);
				}
				if (handle->pending_popup_menu) {
					int index;
					handle->pending_popup_menu = false;
					auto popup_menu = handle->popup_data.menu;
					auto current_cursor = handle->cursor;
					handle->cursor = nullptr;
					index = TrackPopupMenu(handle->popup_data.menu->menu, handle->popup_data.alignment | TPM_RETURNCMD, handle->popup_data.point.x, handle->popup_data.point.y, 0, handle->hwnd, NULL);
					handle->cursor = current_cursor;
					DestroyMenu(handle->popup_data.menu->menu);
					if (index != 0) {
						PGPopupCallback callback = popup_menu->callbacks[index];
						if (callback) {
							assert(popup_menu->data.size() > (index - BASE_INDEX));
							callback(popup_menu->control, popup_menu->data[index - BASE_INDEX]);
						}
					}
					delete popup_menu;
				}
				while (handle->pending_confirmation_box) {
					handle->pending_confirmation_box = false;
					PGResponse response = PGResponseCancel;
					int retval = MessageBox(handle->hwnd, handle->confirmation_box_data.message.c_str(), handle->confirmation_box_data.title.c_str(), MB_YESNOCANCEL | MB_ICONWARNING);
					if (retval == IDNO) {
						response = PGResponseNo;
					} else if (retval == IDYES) {
						response = PGResponseYes;
					}
					handle->confirmation_box_data.callback(handle, handle->confirmation_box_data.control, handle->confirmation_box_data.data, response);
				}
				break;
			}
			break;
		}
		case WM_CLOSE:
			if (!handle->manager->CloseControlManager())
				return 0;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

PGPoint GetMousePosition(PGWindowHandle window) {
	POINT point;
	if (!GetCursorPos(&point)) {
		return PGPoint(-1, -1);
	}
	if (!ScreenToClient(window->hwnd, &point)) {
		return PGPoint(-1, -1);
	}
	return PGPoint((PGScalar)point.x, (PGScalar)point.y);
}

PGMouseButton GetMouseState(PGWindowHandle window) {
	PGMouseButton button = PGMouseButtonNone;
	if ((GetKeyState(VK_LBUTTON) & 0x80) != 0) {
		button |= PGLeftMouseButton;
	}
	if ((GetKeyState(VK_RBUTTON) & 0x80) != 0) {
		button |= PGRightMouseButton;
	}
	if ((GetKeyState(VK_MBUTTON) & 0x80) != 0) {
		button |= PGMiddleMouseButton;
	}
	return button;
}

PGWindowHandle PGCreateWindow(PGWorkspace* workspace, std::vector<std::shared_ptr<TextFile>> initial_files) {
	return PGCreateWindow(workspace, PGPoint(CW_USEDEFAULT, CW_USEDEFAULT), initial_files);
}

PGWindowHandle PGCreateWindow(PGWorkspace* workspace, PGPoint position, std::vector<std::shared_ptr<TextFile>> initial_files) {
	HINSTANCE hInstance = GetModuleHandle(nullptr);
	// The parameters to CreateWindow explained:
	// szWindowClass: the name of the application
	// szTitle: the text that appears in the title bar
	// WS_OVERLAPPEDWINDOW: the type of window to create
	// CW_USEDEFAULT, CW_USEDEFAULT: initial position (x, y)
	// 500, 100: initial size (width, length)
	// NULL: the parent of this window
	// NULL: this application does not have a menu bar
	// hInstance: the first parameter from WinMain
	// NULL: not used in this application
	HWND hWnd = CreateWindow(
		szWindowClass,
		szTitle,
		(WS_OVERLAPPEDWINDOW),
		position.x, position.y,
		1016, 738,
		nullptr,
		nullptr,
		hInstance,
		nullptr
	);

	if (!hWnd) {
		return nullptr;
	}

	SetWindowLong(hWnd, GWL_EXSTYLE, WS_EX_COMPOSITED);

	PGWindowHandle res = new PGWindow(workspace);
	if (!res) {
		return nullptr;
	}
	workspace->AddWindow(res);
	res->hwnd = hWnd;
	res->cursor = cursor_standard;
	res->renderer = InitializeRenderer();
	SetHWNDHandle(hWnd, res);

	RegisterDropWindow(res, &res->drop_target);

	ControlManager* manager = new ControlManager(res);
	manager->SetPosition(PGPoint(0, 0));
	manager->SetSize(PGSize(1000, 700));
	manager->SetAnchor(PGAnchorLeft | PGAnchorRight | PGAnchorTop | PGAnchorBottom);
	res->manager = manager;

	res->timer = CreateTimer(res, MAX_REFRESH_FREQUENCY, [](PGWindowHandle res) {
		SendMessage(res->hwnd, WM_COMMAND, 0, 0);
	}, PGTimerFlagsNone);

	ProjectExplorer* explorer = new ProjectExplorer(res);
	explorer->SetAnchor(PGAnchorBottom | PGAnchorLeft);
	explorer->fixed_width = 200;
	explorer->percentage_height = 1;
	explorer->minimum_width = 50;

	StatusBar* bar = new StatusBar(res);
	bar->SetAnchor(PGAnchorLeft | PGAnchorBottom);
	bar->percentage_width = 1;
	bar->fixed_height = STATUSBAR_HEIGHT;
	explorer->vertical_anchor = bar;

	Splitter *splitter = new Splitter(res, true);
	splitter->SetAnchor(PGAnchorBottom | PGAnchorLeft);
	splitter->horizontal_anchor = explorer;
	splitter->vertical_anchor = bar;
	splitter->fixed_width = 4;
	splitter->percentage_height = 1;

	manager->AddControl(bar);
	manager->AddControl(explorer);
	manager->AddControl(splitter);

	manager->statusbar = bar;
	manager->active_projectexplorer = explorer;
	manager->splitter = splitter;

	manager->SetTextFieldLayout(1, 1, initial_files);

	auto handle = PGCreateMenu(res, manager);
	auto file = PGCreatePopupMenu(res, manager);
	PGPopupMenuInsertEntry(file, PGPopupInformation(file, "New File", "Ctrl+N"), [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertEntry(file, PGPopupInformation(file, "Open File", "Ctrl+O"), [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertEntry(file, PGPopupInformation(file, "Open Folder", "Ctrl+Shift+O"), [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertEntry(file, "Open Recent", [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertEntry(file, "Reopen with Encoding", [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertEntry(file, "New View into File", [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertSeparator(file);
	PGPopupMenuInsertEntry(file, PGPopupInformation(file, "Save", "Ctrl+S"), [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertEntry(file, PGPopupInformation(file, "Save As", "Ctrl+Shift+S"), [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertSeparator(file);
	PGPopupMenuInsertEntry(file, PGPopupInformation(file, "New Window", "Ctrl+Shift+N"), [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertEntry(file, PGPopupInformation(file, "Close Window", "Ctrl+Shift+W"), [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertSeparator(file);
	PGPopupMenuInsertEntry(file, PGPopupInformation(file, "Close File", "Ctrl+W"), [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertEntry(file, "Close All Files", [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertSeparator(file);
	PGPopupMenuInsertEntry(file, "Exit", [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertSubmenu(handle, file, "File");

	auto edit = PGCreatePopupMenu(res, manager);
	PGPopupMenuInsertEntry(edit, PGPopupInformation(edit, "New File", "Ctrl+N"), [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertSubmenu(handle, edit, "Edit");

	auto view = PGCreatePopupMenu(res, manager);
	PGPopupMenuInsertEntry(view, PGPopupInformation(view, "New File", "Ctrl+N"), [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertSubmenu(handle, view, "View");

	auto project = PGCreatePopupMenu(res, manager);
	PGPopupMenuInsertEntry(project, PGPopupInformation(project, "New File", "Ctrl+N"), [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertSubmenu(handle, project, "Project");

	auto build = PGCreatePopupMenu(res, manager);
	PGPopupMenuInsertEntry(build, PGPopupInformation(build, "New File", "Ctrl+N"), [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertSubmenu(handle, build, "Build");

	auto tools = PGCreatePopupMenu(res, manager);
	PGPopupMenuInsertEntry(tools, PGPopupInformation(tools, "New File", "Ctrl+N"), [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertSubmenu(handle, tools, "Tools");

	auto preferences = PGCreatePopupMenu(res, manager);
	PGPopupMenuInsertEntry(preferences, PGPopupInformation(preferences, "New File", "Ctrl+N"), [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertSubmenu(handle, preferences, "Preferences");

	auto help = PGCreatePopupMenu(res, manager);
	PGPopupMenuInsertEntry(help, PGPopupInformation(help, "New File", "Ctrl+N"), [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertSubmenu(handle, help, "Help");

	PGSetWindowMenu(res, handle);


	return res;
}

void DestroyWindow(PGWindowHandle window) {
	window->workspace->RemoveWindow(window);
	auto windows = window->workspace->GetWindows();
	if (windows.size() == 0) {
		open_workspaces.erase(std::find(open_workspaces.begin(), open_workspaces.end(), window->workspace));
		delete window->workspace;
	} else {
		window->workspace->WriteWorkspace();
	}
	delete window->manager;
	DeleteRenderer(window->renderer);
	DeleteTimer(window->timer);
	UnregisterDropWindow(window, window->drop_target);
	handle_map.erase(window->hwnd);
	DestroyWindow(window->hwnd);
	free(window);
}

void PGCloseWindow(PGWindowHandle window) {
	if (!window) return;
	window->pending_destroy = true;
}

void ShowWindow(PGWindowHandle window) {
	if (!window) return;
	ShowWindow(window->hwnd, cmdshow);
	UpdateWindow(window->hwnd);
}

void HideWindow(PGWindowHandle window) {

}

void RefreshWindow(PGWindowHandle window, bool redraw_now) {
	window->manager->RefreshWindow(redraw_now);
}

void RefreshWindow(PGWindowHandle window, PGIRect rectangle, bool redraw_now) {
	window->manager->RefreshWindow(rectangle, redraw_now);
}

void RedrawWindow(PGWindowHandle window) {
	InvalidateRect(window->hwnd, nullptr, true);
}

void RedrawWindow(PGWindowHandle window, PGIRect rectangle) {
	RECT rect;
	rect.top = rectangle.y;
	rect.left = rectangle.x;
	rect.right = rect.left + rectangle.width;
	rect.bottom = rect.top + rectangle.height;
	InvalidateRect(window->hwnd, &rect, true);
}

PGSize GetWindowSize(PGWindowHandle window) {
	RECT rect;
	GetWindowRect(window->hwnd, &rect);
	return PGSize(rect.right - rect.left, rect.bottom - rect.top);
}

PGPoint PGGetWindowPosition(PGWindowHandle window) {
	RECT rect;
	GetWindowRect(window->hwnd, &rect);
	return PGPoint(rect.left, rect.top);

}

Control* GetFocusedControl(PGWindowHandle window) {
	return window->manager->GetActiveControl();
}

void RegisterControl(PGWindowHandle window, Control *control) {
	if (window->manager != nullptr)
		window->manager->AddControl(control);
}

PGTime GetTime() {
	return (PGTime)GetTickCount();
}

void SetWindowTitle(PGWindowHandle window, std::string title) {
	SetWindowText(window->hwnd, title.c_str());
}

void SetClipboardTextOS(PGWindowHandle window, std::string text) {
	if (text.size() == 0) return;
	if (OpenClipboard(window->hwnd)) {
		char* result = nullptr;
		size_t length = PGConvertText(text, &result, PGEncodingUTF8, PGEncodingUTF16Platform);
		assert(length > 0);
		HGLOBAL clipbuffer;
		char * buffer;
		EmptyClipboard();
		clipbuffer = GlobalAlloc(GMEM_DDESHARE, length + 2);
		buffer = (char*)GlobalLock(clipbuffer);
		memcpy(buffer, result, length * sizeof(char));
		buffer[length] = '\0';
		buffer[length + 1] = '\0';
		GlobalUnlock(clipbuffer);
		SetClipboardData(CF_UNICODETEXT, clipbuffer);
		CloseClipboard();
		free(result);
	}
}

std::string GetClipboardText(PGWindowHandle window) {
	if (OpenClipboard(window->hwnd)) {
		char* result = nullptr;

		// get the text from the clipboard
		HANDLE data = GetClipboardData(CF_UNICODETEXT);
		std::string text = "";
		if (data) {
			text = std::string(((char*)data), UCS2Length((char*)data));
		}
		CloseClipboard();
		if (text.size() == 0) return nullptr;
		// on Windows we assume the text on the clipboard is encoded in UTF16: convert to UTF8
		size_t length = PGConvertText(text, &result, PGEncodingUTF16Platform, PGEncodingUTF8);
		assert(length > 0);
		text = std::string(result, length);
		free(result);
		assert(utf8_strlen(text) >= 0);
		return text;
	}
	return nullptr;
}

PGLineEnding GetSystemLineEnding() {
	return PGLineEndingWindows;
}

char GetSystemPathSeparator() {
	return '\\';
}

VOID CALLBACK TimerRoutine(PVOID lpParam, BOOLEAN TimerOrWaitFired) {
	PGTimerParameter* parameter = (PGTimerParameter*)lpParam;
	parameter->callback(parameter->window);
}

PGTimerHandle CreateTimer(PGWindowHandle wnd, int ms, PGTimerCallback callback, PGTimerFlags flags) {
	HANDLE timer;
	ULONG timer_flags = 0;
	if (timer_flags & PGTimerExecuteOnce) timer_flags |= WT_EXECUTEONLYONCE;

	PGTimerParameter* parameter = new PGTimerParameter();
	parameter->callback = callback;
	parameter->window = wnd;

	if (!CreateTimerQueueTimer(
		&timer,
		nullptr,
		TimerRoutine,
		(void*)parameter,
		(DWORD)ms,
		flags & PGTimerExecuteOnce ? (DWORD)0 : (DWORD)ms,
		timer_flags))
		return nullptr;

	PGTimerHandle handle = new PGTimer();
	handle->timer = timer;
	handle->parameter = parameter;
	return handle;
}

void DeleteTimer(PGTimerHandle handle) {
	DeleteTimerQueueTimer(nullptr, handle->timer, nullptr);
	delete handle->parameter;
}

bool WindowHasFocus(PGWindowHandle window) {
	return true;
	HWND hwnd = GetForegroundWindow();
	return GetDlgCtrlID(hwnd) == GetDlgCtrlID(window->hwnd);
}

void SetCursor(PGWindowHandle window, PGCursorType type) {
	HCURSOR cursor = nullptr;
	switch (type) {
		case PGCursorStandard:
			cursor = cursor_standard;
			break;
		case PGCursorCrosshair:
			cursor = cursor_crosshair;
			break;
		case PGCursorHand:
			cursor = cursor_hand;
			break;
		case PGCursorIBeam:
			cursor = cursor_ibeam;
			break;
		case PGCursorWait:
			cursor = cursor_wait;
			break;
		case PGCursorResizeHorizontal:
			cursor = cursor_horizontal_resize;
			break;
		case PGCursorResizeVertical:
			cursor = cursor_vertical_resize;
			break;
		default:
			break;
	}
	window->cursor = cursor;
}

void* GetWindowManager(PGWindowHandle window) {
	return window->manager;
}

PGRendererHandle GetRendererHandle(PGWindowHandle window) {
	return window->renderer;
}

static PGPopupMenuHandle _create_menu(PGWindowHandle window, Control* control, HMENU hmenu, bool is_popupmenu) {
	PGPopupMenuHandle handle = new PGPopupMenu();
	handle->menu = hmenu;
	handle->window = window;
	handle->control = control;
	handle->font = PGCreateFont("segoe ui", false, false);
	handle->text_size = 0;
	handle->hotkey_size = 0;
	handle->is_popupmenu = is_popupmenu;
	SetTextFontSize(handle->font, 13);


	PGColor background_color = is_popupmenu ? PGStyleManager::GetColor(PGColorMenuBackground) : PGStyleManager::GetColor(PGColorMainMenuBackground);

	MENUINFO mi = { 0 };
	mi.cbSize = sizeof(MENUINFO);
	mi.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;// | MIM_STYLE;
	//mi.dwStyle = MNS_MODELESS;
	mi.hbrBack = CreateSolidBrush(RGB(background_color.r, background_color.g, background_color.b));

	SetMenuInfo(handle->menu, &mi);

	return handle;
}

PGPopupMenuHandle PGCreatePopupMenu(PGWindowHandle window, Control* control) {
	return _create_menu(window, control, CreatePopupMenu(), true);
}

PGPopupMenuHandle PGCreateMenu(PGWindowHandle window, Control* control) {
	return _create_menu(window, control, CreateMenu(), false);
}

void PGPopupMenuInsertEntry(PGPopupMenuHandle handle, PGPopupInformation information, PGPopupCallback callback, PGPopupMenuFlags flags) {
	int append_flags = MF_OWNERDRAW;
	if (flags & PGPopupMenuChecked) append_flags |= MF_CHECKED;
	if (flags & PGPopupMenuGrayed) append_flags |= MF_GRAYED;
	int index = handle->index;

	handle->text_size = max(MeasureTextWidth(handle->font, information.text), handle->text_size);
	handle->hotkey_size = max(MeasureTextWidth(handle->font, information.hotkey), handle->hotkey_size);

	PGPopupInformation* info = new PGPopupInformation(information);
	info->type = PGPopupTypeEntry;
	AppendMenu(handle->menu, append_flags, index, (LPCSTR)info);
	handle->data.push_back(info);
	handle->callbacks[index] = callback;
	handle->index++;
}

void PGPopupMenuInsertSeparator(PGPopupMenuHandle handle) {
	PGPopupInformation* info = new PGPopupInformation(handle);
	info->type = PGPopupTypeSeparator;
	info->text = "";
	AppendMenu(handle->menu, MF_SEPARATOR | MF_OWNERDRAW, 0, (LPCSTR)info);
	handle->data.push_back(info);
	handle->index++;
}

void PGPopupMenuInsertSubmenu(PGPopupMenuHandle handle, PGPopupMenuHandle submenu, std::string name) {
	PGPopupInformation* info = new PGPopupInformation(handle);
	info->type = !handle->is_popupmenu ? PGPopupTypeMenu : PGPopupTypeSubmenu;
	info->text = name;
	AppendMenu(handle->menu, MF_POPUP | MF_OWNERDRAW, (UINT_PTR)submenu->menu, (LPCSTR)info);
	handle->data.push_back(info);
	handle->index++;
}

void PGDisplayPopupMenu(PGPopupMenuHandle handle, PGTextAlign align) {
	POINT pt;
	GetCursorPos(&pt);
	PGDisplayPopupMenu(handle, PGPoint(pt.x, pt.y), align);
}


void PGDisplayPopupMenu(PGPopupMenuHandle handle, PGPoint point, PGTextAlign align) {
	UINT alignment = 0;
	if (align & PGTextAlignLeft) {
		alignment |= TPM_LEFTALIGN;
	} else if (align & PGTextAlignHorizontalCenter) {
		alignment |= TPM_CENTERALIGN;
	} else {
		alignment |= TPM_RIGHTALIGN;
	}
	if (align & PGTextAlignTop) {
		alignment |= TPM_TOPALIGN;
	} else if (align & PGTextAlignVerticalCenter) {
		alignment |= TPM_VCENTERALIGN;
	} else {
		alignment |= TPM_BOTTOMALIGN;
	}
	handle->window->pending_popup_menu = true;
	handle->window->popup_data.menu = handle;
	handle->window->popup_data.alignment = alignment;
	handle->window->popup_data.point = point;
}

void PGSetWindowMenu(PGWindowHandle window, PGPopupMenuHandle menu) {
	window->menu = menu;
	SetMenu(window->hwnd, menu->menu);
}

void OpenFolderInExplorer(std::string path) {
	// FIXME: if path is directory don't just select but go into the path
	std::string parameter = "explorer.exe /select,\"" + path + "\"";
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	if (CreateProcess(NULL, (LPSTR)parameter.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	} else {
		DWORD dw = GetLastError();
		LPVOID lpMsgBuf;

		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL);

		Logger::WriteLogMessage(std::string((char*)lpMsgBuf));
	}
}

void OpenFolderInTerminal(std::string path) {
	// FIXME: if path is directory don't just select but go into the path

	std::string parameter;
	if (!PGSettingsManager::GetSetting("default_terminal", parameter)) {
		parameter = "cmd.exe";
	}
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	std::string directory = PGFile(path).Directory();
	if (CreateProcess(NULL, (LPSTR)parameter.c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, directory.c_str(), &si, &pi)) {
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	} else {
		LogLastError();
	}
}

PGPoint ConvertWindowToScreen(PGWindowHandle window, PGPoint point) {
	POINT p;
	p.x = point.x; p.y = point.y;
	ClientToScreen(window->hwnd, &p);
	return PGPoint(p.x, p.y);
}

std::vector<std::string> ShowOpenFileDialog(bool allow_files, bool allow_directories, bool allow_multiple_selection) {
	std::vector<std::string> files;

	IFileOpenDialog *pFileOpen;
	HRESULT hr;

	// Create the FileOpenDialog object.
	hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
		IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

	if (SUCCEEDED(hr)) {
		// Set the options on the dialog.
		DWORD dwFlags;

		std::string title = UTF8toUCS2("Open File");
		hr = pFileOpen->SetTitle((LPCWSTR)title.c_str());
		if (!SUCCEEDED(hr)) return files;

		// Before setting, always get the options first in order 
		// not to override existing options.
		hr = pFileOpen->GetOptions(&dwFlags);
		if (!SUCCEEDED(hr)) return files;

		if (allow_directories)
			dwFlags |= FOS_PICKFOLDERS;
		if (allow_multiple_selection)
			dwFlags |= FOS_ALLOWMULTISELECT;
		dwFlags |= FOS_FORCEFILESYSTEM;

		hr = pFileOpen->SetOptions(dwFlags);

		// Show the Open dialog box.
		hr = pFileOpen->Show(NULL);

		// Get the file name from the dialog box.
		if (!SUCCEEDED(hr)) return files;

		IShellItemArray *pArray;
		hr = pFileOpen->GetResults(&pArray);
		if (!SUCCEEDED(hr)) return files;

		DWORD FILECOUNT = 0;
		hr = pArray->GetCount(&FILECOUNT);
		if (!SUCCEEDED(hr)) return files;

		for (DWORD i = 0; i < FILECOUNT; i++) {
			IShellItem* pItem;
			hr = pArray->GetItemAt(i, &pItem);
			if (!SUCCEEDED(hr)) return files;

			PWSTR pszFilePath;
			hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
			if (!SUCCEEDED(hr)) return files;

			files.push_back(UCS2toUTF8(pszFilePath));

			CoTaskMemFree(pszFilePath);
			pItem->Release();
		}
		pArray->Release();


		pFileOpen->Release();
	} else {
		// FIXME: use GetOpenFileName() instead
		assert(0);
	}

	return files;
}

std::string ShowSaveFileDialog() {
	std::string res = std::string("");
	IFileSaveDialog *pFileSave;
	HRESULT hr;

	// Create the FileOpenDialog object.
	hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL,
		IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileSave));

	if (SUCCEEDED(hr)) {
		// Set the options on the dialog.
		DWORD dwFlags;

		std::string title = UTF8toUCS2("Save File");
		hr = pFileSave->SetTitle((LPCWSTR)title.c_str());
		if (!SUCCEEDED(hr)) return res;

		// Before setting, always get the options first in order 
		// not to override existing options.
		hr = pFileSave->GetOptions(&dwFlags);
		if (!SUCCEEDED(hr)) return res;

		dwFlags |= FOS_FORCEFILESYSTEM;

		hr = pFileSave->SetOptions(dwFlags);

		// Show the Open dialog box.
		hr = pFileSave->Show(NULL);

		// Get the file name from the dialog box.
		if (!SUCCEEDED(hr)) return res;

		IShellItem *pItem;
		hr = pFileSave->GetResult(&pItem);
		if (!SUCCEEDED(hr)) return res;

		PWSTR pszFilePath;
		hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
		if (!SUCCEEDED(hr)) return res;

		res = UCS2toUTF8(pszFilePath);

		CoTaskMemFree(pszFilePath);
		pItem->Release();

		pFileSave->Release();
	} else {
		// FIXME: use old API instead
		assert(0);
	}
	return res;
}

PGWindowHandle GetHWNDHandle(HWND hwnd) {
	if (handle_map.count(hwnd) == 0) return nullptr;
	return handle_map[hwnd];
}

void SetHWNDHandle(HWND hwnd, PGWindowHandle window) {
	assert(handle_map.count(hwnd) == 0);
	handle_map[hwnd] = window;
}

void PGMessageBox(PGWindowHandle window, std::string title, std::string message) {
	MessageBox(window->hwnd, message.c_str(), title.c_str(), MB_OK);
}

PGResponse PGConfirmationBox(PGWindowHandle window, std::string title, std::string message) {
	int retval = MessageBox(window->hwnd, message.c_str(), title.c_str(), MB_YESNOCANCEL | MB_ICONWARNING);
	if (retval == IDNO) {
		return PGResponseNo;
	} else if (retval == IDYES) {
		return PGResponseYes;
	}
	return PGResponseCancel;
}

void PGConfirmationBox(PGWindowHandle window, std::string title, std::string message, PGConfirmationCallback callback, Control* control, void* data) {
	window->pending_confirmation_box = true;
	window->confirmation_box_data.callback = callback;
	window->confirmation_box_data.control = control;
	window->confirmation_box_data.message = message;
	window->confirmation_box_data.data = data;
	window->confirmation_box_data.title = title;
}

std::string GetOSName() {
	return "windows";
}

PGWorkspace* PGGetWorkspace(PGWindowHandle window) {
	return window->workspace;
}

void PGLoadWorkspace(PGWindowHandle window, nlohmann::json& j) {
	if (j.count("dimensions") > 0) {
		nlohmann::json dim = j["dimensions"];
		if (dim.count("width") > 0 && dim["width"].is_number() &&
			dim.count("height") > 0 && dim["height"].is_number() &&
			dim.count("x") > 0 && dim["x"].is_number() &&
			dim.count("y") > 0 && dim["y"].is_number()) {
			int x = dim["x"];
			int y = dim["y"];
			int width = dim["width"];
			int height = dim["height"];

			SetWindowPos(window->hwnd, 0, x, y, width, height, SWP_NOZORDER);
		}
	}
	if (j.count("controls") > 0) {
		window->manager->LoadWorkspace(j["controls"]);
	}
}

void PGWriteWorkspace(PGWindowHandle window, nlohmann::json& j) {
	PGSize window_size = GetWindowSize(window);
	j["dimensions"]["width"] = window_size.width;
	j["dimensions"]["height"] = window_size.height;
	PGPoint window_position = PGGetWindowPosition(window);
	j["dimensions"]["x"] = window_position.x;
	j["dimensions"]["y"] = window_position.y;
	j["controls"] = nlohmann::json::object();
	window->manager->WriteWorkspace(j["controls"]);
}

static lng filetime_to_lng(FILETIME filetime) {
	ULARGE_INTEGER i;
	i.LowPart = filetime.dwLowDateTime;
	i.HighPart = filetime.dwHighDateTime;
	return (lng)(i.QuadPart);
}

PGFileInformation PGGetFileFlags(std::string path) {
	PGFileInformation info;
	std::string ucs2_path = UTF8toUCS2(path);
	HANDLE handle = CreateFile2((LPCWSTR)ucs2_path.c_str(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, NULL);
	if (handle == INVALID_HANDLE_VALUE) {
		DWORD error = GetLastError();
		if (error == ERROR_FILE_NOT_FOUND) {
			info.flags = PGFileFlagsFileNotFound;
		} else {
			info.flags = PGFileFlagsErrorOpeningFile;
		}
		return info;
	} else {
		info.flags = PGFileFlagsEmpty;
	}
	FILETIME creation_time;
	FILETIME last_write_time;
	GetFileTime(handle, &creation_time, NULL, &last_write_time);

	info.creation_time = filetime_to_lng(creation_time);
	info.modification_time = filetime_to_lng(last_write_time);


	ULARGE_INTEGER i;
	i.LowPart = GetFileSize(handle, &i.HighPart);
	info.file_size = (lng)(i.QuadPart);

	CloseHandle(handle);
	return info;
}

PGIOError PGRenameFile(std::string source, std::string dest) {
	std::string ucs2_source = UTF8toUCS2(source);
	std::string ucs2_target = UTF8toUCS2(dest);
	if (!MoveFileExW((LPCWSTR)ucs2_source.c_str(), (LPCWSTR)ucs2_target.c_str(), MOVEFILE_REPLACE_EXISTING)) {
		auto error = GetLastError();
		switch (error) {
			case ERROR_FILE_NOT_FOUND:
				return PGIOErrorFileNotFound;
			case ERROR_ACCESS_DENIED:
				return PGIOErrorPermissionDenied;
			default:
				return PGIOErrorOther;
		}
	}
	return PGIOSuccess;
}

PGIOError PGRemoveFile(std::string source) {
	std::string ucs2_path = UTF8toUCS2(source);
	if (!DeleteFileW((LPCWSTR)ucs2_path.c_str())) {
		auto error = GetLastError();
		switch (error) {
			case ERROR_FILE_NOT_FOUND:
				return PGIOErrorFileNotFound;
			case ERROR_ACCESS_DENIED:
				return PGIOErrorPermissionDenied;
			default:
				return PGIOErrorOther;
		}
	}
	return PGIOSuccess;
}

PGDirectoryFlags PGGetDirectoryFiles(std::string directory, std::vector<PGFile>& directories, std::vector<PGFile>& files) {
	DIR *dp;
	struct dirent *ep;
	dp = opendir(directory.c_str());
	if (dp == NULL) {
		return PGDirectoryNotFound;
	}

	while (ep = readdir(dp)) {
		std::string filename = ep->d_name;
		if (filename[0] == '.') continue;

		if (ep->d_type == DT_DIR) {
			directories.push_back(PGFile(filename));
		} else if (ep->d_type == DT_REG) {
			files.push_back(PGFile(filename));
		}
	}

	(void)closedir(dp);

	return PGDirectorySuccess;
}

void PGLogMessage(std::string text) {
	for (auto it = handle_map.begin(); it != handle_map.end(); it++) {
		it->second->manager->statusbar->SetText(text);
		break;
	}
}
