#include <gui/screens/PatchesScreen.hpp>
#include <gui/screens/OptionsScreen.hpp>
#include <gui/screens/PatchErrorScreen.hpp>
#include <gui/screens/Touch_StartMenuScreen.hpp>
#include <gui/buttons/OptionButton.hpp>
#include <gui/buttons/Touch_TButton.hpp>
#include <gui/buttons/Touch_THeader.hpp>
#include <gui/NinePatchFactory.hpp>
#include <gui/NinePatchLayer.hpp>
#include <Minecraft.hpp>
#include <NinecraftApp.hpp>
#include <Options.hpp>
#include <input/Mouse.hpp>
#include <patch/PatchManager.hpp>
#include <rendering/Font.hpp>
#include <rendering/Tesselator.hpp>
#include <util/IntRectangle.hpp>
#include <algorithm>
#include <sstream>

static void renderStyledDialogBox(Screen* scr, Minecraft* mc, int dx, int dy, int dw, int dh, bool withProgressBar, int progressYOffset = -1) {
	if (!mc->options.classicGUI) {
		int32_t colorA, colorB;
		mc->options.getNeonColors(colorA, colorB);
		int32_t glowCol = (colorA & 0x00FFFFFF) | 0x44000000;
		scr->fill(dx - 2, dy - 2, dx + dw + 2, dy + dh + 2, glowCol);
		scr->fillGradient(dx + 1, dy + 1, dx + dw - 1, dy + dh - 1, 0xF20D0214, 0xF21E042D);
		scr->fill(dx, dy, dx + dw, dy + 1, colorA);
		scr->fill(dx, dy + dh - 1, dx + dw, dy + dh, colorA);
		scr->fill(dx, dy, dx + 1, dy + dh, colorA);
		scr->fill(dx + dw - 1, dy, dx + dw, dy + dh, colorA);
		scr->fill(dx + 8, dy + 22, dx + dw - 8, dy + 23, (colorB & 0x00FFFFFF) | 0x88000000);
		if (withProgressBar) {
			int barX0 = dx + 14;
			int barX1 = dx + dw - 14;
			int barW = barX1 - barX0;
			int py = (progressYOffset > 0) ? (dy + progressYOffset) : (dy + dh - 13);
			scr->fill(barX0, py, barX1, py + 5, 0x88000000);
			if (barW > 20) {
				static int animTick = 0;
				animTick = (animTick + 1) % 120;
				int segW = barW / 3;
				int pos = (animTick < 60) ? ((animTick * (barW - segW)) / 60) : (((120 - animTick) * (barW - segW)) / 60);
				scr->fill(barX0 + pos, py, barX0 + pos + segW, py + 5, colorB);
			}
		}
	} else {
		NinePatchFactory npf(mc->texturesPtr, "gui/spritesheet.png");
		NinePatchLayer* dlgBg = npf.createSymmetrical({34, 43, 14, 14}, 3, 3, (float)dw, (float)dh);
		if (dlgBg) {
			dlgBg->draw(Tesselator::instance, (float)dx, (float)dy);
			delete dlgBg;
		}
	}
}

PatchesScreen::PatchesScreen(bool returnStart, bool inWorld)
	: returnToStartMenu(returnStart),
	  optionsIsInWorld(inWorld),
	  header(nullptr),
	  btnBack(nullptr),
	  btnRefresh(nullptr),
	  btnFolder(nullptr),
	  panelLayer(nullptr),
	  modalMode(MODAL_NONE),
	  btnModalConfirm(nullptr),
	  btnModalCancel(nullptr),
	  scrollY(0),
	  contentHeight(0),
	  isDragging(false),
	  dragStartMouseY(0),
	  dragStartScrollY(0),
	  statusColor(0xFF55FF55) {
}

PatchesScreen::~PatchesScreen() {
	this->clearRowControls();
	delete this->header;
	delete this->btnBack;
	delete this->btnRefresh;
	delete this->btnFolder;
	delete this->panelLayer;
	delete this->btnModalConfirm;
	delete this->btnModalCancel;
}

void PatchesScreen::clearRowControls() {
	for (auto& rc : this->rowControls) {
		auto removeBtn = [&](Button* b) {
			if (!b) return;
			this->buttons.erase(std::remove(this->buttons.begin(), this->buttons.end(), b), this->buttons.end());
			this->field_2C.erase(std::remove(this->field_2C.begin(), this->field_2C.end(), b), this->field_2C.end());
		};
		removeBtn(rc.btnToggle);
		removeBtn(rc.btnRename);
		removeBtn(rc.btnDelete);
		removeBtn(rc.btnUp);
		removeBtn(rc.btnDown);
		delete rc.btnToggle;
		delete rc.btnRename;
		delete rc.btnDelete;
		delete rc.btnUp;
		delete rc.btnDown;
	}
	this->rowControls.clear();
}

void PatchesScreen::init() {
	this->buttons.clear();
	this->field_2C.clear();
	this->clearRowControls();

	delete this->header;
	delete this->btnBack;
	delete this->btnRefresh;
	delete this->btnFolder;
	delete this->panelLayer;
	delete this->btnModalConfirm;
	delete this->btnModalCancel;

	this->header = new Touch::THeader(0, "Manage Patches");
	this->btnBack = new Touch::TButton(1, "Back", this->minecraft);
	this->btnRefresh = new Touch::TButton(2, "Refresh", this->minecraft);
	this->btnFolder = new Touch::TButton(3, "Open Folder", this->minecraft);

	this->btnModalConfirm = new Touch::TButton(4, "Confirm", this->minecraft);
	this->btnModalConfirm->width = 64;
	this->btnModalConfirm->height = 20;
	this->btnModalConfirm->visible = false;
	this->btnModalConfirm->active = false;

	this->btnModalCancel = new Touch::TButton(5, "Cancel", this->minecraft);
	this->btnModalCancel->width = 64;
	this->btnModalCancel->height = 20;
	this->btnModalCancel->visible = false;
	this->btnModalCancel->active = false;

	NinePatchFactory npf(this->minecraft->texturesPtr, "gui/spritesheet.png");
	this->panelLayer = npf.createSymmetrical(IntRectangle{34, 43, 14, 14}, 3, 3, 32.0f, 32.0f);

	this->buttons.push_back(this->header);
	this->buttons.push_back(this->btnBack);
	this->buttons.push_back(this->btnRefresh);
	this->buttons.push_back(this->btnFolder);
	this->buttons.push_back(this->btnModalConfirm);
	this->buttons.push_back(this->btnModalCancel);

	PatchManager::instance().scanPatches();
	this->rebuildPatchRows();
	this->setupPositions();
}

void PatchesScreen::rebuildPatchRows() {
	this->clearRowControls();
	const auto& patches = PatchManager::instance().getPatches();

	int nextId = 100;
	for (size_t i = 0; i < patches.size(); ++i) {
		const PatchEntry& p = patches[i];
		RowControls rc;
		rc.patchId = p.id;

		ImageDef idef;
		idef.field_24 = 1;
		idef.field_0 = "gui/touchgui.png";
		idef.field_4 = 0;
		idef.field_8 = 0;
		idef.field_14 = 160;
		idef.field_18 = 206;
		idef.field_1C = 38;
		idef.field_20 = 20;
		idef.width = 38.0f;
		idef.height = 20.0f;
		rc.btnToggle = new OptionButton(p.enabled ? 1 : 0);
		rc.btnToggle->buttonID = nextId++;
		rc.btnToggle->setImageDef(idef, 1);

		rc.btnRename = new Touch::TButton(nextId++, "Rename", this->minecraft);
		rc.btnRename->width = 46;
		rc.btnRename->height = 20;

		rc.btnDelete = new Touch::TButton(nextId++, "Delete", this->minecraft);
		rc.btnDelete->width = 44;
		rc.btnDelete->height = 20;

		rc.btnUp = new Touch::TButton(nextId++, "^", this->minecraft);
		rc.btnUp->width = 16;
		rc.btnUp->height = 20;

		rc.btnDown = new Touch::TButton(nextId++, "v", this->minecraft);
		rc.btnDown->width = 16;
		rc.btnDown->height = 20;

		this->buttons.push_back(rc.btnToggle);
		this->buttons.push_back(rc.btnRename);
		this->buttons.push_back(rc.btnDelete);
		this->buttons.push_back(rc.btnUp);
		this->buttons.push_back(rc.btnDown);
		this->rowControls.push_back(rc);
	}

	this->contentHeight = (int)patches.size() * 36 + 6;
}

void PatchesScreen::setupPositions() {
	if (this->header) {
		this->header->posX = 0;
		this->header->posY = 0;
		this->header->width = this->width;
		this->header->height = 26;
	}
	if (this->btnBack) {
		this->btnBack->posX = 4;
		this->btnBack->posY = 4;
		this->btnBack->width = 44;
		this->btnBack->height = 18;
	}
	if (this->btnFolder) {
		this->btnFolder->width = 74;
		this->btnFolder->height = 18;
		this->btnFolder->posX = this->width - this->btnFolder->width - 4;
		this->btnFolder->posY = 4;
#if defined(ANDROID) || defined(MCPE_IOS)
		this->btnFolder->visible = false;
		this->btnFolder->active = false;
#endif
	}
	if (this->btnRefresh) {
		this->btnRefresh->width = 52;
		this->btnRefresh->height = 18;
#if defined(ANDROID) || defined(MCPE_IOS)
		this->btnRefresh->posX = this->width - this->btnRefresh->width - 4;
#else
		this->btnRefresh->posX = (this->btnFolder ? this->btnFolder->posX : this->width) - this->btnRefresh->width - 4;
#endif
		this->btnRefresh->posY = 4;
	}

	int dw = std::min(260, this->width - 28);
	int dh = 86;
	int dx = (this->width - dw) / 2;
	int dy = (this->height - dh) / 2;
	if (this->btnModalConfirm) {
		this->btnModalConfirm->posX = dx + dw / 2 - this->btnModalConfirm->width - 6;
		this->btnModalConfirm->posY = dy + dh - 26;
	}
	if (this->btnModalCancel) {
		this->btnModalCancel->posX = dx + dw / 2 + 6;
		this->btnModalCancel->posY = dy + dh - 26;
	}
}

void PatchesScreen::openRenameModal(const std::string& patchId, const std::string& currentName) {
	this->modalMode = MODAL_RENAME;
	this->modalTargetId = patchId;
	this->modalTargetDisplay = currentName;
	this->renameInputText = currentName;
	if (this->btnModalConfirm) {
		this->btnModalConfirm->maybeTextOnButton = "Save";
		this->btnModalConfirm->visible = true;
		this->btnModalConfirm->active = true;
	}
	if (this->btnModalCancel) {
		this->btnModalCancel->visible = true;
		this->btnModalCancel->active = true;
	}
	this->setupPositions();
}

void PatchesScreen::openDeleteModal(const std::string& patchId, const std::string& displayName) {
	this->modalMode = MODAL_DELETE;
	this->modalTargetId = patchId;
	this->modalTargetDisplay = displayName;
	if (this->btnModalConfirm) {
		this->btnModalConfirm->maybeTextOnButton = "Delete";
		this->btnModalConfirm->visible = true;
		this->btnModalConfirm->active = true;
	}
	if (this->btnModalCancel) {
		this->btnModalCancel->visible = true;
		this->btnModalCancel->active = true;
	}
	this->setupPositions();
}

void PatchesScreen::closeModal() {
	this->modalMode = MODAL_NONE;
	this->modalTargetId.clear();
	this->modalTargetDisplay.clear();
	this->renameInputText.clear();
	if (this->btnModalConfirm) {
		this->btnModalConfirm->visible = false;
		this->btnModalConfirm->active = false;
	}
	if (this->btnModalCancel) {
		this->btnModalCancel->visible = false;
		this->btnModalCancel->active = false;
	}
}

void PatchesScreen::commitPatchSelectionChange(const std::vector<std::string>& prevEnabled) {
	PatchManager& pm = PatchManager::instance();
	std::string valErr;
	if (!pm.validateEnabledPatches(valErr)) {
		pm.restoreEnabledIds(prevEnabled);
		pm.saveEnabledConfig();
		pm.reportError(valErr);
		this->minecraft->setScreen(new PatchErrorScreen(valErr, PatchErrorScreen::RETURN_PATCHES, this->optionsIsInWorld));
		return;
	}

	if (pm.needsCodeRebuildOrRelaunch()) {
		this->statusMsg = "Building code patches...";
		this->statusColor = 0xFFFFFF55;
		pm.startApplyPatchesAsync(prevEnabled);
		return;
	}

	NinecraftApp* app = (NinecraftApp*)this->minecraft;
	if (!pm.applyRuntimeAssetReload(app)) {
		std::string err = pm.getLastError();
		pm.restoreEnabledIds(prevEnabled);
		pm.saveEnabledConfig();
		pm.applyRuntimeAssetReload(app);
		pm.reportError(err);
		this->minecraft->setScreen(new PatchErrorScreen(err, PatchErrorScreen::RETURN_PATCHES, this->optionsIsInWorld));
		return;
	}

	pm.saveEnabledConfig();
	this->statusMsg = "Applied patch changes!";
	this->statusColor = 0xFF55FF55;
	this->rebuildPatchRows();
}

void PatchesScreen::render(int32_t mx, int32_t my, float pt) {
	if (!this->optionsIsInWorld) {
		this->renderMenuBackground(pt);
	}
	this->renderBackground(0);

	PatchManager& pm = PatchManager::instance();
	bool building = pm.isBuildRunning();
	bool modalOpen = (this->modalMode != MODAL_NONE);
	bool interactive = !building && !modalOpen;

	if (this->btnBack) this->btnBack->active = interactive;
	if (this->btnRefresh) this->btnRefresh->active = interactive;
	if (this->btnFolder) this->btnFolder->active = interactive;

	int listTop = 42;
	int listBottom = this->height - 20;
	if (listBottom < listTop + 40) listBottom = listTop + 40;
	int listH = listBottom - listTop;

	if (this->font) {
		std::string folderInfo = "Patches Folder: " + pm.getPatchesDir();
		this->drawString(this->font, folderInfo, 12, 30, 0xFFCCCCCC);
	}

	if (this->isDragging && interactive && this->minecraft && this->minecraft->field_20 > 0) {
		int guiMY = this->height * Mouse::getY() / this->minecraft->field_20 - 1;
		int dy = guiMY - this->dragStartMouseY;
		this->scrollY = this->dragStartScrollY - dy;
	}

	int maxScroll = std::max(0, this->contentHeight - listH);
	if (this->scrollY < 0) this->scrollY = 0;
	if (this->scrollY > maxScroll) this->scrollY = maxScroll;

	int panelX = 8;
	int panelY = listTop - 2;
	int panelW = this->width - 16;
	int panelH = listBottom - listTop + 4;
	if (!this->minecraft->options.classicGUI) {
		int32_t colorA, colorB;
		this->minecraft->options.getNeonColors(colorA, colorB);
		this->fillGradient(panelX + 1, panelY + 1, panelX + panelW - 1, panelY + panelH - 1, 0xCC0D0214, 0xCC1E042D);
		this->fill(panelX, panelY, panelX + panelW, panelY + 1, colorA);
		this->fill(panelX, panelY + panelH - 1, panelX + panelW, panelY + panelH, colorA);
		this->fill(panelX, panelY, panelX + 1, panelY + panelH, colorA);
		this->fill(panelX + panelW - 1, panelY, panelX + panelW, panelY + panelH, colorA);
	} else if (this->panelLayer) {
		this->panelLayer->setSize((float)panelW, (float)panelH);
		this->panelLayer->draw(Tesselator::instance, (float)panelX, (float)panelY);
	}

	const auto& patches = pm.getPatches();
	int rowH = 36;
	int curY = listTop + 3 - this->scrollY;

	for (size_t i = 0; i < patches.size() && i < this->rowControls.size(); ++i) {
		const PatchEntry& p = patches[i];
		RowControls& rc = this->rowControls[i];
		bool vis = (curY + rowH - 4 >= listTop && curY <= listBottom);

		if (vis) {
			uint32_t rowBg = !p.loadError.empty() ? 0x55AA2222
				: (p.enabled ? 0x44227733 : ((i % 2 == 0) ? 0x28FFFFFF : 0x14FFFFFF));
			this->fill(14, curY, this->width - 16, curY + rowH - 4, rowBg);

			std::string titleLine = p.displayName;
			if (!p.version.empty()) titleLine += " v" + p.version;
			if (!p.loadError.empty()) titleLine += " [Error]";

			std::ostringstream subSS;
			subSS << p.fileCount << (p.fileCount == 1 ? " file" : " files");
			if (!p.author.empty()) subSS << " - by " << p.author;
			if (!p.loadError.empty()) subSS << " - " << p.loadError;

			int btnRight = this->width - 20;
			if (rc.btnDelete) {
				rc.btnDelete->posX = btnRight - rc.btnDelete->width;
				rc.btnDelete->posY = curY + 6;
				rc.btnDelete->visible = true;
				rc.btnDelete->active = interactive;
				btnRight = rc.btnDelete->posX - 3;
			}
			if (rc.btnRename) {
				rc.btnRename->posX = btnRight - rc.btnRename->width;
				rc.btnRename->posY = curY + 6;
				rc.btnRename->visible = true;
				rc.btnRename->active = interactive;
				btnRight = rc.btnRename->posX - 3;
			}
			if (rc.btnToggle) {
				rc.btnToggle->boolValue = p.enabled ? 1 : 0;
				rc.btnToggle->posX = btnRight - rc.btnToggle->width;
				rc.btnToggle->posY = curY + 6;
				rc.btnToggle->visible = true;
				rc.btnToggle->active = interactive;
				btnRight = rc.btnToggle->posX - 3;
			}
			if (rc.btnDown) {
				rc.btnDown->posX = btnRight - rc.btnDown->width;
				rc.btnDown->posY = curY + 6;
				rc.btnDown->visible = p.enabled;
				rc.btnDown->active = p.enabled && interactive;
				if (p.enabled) btnRight = rc.btnDown->posX - 2;
			}
			if (rc.btnUp) {
				rc.btnUp->posX = btnRight - rc.btnUp->width;
				rc.btnUp->posY = curY + 6;
				rc.btnUp->visible = p.enabled;
				rc.btnUp->active = p.enabled && interactive;
				if (p.enabled) btnRight = rc.btnUp->posX - 4;
			}

			std::string subLine = subSS.str();
			int maxTextPx = std::max(50, btnRight - 22);
			if (this->font) {
				while (subLine.size() > 3 && this->font->width(subLine) > maxTextPx) {
					subLine.resize(subLine.size() - 4);
					subLine += "...";
				}
				while (titleLine.size() > 3 && this->font->width(titleLine) > maxTextPx) {
					titleLine.resize(titleLine.size() - 4);
					titleLine += "...";
				}
				uint32_t titleCol = !p.loadError.empty() ? 0xFFFF6666 : (p.enabled ? 0xFF77FF77 : 0xFFFFFFFF);
				this->drawString(this->font, titleLine, 18, curY + 5, titleCol);
				this->drawString(this->font, subLine, 18, curY + 18, 0xFFBBBBBB);
			}
		} else {
			if (rc.btnToggle) { rc.btnToggle->visible = false; rc.btnToggle->active = false; }
			if (rc.btnRename) { rc.btnRename->visible = false; rc.btnRename->active = false; }
			if (rc.btnDelete) { rc.btnDelete->visible = false; rc.btnDelete->active = false; }
			if (rc.btnUp) { rc.btnUp->visible = false; rc.btnUp->active = false; }
			if (rc.btnDown) { rc.btnDown->visible = false; rc.btnDown->active = false; }
		}

		curY += rowH;
	}

	if (patches.empty() && this->font) {
		std::string emptyMsg = "No patches found in '" + pm.getPatchesDir() + "/'.";
		this->drawCenteredString(this->font, emptyMsg, this->width / 2, listTop + 26, 0xFFCCCCCC);
		this->drawCenteredString(this->font, "Place patch folders or .zip files inside that folder.", this->width / 2, listTop + 40, 0xFF999999);
	}

	if (maxScroll > 0) {
		int barX = this->width - 13;
		this->fill(barX, listTop, barX + 3, listBottom, 0x44FFFFFF);
		int thumbH = std::max(14, (listH * listH) / std::max(listH, this->contentHeight));
		int thumbY = listTop + ((listH - thumbH) * this->scrollY) / maxScroll;
		this->fill(barX, thumbY, barX + 3, thumbY + thumbH, 0xFFCCCCCC);
	}

	if (modalOpen) {
		this->fill(0, 0, this->width, this->height, 0x88000000);
		int dw = std::min(260, this->width - 28);
		int dh = 86;
		int dx = (this->width - dw) / 2;
		int dy = (this->height - dh) / 2;
		renderStyledDialogBox(this, this->minecraft, dx, dy, dw, dh, false);

		if (this->modalMode == MODAL_RENAME && this->font) {
			this->drawCenteredString(this->font, "Rename Patch", this->width / 2, dy + 8, 0xFFFFFF66);
			int boxX = dx + 14;
			int boxY = dy + 30;
			int boxW = dw - 28;
			int boxH = 20;
			this->fill(boxX - 1, boxY - 1, boxX + boxW + 1, boxY + boxH + 1, 0xFFAAAAAA);
			this->fill(boxX, boxY, boxX + boxW, boxY + boxH, 0xFF141418);
			static int caretBlink = 0;
			caretBlink = (caretBlink + 1) % 40;
			std::string displayTxt = this->renameInputText + ((caretBlink < 20) ? "_" : "");
			this->drawString(this->font, displayTxt, boxX + 5, boxY + 6, 0xFFFFFFFF);
		} else if (this->modalMode == MODAL_DELETE && this->font) {
			this->drawCenteredString(this->font, "Delete Patch?", this->width / 2, dy + 8, 0xFFFF6666);
			std::string q = "Remove '" + this->modalTargetDisplay + "'?";
			this->drawCenteredString(this->font, q, this->width / 2, dy + 30, 0xFFFFFFFF);
			this->drawCenteredString(this->font, "This deletes its files permanently.", this->width / 2, dy + 43, 0xFFBBBBBB);
		}
	}

	Screen::render(mx, my, pt);

	if (!this->statusMsg.empty() && this->font) {
		this->drawCenteredString(this->font, this->statusMsg, this->width / 2, this->height - 13, this->statusColor);
	}

	if (building && this->font) {
		this->fill(0, 0, this->width, this->height, 0x88000000);
		int dw = std::min(280, this->width - 24);
		int dh = 68;
		int dx = (this->width - dw) / 2;
		int dy = (this->height - dh) / 2;
		renderStyledDialogBox(this, this->minecraft, dx, dy, dw, dh, true);
		this->drawCenteredString(this->font, "Applying Patches...", this->width / 2, dy + 9, 0xFFFFFF66);
		std::string line = pm.getBuildStatus();
		while (line.size() > 3 && this->font->width(line) > dw - 20) {
			line.resize(line.size() - 4);
			line += "...";
		}
		this->drawCenteredString(this->font, line, this->width / 2, dy + 33, 0xFFEEEEEE);
	}
}

void PatchesScreen::tick() {
	PatchManager& pm = PatchManager::instance();
	bool ok = false, willRelaunch = false;
	std::string err;
	if (pm.pollAsyncResult(ok, willRelaunch, err)) {
		if (!ok) {
			this->minecraft->setScreen(new PatchErrorScreen(err, PatchErrorScreen::RETURN_PATCHES, this->optionsIsInWorld));
			return;
		}
		if (willRelaunch) {
			if (!pm.relaunchPendingBinary()) {
				this->minecraft->setScreen(new PatchErrorScreen(pm.getLastError(), PatchErrorScreen::RETURN_PATCHES, this->optionsIsInWorld));
			}
			return;
		}
		if (pm.hasEnabledAssetOverrides()) {
			pm.applyRuntimeAssetReload((NinecraftApp*)this->minecraft);
		}
		this->rebuildPatchRows();
	}
	Screen::tick();
}

bool_t PatchesScreen::renderGameBehind() {
	return this->optionsIsInWorld ? 1 : 0;
}

bool_t PatchesScreen::isInGameScreen() {
	return 0;
}

bool_t PatchesScreen::handleBackEvent(bool_t a2) {
	if (a2) return 1;
	if (PatchManager::instance().isBuildRunning()) return 1;
	if (this->modalMode != MODAL_NONE) {
		this->closeModal();
		return 1;
	}
	if (this->returnToStartMenu) {
		this->minecraft->screenChooser.setScreen(START_MENU_SCREEN);
	} else {
		this->minecraft->setScreen(new OptionsScreen(this->optionsIsInWorld));
	}
	return 1;
}

void PatchesScreen::buttonClicked(Button* btn) {
	if (!btn || PatchManager::instance().isBuildRunning()) return;

	if (this->modalMode != MODAL_NONE) {
		if (btn == this->btnModalCancel) {
			this->closeModal();
			return;
		}
		if (btn == this->btnModalConfirm) {
			PatchManager& pm = PatchManager::instance();
			std::vector<std::string> prev = pm.getEnabledIdsSnapshot();
			std::string opErr;
			if (this->modalMode == MODAL_RENAME) {
				if (!pm.renamePatch(this->modalTargetId, this->renameInputText, opErr)) {
					this->statusMsg = opErr;
					this->statusColor = 0xFFFF6666;
				} else {
					this->statusMsg = "Renamed patch to '" + this->renameInputText + "'.";
					this->statusColor = 0xFF55FF55;
					this->closeModal();
					this->rebuildPatchRows();
				}
			} else if (this->modalMode == MODAL_DELETE) {
				std::string deletedName = this->modalTargetDisplay;
				PatchEntry* pe = pm.findPatch(this->modalTargetId);
				bool wasEnabled = pe && pe->enabled;
				if (!pm.deletePatch(this->modalTargetId, opErr)) {
					this->statusMsg = opErr;
					this->statusColor = 0xFFFF6666;
					this->closeModal();
				} else {
					this->closeModal();
					this->rebuildPatchRows();
					if (wasEnabled) {
						this->commitPatchSelectionChange(prev);
					} else {
						this->statusMsg = "Deleted patch '" + deletedName + "'.";
						this->statusColor = 0xFF55FF55;
					}
				}
			}
			return;
		}
		return;
	}

	if (btn == this->btnBack) {
		this->handleBackEvent(false);
		return;
	}
	if (btn == this->btnRefresh) {
		PatchManager::instance().scanPatches();
		this->rebuildPatchRows();
		this->statusMsg = "Rescanned patches folder.";
		this->statusColor = 0xFFAAAAAA;
		return;
	}
	if (btn == this->btnFolder) {
		PatchManager::openPatchesFolderInOS();
		this->statusMsg = "Opened patches folder.";
		this->statusColor = 0xFFAAAAAA;
		return;
	}

	for (size_t i = 0; i < this->rowControls.size(); ++i) {
		RowControls& rc = this->rowControls[i];
		PatchManager& pm = PatchManager::instance();
		PatchEntry* pe = pm.findPatch(rc.patchId);
		if (!pe) continue;

		if (btn == rc.btnToggle) {
			std::vector<std::string> prev = pm.getEnabledIdsSnapshot();
			bool nextState = !pe->enabled;
			if (nextState && !pe->loadError.empty()) {
				pm.reportError(pe->loadError);
				this->minecraft->setScreen(new PatchErrorScreen(pe->loadError, PatchErrorScreen::RETURN_PATCHES, this->optionsIsInWorld));
				return;
			}
			pm.setPatchEnabled(rc.patchId, nextState);
			this->commitPatchSelectionChange(prev);
			return;
		}
		if (btn == rc.btnRename) {
			this->openRenameModal(pe->id, pe->displayName);
			return;
		}
		if (btn == rc.btnDelete) {
			this->openDeleteModal(pe->id, pe->displayName);
			return;
		}
		if (btn == rc.btnUp) {
			std::vector<std::string> prev = pm.getEnabledIdsSnapshot();
			if (pm.movePatchPriority(rc.patchId, -1)) {
				this->commitPatchSelectionChange(prev);
			}
			return;
		}
		if (btn == rc.btnDown) {
			std::vector<std::string> prev = pm.getEnabledIdsSnapshot();
			if (pm.movePatchPriority(rc.patchId, +1)) {
				this->commitPatchSelectionChange(prev);
			}
			return;
		}
	}
}

void PatchesScreen::mouseClicked(int32_t mx, int32_t my, int32_t btn) {
	if (PatchManager::instance().isBuildRunning()) return;
	if (this->modalMode == MODAL_NONE) {
		if (btn == 4) {
			this->scrollY -= 24;
			return;
		}
		if (btn == 5) {
			this->scrollY += 24;
			return;
		}
		if (btn == 1 && my >= 40 && my <= this->height - 20) {
			this->isDragging = true;
			this->dragStartMouseY = my;
			this->dragStartScrollY = this->scrollY;
		}
	}
	Screen::mouseClicked(mx, my, btn);
}

void PatchesScreen::mouseReleased(int32_t mx, int32_t my, int32_t btn) {
	if (btn == 1) this->isDragging = false;
	if (PatchManager::instance().isBuildRunning()) return;
	Screen::mouseReleased(mx, my, btn);
}

void PatchesScreen::keyPressed(int32_t key) {
	if (this->modalMode == MODAL_RENAME) {
		if (key == 8 || key == 127) { // Backspace
			if (!this->renameInputText.empty()) {
				this->renameInputText.pop_back();
			}
			return;
		}
		if (key == 13 || key == 10) { // Enter
			this->buttonClicked(this->btnModalConfirm);
			return;
		}
		if (key == 27) { // Escape
			this->closeModal();
			return;
		}
		return;
	}
	Screen::keyPressed(key);
}

void PatchesScreen::keyboardNewChar(const std::string& ch, bool_t flag) {
	if (this->modalMode == MODAL_RENAME) {
		for (char c : ch) {
			if (c >= 32 && c < 127 && c != '/' && c != '\\' && c != ':' && c != '*' && c != '?' && c != '"' && c != '<' && c != '>' && c != '|') {
				if (this->renameInputText.size() < 40) {
					this->renameInputText.push_back(c);
				}
			}
		}
		return;
	}
	Screen::keyboardNewChar(ch, flag);
}
