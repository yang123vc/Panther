
#include "unicode.h"
#include "xml.h"


#include <vector>

const PGSyntaxType PGXMLElementName = PGSyntaxKeyword;
const PGSyntaxType PGXMLAttributeName = PGSyntaxClass3;

enum PGParserXMLState {
	PGParserXMLDefault,
	PGParserXMLComment,
	PGParserXMLInsideStartTag,
	PGParserXMLInsideEndTag,
	PGParserXMLElementName,
	PGParserXMLInElement,
	PGParserXMLOpenValue,
	PGParserXMLSpecialName,
	PGParserXMLStartValue
};

struct XMLToken {
	std::string name;
	XMLToken(std::string name) : name(name) { }
};

struct XMLParserState {
	PGParserXMLState state;
	std::string current_token;
	std::vector<XMLToken> open_tokens;
};

PGParserState XMLHighlighter::IncrementalParseLine(TextLine& line, lng linenr, PGParserState s, PGParseErrors& errors, PGSyntax& current) {
	XMLParserState* state = (XMLParserState*)s;
	char* text = line.GetLine();
	lng size = line.GetLength();
	// free the current syntax of this line
	current.syntax.clear();
	for (lng i = 0; i < size; i++) {
		int utf8_length = utf8_character_length(text[i]);
		if (utf8_length > 1) {
			// special characters are not supported in the keyword highlighter (for now)
			i += utf8_length - 1;
			continue;
		}
		if (text[i] == '<') {
			if (state->state == PGParserXMLDefault) {
				current.syntax.push_back(PGSyntaxNode(PGSyntaxNone, i + 1));
				state->state = PGParserXMLElementName;
			} else if (state->state != PGParserXMLComment && state->state != PGParserXMLOpenValue && state->state != PGParserXMLDefault) {
				// Parse Error
				continue;
			}
		} else if (text[i] == '>') {
			if (state->state == PGParserXMLInElement) {
				state->state = PGParserXMLDefault;
				current.syntax.push_back(PGSyntaxNode(PGSyntaxNone, i + 1));
			} else if (state->state == PGParserXMLElementName || state->state == PGParserXMLSpecialName) {
				current.syntax.push_back(PGSyntaxNode(PGXMLElementName, i));
				current.syntax.push_back(PGSyntaxNode(PGSyntaxNone, i + 1));
				state->state = PGParserXMLDefault;
			} else if (state->state != PGParserXMLComment && state->state != PGParserXMLOpenValue) {
				// Parse Error
				continue;
			}
		} else if (text[i] == ' ' || text[i] == '\t') {
			if (state->state == PGParserXMLElementName || state->state == PGParserXMLSpecialName) {
				current.syntax.push_back(PGSyntaxNode(PGXMLElementName, i));
				state->state = PGParserXMLInElement;
			}
		} else if (text[i] == '=') {
			if (state->state == PGParserXMLInElement) {
				// Attribute Name
				current.syntax.push_back(PGSyntaxNode(PGXMLAttributeName, i));
				state->state = PGParserXMLStartValue;
			}
		} else if (text[i] == '"' || text[i] == '\'') {
			if (state->state == PGParserXMLStartValue) {
				current.syntax.push_back(PGSyntaxNode(PGSyntaxNone, i));
				state->state = PGParserXMLOpenValue;
			} else if (state->state == PGParserXMLOpenValue) {
				// Attribute Value
				current.syntax.push_back(PGSyntaxNode(PGSyntaxString, i + 1));
				state->state = PGParserXMLInElement;
			} else if (state->state != PGParserXMLComment && state->state != PGParserXMLOpenValue && state->state != PGParserXMLDefault) {
				// Parse Error
			}
		} else if (text[i] == '!') {
			if (state->state == PGParserXMLElementName) {
				state->state = PGParserXMLSpecialName;
			}
		} else if (text[i] == '-') {
			if (state->state == PGParserXMLSpecialName) {
				if (i + 1 < size && text[i + 1] == '-') {
					state->state = PGParserXMLComment;
					i++;
				}
			} else if (state->state == PGParserXMLComment) {
				if (i + 2 < size && text[i + 1] == '-' && text[i + 2] == '>') {
					state->state = PGParserXMLDefault;
					i += 2;
					current.syntax.push_back(PGSyntaxNode(PGSyntaxComment, i));
				}
			}
		}
	}
	if (state->state == PGParserXMLComment) {
		current.syntax.push_back(PGSyntaxNode(PGSyntaxComment, size));
	} else {
		// FIXME: error
		state->state = PGParserXMLDefault;
	}
	return state;
}

PGParserState XMLHighlighter::GetDefaultState() {
	XMLParserState* state = new XMLParserState();
	state->state = PGParserXMLDefault;
	return state;
}

PGParserState XMLHighlighter::CopyParserState(const PGParserState inp) {
	XMLParserState* original = (XMLParserState*)inp;
	XMLParserState* state = new XMLParserState();
	state->state = original->state;
	state->open_tokens.insert(state->open_tokens.begin(), original->open_tokens.begin(), original->open_tokens.end());
	return state;
}

void XMLHighlighter::DeleteParserState(PGParserState inp) {
	delete (XMLParserState*) inp;
}

bool XMLHighlighter::StateEquivalent(const PGParserState a, const PGParserState b) {
	XMLParserState* a_xml = (XMLParserState*)a;
	XMLParserState* b_xml = (XMLParserState*)b;
	return a_xml->state == b_xml->state;
}