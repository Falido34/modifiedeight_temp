#pragma once
#include <_types.h>
#include <gui/Screen.hpp>
#include <string>
#include <vector>

namespace Touch {
	struct TButton;
	struct THeader;
}

struct PatchErrorScreen : Screen {
	enum ReturnTarget {
		RETURN_START_MENU = 0,
		RETURN_OPTIONS = 1,
		RETURN_PATCHES = 2
	};

	std::string rawError;
	std::vector<std::string> wrappedLines;
	int wrappedForWidth;
	ReturnTarget returnTarget;
	bool optionsIsInWorld;

	Touch::THeader* header;
	Touch::TButton* btnBack;
	Touch::TButton* btnCopy;
	Touch::TButton* btnPatches;

	int scrollY;
	int contentHeight;
	bool isDragging;
	int dragStartMouseY;
	int dragStartScrollY;
	std::string statusBanner;
	uint32_t statusColor;

	PatchErrorScreen(const std::string& errorMsg, ReturnTarget target = RETURN_START_MENU, bool inWorld = false);
	virtual ~PatchErrorScreen();

	void rebuildWrappedLines();

	virtual void init();
	virtual void setupPositions();
	virtual void render(int32_t mx, int32_t my, float pt);
	virtual void tick();
	virtual bool_t handleBackEvent(bool_t a2);
	virtual bool_t renderGameBehind();
	virtual bool_t isInGameScreen();
	virtual void buttonClicked(Button* btn);
	virtual void mouseClicked(int32_t mx, int32_t my, int32_t btn);
	virtual void mouseReleased(int32_t mx, int32_t my, int32_t btn);
	virtual void keyPressed(int32_t keyCode);
};
