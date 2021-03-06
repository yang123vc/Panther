#pragma once

#include "utils.h"
#include "textbuffer.h"
#include "textposition.h"

struct PGRegex;
typedef PGRegex* PGRegexHandle;

typedef void PGMatchCallback(void* data, std::string filename, const std::vector<std::string>& lines, const std::vector<PGCursorRange>& matches, lng initial_line);

#define PGREGEX_MAXIMUM_MATCHES 16

struct PGRegexMatch {
	bool matched;
	PGTextRange groups[PGREGEX_MAXIMUM_MATCHES];
};

typedef int PGRegexFlags;

extern const PGRegexFlags PGRegexFlagsNone;
extern const PGRegexFlags PGRegexCaseInsensitive;
extern const PGRegexFlags PGRegexWholeWordSearch;

bool PGRegexHasErrors(PGRegexHandle handle);
std::string PGGetRegexError(PGRegexHandle handle);
std::string PGGetRegexPattern(PGRegexHandle handle);
PGRegexHandle PGCompileRegex(std::string pattern, bool is_regex, PGRegexFlags);
PGRegexMatch PGMatchRegex(PGRegexHandle handle, PGTextRange context, PGDirection direction);
PGRegexMatch PGMatchRegex(PGRegexHandle handle, const char* data, lng size, PGDirection direction);
PGRegexMatch PGMatchRegex(PGRegexHandle handle, std::string& context, PGDirection direction);
int PGRegexNumberOfCapturingGroups(PGRegexHandle handle);
void PGDeleteRegex(PGRegexHandle handle);

