#pragma once
#include <gui/GuiElementContainer.hpp>
#include <memory>
#include <string>

namespace Touch {
	struct TButton;
}
struct Button;
struct OptionButton;

struct PatchesManageOption : GuiElementContainer {
	std::shared_ptr<Touch::TButton> manageButton;
	Button* pressedButton;
	bool_t isInWorld;

	PatchesManageOption(Minecraft* mc, bool_t inWorld);
	virtual ~PatchesManageOption();
	virtual void render(Minecraft* mc, int32_t mx, int32_t my);
	virtual void setupPositions();
	virtual void mouseClicked(Minecraft* mc, int32_t mx, int32_t my, int32_t btn);
	virtual void mouseReleased(Minecraft* mc, int32_t mx, int32_t my, int32_t btn);
};

struct PatchToggleOption : GuiElementContainer {
	std::string patchId;
	std::string labelText;
	std::shared_ptr<OptionButton> toggleButton;
	Button* pressedButton;
	bool_t isInWorld;

	PatchToggleOption(Minecraft* mc, const std::string& id, const std::string& label, bool enabled, bool_t inWorld);
	virtual ~PatchToggleOption();
	void syncButtonLabel();
	virtual void render(Minecraft* mc, int32_t mx, int32_t my);
	virtual void setupPositions();
	virtual void mouseClicked(Minecraft* mc, int32_t mx, int32_t my, int32_t btn);
	virtual void mouseReleased(Minecraft* mc, int32_t mx, int32_t my, int32_t btn);
};
