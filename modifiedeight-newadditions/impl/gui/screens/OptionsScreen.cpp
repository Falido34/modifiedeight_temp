#include <gui/screens/OptionsScreen.hpp>
#include <gui/pane/ScrollingPane.hpp>
#include <Minecraft.hpp>
#include <gui/OptionsGroup.hpp>
#include <gui/pane/OptionsPane.hpp>
#include <gui/buttons/CategoryButton.hpp>
#include <gui/screens/PatchErrorScreen.hpp>
#include <gui/screens/PauseScreen.hpp>
#include <patch/PatchManager.hpp>
#include <rendering/Font.hpp>
#include <gui/buttons/Touch_TButton.hpp>
#include <gui/buttons/Touch_THeader.hpp>
#include <gui/NinePatchFactory.hpp>
#include <gui/NinePatchLayer.hpp>
#include <rendering/Tesselator.hpp>
#include <util/IntRectangle.hpp>
#include <cpputils.hpp>

int32_t OptionsScreen::lastCategory = 0;

OptionsScreen::OptionsScreen(bool_t a2) {
	this->isInWorld = a2;
	this->headerOptions = 0;
	this->buttonBack = 0;
	this->field_5C = 0;
	this->field_7C = 0;
	this->field_80 = 0;
	this->field_84 = 0;
}

void OptionsScreen::closeScreen() {
	this->minecraft->options.save();
	if(this->isInWorld) {
		this->minecraft->setScreen(new PauseScreen(0));
	} else {
		this->minecraft->screenChooser.setScreen(START_MENU_SCREEN);
	}
}
int32_t OptionsScreen::createCategoryButton(int32_t a2, int32_t a3, struct ImageDef& a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8) {
	CategoryButton* v12 = new CategoryButton(a2, this->field_7C, this->field_80, (Button**)&this->field_5C);
	a4.field_24 = 1;
	a4.field_14 = a5;
	a4.field_18 = a6;
	a4.field_1C = 28;
	a4.field_20 = 28;
	v12->posX = a7;
	v12->posY = a8;
	v12->setImageDef(a4, 1);
	v12->height = a3;
	v12->width = a3;
	this->field_60.emplace_back(v12);
	return a2 + 1;
}
void OptionsScreen::createCategoryButtons(void) {
	NinePatchFactory a1(this->minecraft->texturesPtr, "gui/spritesheet.png");
	this->field_7C = a1.createSymmetrical({8, 32, 8, 8}, 2, 2, this->field_88, this->field_88);
	this->field_80 = a1.createSymmetrical({0, 32, 8, 8}, 2, 2, this->field_88, this->field_88);
	ImageDef v11;
	v11.field_0 = "gui/touchgui2.png";
	v11.width = v11.height = this->field_88;
	int32_t v7 = this->createCategoryButton(2, this->field_88, v11, 134, 0, 0, 0);
	int32_t v8 = this->createCategoryButton(v7, this->field_88, v11, 106, 0, 0, 0);
	int32_t v9 = this->createCategoryButton(v8, this->field_88, v11, 134, 28, 0, 0);
	int32_t v10 = this->createCategoryButton(v9, this->field_88, v11, 106, 28, 0, 0);
	int32_t v12 = this->createCategoryButton(v10, this->field_88, v11, 134, 56, 0, 0);
	this->createCategoryButton(v12, this->field_88, v11, 106, 56, 0, 0);
}
void OptionsScreen::generateOptionScreens(void) {
	for (auto p : this->optionPanes) delete p;
	this->optionPanes.clear();

	this->optionPanes.emplace_back(new OptionsPane());
	this->optionPanes.emplace_back(new OptionsPane());
	this->optionPanes.emplace_back(new OptionsPane());
	this->optionPanes.emplace_back(new OptionsPane());
	this->optionPanes.emplace_back(new OptionsPane());
	this->optionPanes.emplace_back(new OptionsPane());

	this->optionPanes[0]->createOptionsGroup("options.group.game")->addOptionItem(&Options::Option::NAME, this->minecraft)->addOptionItem(&Options::Option::DIFFICULTY, this->minecraft)->addOptionItem(&Options::Option::THIRD_PERSON, this->minecraft)->addOptionItem(&Options::Option::SERVER_VISIBLE, this->minecraft)->addOptionItem(&Options::Option::SHOW_COORDINATES, this->minecraft)->addOptionItem(&Options::Option::DEBUG_SCREEN, this->minecraft)->addOptionItem(&Options::Option::HUD_CAMERA_BUTTON, this->minecraft)->addOptionItem(&Options::Option::EXTENDED_INVENTORY, this->minecraft)->addOptionItem(&Options::Option::SHOW_CAPE, this->minecraft)->addOptionItem(&Options::Option::SKIN_SOURCE, this->minecraft);
	this->optionPanes[0]->createOptionsGroup("options.group.realms")->addLoginItem(this->minecraft);
	this->optionPanes[1]->createOptionsGroup("options.group.input")->addOptionItem(&Options::Option::SENSITIVITY, this->minecraft)->addOptionItem(&Options::Option::INVERT_MOUSE, this->minecraft)->addOptionItem(&Options::Option::LEFT_HANDED, this->minecraft)->addOptionItem(&Options::Option::USE_TOUCHSCREEN, this->minecraft)->addOptionItem(&Options::Option::USE_TOUCH_JOYPAD, this->minecraft)->addOptionItem(&Options::Option::PIXELS_PER_MILLIMETER, this->minecraft)->addOptionItem(&Options::Option::GUI_SCALE, this->minecraft)->addControllerLayoutItem(this->minecraft, this->isInWorld)->addOptionItem(&Options::Option::CONTROLLER_SENSITIVITY, this->minecraft)->addOptionItem(&Options::Option::CONTROLLER_CURSOR_SENSITIVITY, this->minecraft)->addOptionItem(&Options::Option::SPRINT, this->minecraft)->addOptionItem(&Options::Option::AUTO_JUMP, this->minecraft)->addOptionItem(&Options::Option::SWAP_JUMP_AND_SNEAK, this->minecraft);
	if(this->minecraft->platform()->supportsVibration()) {
		this->optionPanes[1]->createOptionsGroup("options.group.feedback")->addOptionItem(&Options::Option::DESTROY_VIBRATION, this->minecraft);
	}
	this->optionPanes[2]->createOptionsGroup("options.group.graphics")->addOptionItem(&Options::Option::RENDER_DISTANCE, this->minecraft)->addOptionItem(&Options::Option::BRIGHTNESS, this->minecraft)->addOptionItem(&Options::Option::FOV, this->minecraft)->addOptionItem(&Options::Option::FOG_ENABLED, this->minecraft)->addOptionItem(&Options::Option::MORNING_FOG, this->minecraft)->addOptionItem(&Options::Option::SUN_GLOW, this->minecraft)->addOptionItem(&Options::Option::GRAPHICS, this->minecraft)->addOptionItem(&Options::Option::FANCY_SKIES, this->minecraft)->addOptionItem(&Options::Option::CLASSIC_TEXTURES, this->minecraft)->addOptionItem(&Options::Option::ANIMATE_TEXTURES, this->minecraft)->addOptionItem(&Options::Option::ANIMATE_WATER, this->minecraft)->addOptionItem(&Options::Option::ANIMATE_LAVA, this->minecraft)->addOptionItem(&Options::Option::ANIMATE_FIRE, this->minecraft)->addOptionItem(&Options::Option::SMOOTH_CHUNKS, this->minecraft);
	this->optionPanes[2]->createOptionsGroup("options.group.graphics.experimental")->addOptionItem(&Options::Option::HIDE_GUI, this->minecraft);
	this->optionPanes[3]->createOptionsGroup("options.group.audio")->addOptionItem(&Options::Option::SOUND, this->minecraft)->addOptionItem(&Options::Option::MUSIC, this->minecraft);
	
	auto grp = this->optionPanes[4]->createOptionsGroup("options.newadditions");
#if !defined(ANDROID) && !defined(MCPE_IOS) && !defined(__APPLE__)
	grp->addOptionItem(&Options::Option::MARKETPLACE, this->minecraft);
#endif
	grp->addOptionItem(&Options::Option::SHOW_FPS, this->minecraft);
	grp->addOptionItem(&Options::Option::DISCORD_RPC, this->minecraft);
	grp->addOptionItem(&Options::Option::PANORAMA_ANGLE, this->minecraft);

	if (this->minecraft->options.newAdditions == 1) {
		grp->addOptionItem(&Options::Option::CHAT_COLOR, this->minecraft);
		grp->addOptionItem(&Options::Option::CHAT_BG_COLOR, this->minecraft);
		grp->addOptionItem(&Options::Option::CLASSIC_BACKGROUND, this->minecraft);
		grp->addOptionItem(&Options::Option::CLASSIC_GUI, this->minecraft);
		grp->addOptionItem(&Options::Option::NEON_COLOR_THEME, this->minecraft);
	}

	this->optionPanes[5]->createOptionsGroup("options.patches")->addPatchesSection(this->minecraft, this->isInWorld);
	PatchManager::instance().runPatchOptionsCallbacks(this->optionPanes[5], this->minecraft);
}
void OptionsScreen::selectCategory(int32_t a2) {
	int32_t v2 = 0;
	for(auto&& p: this->field_60) {
		bool_t v5 = a2 == v2++;
		if(v5) {
			this->field_5C = p;
		}
		p->text = v5;
	}
	if(a2 < this->optionPanes.size()) {
		this->selectedCategory = this->optionPanes[a2];
		OptionsScreen::lastCategory = a2;
	}
}

OptionsScreen::~OptionsScreen() {
	if(this->buttonBack) {
		delete this->buttonBack;
		this->buttonBack = 0;
	}
	if(this->headerOptions) {
		delete this->headerOptions;
		this->headerOptions = 0;
	}
	safeRemove<NinePatchLayer>(this->field_7C);
	safeRemove<NinePatchLayer>(this->field_80);
	for(auto&& p: this->field_60) {
		if(p) {
			delete p;
			p = 0;
		}
	}
	for(auto&& p: this->optionPanes) {
		if(p) {
			delete p;
			p = 0;
		}
	}
	this->field_60.clear();
	this->optionPanes.clear();
}
void OptionsScreen::render(int32_t a2, int32_t a3, float a4) {
	OptionsPane* selectedCategory; // r0
	Minecraft* minecraft; // r7
	int width; // r0
	int height; // r10
	OptionsPane* v12; // r4

	selectedCategory = this->selectedCategory;
	if(selectedCategory && selectedCategory->suppressOtherGUI()) {
		if(this->selectedCategory) {
			this->selectedCategory->topRender(this->minecraft, a2, a3);
		}
	} else {
		if(!this->isInWorld) {
			if(this->renderGameBehind()) {
				this->renderMenuBackground(a4);
			}
		}
		this->renderBackground(0);
		this->fill(0, 0, this->field_88 + 10, this->height, 0xFF958782);
		Screen::render(a2, a3, a4);
		minecraft = this->minecraft;
		width = this->width;
		height = this->height;
		v12 = this->selectedCategory;
		if(v12) {
			v12->render(minecraft, width * a2 / minecraft->field_1C, height * a3 / minecraft->field_20 - 1);
		}

		static bool prevClassicTextures = this->minecraft->options.classicTextures;
		static bool showRestartNotice = false;
		static int  restartNoticeTimer = 0;
		if (this->minecraft->options.classicTextures != prevClassicTextures) {
			showRestartNotice = true;
			restartNoticeTimer = 180; // ~3 seconds at 60fps
			prevClassicTextures = this->minecraft->options.classicTextures;
		}
		if (showRestartNotice) {
			this->drawCenteredString(this->font, "Restart game to apply changes", this->width / 2, this->height - 15, 0xFFFFAA44);
			if (restartNoticeTimer > 0) {
				--restartNoticeTimer;
			} else {
				showRestartNotice = false;
			}
		}

		if (PatchManager::instance().isBuildRunning() && this->font) {
			this->fill(0, 0, this->width, this->height, 0x88000000);
			int dw = (this->width - 24 < 280) ? (this->width - 24) : 280;
			int dh = 68;
			int dx = (this->width - dw) / 2;
			int dy = (this->height - dh) / 2;
			if (!this->minecraft->options.classicGUI) {
				int32_t colorA, colorB;
				this->minecraft->options.getNeonColors(colorA, colorB);
				int32_t glowCol = (colorA & 0x00FFFFFF) | 0x44000000;
				this->fill(dx - 2, dy - 2, dx + dw + 2, dy + dh + 2, glowCol);
				this->fillGradient(dx + 1, dy + 1, dx + dw - 1, dy + dh - 1, 0xF00D0214, 0xF01E042D);
				this->fill(dx, dy, dx + dw, dy + 1, colorA);
				this->fill(dx, dy + dh - 1, dx + dw, dy + dh, colorA);
				this->fill(dx, dy, dx + 1, dy + dh, colorA);
				this->fill(dx + dw - 1, dy, dx + dw, dy + dh, colorA);
				this->fill(dx + 8, dy + 22, dx + dw - 8, dy + 23, (colorB & 0x00FFFFFF) | 0x88000000);
				// Animated neon activity bar
				int barX0 = dx + 14;
				int barX1 = dx + dw - 14;
				int barW = barX1 - barX0;
				this->fill(barX0, dy + dh - 13, barX1, dy + dh - 8, 0x88000000);
				if (barW > 20) {
					static int animTick = 0;
					animTick = (animTick + 1) % 120;
					int segW = barW / 3;
					int pos = (animTick < 60) ? ((animTick * (barW - segW)) / 60) : (((120 - animTick) * (barW - segW)) / 60);
					this->fill(barX0 + pos, dy + dh - 13, barX0 + pos + segW, dy + dh - 8, colorB);
				}
			} else {
				NinePatchFactory npf(this->minecraft->texturesPtr, "gui/spritesheet.png");
				NinePatchLayer* dlgBg = npf.createSymmetrical({34, 43, 14, 14}, 3, 3, (float)dw, (float)dh);
				if (dlgBg) {
					dlgBg->draw(Tesselator::instance, (float)dx, (float)dy);
					delete dlgBg;
				}
				NinePatchLayer* insetBg = npf.createSymmetrical({0, 32, 8, 8}, 2, 2, (float)(dw - 16), 24.0f);
				if (insetBg) {
					insetBg->draw(Tesselator::instance, (float)(dx + 8), (float)(dy + 26));
					delete insetBg;
				}
			}
			this->drawCenteredString(this->font, "Applying Patches...", this->width / 2, dy + 9, 0xFFFFFF66);
			std::string status = PatchManager::instance().getBuildStatus();
			while (status.size() > 3 && this->font->width(status) > dw - 20) {
				status.resize(status.size() - 4);
				status += "...";
			}
			this->drawCenteredString(this->font, status, this->width / 2, dy + 33, 0xFFEEEEEE);
		}
	}
}
void OptionsScreen::init() {
	this->field_88 = 28;
	this->headerOptions = new Touch::THeader(0, "Options");
	this->buttonBack = new Touch::TButton(1, "Back", 0);
	this->buttonBack->width = 38;
	this->buttonBack->height = 18;
	this->buttonBack->init(this->minecraft);
	this->buttons.emplace_back(this->headerOptions);
	this->buttons.emplace_back(this->buttonBack);
	this->createCategoryButtons();
	for(auto&& i = this->field_60.begin(); i != this->field_60.end(); ++i) {
		this->buttons.emplace_back(*i);
		this->field_2C.emplace_back(*i);
	}
	this->generateOptionScreens();
}
void OptionsScreen::setupPositions() {
	int32_t v2 = this->field_60.size();
	int32_t availH = this->height - this->headerOptions->height - 6;
	int32_t btnH = this->field_88;
	int32_t step = btnH + 1;
	if (v2 > 0 && v2 * step > availH) {
		step = availH / v2;
		if (step < 18) step = 18;
		btnH = step - 1;
	}
	int32_t v3 = (availH - (v2 * step - 1)) / 2;
	if (v3 < 0) v3 = 0;

	for(int32_t i = 0; i < this->field_60.size(); ++i) {
		this->field_60[i]->height = btnH;
		this->field_60[i]->posY = this->headerOptions->height + 3 + v3 + step * i;
		this->field_60[i]->posX = 5;
	}
	this->buttonBack->posX = 4;
	this->buttonBack->posY = 4;
	this->headerOptions->posX = 0;
	this->headerOptions->posY = 0;
	this->headerOptions->width = this->width;
	this->headerOptions->height = this->buttonBack->height + 8;
	for(auto&& p: this->optionPanes) {
		if(this->field_60.size()) {
			p->posX = this->field_60[0]->width + 20;
			p->posY = this->headerOptions->height + 3;
			p->width = this->width - this->field_60[0]->width - 20;
			p->height = this->height - this->headerOptions->height - 3;
			p->setupPositions();
		}
	}
	int32_t startCategory = OptionsScreen::lastCategory;
	if(startCategory < 0 || startCategory >= (int32_t)this->optionPanes.size()) startCategory = 0;
	this->selectCategory(startCategory);
}
bool_t OptionsScreen::handleBackEvent(bool_t a2) {
	if (PatchManager::instance().isBuildRunning()) return 1;
	if(!a2) {
		if(!this->selectedCategory || !this->selectedCategory->suppressOtherGUI() || !this->selectedCategory->backPressed(this->minecraft, 0)) {
			this->closeScreen();
		}
	}
	return 1;
}
void OptionsScreen::tick() {
	PatchManager& pm = PatchManager::instance();
	bool ok = false, willRelaunch = false;
	std::string err;
	if (pm.pollAsyncResult(ok, willRelaunch, err)) {
		if (!ok) {
			this->minecraft->setScreen(new PatchErrorScreen(err, PatchErrorScreen::RETURN_OPTIONS, this->isInWorld));
			return;
		}
		if (willRelaunch) {
			if (!pm.relaunchPendingBinary()) {
				this->minecraft->setScreen(new PatchErrorScreen(pm.getLastError(), PatchErrorScreen::RETURN_OPTIONS, this->isInWorld));
			}
			return;
		}
	}

	static int lastNewAdditions = this->minecraft->options.newAdditions;
	if (this->minecraft->options.newAdditions != lastNewAdditions) {
		lastNewAdditions = this->minecraft->options.newAdditions;
		int selectedIdx = 0;
		for(int i = 0; i < this->optionPanes.size(); i++) {
			if (this->optionPanes[i] == this->selectedCategory) {
				selectedIdx = i;
				break;
			}
		}
		this->generateOptionScreens();
		this->setupPositions();
		this->selectCategory(selectedIdx);
	}

	if(this->selectedCategory) {
		this->selectedCategory->tick(this->minecraft);
	}
	Screen::tick();
}
void OptionsScreen::removed() {
}
bool_t OptionsScreen::renderGameBehind() {
	return 1;
}
void OptionsScreen::setTextboxText(const std::string& a2) {
	if(this->selectedCategory) {
		this->selectedCategory->setTextboxText(a2);
	}
}
void OptionsScreen::buttonClicked(Button* a2) {
	if (PatchManager::instance().isBuildRunning()) return;
	if(a2 == this->buttonBack) {
		this->closeScreen();
	} else {
		int32_t buttonID = a2->buttonID;
		if((uint32_t)(buttonID - 2) <= 5) {
			this->selectCategory(buttonID - this->field_60[0]->buttonID);
		}
	}
}
void OptionsScreen::mouseClicked(int32_t a2, int32_t a3, int32_t a4) {
	if (PatchManager::instance().isBuildRunning()) return;
	if (a4 == 4 || a4 == 5) {
		if (this->selectedCategory && this->selectedCategory->scrollingPane) {
			float dir = (a4 == 4) ? 20.0f : -20.0f;
			float currentY = this->selectedCategory->scrollingPane->getContentOffset()->y;
			float newY = currentY + dir;
			int v8 = this->selectedCategory->field_38;
			int height = this->selectedCategory->height;
			float minY = (v8 > height) ? (float)(height - v8) : 0.0f;
			if (newY > 0.0f) newY = 0.0f;
			if (newY < minY) newY = minY;
			this->selectedCategory->scrollingPane->getContentOffset()->y = newY;
		}
		return;
	}
	bool_t v8; // r0
	OptionsPane* selectedCategory; // r4

	v8 = this->selectedCategory->suppressOtherGUI();
	selectedCategory = this->selectedCategory;
	if(v8) {
		if(selectedCategory) {
			selectedCategory->focusuedMouseClicked(this->minecraft, a2, a3, a4);
		}
	} else {
		if(selectedCategory) {
			selectedCategory->mouseClicked(this->minecraft, a2, a3, a4);
		}
		Screen::mouseClicked(a2, a3, a4);
	}
}
void OptionsScreen::mouseReleased(int32_t a2, int32_t a3, int32_t a4) {
	bool_t v8; // r0
	OptionsPane* selectedCategory; // r4

	v8 = this->selectedCategory->suppressOtherGUI();
	selectedCategory = this->selectedCategory;
	if(v8) {
		if(selectedCategory) {
			selectedCategory->focusuedMouseReleased(this->minecraft, a2, a3, a4);
		}
	} else {
		if(selectedCategory) {
			selectedCategory->mouseReleased(this->minecraft, a2, a3, a4);
		}
		Screen::mouseReleased(a2, a3, a4);
	}
}
void OptionsScreen::keyPressed(int32_t a2) {
	if(this->selectedCategory) {
		this->selectedCategory->keyPressed(this->minecraft, a2);
	}
}
void OptionsScreen::keyboardNewChar(const std::string& a2, bool_t a3) {
	if(this->selectedCategory) {
		this->selectedCategory->keyboardNewChar(this->minecraft, a2, a3);
	}
}
