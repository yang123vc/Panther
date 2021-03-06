
#include "textfile.h"


class StreamingTextFile : public TextFile {
public:
	~StreamingTextFile();

	static std::shared_ptr<TextFile> OpenTextFile(std::string filename, PGFileError& error, bool ignore_binary = false);

	TextLine GetLine(lng linenumber);

	void InsertText(std::vector<Cursor>& cursors, char character);
	void InsertText(std::vector<Cursor>& cursors, PGUTF8Character character);
	void InsertText(std::vector<Cursor>& cursors, std::string text);

	void DeleteCharacter(std::vector<Cursor>& cursors, PGDirection direction);
	void DeleteWord(std::vector<Cursor>& cursors, PGDirection direction);

	void AddNewLine(std::vector<Cursor>& cursors);
	void AddNewLine(std::vector<Cursor>& cursors, std::string text);

	void DeleteLines(std::vector<Cursor>& cursors);
	void DeleteLine(std::vector<Cursor>& cursors, PGDirection direction);

	void AddEmptyLine(std::vector<Cursor>& cursors, PGDirection direction);
	void MoveLines(std::vector<Cursor>& cursors, int offset);

	std::string GetText();
	std::string CutText(std::vector<Cursor>& cursors);
	std::string CopyText(std::vector<Cursor>& cursors);

	void PasteText(std::vector<Cursor>& cursors, std::string& text);
	void RegexReplace(std::vector<Cursor>& cursors, PGRegexHandle regex, std::string& replacement);
	bool Reload(PGFileError& error);

	void ChangeIndentation(PGLineIndentation indentation);
	void RemoveTrailingWhitespace();

	void Undo(TextView* view);
	void Redo(TextView* view);

	void SaveChanges();
	void SetLanguage(PGLanguage* language);

	lng GetLineCount();
	void IndentText(std::vector<Cursor>& cursors, PGDirection direction);

	PGScalar GetMaxLineWidth(PGFontHandle font);
	PGStoreFileType WorkspaceFileStorage();

	PGTextRange FindMatch(PGRegexHandle regex_handle, PGDirection direction, lng start_line, lng start_character, lng end_line, lng end_character, bool wrap);
	PGTextRange FindMatch(PGRegexHandle regex_handle, PGDirection direction, PGTextBuffer* start_buffer, lng start_position, PGTextBuffer* end_buffer, lng end_position, bool wrap);

	void FindMatchesWithContext(FindAllInformation* info, PGRegexHandle regex_handle, int context_lines, PGMatchCallback callback, void* data);
	void ConvertToIndentation(PGLineIndentation indentation);

	PGTextBuffer* GetBuffer(lng line);

	PGTextBuffer* GetBufferFromWidth(double width);
	PGTextBuffer* GetFirstBuffer();
	PGTextBuffer* GetLastBuffer();
private:
	PGEncoderHandle decoder = nullptr;

	char* output = nullptr;
	lng output_size = 0;
	char* intermediate_buffer = nullptr;
	lng intermediate_size = 0;

	char* cached_buffer = nullptr;
	size_t cached_index = 0;
	size_t cached_size = 0;
	
	StreamingTextFile(PGFileHandle handle, std::string filename);

	lng ReadIntoBuffer(char*& buffer, lng& start_index);
	bool ReadBlock();

	PGFileHandle handle;
};