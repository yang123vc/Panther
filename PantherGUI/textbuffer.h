#pragma once

#include "syntax.h"
#include "utils.h"
#include <string>
#include <vector>

#define TEXT_BUFFER_SIZE 100

class TextFile;
struct TextLine;
struct PGTextBuffer;

struct PGCursorPosition {
	lng line;
	lng position;
	lng character;
};

struct PGBufferUpdate {
	lng split_point = 0;
	PGTextBuffer* new_buffer = nullptr;
	PGBufferUpdate(lng offset) : split_point(offset), new_buffer(nullptr) { }
	PGBufferUpdate(lng split_point, PGTextBuffer* new_buffer) : split_point(split_point), new_buffer(new_buffer) { }
};

struct PGTextBuffer {
public:
	PGTextBuffer(const char* text, lng size, lng start_line);
	~PGTextBuffer();

	char* buffer = nullptr;
	ulng buffer_size = 0;
	ulng current_size = 0;
	ulng start_line = 0;

	PGSyntax* syntax = nullptr;
	PGParserState state = nullptr;
	bool parsed = false;

	PGTextBuffer* prev = nullptr;
	PGTextBuffer* next = nullptr;

	void Extend(ulng new_size);

	static lng GetBuffer(std::vector<PGTextBuffer*>& buffers, lng line);

	// insert text into the specified buffer, "text" should not contain newlines
	// this function accounts for extending buffers and creating new buffers
	static PGBufferUpdate InsertText(std::vector<PGTextBuffer*>& buffers, PGTextBuffer* buffer, ulng position, std::string text, ulng linecount);

	// delete text from the specified buffer, text is deleted rightwards =>
	// this function can merge adjacent buffers together, if two adjacent buffers
	// are almost empty
	static PGBufferUpdate DeleteText(std::vector<PGTextBuffer*>& buffers, PGTextBuffer* buffer, ulng position, ulng size);


	// insert text into the current buffer, this should only be called if the text fits within the buffer
	void InsertText(ulng position, std::string text);
	// delete text from the current buffer without newlines
	void DeleteText(ulng position, ulng size);
	// delete text from the current buffer with potential newlines
	// returns the amount of deleted lines
	lng DeleteLines(ulng position, ulng end_position);
	lng DeleteLines(ulng start);

	// gets the total amount of lines in the buffer, parameter should be total amount of lines in the text file
	lng GetLineCount(lng total_lines);
	// gets all the lines in the buffer as TextLines
	std::vector<TextLine> GetLines();
	ulng GetBufferLocationFromCursor(lng line, lng character);
	PGCursorPosition GetCursorFromPosition(ulng position);
	TextLine GetLineFromPosition(ulng position);
};

struct TextLine {
	friend class TextLineIterator;
public:
	TextLine() : line(nullptr), length(0) { syntax.end = -1; }
	TextLine(char* line, lng length, PGSyntax syntax) : line(line), length(length), syntax(syntax) { }
	TextLine(PGTextBuffer* buffer, lng line);

	lng GetLength(void) { return length; }
	char* GetLine(void) { return line; }
	bool IsValid() { return line != nullptr; }

	PGSyntax syntax;
private:
	char* line;
	lng length;
};

class TextLineIterator {
	friend class TextFile;
public:
	TextLine GetLine();
	friend TextLineIterator& operator++(TextLineIterator& element) {
		element.NextLine();
		return element;
	}
	friend TextLineIterator& operator--(TextLineIterator& element) {
		element.PrevLine();
		return element;
	}
	friend TextLineIterator& operator++(TextLineIterator& element, int i) {
		return ++element;
	}
	friend TextLineIterator& operator--(TextLineIterator& element, int i) {
		return ++element;
	}

	PGTextBuffer* CurrentBuffer() { return buffer; }
	TextLineIterator(TextFile* textfile, PGTextBuffer* buffer);
private:
	TextLineIterator(TextFile* textfile, lng current_line);

	void PrevLine();
	void NextLine();

	PGTextBuffer* buffer;
	lng start_position, end_position;
	TextFile* textfile;
	lng current_line;
	TextLine textline;
};