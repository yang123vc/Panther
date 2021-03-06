#pragma once

#include "syntaxhighlighter.h"
#include "language.h"

class XMLHighlighter : public SyntaxHighlighter {
public:
	SyntaxHighlighterType GetType() { return PGSyntaxHighlighterIncremental; }
	PGParserState IncrementalParseLine(TextLine& line, lng linenr, PGParserState state, PGParseErrors& errors, PGSyntax& syntax);
	PGParserState GetDefaultState();
	PGParserState CopyParserState(const PGParserState state);
	bool StateEquivalent(const PGParserState a, const PGParserState b);
	void DeleteParserState(PGParserState state);
};

class XMLLanguage : public PGLanguage {
	std::string GetName() { return "XML"; }
	SyntaxHighlighter* CreateHighlighter() { return new XMLHighlighter(); }
	bool MatchesFileExtension(std::string extension) { return extension == "xml"; }
	virtual std::string GetExtension() { return "XML"; }
	PGColor GetColor() { return PGColor(34, 139, 34); }
};
