#pragma once
#include <_types.h>
#include <_AssetFile.hpp>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

struct NinecraftApp;
struct Minecraft;
struct Gui;
struct OptionsPane;

typedef void (*PatchHudRenderFn)(Minecraft*, Gui*);
typedef void (*PatchOptionsPopulateFn)(OptionsPane*, Minecraft*);

struct PatchEntry {
	std::string id;
	std::string displayName;
	std::string version;
	std::string author;
	std::string description;
	std::string sourcePath;
	std::string rootDir;
	bool isZip = false;
	bool enabled = false;
	bool hasCode = false;
	bool hasAssets = false;
	bool hasNativeLib = false;
	int fileCount = 0;
	std::vector<std::string> files;
	std::string loadError;
};

struct PatchManager {
	static PatchManager& instance();

	void handleCommandLine(int argc, char** argv);
	bool maybeRelaunchIntoPatchedOnBoot();
	void setDataPath(const std::string& path);

	std::string getPatchesDir();
	std::string getBaseBinaryPath();
	bool isPatchedBinary() const;
	std::string getActiveCodeFingerprint() const;

	void scanPatches();
	std::vector<PatchEntry>& getPatches();
	const std::vector<PatchEntry>& getPatches() const;
	PatchEntry* findPatch(const std::string& id);

	bool setPatchEnabled(const std::string& id, bool enabled);
	bool movePatchPriority(const std::string& id, int delta);
	bool renamePatch(const std::string& id, const std::string& newName, std::string& outErr);
	bool deletePatch(const std::string& id, std::string& outErr);
	bool saveEnabledConfig();
	std::vector<std::string> getEnabledIdsSnapshot() const;
	void restoreEnabledIds(const std::vector<std::string>& ids);
	int getEnabledCount() const;

	bool initializeOnStartup();
	bool validateEnabledPatches(std::string& outErr);

	AssetFile readAssetFile(const std::string& path, std::string* outPatchId = nullptr, std::string* outDiskPath = nullptr);
	bool hasEnabledAssetOverrides() const;
	std::vector<std::string> collectTextureNamesFromActivePatches(bool isTerrain);
	bool applyRuntimeAssetReload(NinecraftApp* app);

	std::string computeWantedCodeFingerprint() const;
	bool needsCodeRebuildOrRelaunch() const;

	bool startApplyPatchesAsync(const std::vector<std::string>& previousEnabled);
	bool isBuildRunning() const;
	std::string getBuildStatus() const;
	bool pollAsyncResult(bool& outSuccess, bool& outWillRelaunch, std::string& outError);
	bool relaunchPendingBinary();

	void reportError(const std::string& msg);
	void clearError();
	bool hasError() const;
	std::string getLastError() const;

	static std::string stripAnsiCodes(const std::string& s);
	static bool copyTextToClipboard(const std::string& text);
	static void openPatchesFolderInOS();

	void registerHudRenderCallback(PatchHudRenderFn fn);
	void runHudRenderCallbacks(Minecraft* mc, Gui* gui);
	void registerPatchOptionsCallback(PatchOptionsPopulateFn fn);
	void runPatchOptionsCallbacks(OptionsPane* pane, Minecraft* mc);

private:
	PatchManager();
	~PatchManager();
	PatchManager(const PatchManager&) = delete;
	PatchManager& operator=(const PatchManager&) = delete;

	void ensureDirRecursive(const std::string& dir);
	std::string findBaseSourceRoot() const;
	std::string findBaseBuildRoot() const;
	std::string getExecutablePath() const;
	bool loadNativeLibrariesForActivePatches(std::string& outErr);

	std::vector<PatchEntry> patches_;
	std::vector<std::string> loadedNativeLibs_;
	std::string dataPath_;
	std::string exePath_;
	std::string baseBinaryArg_;
	bool relaunchFlag_ = false;
	std::string lastError_;

	mutable std::mutex mutex_;
	std::atomic<bool> buildRunning_{false};
	std::atomic<bool> asyncDone_{false};
	bool asyncSuccess_ = false;
	bool asyncWillRelaunch_ = false;
	std::string asyncError_;
	std::string buildStatus_;
	std::string pendingRelaunchExe_;
	std::vector<std::string> pendingRollbackIds_;
};
