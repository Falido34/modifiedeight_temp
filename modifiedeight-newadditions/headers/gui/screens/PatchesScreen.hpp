#pragma once
#include <_types.h>
#include <gui/Screen.hpp>
#include <string>
#include <vector>

namespace Touch {
	struct TButton;
	struct THeader;
}
struct OptionButton;
struct NinePatchLayer;

struct PatchesScreen : Screen {
	enum ModalMode {
		MODAL_NONE = 0,
		MODAL_RENAME = 1,
		MODAL_DELETE = 2
	};

	struct RowControls {
		std::string patchId;
		OptionButton* btnToggle;
		Touch::TButton* btnRename;
		Touch::TButton* btnDelete;
		Touch::TButton* btnUp;
		Touch::TButton* btnDown;
	};

	bool returnToStartMenu;
	bool optionsIsInWorld;

	Touch::THeader* header;
	Touch::TButton* btnBack;
	Touch::TButton* btnRefresh;
	Touch::TButton* btnFolder;
	NinePatchLayer* panelLayer;
	std::vector<RowControls> rowControls;

	ModalMode modalMode;
	std::string modalTargetId;
	std::string modalTargetDisplay;
	std::string renameInputText;
	Touch::TButton* btnModalConfirm;
	Touch::TButton* btnModalCancel;

	int scrollY;
	int contentHeight;
	bool isDragging;
	int dragStartMouseY;
	int dragStartScrollY;
	std::string statusMsg;
	uint32_t statusColor;

	PatchesScreen(bool returnStart = false, bool inWorld = false);
	virtual ~PatchesScreen();

	void clearRowControls();
	void rebuildPatchRows();
	void commitPatchSelectionChange(const std::vector<std::string>& prevEnabled);
	void openRenameModal(const std::string& patchId, const std::string& currentName);
	void openDeleteModal(const std::string& patchId, const std::string& displayName);
	void closeModal();

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
	virtual void keyPressed(int32_t key);
	virtual void keyboardNewChar(const std::string& ch, bool_t flag);
};
