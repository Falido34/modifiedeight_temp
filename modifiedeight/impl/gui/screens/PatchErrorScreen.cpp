#include <gui/screens/PatchErrorScreen.hpp>
#include <gui/screens/FirstLaunchAcknowledgementScreen.hpp>
#include <gui/screens/OptionsScreen.hpp>
#include <gui/screens/PatchesScreen.hpp>
#include <gui/buttons/Touch_TButton.hpp>
#include <gui/buttons/Touch_THeader.hpp>
#include <Minecraft.hpp>
#include <input/Mouse.hpp>
#include <patch/PatchManager.hpp>
#include <rendering/Font.hpp>
#include <algorithm>

PatchErrorScreen::PatchErrorScreen(const std::string& errorMsg, ReturnTarget target, bool inWorld)
	: rawError(PatchManager::stripAnsiCodes(errorMsg)),
	  wrappedForWidth(-1),
	  returnTarget(target),
	  optionsIsInWorld(inWorld),
	  header(nullptr),
	  btnBack(nullptr),
	  btnCopy(nullptr),
	  btnPatches(nullptr),
	  scrollY(0),
	  contentHeight(0),
	  isDragging(false),
	  dragStartMouseY(0),
	  dragStartScrollY(0),
	  statusColor(0xFF55FF55) {
	if (this->rawError.empty()) {
		this->rawError = "Unknown error occurred while loading patch.";
	}
}

PatchErrorScreen::~PatchErrorScreen() {
	delete this->header;
	delete this->btnBack;
	delete this->btnCopy;
	delete this->btnPatches;
}

void PatchErrorScreen::init() {
	this->buttons.clear();
	this->field_2C.clear();

	delete this->header;
	delete this->btnBack;
	delete this->btnCopy;
	delete this->btnPatches;

	this->header = new Touch::THeader(0, "Patch Load Error");
	this->btnBack = new Touch::TButton(1, (this->returnTarget == RETURN_START_MENU) ? "Continue" : "Back", this->minecraft);
	this->btnCopy = new Touch::TButton(2, "Copy Error", this->minecraft);
	this->btnPatches = new Touch::TButton(3, "Patches", this->minecraft);

	this->buttons.push_back(this->header);
	this->buttons.push_back(this->btnBack);
	this->buttons.push_back(this->btnCopy);
	this->buttons.push_back(this->btnPatches);

	this->field_2C.push_back(this->btnCopy);
	this->field_2C.push_back(this->btnPatches);
	this->field_2C.push_back(this->btnBack);

	this->wrappedForWidth = -1;
	this->setupPositions();
}

void PatchErrorScreen::rebuildWrappedLines() {
	this->wrappedLines.clear();
	if (!this->font) return;

	int boxW = this->width - 28;
	int maxLinePx = (boxW > 40) ? (boxW - 14) : 40;
	this->wrappedForWidth = this->width;

	std::string cur;
	for (size_t i = 0; i <= this->rawError.size(); ++i) {
		char ch = (i < this->rawError.size()) ? this->rawError[i] : '\n';
		if (ch == '\r') continue;
		if (ch == '\n') {
			// Hard-wrap long lines smoothly by words then characters
			std::string rem = cur;
			cur.clear();
			if (rem.empty()) {
				this->wrappedLines.push_back("");
				continue;
			}
			while (!rem.empty()) {
				if (this->font->width(rem) <= maxLinePx) {
					this->wrappedLines.push_back(rem);
					break;
				}
				size_t cut = 0;
				size_t lastSpace = std::string::npos;
				for (size_t k = 1; k <= rem.size(); ++k) {
					if (rem[k - 1] == ' ' || rem[k - 1] == '/' || rem[k - 1] == ':' || rem[k - 1] == '\\') {
						lastSpace = k;
					}
					if (this->font->width(rem.substr(0, k)) > maxLinePx) {
						cut = (k > 1) ? (k - 1) : 1;
						break;
					}
					cut = k;
				}
				if (lastSpace != std::string::npos && lastSpace > 4 && lastSpace <= cut) {
					cut = lastSpace;
				}
				this->wrappedLines.push_back(rem.substr(0, cut));
				rem = rem.substr(cut);
			}
		} else if (ch == '\t') {
			cur += "  ";
		} else {
			cur.push_back(ch);
		}
	}

	this->contentHeight = (int)this->wrappedLines.size() * 10 + 8;
}

void PatchErrorScreen::setupPositions() {
	if (this->header) {
		this->header->posX = 0;
		this->header->posY = 0;
		this->header->width = this->width;
		this->header->height = 24;
	}

	int btnH = 22;
	int bottomY = this->height - btnH - 8;
	int btnW = std::min(100, std::max(64, (this->width - 36) / 3));
	int totalW = btnW * 3 + 16;
	int startX = (this->width - totalW) / 2;

	if (this->btnCopy) {
		this->btnCopy->posX = startX;
		this->btnCopy->posY = bottomY;
		this->btnCopy->width = btnW;
		this->btnCopy->height = btnH;
	}
	if (this->btnPatches) {
		this->btnPatches->posX = startX + btnW + 8;
		this->btnPatches->posY = bottomY;
		this->btnPatches->width = btnW;
		this->btnPatches->height = btnH;
	}
	if (this->btnBack) {
		this->btnBack->posX = startX + (btnW + 8) * 2;
		this->btnBack->posY = bottomY;
		this->btnBack->width = btnW;
		this->btnBack->height = btnH;
	}

	this->rebuildWrappedLines();
}

void PatchErrorScreen::render(int32_t mx, int32_t my, float pt) {
	this->renderDirtBackground(0);

	if (this->wrappedForWidth != this->width) {
		this->rebuildWrappedLines();
	}

	int boxX = 12;
	int boxY = 30;
	int boxW = this->width - 24;
	int boxH = this->height - 74;
	if (boxH < 40) boxH = 40;

	if (this->isDragging && this->minecraft && this->minecraft->field_20 > 0) {
		int guiMY = this->height * Mouse::getY() / this->minecraft->field_20 - 1;
		int dy = guiMY - this->dragStartMouseY;
		this->scrollY = this->dragStartScrollY - dy;
	}

	int maxScroll = std::max(0, this->contentHeight - boxH + 6);
	if (this->scrollY < 0) this->scrollY = 0;
	if (this->scrollY > maxScroll) this->scrollY = maxScroll;

	// Outer red-accented border + dark log panel
	this->fill(boxX - 1, boxY - 1, boxX + boxW + 1, boxY + boxH + 1, 0xFFCC4444);
	this->fill(boxX, boxY, boxX + boxW, boxY + boxH, 0xCC121216);

	if (this->font) {
		int y = boxY + 6 - this->scrollY;
		for (size_t i = 0; i < this->wrappedLines.size(); ++i) {
			if (y + 9 >= boxY + 2 && y <= boxY + boxH - 10) {
				const std::string& line = this->wrappedLines[i];
				int32_t col = 0xFFE8E8E8;
				if (line.find("error:") != std::string::npos || line.find("FAILED") != std::string::npos || line.find("Failed") != std::string::npos) {
					col = 0xFFFF6666;
				} else if (line.find("warning:") != std::string::npos) {
					col = 0xFFFFCC55;
				}
				this->font->drawShadow(line, (float)(boxX + 6), (float)y, col);
			}
			y += 10;
		}
	}

	// Scrollbar when scrollable
	if (maxScroll > 0) {
		int barX = boxX + boxW - 5;
		this->fill(barX, boxY + 2, barX + 3, boxY + boxH - 2, 0x55FFFFFF);
		int trackH = boxH - 4;
		int thumbH = std::max(14, (trackH * boxH) / std::max(boxH, this->contentHeight));
		int thumbY = boxY + 2 + ((trackH - thumbH) * this->scrollY) / maxScroll;
		this->fill(barX, thumbY, barX + 3, thumbY + thumbH, 0xFFDD5555);
	}

	if (!this->statusBanner.empty() && this->font) {
		this->drawCenteredString(this->font, this->statusBanner, this->width / 2, boxY + boxH + 3, this->statusColor);
	}

	Screen::render(mx, my, pt);
}

void PatchErrorScreen::tick() {
	Screen::tick();
}

bool_t PatchErrorScreen::renderGameBehind() {
	return 0;
}

bool_t PatchErrorScreen::isInGameScreen() {
	return 0;
}

bool_t PatchErrorScreen::handleBackEvent(bool_t a2) {
	if (a2) return 1;
	PatchManager::instance().clearError();
	if (this->returnTarget == RETURN_OPTIONS) {
		this->minecraft->setScreen(new OptionsScreen(this->optionsIsInWorld));
	} else if (this->returnTarget == RETURN_PATCHES) {
		this->minecraft->setScreen(new PatchesScreen(false, this->optionsIsInWorld));
	} else {
		if (!this->minecraft->options.firstLaunchAcknowledged) {
			this->minecraft->setScreen(new FirstLaunchAcknowledgementScreen());
		} else {
			this->minecraft->screenChooser.setScreen(START_MENU_SCREEN);
		}
	}
	return 1;
}

void PatchErrorScreen::buttonClicked(Button* btn) {
	if (!btn) return;
	if (btn == this->btnCopy) {
		bool copied = PatchManager::copyTextToClipboard(this->rawError);
		if (copied) {
			this->statusBanner = "Copied exact error message to clipboard!";
			this->statusColor = 0xFF55FF55;
		} else {
			this->statusBanner = "Saved error to " + PatchManager::instance().getPatchesDir() + "/last_error.txt";
			this->statusColor = 0xFFFFDD55;
		}
		return;
	}
	if (btn == this->btnPatches) {
		PatchManager::instance().clearError();
		this->minecraft->setScreen(new PatchesScreen(this->returnTarget == RETURN_START_MENU, this->optionsIsInWorld));
		return;
	}
	if (btn == this->btnBack) {
		this->handleBackEvent(false);
	}
}

void PatchErrorScreen::mouseClicked(int32_t mx, int32_t my, int32_t btn) {
	if (btn == 4) {
		this->scrollY -= 24;
		return;
	}
	if (btn == 5) {
		this->scrollY += 24;
		return;
	}
	int boxX = 12;
	int boxY = 30;
	int boxW = this->width - 24;
	int boxH = this->height - 74;
	if (btn == 1 && mx >= boxX && mx <= boxX + boxW && my >= boxY && my <= boxY + boxH) {
		this->isDragging = true;
		this->dragStartMouseY = my;
		this->dragStartScrollY = this->scrollY;
	}
	Screen::mouseClicked(mx, my, btn);
}

void PatchErrorScreen::mouseReleased(int32_t mx, int32_t my, int32_t btn) {
	if (btn == 1) {
		this->isDragging = false;
	}
	Screen::mouseReleased(mx, my, btn);
}

void PatchErrorScreen::keyPressed(int32_t keyCode) {
	if (keyCode == 273 || keyCode == 'w' || keyCode == 'W') {
		this->scrollY -= 20;
	} else if (keyCode == 274 || keyCode == 's' || keyCode == 'S') {
		this->scrollY += 20;
	} else if (keyCode == 280) { // PageUp
		this->scrollY -= 80;
	} else if (keyCode == 281) { // PageDown
		this->scrollY += 80;
	}
	Screen::keyPressed(keyCode);
}
