#pragma once

#pragma once

#include "button.h"
#include "container.h"
#include "simpletextfield.h"
#include "togglebutton.h"
#include "scrollbar.h"

struct PGDirectory;

struct PGDirectory {
	std::string path;
	std::vector<PGDirectory*> directories;
	std::vector<PGFile> files;
	lng last_modified_time;
	bool loaded_files;
	bool expanded;

	void FindFile(lng file_number, PGDirectory** directory, PGFile* file);

	PGDirectory(std::string path);
	~PGDirectory();
	// Returns the number of files displayed by this directory
	lng DisplayedFiles();
	void Update();
};

class ProjectExplorer : public PGContainer {
public:
	ProjectExplorer(PGWindowHandle window);
	~ProjectExplorer();

	void PeriodicRender(void);

	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseMove(int x, int y, PGMouseButton buttons);
	void MouseWheel(int x, int y, double distance, PGModifier modifier);

	void OnResize(PGSize old_size, PGSize new_size);

	void Draw(PGRendererHandle renderer, PGIRect* rect);
	
	PG_CONTROL_KEYBINDINGS;
private:
	PGFontHandle font;

	std::vector<PGDirectory*> directories;

	bool dragging_scrollbar;
	double scrollbar_offset;

	Scrollbar* scrollbar;

	std::vector<lng> selected_files;
	
	enum PGSelectFileType {
		PGSelectSingleFile,
		PGSelectAddSingleFile,
		PGSelectAddRangeFile
	};

	void FindFile(lng file_number, PGDirectory** directory, PGFile* file);
	void SelectFile(lng selected_file, PGSelectFileType type);

	lng TotalFiles();
	lng MaximumScrollOffset();
	lng RenderedFiles();

	void DrawFile(PGRendererHandle renderer, PGFile file, PGScalar x, PGScalar& y, bool selected);
	void DrawDirectory(PGRendererHandle renderer, PGDirectory& directory, PGScalar x, PGScalar& y, lng& current_offset, lng offset, lng& selection);
};