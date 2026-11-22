#include <gui/screens/JukeboxScreen.hpp>
#include <sound/JukeboxManager.hpp>
#include <Minecraft.hpp>
#include <entity/Player.hpp>
#include <entity/LocalPlayer.hpp>
#include <rendering/Font.hpp>
#include <gui/buttons/Button.hpp>
#include <gui/NinePatchFactory.hpp>
#include <gui/NinePatchLayer.hpp>
#include <util/IntRectangle.hpp>
#include <rendering/Tesselator.hpp>
#include <rendering/Textures.hpp>
#include <cpputils.hpp>
#include <unigl.h>
#include <cmath>
#include <cstdio>
#include <algorithm>

static void renderMcpeBtn(NinePatchLayer* normal, NinePatchLayer* pressed, Font* font, int32_t x, int32_t y, int32_t w, int32_t h, const std::string& text, bool hov, bool enabled = true, uint32_t customColor = 0) {
	NinePatchLayer* layer = (enabled && hov) ? pressed : normal;
	if (layer) {
		layer->setSize((float)w, (float)h);
		if (!enabled) {
			glColor4f(0.75f, 0.75f, 0.75f, 1.0f);
		} else {
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		}
		layer->draw(Tesselator::instance, (float)x, (float)y);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}
	int32_t tw = font->width(text);
	int32_t tx = x + (w - tw) / 2;
	int32_t ty = y + (h - 8) / 2;
	uint32_t col = !enabled ? 0xFFA0A0A0 : (customColor ? customColor : (hov ? 0xFFFFFFA0 : 0xFFE0E0E0));
	font->drawShadow(text, tx, ty, col);
}

static void renderMcpeCloseBtn(Minecraft* mc, int32_t x, int32_t y, int32_t size, bool hov) {
	mc->texturesPtr->loadAndBindTexture("gui/spritesheet.png");
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if (hov) {
		glColor4f(1.0f, 1.0f, 0.85f, 1.0f);
	} else {
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}
	float u0 = 60.0f / 256.0f;
	float v0 = 0.0f / 256.0f;
	float u1 = 78.0f / 256.0f;
	float v1 = 18.0f / 256.0f;
	Tesselator& t = Tesselator::instance;
	t.begin(4);
	t.vertexUV((float)x, (float)(y + size), 0.0f, u0, v1);
	t.vertexUV((float)(x + size), (float)(y + size), 0.0f, u1, v1);
	t.vertexUV((float)(x + size), (float)y, 0.0f, u1, v0);
	t.vertexUV((float)x, (float)y, 0.0f, u0, v0);
	t.draw(1);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

JukeboxScreen::JukeboxScreen(int32_t x, int32_t y, int32_t z)
	: blockX(x), blockY(y), blockZ(z), scrollOffset(0),
	  dialogX(0), dialogY(0), dialogW(256), dialogH(208),
	  bgPanel(0), insetPanel(0), btnNormal(0), btnPressed(0), slotNormal(0), slotSelected(0) {
}

JukeboxScreen::~JukeboxScreen() {
	safeRemove(this->bgPanel);
	safeRemove(this->insetPanel);
	safeRemove(this->btnNormal);
	safeRemove(this->btnPressed);
	safeRemove(this->slotNormal);
	safeRemove(this->slotSelected);
}

void JukeboxScreen::init() {
	this->buttons.clear();
	this->dialogW = std::min(256, this->width - 16);
	this->dialogH = std::min(208, this->height - 16);
	this->dialogX = (this->width - this->dialogW) / 2;
	this->dialogY = (this->height - this->dialogH) / 2;

	safeRemove(this->bgPanel);
	safeRemove(this->insetPanel);
	safeRemove(this->btnNormal);
	safeRemove(this->btnPressed);
	safeRemove(this->slotNormal);
	safeRemove(this->slotSelected);

	NinePatchFactory npf(this->minecraft->texturesPtr, "gui/spritesheet.png");
	this->bgPanel = npf.createSymmetrical({0, 0, 16, 16}, 4, 4, (float)this->dialogW, (float)this->dialogH);
	this->insetPanel = npf.createSymmetrical({0, 32, 8, 8}, 3, 3, 32.0f, 32.0f);
	this->btnNormal = npf.createSymmetrical({8, 32, 8, 8}, 2, 2, 32.0f, 18.0f);
	this->btnPressed = npf.createSymmetrical({0, 32, 8, 8}, 2, 2, 32.0f, 18.0f);
	this->slotNormal = npf.createSymmetrical({0, 32, 8, 8}, 3, 3, 32.0f, 18.0f);
	this->slotSelected = npf.createSymmetrical({10, 42, 16, 16}, 5, 5, 32.0f, 18.0f);

	JukeboxManager::instance.loadTracks();
}

void JukeboxScreen::tick() {
	if (!this->minecraft->player) {
		this->minecraft->setScreen(0);
		return;
	}
	float dx = ((float)this->blockX + 0.5f) - this->minecraft->player->posX;
	float dy = ((float)this->blockY + 0.5f) - this->minecraft->player->posY;
	float dz = ((float)this->blockZ + 0.5f) - this->minecraft->player->posZ;
	if (dx * dx + dy * dy + dz * dz > 64.0f) {
		this->minecraft->setScreen(0);
	}
}

bool_t JukeboxScreen::renderGameBehind() { return 1; }
bool_t JukeboxScreen::isInGameScreen() { return 0; }

void JukeboxScreen::buttonClicked(Button* btn) {
}

void JukeboxScreen::mouseClicked(int32_t mx, int32_t my, int32_t btn) {
	const auto& tracks = JukeboxManager::instance.tracks;
	int32_t totalTracks = (int32_t)tracks.size();

	int32_t x0 = this->dialogX;
	int32_t y0 = this->dialogY;
	int32_t x1 = x0 + this->dialogW;
	int32_t y1 = y0 + this->dialogH;

	int32_t listBoxY = y0 + 62;
	int32_t listBoxH = this->dialogH - 62 - 28;
	int32_t listY = listBoxY + 4;
	int32_t listH = listBoxH - 8;
	int32_t itemH = 19;
	int32_t visibleCount = listH / itemH;
	if (visibleCount < 1) visibleCount = 1;

	if (btn == 4) {
		if (this->scrollOffset > 0) this->scrollOffset--;
		return;
	}
	if (btn == 5) {
		if (this->scrollOffset + visibleCount < totalTracks) this->scrollOffset++;
		return;
	}

	int32_t closeX = x1 - 21;
	int32_t closeY = y0 + 3;
	if (mx >= closeX && mx <= closeX + 18 && my >= closeY && my <= closeY + 18) {
		this->minecraft->setScreen(0);
		return;
	}

	int32_t ctrlY = y0 + 40;
	int32_t ctrlH = 18;
	int32_t innerW = this->dialogW - 16;
	int32_t modeW = 66;
	int32_t prevW = 22;
	int32_t nextW = 22;
	int32_t stopW = 40;
	int32_t gap = 3;
	int32_t playPauseW = innerW - modeW - prevW - stopW - nextW - gap * 4;
	if (playPauseW < 44) playPauseW = 44;

	int32_t modeX = x0 + 8;
	if (mx >= modeX && mx <= modeX + modeW && my >= ctrlY && my <= ctrlY + ctrlH) {
		JukeboxManager::instance.cyclePlaybackMode();
		return;
	}

	int32_t prevX = modeX + modeW + gap;
	if (mx >= prevX && mx <= prevX + prevW && my >= ctrlY && my <= ctrlY + ctrlH) {
		JukeboxManager::instance.playPrevTrack();
		return;
	}

	int32_t playPauseX = prevX + prevW + gap;
	if (mx >= playPauseX && mx <= playPauseX + playPauseW && my >= ctrlY && my <= ctrlY + ctrlH) {
		if (JukeboxManager::instance.isPlayingAt(this->blockX, this->blockY, this->blockZ)) {
			JukeboxManager::instance.togglePause();
		} else if (!tracks.empty()) {
			int idx = (JukeboxManager::instance.currentTrackIndex >= 0) ? JukeboxManager::instance.currentTrackIndex : 0;
			JukeboxManager::instance.playTrack(idx, this->blockX, this->blockY, this->blockZ);
		}
		return;
	}

	int32_t stopX = playPauseX + playPauseW + gap;
	if (mx >= stopX && mx <= stopX + stopW && my >= ctrlY && my <= ctrlY + ctrlH) {
		if (JukeboxManager::instance.isPlayingAt(this->blockX, this->blockY, this->blockZ)) {
			JukeboxManager::instance.stopAt(this->blockX, this->blockY, this->blockZ);
		}
		return;
	}

	int32_t nextX = stopX + stopW + gap;
	if (mx >= nextX && mx <= nextX + nextW && my >= ctrlY && my <= ctrlY + ctrlH) {
		JukeboxManager::instance.playNextTrack();
		return;
	}

	bool hasScroll = (totalTracks > visibleCount);
	int32_t scrollX = x1 - 26;
	if (hasScroll) {
		if (this->scrollOffset > 0) {
			if (mx >= scrollX && mx <= scrollX + 16 && my >= listY && my <= listY + 16) {
				this->scrollOffset--;
				return;
			}
		}
		if (this->scrollOffset + visibleCount < totalTracks) {
			int32_t downY = listBoxY + listBoxH - 20;
			if (mx >= scrollX && mx <= scrollX + 16 && my >= downY && my <= downY + 16) {
				this->scrollOffset++;
				return;
			}
		}
	}

	int32_t itemX = x0 + 12;
	int32_t itemW = hasScroll ? (this->dialogW - 42) : (this->dialogW - 24);
	for (int32_t i = 0; i < visibleCount; ++i) {
		int32_t tIdx = this->scrollOffset + i;
		if (tIdx >= totalTracks) break;
		int32_t iy = listY + i * itemH;
		if (mx >= itemX && mx <= itemX + itemW && my >= iy && my <= iy + itemH - 2) {
			JukeboxManager::instance.playTrack(tIdx, this->blockX, this->blockY, this->blockZ);
			return;
		}
	}

	int32_t doneW = 80;
	int32_t doneH = 18;
	int32_t doneY = y1 - 23;
	int32_t doneX = x0 + (this->dialogW - doneW) / 2;
	if (mx >= doneX && mx <= doneX + doneW && my >= doneY && my <= doneY + doneH) {
		this->minecraft->setScreen(0);
		return;
	}

	Screen::mouseClicked(mx, my, btn);
}

void JukeboxScreen::keyPressed(int32_t key) {
	if (key == 27 || key == 6) this->minecraft->setScreen(0);
}

void JukeboxScreen::render(int32_t mx, int32_t my, float a4) {
	this->renderBackground(0);

	int32_t x0 = this->dialogX;
	int32_t y0 = this->dialogY;
	int32_t x1 = x0 + this->dialogW;
	int32_t y1 = y0 + this->dialogH;

	if (this->bgPanel) {
		this->bgPanel->setSize((float)this->dialogW, (float)this->dialogH);
		this->bgPanel->draw(Tesselator::instance, (float)x0, (float)y0);
	}

	std::string title = "Jukebox";
	int32_t tw = this->font->width(title);
	this->font->draw(title, (float)(x0 + (this->dialogW - tw) / 2), (float)(y0 + 7), 0xFF404040);

	int32_t closeX = x1 - 21;
	int32_t closeY = y0 + 3;
	bool closeHov = (mx >= closeX && mx <= closeX + 18 && my >= closeY && my <= closeY + 18);
	renderMcpeCloseBtn(this->minecraft, closeX, closeY, 18, closeHov);

	int32_t statusX = x0 + 8;
	int32_t statusY = y0 + 21;
	int32_t statusW = this->dialogW - 16;
	int32_t statusH = 16;
	if (this->insetPanel) {
		this->insetPanel->setSize((float)statusW, (float)statusH);
		this->insetPanel->draw(Tesselator::instance, (float)statusX, (float)statusY);
	}

	bool isThisPlaying = JukeboxManager::instance.isPlayingAt(this->blockX, this->blockY, this->blockZ);
	bool isPaused = JukeboxManager::instance.isPaused;
	std::string statusText;
	uint32_t statusCol = 0xFFC0C0C0;
	if (isThisPlaying) {
		if (isPaused) {
			statusText = "|| " + JukeboxManager::instance.getCurrentTrackName() + " (Paused)";
			statusCol = 0xFFFFCC55;
		} else {
			statusText = "> " + JukeboxManager::instance.getCurrentTrackName();
			statusCol = 0xFF77FF77;
		}
	} else {
		statusText = "Select a track to play";
		statusCol = 0xFFC8C8C8;
	}
	this->font->drawShadow(statusText, statusX + 5, statusY + 4, statusCol);

	int32_t ctrlY = y0 + 40;
	int32_t ctrlH = 18;
	int32_t innerW = this->dialogW - 16;
	int32_t modeW = 66;
	int32_t prevW = 22;
	int32_t nextW = 22;
	int32_t stopW = 40;
	int32_t gap = 3;
	int32_t playPauseW = innerW - modeW - prevW - stopW - nextW - gap * 4;
	if (playPauseW < 44) playPauseW = 44;

	int32_t modeX = x0 + 8;
	bool modeHov = (mx >= modeX && mx <= modeX + modeW && my >= ctrlY && my <= ctrlY + ctrlH);
	renderMcpeBtn(this->btnNormal, this->btnPressed, this->font, modeX, ctrlY, modeW, ctrlH, JukeboxManager::instance.getPlaybackModeName(), modeHov);

	int32_t prevX = modeX + modeW + gap;
	bool prevHov = (mx >= prevX && mx <= prevX + prevW && my >= ctrlY && my <= ctrlY + ctrlH);
	renderMcpeBtn(this->btnNormal, this->btnPressed, this->font, prevX, ctrlY, prevW, ctrlH, "<<", prevHov);

	int32_t playPauseX = prevX + prevW + gap;
	bool ppHov = (mx >= playPauseX && mx <= playPauseX + playPauseW && my >= ctrlY && my <= ctrlY + ctrlH);
	std::string ppStr = isThisPlaying ? (isPaused ? "Resume" : "Pause") : "Play";
	uint32_t ppCol = isThisPlaying ? (isPaused ? 0xFF88FF88 : 0xFFFFFF88) : 0;
	renderMcpeBtn(this->btnNormal, this->btnPressed, this->font, playPauseX, ctrlY, playPauseW, ctrlH, ppStr, ppHov, true, ppCol);

	int32_t stopX = playPauseX + playPauseW + gap;
	bool stopHov = (mx >= stopX && mx <= stopX + stopW && my >= ctrlY && my <= ctrlY + ctrlH);
	renderMcpeBtn(this->btnNormal, this->btnPressed, this->font, stopX, ctrlY, stopW, ctrlH, "Stop", stopHov, isThisPlaying);

	int32_t nextX = stopX + stopW + gap;
	bool nextHov = (mx >= nextX && mx <= nextX + nextW && my >= ctrlY && my <= ctrlY + ctrlH);
	renderMcpeBtn(this->btnNormal, this->btnPressed, this->font, nextX, ctrlY, nextW, ctrlH, ">>", nextHov);

	int32_t listBoxX = x0 + 8;
	int32_t listBoxY = y0 + 62;
	int32_t listBoxW = this->dialogW - 16;
	int32_t listBoxH = this->dialogH - 62 - 28;
	if (this->insetPanel) {
		this->insetPanel->setSize((float)listBoxW, (float)listBoxH);
		this->insetPanel->draw(Tesselator::instance, (float)listBoxX, (float)listBoxY);
	}

	int32_t listY = listBoxY + 4;
	int32_t listH = listBoxH - 8;
	int32_t itemH = 19;
	int32_t visibleCount = listH / itemH;
	if (visibleCount < 1) visibleCount = 1;

	const auto& tracks = JukeboxManager::instance.tracks;
	int32_t totalTracks = (int32_t)tracks.size();
	bool hasScroll = (totalTracks > visibleCount);
	int32_t itemX = x0 + 12;
	int32_t itemW = hasScroll ? (this->dialogW - 42) : (this->dialogW - 24);

	if (totalTracks == 0) {
		std::string emptyMsg = "No tracks found";
		int32_t ew = this->font->width(emptyMsg);
		this->font->drawShadow(emptyMsg, listBoxX + (listBoxW - ew) / 2, listBoxY + (listBoxH - 8) / 2, 0xFFAAAAAA);
	} else {
		for (int32_t i = 0; i < visibleCount; ++i) {
			int32_t tIdx = this->scrollOffset + i;
			if (tIdx >= totalTracks) break;
			int32_t iy = listY + i * itemH;

			bool hov = (mx >= itemX && mx <= itemX + itemW && my >= iy && my <= iy + itemH - 2);
			bool active = (isThisPlaying && JukeboxManager::instance.currentTrackIndex == tIdx);

			NinePatchLayer* rowBg = hov ? this->btnPressed : this->btnNormal;
			if (rowBg) {
				rowBg->setSize((float)itemW, (float)(itemH - 2));
				rowBg->draw(Tesselator::instance, (float)itemX, (float)iy);
			}
			if (active && this->slotSelected) {
				this->slotSelected->setSize((float)(itemW + 2), (float)(itemH));
				this->slotSelected->draw(Tesselator::instance, (float)(itemX - 1), (float)(iy - 1));
			}

			char tNumBuf[16];
			snprintf(tNumBuf, sizeof(tNumBuf), "%d. ", tIdx + 1);
			std::string trackLabel = std::string(tNumBuf) + tracks[tIdx].name;
			uint32_t textCol = active ? 0xFF88FF88 : (hov ? 0xFFFFFFA0 : 0xFFE0E0E0);
			this->font->drawShadow(trackLabel, itemX + 5, iy + 4, textCol);
		}
	}

	if (hasScroll) {
		int32_t scrollX = x1 - 26;
		int32_t upY = listY;
		bool canUp = (this->scrollOffset > 0);
		bool upHov = (canUp && mx >= scrollX && mx <= scrollX + 16 && my >= upY && my <= upY + 16);
		renderMcpeBtn(this->btnNormal, this->btnPressed, this->font, scrollX, upY, 16, 16, "^", upHov, canUp);

		int32_t downY = listBoxY + listBoxH - 20;
		bool canDown = (this->scrollOffset + visibleCount < totalTracks);
		bool downHov = (canDown && mx >= scrollX && mx <= scrollX + 16 && my >= downY && my <= downY + 16);
		renderMcpeBtn(this->btnNormal, this->btnPressed, this->font, scrollX, downY, 16, 16, "v", downHov, canDown);

		int32_t trackTop = upY + 18;
		int32_t trackBottom = downY - 2;
		int32_t trackH = trackBottom - trackTop;
		if (trackH > 10) {
			this->fill(scrollX + 5, trackTop, scrollX + 11, trackBottom, 0xFF333333);
			float thumbRatio = (float)visibleCount / (float)totalTracks;
			int32_t thumbH = std::max(8, (int32_t)(thumbRatio * (float)trackH));
			float posRatio = (float)this->scrollOffset / (float)(totalTracks - visibleCount);
			int32_t thumbY = trackTop + (int32_t)(posRatio * (float)(trackH - thumbH));
			this->fill(scrollX + 5, thumbY, scrollX + 11, thumbY + thumbH, 0xFF9A9A9A);
			this->fill(scrollX + 5, thumbY, scrollX + 11, thumbY + 1, 0xFFC6C6C6);
			this->fill(scrollX + 5, thumbY + thumbH - 1, scrollX + 11, thumbY + thumbH, 0xFF555555);
		}
	}

	int32_t doneW = 80;
	int32_t doneH = 18;
	int32_t doneY = y1 - 23;
	int32_t doneX = x0 + (this->dialogW - doneW) / 2;
	bool doneHov = (mx >= doneX && mx <= doneX + doneW && my >= doneY && my <= doneY + doneH);
	renderMcpeBtn(this->btnNormal, this->btnPressed, this->font, doneX, doneY, doneW, doneH, "Done", doneHov);
}
