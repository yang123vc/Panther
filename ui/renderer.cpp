
#include "windowfunctions.h"
#include "control.h"
#include "renderer.h"
#include "unicode.h"

#include "replaymanager.h"

#include <SkColorFilter.h>
#include <SkGradientShader.h>
#include <SkDashPathEffect.h>

SkBitmap* PGGetBitmap(PGBitmapHandle handle) {
	return handle->bitmap;
}

static SkColor CreateSkColor(PGColor color) {
	return SkColorSetARGB(color.a, color.r, color.g, color.b);
}

static SkPoint CreateSkPoint(PGPoint point) {
	SkPoint p = { point.x, point.y };
	return p;
}

static SkRect CreateSkRect(PGRect rectangle) {
	SkRect rect;
	rect.fLeft = rectangle.x;
	rect.fTop = rectangle.y;
	rect.fRight = rectangle.x + rectangle.width;
	rect.fBottom = rectangle.y + rectangle.height;
	return rect;
}

static SkPaint::Style PGDrawStyleConvert(PGDrawStyle style) {
	if (style == PGDrawStyleStroke) {
		return SkPaint::kStroke_Style;
	}
	return SkPaint::kFill_Style;
}

SkPaint* CreateTextPaint() {
	SkPaint* textpaint = new SkPaint();
	textpaint->setTextSize(SkIntToScalar(15));
	textpaint->setAntiAlias(true);
	textpaint->setAutohinted(false);
	textpaint->setDither(false);
	textpaint->setHinting(SkPaint::Hinting::kFull_Hinting);
	textpaint->setDevKernText(true);
	textpaint->setLinearText(true);
	textpaint->setSubpixelText(true);
	textpaint->setStyle(SkPaint::kFill_Style);
	textpaint->setTextEncoding(SkPaint::kUTF8_TextEncoding);
	textpaint->setTextAlign(SkPaint::kLeft_Align);
	return textpaint;
}

std::map<std::string, sk_sp<SkTypeface>> loaded_fonts;
static sk_sp<SkTypeface> CreateFontFromFile(std::string path) {
	path = PGPathJoin(PGApplicationPath(), path);
	if (loaded_fonts.count(path) == 0) {
		if (PGGlobalReplayManager::running_replay) {
			lng result_size = 0;
			PGFileError err = PGFileSuccess;
			void* data = PGGlobalReplayManager::ReadFile(path, result_size, err);
			if (data && err == PGFileSuccess) {
				auto stream = new SkMemoryStream(data, result_size, true);
				loaded_fonts[path] = SkTypeface::MakeFromStream(stream);
				panther::DestroyFileContents(data);
			}
		} else {
			if (PGGlobalReplayManager::recording_replay) {
				lng result_size = 0;
				PGFileError err = PGFileSuccess;
				void* data = panther::ReadFile(path, result_size, err);
				if (data) {
					auto stream = new SkMemoryStream(data, result_size);
					loaded_fonts[path] = SkTypeface::MakeFromStream(stream);
					panther::DestroyFileContents(data);
				}
			} else {
				loaded_fonts[path] = SkTypeface::MakeFromFile(path.c_str());
			}
		}
	}
	return loaded_fonts[path];
}

struct StyledFont {
	sk_sp<SkTypeface> normal = nullptr;
	sk_sp<SkTypeface> bold = nullptr;
	sk_sp<SkTypeface> italic = nullptr;

	StyledFont() : normal(nullptr), bold(nullptr), italic(nullptr) { }
};

enum SkiaFontFace {
	SkiaNormal,
	SkiaItalic,
	SkiaBold
};

std::map<std::string, StyledFont> named_fonts;
static sk_sp<SkTypeface> CreateFontFromName(std::string name, SkiaFontFace fontface) {
	if (named_fonts.count(name) == 0) {
		named_fonts[name] = StyledFont();
	}
	StyledFont& font = named_fonts[name];
	switch (fontface) {
		case SkiaNormal:
			if (!font.normal) {
				auto style = SkFontStyle(SkFontStyle::kLight_Weight, SkFontStyle::kNormal_Width, SkFontStyle::kUpright_Slant);
				font.normal = SkTypeface::MakeFromName(name.c_str(), style);
			}
			return font.normal;
		case SkiaItalic:
			if (!font.italic) {
				auto style = SkFontStyle(SkFontStyle::kLight_Weight, SkFontStyle::kNormal_Width, SkFontStyle::kItalic_Slant);
				font.italic = SkTypeface::MakeFromName(name.c_str(), style);
			}
			return font.italic;
		case SkiaBold:
			if (!font.bold) {
				auto style = SkFontStyle(SkFontStyle::kBold_Weight, SkFontStyle::kNormal_Width, SkFontStyle::kUpright_Slant);
				font.bold = SkTypeface::MakeFromName(name.c_str(), style);
			}
			return font.bold;
	}
	assert(0);
	return nullptr;
}

static void CreateFallbackFonts(PGFontHandle font) {
	SkPaint* fallback_paint = CreateTextPaint();
	auto fallback_font = CreateFontFromFile("data/fonts/NotoSansHans-Regular.otf");
	assert(fallback_font);
	fallback_paint->setTypeface(fallback_font);
	font->fallback_paints.push_back(fallback_paint);
}


PGFontHandle PGCreateFont(PGFontType type) {
	switch (type) {
		case PGFontTypeTextField:
			return PGCreateFont();
		case PGFontTypeUI:
			return PGCreateFont("myriad", true, true);
		case PGFontTypePopup:
			return PGCreateFont("segoe ui", true, true);
	}
	return nullptr;
}

PGFontHandle PGCreateFont() {
#ifdef WIN32
	return PGCreateFont("Consolas", true, true);
#else
	return PGCreateFont("menlo", true, true);
	return PGCreateFont("/Users/myth/Programs/Panther/data/fonts/SourceCodePro-Semibold.ttf");
#endif
}

PGFontHandle PGCreateFont(const char* fontname, bool italic, bool bold) {
	PGFontHandle font = new PGFont();

	font->normaltext = CreateTextPaint();
	auto main_font = CreateFontFromName(fontname, SkiaNormal);
	font->normaltext->setTypeface(main_font);
	if (bold) {
		font->boldtext = CreateTextPaint();
		main_font = CreateFontFromName(fontname, SkiaBold);
		font->boldtext->setTypeface(main_font);
	}
	if (italic) {
		font->italictext = CreateTextPaint();
		main_font = CreateFontFromName(fontname, SkiaItalic);
		font->italictext->setTypeface(main_font);
	}
	font->textpaint = font->normaltext;

	CreateFallbackFonts(font);

	return font;
}

PGFontHandle PGCreateFont(const char* filename) {
	PGFontHandle font = new PGFont();

	auto main_font = CreateFontFromFile(filename);
	font->normaltext->setTypeface(main_font);
	font->textpaint = font->normaltext;

	CreateFallbackFonts(font);

	return font;
}

void PGDestroyFont(PGFontHandle font) {
	if (font->normaltext) delete font->normaltext;
	if (font->boldtext) delete font->boldtext;
	if (font->italictext) delete font->italictext;
	delete font;
}

PGRendererHandle InitializeRenderer() {
	SkPaint* paint = CreateTextPaint();
	PGRendererHandle renderer = new PGRenderer();
	paint->setStyle(SkPaint::kFill_Style);
	renderer->canvas = nullptr;
	renderer->paint = paint;
	return renderer;
}

void RenderControlsToBitmap(PGRendererHandle renderer, SkBitmap& bitmap, PGIRect rect, ControlManager* manager, PGScalar scale_factor) {
	//bitmap.setConfig(SkBitmap::kARGB_8888_Config, canvas_width, canvas_height);
	lng w = (lng)(rect.width * scale_factor);
	lng h = (lng)(rect.height * scale_factor);
	if ((lng)bitmap.width() != w || (lng)bitmap.height() != h) {
		manager->Invalidate();
		bitmap.allocN32Pixels(w, h);
		bitmap.setAlphaType(kOpaque_SkAlphaType);
#ifdef PANTHER_DEBUG
		SkCanvas canvas(bitmap);
		canvas.clear(SkColorSetARGB(255, 255, 0, 255));
#endif
	}

	SkCanvas canvas(bitmap);
	canvas.scale(scale_factor, scale_factor);

	renderer->canvas = &canvas;

	manager->Draw(renderer);
}


void RenderTriangle(PGRendererHandle handle, PGPoint a, PGPoint b, PGPoint c, PGColor color, PGDrawStyle drawStyle) {
	if (PGGlobalReplayManager::running_replay) return;
	SkPath path;
	SkPoint points[] = { {a.x, a.y}, {b.x, b.y}, {c.x, c.y} }; // triangle
	path.addPoly(points, 3, true);
	handle->paint->setColor(SkColorSetARGB(color.a, color.r, color.g, color.b));
	handle->paint->setStyle(PGDrawStyleConvert(drawStyle));
	handle->canvas->drawPath(path, *handle->paint);
}

void RenderRectangle(PGRendererHandle handle, PGRect rectangle, PGColor color, PGDrawStyle drawStyle, PGScalar width) {
	if (PGGlobalReplayManager::running_replay) return;
	SkRect rect = CreateSkRect(rectangle);
	handle->paint->setStyle(PGDrawStyleConvert(drawStyle));
	handle->paint->setColor(CreateSkColor(color));
	handle->paint->setStrokeWidth(width);
	handle->canvas->drawRect(rect, *handle->paint);
}

void RenderPolygon(PGRendererHandle handle, PGPolygon polygon, PGColor color, double stroke_width) {
	if (PGGlobalReplayManager::running_replay) return;
	if (polygon.points.size() == 0) return;
	SkPath path;
	path.moveTo(CreateSkPoint(polygon.points[0]));
	for (auto it = polygon.points.begin() + 1; it != polygon.points.end(); it++) {
		path.lineTo(CreateSkPoint(*it));
	}
	if (polygon.closed) {
		path.close();
	}
	if (stroke_width <= 0) {
		handle->paint->setStyle(SkPaint::kStrokeAndFill_Style);
	} else {
		handle->paint->setStyle(SkPaint::kStroke_Style);
		handle->paint->setStrokeWidth(stroke_width);
	}
	handle->paint->setColor(CreateSkColor(color));
	handle->canvas->drawPath(path, *handle->paint);
}

void RenderGradient(PGRendererHandle handle, PGRect rectangle, PGColor left, PGColor right) {
	if (PGGlobalReplayManager::running_replay) return;
	SkRect rect;
	rect.fLeft = rectangle.x;
	rect.fTop = rectangle.y;
	rect.fRight = rectangle.x + rectangle.width;
	rect.fBottom = rectangle.y + rectangle.height;

	SkPoint points[2] = {
		SkPoint::Make(rectangle.x, rectangle.y + rectangle.height / 2),
		SkPoint::Make(rectangle.x + rectangle.width, rectangle.y + rectangle.height / 2)
	};
	SkColor colors[2] = { CreateSkColor(left), CreateSkColor(right) };
	handle->paint->setColor(colors[1]);
	handle->paint->setShader(SkGradientShader::MakeLinear(
		points, colors, nullptr, 2,
		SkShader::kClamp_TileMode, 0, nullptr));
	handle->paint->setStyle(SkPaint::kFill_Style);
	handle->paint->setStrokeWidth(0.0f);
	handle->canvas->drawRect(rect, *handle->paint);
	handle->paint->setShader(nullptr);
}


void RenderCircle(PGRendererHandle handle, PGCircle circle, PGColor color, PGDrawStyle drawStyle) {
	if (PGGlobalReplayManager::running_replay) return;
	handle->paint->setAntiAlias(true);
	handle->paint->setStyle(PGDrawStyleConvert(drawStyle));
	handle->paint->setColor(SkColorSetARGB(color.a, color.r, color.g, color.b));
	handle->canvas->drawCircle(circle.x, circle.y, circle.radius, *handle->paint);
}

void RenderLine(PGRendererHandle handle, PGLine line, PGColor color, int width) {
	if (PGGlobalReplayManager::running_replay) return;
	handle->paint->setColor(SkColorSetARGB(color.a, color.r, color.g, color.b));
	handle->paint->setStrokeWidth(width);
	handle->canvas->drawLine(line.start.x, line.start.y, line.end.x, line.end.y, *handle->paint);
}

void RenderDashedLine(PGRendererHandle handle, PGLine line, PGColor color, PGScalar line_width, PGScalar spacing_width, int width) {
	if (PGGlobalReplayManager::running_replay) return;
	handle->paint->setColor(SkColorSetARGB(color.a, color.r, color.g, color.b));
	handle->paint->setStrokeWidth(width);

	const SkScalar intervals[] = { line_width, spacing_width };
	handle->paint->setPathEffect(SkDashPathEffect::Make(intervals, 2, 0.0f));
	handle->canvas->drawLine(line.start.x, line.start.y, line.end.x, line.end.y, *handle->paint);
	handle->paint->setPathEffect(nullptr);
}

void RenderText(PGRendererHandle renderer, PGFontHandle font, const char *text, size_t len, PGScalar x, PGScalar y, PGScalar max_position) {
	if (PGGlobalReplayManager::running_replay) return;
	PGScalar x_offset = 0;
	size_t position = 0;
	size_t i = 0;
	for (; i < len; ) {
		int offset = utf8_character_length(text[i]);
		if (offset != 1) {
			// for special unicode characters, we check if the main font can render the character
			if (font->textpaint->getTypeface()->charsToGlyphs(text + i, SkTypeface::kUTF8_Encoding, nullptr, 1) == 0) {
				// if not, we first render the previous characters using the main font
				if (position < i) {
					renderer->canvas->drawText(text + position, i - position, x, y + font->text_offset, *font->textpaint);
					x += font->textpaint->measureText(text + position, i - position);
				}
				bool skip_search = false;
				if (offset < 0) {
					offset = 1;
					skip_search = true;
				}
				position = i + offset;
				bool found_fallback = false;
				if (skip_search) {
					// we try to switch to a fallback font to render this character
					for (auto it = font->fallback_paints.begin(); it != font->fallback_paints.end(); it++) {
						assert((*it)->getTypeface());
						if ((*it)->getTypeface()->charsToGlyphs(text + i, SkTypeface::kUTF8_Encoding, nullptr, 1) != 0) {
							renderer->canvas->drawText(text + i, offset, x, y + font->text_offset, **it);
							x += (*it)->measureText(text + i, offset);
							found_fallback = true;
							break;
						}
					}
				}
				if (!found_fallback) {
					// if we don't have a fallback font we render the ? character
					renderer->canvas->drawText("\xef\xbf\xbd", 3, x, y + font->text_offset, *font->textpaint);
					x += font->character_width;
				}
				x_offset = 0;
			} else {
				x_offset += font->character_width;
			}
		} else {
			if (text[i] == '\t') {
				if (position < i) {
					renderer->canvas->drawText(text + position, i - position, x, y + font->text_offset, *font->textpaint);
					x += font->textpaint->measureText(text + position, i - position);
				}
				position = i + offset;
				PGScalar offset = font->textpaint->measureText(" ", 1) * font->tabwidth;
				// if (render_spaces_always)
				// PGScalar lineheight = GetTextHeight(font);
				// RenderLine(renderer, PGLine(x + 1, y + lineheight / 2, x + offset - 1, y + lineheight / 2), PGColor(255, 255, 255, 100), 0.5f);
				x += offset;
				x_offset = 0;
			} else {
				x_offset += font->character_width;
			}
		}
		i += offset;
		if (x + x_offset >= max_position)
			break;
	}
	if (position < i) {
		renderer->canvas->drawText(text + position, i - position, x, y + font->text_offset, *font->textpaint);
	}
}

PGScalar RenderText(PGRendererHandle renderer, PGFontHandle font, const char *text, size_t len, PGScalar x, PGScalar y, PGTextAlign alignment) {
	PGScalar width = MeasureTextWidth(font, text, len);
	if (alignment & PGTextAlignRight) {
		x -= width;
	} else if (alignment & PGTextAlignHorizontalCenter) {
		x -= width / 2;
	}
	RenderText(renderer, font, text, len, x, y);
	return width;
}

PGScalar RenderString(PGRendererHandle renderer, PGFontHandle font, const std::string& text, PGScalar x, PGScalar y, PGTextAlign alignment) {
	return RenderText(renderer, font, text.c_str(), text.size(), x, y, alignment);
}

void RenderString(PGRendererHandle renderer, PGFontHandle font, const std::string& text, PGScalar x, PGScalar y, PGScalar max_position) {
	RenderText(renderer, font, text.c_str(), text.size(), x, y, max_position);
}

void RenderImage(PGRendererHandle renderer, PGBitmapHandle image, int x, int y) {
	if (PGGlobalReplayManager::running_replay) return;
	renderer->canvas->drawBitmap(*image->bitmap, x, y);
}

void RenderImage(PGRendererHandle renderer, PGBitmapHandle image, PGRect rect) {
	if (PGGlobalReplayManager::running_replay) return;
	renderer->paint->setColor(CreateSkColor(PGColor(255, 255, 255, 255)));
	renderer->canvas->drawBitmapRect(*image->bitmap, CreateSkRect(rect), NULL);
}


PGBitmapHandle CreateBitmapFromSize(PGScalar width, PGScalar height) {
	PGBitmapHandle handle = new PGBitmap();
	handle->bitmap = new SkBitmap();
	handle->bitmap->allocN32Pixels((int)width, (int)height);
	handle->bitmap->allocPixels();
	return handle;
}

PGBitmapHandle CreateBitmapForText(PGFontHandle font, const char* text, size_t length) {
	PGScalar width = MeasureTextWidth(font, text, length);
	PGScalar height = GetTextHeight(font);
	return CreateBitmapFromSize(width, height);
}

PGRendererHandle CreateRendererForBitmap(PGBitmapHandle handle) {
	PGRendererHandle rend = new PGRenderer();
	rend->canvas = new SkCanvas(*handle->bitmap);
	rend->canvas->clear(SkColorSetARGB(0, 0, 0, 0));
	rend->paint = CreateTextPaint();
	return rend;
}

void DeleteImage(PGBitmapHandle handle) {
	delete handle->bitmap;
	delete handle;
}

void DeleteRenderer(PGRendererHandle renderer) {
	//delete renderer->canvas;
	delete renderer;
}

void RenderSquiggles(PGRendererHandle renderer, PGScalar width, PGScalar x, PGScalar y, PGColor color) {
	if (PGGlobalReplayManager::running_replay) return;
	SkPath path;
	PGScalar offset = 3; // FIXME: depend on text height
	PGScalar end = x + width;
	path.moveTo(x, y);
	while (x < end) {
		path.quadTo(x + 1, y + offset, x + 2, y);
		offset = -offset;
		x += 2;
	}
	renderer->paint->setColor(SkColorSetARGB(color.a, color.r, color.g, color.b));
	renderer->paint->setAntiAlias(true);
	renderer->paint->setAutohinted(true);
	renderer->canvas->drawPath(path, *renderer->paint);
}

PGScalar MeasureTextWidth(PGFontHandle font, std::string& text) {
	return MeasureTextWidth(font, text.c_str(), text.size());
}

PGScalar MeasureTextWidth(PGFontHandle font, const char* text) {
	return MeasureTextWidth(font, text, strlen(text));
}

std::vector<PGScalar> CumulativeCharacterWidths(PGFontHandle font, const char* text, size_t length, PGScalar xoffset, PGScalar maximum_width, lng& render_start, lng& render_end) {
	std::vector<PGScalar> cumulative_widths;
	PGScalar text_size = 0;
	bool found_initial_character = false;
	if (font->character_width > 0) {
		// main font is a monospace font
		int regular_elements = 0;
		for (size_t i = 0; i < length; ) {
			if (text_size - xoffset > maximum_width) {
				return cumulative_widths;
			}
			PGScalar current_width = text_size;
			int offset = utf8_character_length(text[i]);
			if (offset == 1) {
				if (text[i] == '\t') {
					text_size += font->tabwidth * font->character_width;
				} else {
					text_size += font->character_width;
				}
			} else if (offset > 0) {
				if (font->textpaint->getTypeface()->charsToGlyphs(text + i, SkTypeface::kUTF8_Encoding, nullptr, 1) == 0) {
					// if the main font does not support the current glyph, look into the fallback fonts
					bool found_fallback = false;
					for (auto it = font->fallback_paints.begin(); it != font->fallback_paints.end(); it++) {
						if ((*it)->getTypeface()->charsToGlyphs(text + i, SkTypeface::kUTF8_Encoding, nullptr, 1) != 0) {
							text_size += (*it)->measureText(text + i, offset);
							found_fallback = true;
							break;
						}
					}
					if (!found_fallback)
						text_size += font->character_width;
				} else {
					text_size += font->character_width;
				}
			} else {
				text_size += font->textpaint->measureText("\xef\xbf\xbd", 1);
				offset = 1;
			}
			if (text_size >= xoffset) {
				if (!found_initial_character) {
					found_initial_character = true;
					render_start = i;
				}
				for (int p = 0; p < offset; p++) {
					cumulative_widths.push_back(current_width - xoffset);
				}
			}
			i += offset;
		}
	} else {
		// main font is not monospace
		for (size_t i = 0; i < length; ) {
			if (text_size - xoffset > maximum_width) {
				return cumulative_widths;
			}
			PGScalar current_width = text_size;
			int offset = utf8_character_length(text[i]);
			if (offset == 1) {
				if (text[i] == '\t') {
					text_size += font->textpaint->measureText(" ", 1) * font->tabwidth;
				} else {
					text_size += font->textpaint->measureText(text + i, 1);
				}
			} else {
				if (font->textpaint->getTypeface()->charsToGlyphs(text + i, SkTypeface::kUTF8_Encoding, nullptr, 1) == 0) {
					// if the main font does not support the current glyph, look into the fallback fonts
					bool found_fallback = false;
					for (auto it = font->fallback_paints.begin(); it != font->fallback_paints.end(); it++) {
						if ((*it)->getTypeface()->charsToGlyphs(text + i, SkTypeface::kUTF8_Encoding, nullptr, 1) != 0) {
							text_size += (*it)->measureText(text + i, offset);
							found_fallback = true;
							break;
						}
					}
					if (!found_fallback)
						text_size += font->textpaint->measureText("\xef\xbf\xbd", 3);
				} else {
					text_size += font->textpaint->measureText(text + i, offset);
				}
			}
			if (text_size >= xoffset) {
				if (!found_initial_character) {
					render_start = i;
					found_initial_character = true;
				}
				for (int p = 0; p < offset; p++) {
					cumulative_widths.push_back(current_width - xoffset);
				}
			}
			assert(offset > 0);
			i += offset;
		}
	}
	if (text_size > xoffset) {
		cumulative_widths.push_back(text_size - xoffset);
	}
	return cumulative_widths;
}

PGScalar MeasureTextWidth(PGFontHandle font, const char* text, size_t length) {
	PGScalar text_size = 0;
	if (font->character_width > 0) {
		// main font is a monospace font
		int regular_elements = 0;
		for (size_t i = 0; i < length; ) {
			int offset = utf8_character_length(text[i]);
			if (offset == 1) {
				if (text[i] == '\t') {
					regular_elements += font->tabwidth;
				} else {
					regular_elements++;
				}
			} else if (offset > 0) {
				if (font->textpaint->getTypeface()->charsToGlyphs(text + i, SkTypeface::kUTF8_Encoding, nullptr, 1) == 0) {
					// if the main font does not support the current glyph, look into the fallback fonts
					bool found_fallback = false;
					for (auto it = font->fallback_paints.begin(); it != font->fallback_paints.end(); it++) {
						assert((*it)->getTypeface());
						if ((*it)->getTypeface()->charsToGlyphs(text + i, SkTypeface::kUTF8_Encoding, nullptr, 1) != 0) {
							text_size += (*it)->measureText(text + i, offset);
							found_fallback = true;
							break;
						}
					}
					if (!found_fallback)
						regular_elements++;
				} else {
					regular_elements++;
				}
			} else {
				text_size += font->textpaint->measureText("\xef\xbf\xbd", 1);
				offset = 1;
			}
			i += offset;
		}
		text_size += regular_elements * font->character_width;
	} else {
		// main font is not monospace
		for (size_t i = 0; i < length; ) {
			int offset = utf8_character_length(text[i]);
			if (offset == 1) {
				if (text[i] == '\t') {
					text_size += font->textpaint->measureText(" ", 1) * font->tabwidth;
				} else {
					text_size += font->textpaint->measureText(text + i, 1);
				}
			} else if (offset > 0) {
				if (font->textpaint->getTypeface()->charsToGlyphs(text + i, SkTypeface::kUTF8_Encoding, nullptr, 1) == 0) {
					// if the main font does not support the current glyph, look into the fallback fonts
					bool found_fallback = false;
					for (auto it = font->fallback_paints.begin(); it != font->fallback_paints.end(); it++) {
						assert((*it)->getTypeface());
						if ((*it)->getTypeface()->charsToGlyphs(text + i, SkTypeface::kUTF8_Encoding, nullptr, 1) != 0) {
							text_size += (*it)->measureText(text + i, offset);
							found_fallback = true;
							break;
						}
					}
					if (!found_fallback)
						text_size += font->textpaint->measureText("\xef\xbf\xbd", 3);
				} else {
					text_size += font->textpaint->measureText(text + i, offset);
				}
			} else {
				text_size += font->textpaint->measureText("\xef\xbf\xbd", 1);
				offset = 1;
			}
			i += offset;
		}
	}
	return text_size;
}

lng GetPositionInLine(PGFontHandle font, PGScalar x, const char* text, size_t length) {
	PGScalar text_size = 0;
	if (font->character_width > 0) {
		// main font is a monospace font
		for (size_t i = 0; i < length; ) {
			PGScalar old_size = text_size;
			int offset = utf8_character_length(text[i]);
			if (offset == 1) {
				if (text[i] == '\t') {
					text_size += font->character_width * font->tabwidth;
				} else {
					text_size += font->character_width;
				}
			} else {
				if (font->textpaint->getTypeface()->charsToGlyphs(text + i, SkTypeface::kUTF8_Encoding, nullptr, 1) == 0) {
					// if the main font does not support the current glyph, look into the fallback fonts
					bool found_fallback = false;
					for (auto it = font->fallback_paints.begin(); it != font->fallback_paints.end(); it++) {
						if ((*it)->getTypeface()->charsToGlyphs(text + i, SkTypeface::kUTF8_Encoding, nullptr, 1) != 0) {
							text_size += (*it)->measureText(text + i, offset);
							found_fallback = true;
							break;
						}
					}
					if (!found_fallback)
						text_size += font->character_width;
				} else {
					text_size += font->character_width;
				}
			}
			assert(offset > 0);
			if (text_size > x) {
				return i;
			}
			i += offset;
		}
	} else {
		// FIXME: main font is not monospace
		assert(0);
	}
	return length;
}


PGScalar GetTextHeight(PGFontHandle font) {
	SkPaint::FontMetrics metrics;
	font->textpaint->getFontMetrics(&metrics, 0);
	return metrics.fDescent - metrics.fAscent;
}

void RenderCaret(PGRendererHandle renderer, PGFontHandle font, PGScalar selection_offset, PGScalar x, PGScalar y, PGColor color) {
	if (PGGlobalReplayManager::running_replay) return;
	PGScalar line_height = GetTextHeight(font);
	RenderLine(renderer, PGLine(x + selection_offset, y, x + selection_offset, y + line_height), color);
}

void RenderSelection(PGRendererHandle renderer, PGFontHandle font, const char *text, size_t len,
	PGScalar x, PGScalar y, lng start, lng end,
	lng render_start, lng render_end, std::vector<PGScalar>& character_widths,
	PGColor selection_color) {
	if (PGGlobalReplayManager::running_replay) return;
	// if the entire line is selected and the selection continues no the next line
	// we render one extra character to indicate the selected newline
	bool padding = end == len + 1;
	start = std::min((lng)character_widths.size() - 1, std::max((lng)0, start - render_start));
	end = std::min((lng)character_widths.size() - 1, std::max((lng)0, end - render_start));

	PGScalar selection_start = character_widths[start];
	PGScalar selection_end = character_widths[end];
	if (padding) {
		selection_end += font->character_width;
	}
	// render the selection rectangle
	PGScalar lineheight = GetTextHeight(font) + 1;
	RenderRectangle(renderer, PGRect(x + selection_start, y, selection_end - selection_start, lineheight), selection_color, PGDrawStyleFill);

	start += render_start;
	end += render_start;
	// if spaces/tabs are only rendered within the selection, render them now by scanning the text
	// if (!render_spaces_always)
	for (lng i = start; i < end;) {
		PGScalar cumsiz = character_widths[i - render_start];
		int offset = utf8_character_length(text[i]);
		if (offset == 1) {
			if (text[i] == '\t') {
				PGScalar text_size = font->textpaint->measureText(" ", 1) * font->tabwidth;
				PGScalar lineheight = GetTextHeight(font);
				RenderLine(renderer, PGLine(x + cumsiz + 1, y + lineheight / 2, x + cumsiz + text_size - 1, y + lineheight / 2), PGColor(255, 255, 255, 64), 1);
			} else if (text[i] == ' ') {
				char bullet_point[3] = { 0, 0, 0 };
				bullet_point[0] |= 0xE2;
				bullet_point[1] |= 0x88;
				bullet_point[2] |= 0x99;
				PGColor color = GetTextColor(font);
				SetTextColor(font, PGColor(255, 255, 255, 64));
				RenderText(renderer, font, bullet_point, 3, x + cumsiz, y);
				SetTextColor(font, color);
			}
		}
		i += offset;
	}


}

void SetTextColor(PGFontHandle font, PGColor color) {
	if (font->normaltext) {
		font->normaltext->setColor(SkColorSetARGB(color.a, color.r, color.g, color.b));
	}
	if (font->boldtext) {
		font->boldtext->setColor(SkColorSetARGB(color.a, color.r, color.g, color.b));
	}
	if (font->italictext) {
		font->italictext->setColor(SkColorSetARGB(color.a, color.r, color.g, color.b));
	}
	for (auto it = font->fallback_paints.begin(); it != font->fallback_paints.end(); it++) {
		(*it)->setColor(SkColorSetARGB(color.a, color.r, color.g, color.b));
	}
}

PGColor GetTextColor(PGFontHandle font) {
	SkColor color = font->textpaint->getColor();
	return PGColor(SkColorGetR(color), SkColorGetG(color), SkColorGetB(color), SkColorGetA(color));
}

void SetTextFontSize(PGFontHandle font, PGScalar height) {
	font->textpaint->setTextSize(height);
	font->character_width = font->textpaint->measureText("i", 1);
	PGScalar max_width = font->textpaint->measureText("W", 1);
	if (panther::abs(font->character_width - max_width) > 0.1f) {
		// Set renderer->character_width to -1 for non-monospace fonts
		font->character_width = -1;
	}
	font->text_offset = font->textpaint->getFontBounds().height() / 2 + font->textpaint->getFontBounds().height() / 4;
	if (font->normaltext) {
		font->normaltext->setTextSize(height);
	}
	if (font->boldtext) {
		font->boldtext->setTextSize(height);
	}
	if (font->italictext) {
		font->italictext->setTextSize(height);
	}
	for (auto it = font->fallback_paints.begin(); it != font->fallback_paints.end(); it++) {
		(*it)->setTextSize(height);
	}
}

void SetTextTabWidth(PGFontHandle font, int tabwidth) {
	font->tabwidth = tabwidth;
}

PGScalar GetTextFontSize(PGFontHandle font) {
	return font->textpaint->getTextSize();
}

void RenderFileIcon(PGRendererHandle renderer, PGFontHandle font, const char *text,
	PGScalar x, PGScalar y, PGScalar width, PGScalar height,
	PGColor text_color, PGColor page_color, PGColor edge_color) {
	if (PGGlobalReplayManager::running_replay) return;

	PGPolygon polygon;
	polygon.closed = true;
	polygon.points.push_back(PGPoint(x, y));
	polygon.points.push_back(PGPoint(x + width * 0.8f, y));
	polygon.points.push_back(PGPoint(x + width, y + height * 0.2f));
	polygon.points.push_back(PGPoint(x + width, y + height));
	polygon.points.push_back(PGPoint(x, y + height));

	RenderPolygon(renderer, polygon, page_color);
	RenderPolygon(renderer, polygon, edge_color, 1);

	// render the two edges of the fold
	RenderLine(renderer, PGLine(PGPoint(x + width * 0.8f, y), PGPoint(x + width * 0.8f, y + height * 0.2f)), edge_color, 1);
	RenderLine(renderer, PGLine(PGPoint(x + width * 0.8f, y + height * 0.2f), PGPoint(x + width, y + height * 0.2f)), edge_color, 1);
	// render the file extension
	size_t length = strlen(text);
	if (length == 0) return;
	if (length > 4) length = 4;
	SetTextColor(font, text_color);
	PGScalar original_size = GetTextFontSize(font);
	PGScalar current_size = original_size;
	PGScalar text_width = MeasureTextWidth(font, text);
	PGScalar text_height = GetTextHeight(font);
	int state = -1;
	// FIXME: font size should not be determined like this constantly
	while (true) {
		if (current_size <= 4 || current_size >= 100) break;
		if (text_height > 0.5f * height || text_width > 0.8f * width) {
			if (state == 0)
				break;
			current_size--;
			state = 1;
		} else if (text_height < 0.5f * height || text_width < 0.8f * width) {
			if (state == 1)
				break;
			current_size++;
			state = 0;
		} else {
			break;
		}
		SetTextFontSize(font, current_size);
		text_width = MeasureTextWidth(font, text);
		text_height = GetTextHeight(font);
	}
	RenderText(renderer, font, text, strlen(text), x + 0.5f * width, y + 0.5f * height - GetTextHeight(font) / 2, PGTextAlignHorizontalCenter);

	SetTextFontSize(font, original_size);
}

void SetTextStyle(PGFontHandle font, PGTextStyle style) {
	if (style == PGTextStyleNormal) {
		assert(font->normaltext);
		font->textpaint = font->normaltext;
	} else if (style == PGTextStyleBold) {
		assert(font->boldtext);
		font->textpaint = font->boldtext;
	} else if (style == PGTextStyleItalic) {
		assert(font->italictext);
		font->textpaint = font->italictext;
	}
}

void SetRenderBounds(PGRendererHandle handle, PGRect rectangle) {
	if (PGGlobalReplayManager::running_replay) return;
	handle->canvas->save();
	handle->canvas->clipRect(CreateSkRect(rectangle));
}

void ClearRenderBounds(PGRendererHandle handle) {
	if (PGGlobalReplayManager::running_replay) return;
	handle->canvas->restore();
}

PGSize PGMeasurePopupItem(PGFontHandle font, PGPopupInformation* information, PGScalar text_width, PGScalar hotkey_width, PGPopupType type) {
	PGSize result;
	if (type == PGPopupTypeSeparator) {
		result.height = 1;
		result.width = 20;
	} else if (type == PGPopupTypeSubmenu || type == PGPopupTypeEntry) {
		result.height = GetTextHeight(font) * 1.3;
		result.width = 20 + text_width + 20 + hotkey_width + 10;
	} else if (type == PGPopupTypeMenu) {
		result.height = GetTextHeight(font) * 1.3;
		result.width = 10 + MeasureTextWidth(font, information->text.c_str());
	}
	return result;
}

#include "style.h"

void PGRenderPopupItem(PGRendererHandle renderer, PGPoint point, PGFontHandle font, PGPopupInformation* info, PGSize size, PGPopupMenuFlags flags, PGScalar text_width, PGScalar hotkey_width, PGPopupType type) {
	if (PGGlobalReplayManager::running_replay) return;
	PGColor background_color;
	PGColor text_color;

	if (type == PGPopupTypeMenu) {
		background_color = PGStyleManager::GetColor(PGColorMainMenuBackground);
		if (flags & PGPopupMenuHighlighted) {
			background_color = PGStyleManager::GetColor(PGColorMenuHover);
		}
		if (flags & PGPopupMenuSelected) {
			background_color = PGStyleManager::GetColor(PGColorMenuBackground);
		}
	} else {
		background_color = PGStyleManager::GetColor(PGColorMenuBackground);
		if (flags & PGPopupMenuSelected) {
			background_color = PGStyleManager::GetColor(PGColorMenuHover);
		}
	}
	text_color = PGStyleManager::GetColor(PGColorMenuText);
	if (flags & PGPopupMenuGrayed) {
		text_color = PGStyleManager::GetColor(PGColorMenuDisabled);
	}
	RenderRectangle(renderer, PGRect(point.x, point.y, size.width, size.height), background_color, PGDrawStyleFill);
	if (type == PGPopupTypeSeparator) {
		RenderLine(renderer, PGLine(point + PGPoint(15, size.height / 2), point + PGPoint(size.width - 15, size.height / 2)), text_color, 1);
	} else if (type == PGPopupTypeSubmenu || type == PGPopupTypeEntry) {
		if (info->image) {
			RenderImage(renderer, info->image, PGRect(point.x + 2, point.y + 2, 16, 16));
		}
		SetTextColor(font, text_color);
		RenderText(renderer, font, info->text.c_str(), info->text.size(), point.x + 20, point.y);
		RenderText(renderer, font, info->hotkey.c_str(), info->hotkey.size(), point.x + 40 + text_width, point.y);
	} else if (type == PGPopupTypeMenu) {
		if (flags & PGPopupMenuSelected) {
			PGColor stroke_color = PGStyleManager::GetColor(PGColorMenuDisabled);
			RenderRectangle(renderer, PGRect(point.x, point.y, size.width, size.height + 10), stroke_color, PGDrawStyleStroke);
		}

		SetTextColor(font, text_color);
		RenderText(renderer, font, info->text.c_str(), info->text.size(), point.x + 10, point.y, PGTextAlignLeft);
	}
}

#include <SkCodec.h>

PGBitmapHandle PGLoadImage(std::string path) {
	SkCodec* codec = SkCodec::NewFromStream(new SkFILEStream(path.c_str()), nullptr);
	if (!codec) return nullptr;

	PGBitmapHandle handle = new PGBitmap();
	handle->bitmap = new SkBitmap();
	auto info = codec->getInfo();
	handle->bitmap->allocPixels(info);
	auto result = codec->getPixels(info, handle->bitmap->getPixels(), handle->bitmap->rowBytes());
	delete codec;
	if (result != SkCodec::kSuccess) {
		delete handle;
		return nullptr;
	}
	return handle;
}
