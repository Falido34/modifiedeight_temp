#include <gui/PatchOptionItem.hpp>
#include <Minecraft.hpp>
#include <NinecraftApp.hpp>
#include <gui/buttons/OptionButton.hpp>
#include <gui/buttons/Touch_TButton.hpp>
#include <gui/screens/PatchErrorScreen.hpp>
#include <gui/screens/PatchesScreen.hpp>
#include <patch/PatchManager.hpp>
#include <rendering/Font.hpp>
#include <sound/SoundEngine.hpp>
#include <sstream>

PatchesManageOption::PatchesManageOption(Minecraft* mc, bool_t inWorld)
	: GuiElementContainer(0, 1, 0, 0, 24, 20),
	  pressedButton(nullptr),
	  isInWorld(inWorld) {
	this->manageButton = std::shared_ptr<Touch::TButton>(new Touch::TButton(9999990, 0, 0, 66, 20, "Manage...", mc));
	this->addChild(this->manageButton);
}

PatchesManageOption::~PatchesManageOption() {}

void PatchesManageOption::render(Minecraft* mc, int32_t mx, int32_t my) {
	int32_t vy = (this->height - 8) / 2;
	PatchManager& pm = PatchManager::instance();
	std::ostringstream ss;
	ss << "Patches (" << pm.getEnabledCount() << "/" << (int)pm.getPatches().size() << " active)";
	mc->font->drawShadow(ss.str(), (float)this->posX, (float)this->posY + (float)vy, 0xFFBBBBBB);
	GuiElementContainer::render(mc, mx, my);
}

void PatchesManageOption::setupPositions() {
	int32_t h = 0;
	for (auto c : this->children) {
		c->posX = this->posX + this->width - c->width - 15;
		c->posY = this->posY + h;
		h += c->height;
	}
	this->height = h;
}

void PatchesManageOption::mouseClicked(Minecraft* mc, int32_t mx, int32_t my, int32_t btn) {
	if (btn != 1 || PatchManager::instance().isBuildRunning()) return;
	if (this->manageButton->clicked(mc, mx, my)) {
		this->pressedButton = this->manageButton.get();
		this->pressedButton->setPressed();
	}
}

void PatchesManageOption::mouseReleased(Minecraft* mc, int32_t mx, int32_t my, int32_t btn) {
	if (this->pressedButton == this->manageButton.get() && this->pressedButton->clicked(mc, mx, my)) {
		mc->soundEngine->playUI("random.click", 1, 1);
		mc->setScreen(new PatchesScreen(false, this->isInWorld));
	}
	this->pressedButton = nullptr;
	this->manageButton->released(mx, my);
}

PatchToggleOption::PatchToggleOption(Minecraft* mc, const std::string& id, const std::string& label, bool enabled, bool_t inWorld)
	: GuiElementContainer(0, 1, 0, 0, 24, 20),
	  patchId(id),
	  labelText(label),
	  pressedButton(nullptr),
	  isInWorld(inWorld) {
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
	this->toggleButton = std::shared_ptr<OptionButton>(new OptionButton(enabled ? 1 : 0));
	this->toggleButton->setImageDef(idef, 1);
	this->addChild(this->toggleButton);
}

PatchToggleOption::~PatchToggleOption() {}

void PatchToggleOption::syncButtonLabel() {
	PatchEntry* pe = PatchManager::instance().findPatch(this->patchId);
	if (pe && this->toggleButton) {
		this->toggleButton->boolValue = pe->enabled ? 1 : 0;
	}
}

void PatchToggleOption::render(Minecraft* mc, int32_t mx, int32_t my) {
	this->syncButtonLabel();
	int32_t vy = (this->height - 8) / 2;
	PatchEntry* pe = PatchManager::instance().findPatch(this->patchId);
	uint32_t col = 0xFFBBBBBB;
	if (pe) {
		if (!pe->loadError.empty()) col = 0xFFFF6666;
		else if (pe->enabled) col = 0xFF77FF77;
	}
	std::string text = this->labelText;
	int maxW = this->width - (this->toggleButton ? this->toggleButton->width : 52) - 24;
	if (mc->font && maxW > 24) {
		while (text.size() > 3 && mc->font->width(text) > maxW) {
			text.resize(text.size() - 4);
			text += "...";
		}
	}
	mc->font->drawShadow(text, (float)this->posX, (float)this->posY + (float)vy, col);
	GuiElementContainer::render(mc, mx, my);
}

void PatchToggleOption::setupPositions() {
	int32_t h = 0;
	for (auto c : this->children) {
		c->posX = this->posX + this->width - c->width - 15;
		c->posY = this->posY + h;
		h += c->height;
	}
	this->height = h;
}

void PatchToggleOption::mouseClicked(Minecraft* mc, int32_t mx, int32_t my, int32_t btn) {
	if (btn != 1 || PatchManager::instance().isBuildRunning()) return;
	if (this->toggleButton->clicked(mc, mx, my)) {
		this->pressedButton = this->toggleButton.get();
		this->pressedButton->setPressed();
	}
}

void PatchToggleOption::mouseReleased(Minecraft* mc, int32_t mx, int32_t my, int32_t btn) {
	if (this->pressedButton == this->toggleButton.get() && this->pressedButton->clicked(mc, mx, my)) {
		mc->soundEngine->playUI("random.click", 1, 1);
		PatchManager& pm = PatchManager::instance();
		PatchEntry* pe = pm.findPatch(this->patchId);
		if (pe) {
			std::vector<std::string> prev = pm.getEnabledIdsSnapshot();
			bool nextState = !pe->enabled;
			if (nextState && !pe->loadError.empty()) {
				pm.reportError(pe->loadError);
				mc->setScreen(new PatchErrorScreen(pe->loadError, PatchErrorScreen::RETURN_OPTIONS, this->isInWorld));
				this->pressedButton = nullptr;
				this->toggleButton->released(mx, my);
				return;
			}
			pm.setPatchEnabled(this->patchId, nextState);
			std::string valErr;
			if (!pm.validateEnabledPatches(valErr)) {
				pm.restoreEnabledIds(prev);
				pm.saveEnabledConfig();
				pm.reportError(valErr);
				mc->setScreen(new PatchErrorScreen(valErr, PatchErrorScreen::RETURN_OPTIONS, this->isInWorld));
			} else if (pm.needsCodeRebuildOrRelaunch()) {
				pm.startApplyPatchesAsync(prev);
			} else {
				NinecraftApp* app = (NinecraftApp*)mc;
				if (!pm.applyRuntimeAssetReload(app)) {
					std::string err = pm.getLastError();
					pm.restoreEnabledIds(prev);
					pm.saveEnabledConfig();
					pm.applyRuntimeAssetReload(app);
					pm.reportError(err);
					mc->setScreen(new PatchErrorScreen(err, PatchErrorScreen::RETURN_OPTIONS, this->isInWorld));
				} else {
					pm.saveEnabledConfig();
					this->syncButtonLabel();
				}
			}
		}
	}
	this->pressedButton = nullptr;
	this->toggleButton->released(mx, my);
}
