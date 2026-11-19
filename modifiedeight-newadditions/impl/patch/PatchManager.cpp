#include <patch/PatchManager.hpp>
#include <patch/PatchedBuildStamp.hpp>

#include <AppPlatform.hpp>
#include <I18n.hpp>
#include <NinecraftApp.hpp>
#include <json/reader.h>
#include <json/value.h>
#include <rendering/LevelRenderer.hpp>
#include <rendering/TextureAtlas.hpp>
#include <rendering/Textures.hpp>
#include <stb_image.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <sys/stat.h>
#include <thread>

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#include <direct.h>
#include <process.h>
#include <windows.h>
#else
#include <dirent.h>
#include <dlfcn.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#ifdef MCPE_HAS_EMBEDDED_SRC
#include <zlib.h>
extern "C" {
extern const uint8_t m8_embedded_src_start[];
extern const uint8_t m8_embedded_src_end[];
}
#endif

#ifndef MCPE_BASE_SOURCE_ROOT
#define MCPE_BASE_SOURCE_ROOT ""
#endif

#ifndef MCPE_BASE_BUILD_ROOT
#define MCPE_BASE_BUILD_ROOT ""
#endif

namespace {

std::string normalizeSlashes(std::string s) {
	for (char& c : s) {
		if (c == '\\') c = '/';
	}
	while (!s.empty() && s.back() == '/') {
		s.pop_back();
	}
	return s;
}

std::string toLowerCopy(std::string s) {
	for (char& c : s) {
		c = (char)std::tolower((unsigned char)c);
	}
	return s;
}

std::string trimCopy(const std::string& s) {
	size_t b = 0;
	while (b < s.size() && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r' || s[b] == '\n')) ++b;
	size_t e = s.size();
	while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r' || s[e - 1] == '\n')) --e;
	return s.substr(b, e - b);
}

bool startsWithStr(const std::string& s, const std::string& prefix) {
	return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

bool endsWithStr(const std::string& s, const std::string& suffix) {
	return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool pathExists(const std::string& p) {
	struct stat st;
	return stat(p.c_str(), &st) == 0;
}

bool isDirectory(const std::string& p) {
	struct stat st;
	if (stat(p.c_str(), &st) != 0) return false;
	return S_ISDIR(st.st_mode);
}

bool isRegularFile(const std::string& p) {
	struct stat st;
	if (stat(p.c_str(), &st) != 0) return false;
	return S_ISREG(st.st_mode);
}

bool getFileSizeAndMtime(const std::string& p, long long& sizeOut, long long& mtimeOut) {
	struct stat st;
	if (stat(p.c_str(), &st) != 0) return false;
	sizeOut = (long long)st.st_size;
	mtimeOut = (long long)st.st_mtime;
	return true;
}

bool mkDirOne(const std::string& p) {
	if (p.empty() || isDirectory(p)) return true;
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
	return _mkdir(p.c_str()) == 0 || isDirectory(p);
#else
	return mkdir(p.c_str(), 0755) == 0 || isDirectory(p);
#endif
}

void ensureDirRecursiveImpl(const std::string& dir) {
	std::string clean = normalizeSlashes(dir);
	if (clean.empty()) return;
	size_t start = 0;
	if (clean.size() >= 2 && clean[1] == ':') {
		start = 2;
	}
	for (size_t i = start; i < clean.size(); ++i) {
		if (clean[i] == '/' && i > 0) {
			mkDirOne(clean.substr(0, i));
		}
	}
	mkDirOne(clean);
}

std::string parentDir(const std::string& p) {
	std::string clean = normalizeSlashes(p);
	size_t pos = clean.find_last_of('/');
	if (pos == std::string::npos) return "";
	if (pos == 0) return "/";
	return clean.substr(0, pos);
}

bool isAbsolutePath(const std::string& p) {
	if (p.empty()) return false;
	if (p[0] == '/') return true;
	if (p.size() >= 2 && p[1] == ':') return true;
	return false;
}

std::string makeAbsolutePath(const std::string& p) {
	std::string clean = normalizeSlashes(p);
	if (clean.empty() || isAbsolutePath(clean)) return clean;
	char cwdBuf[2048];
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
	if (_getcwd(cwdBuf, sizeof(cwdBuf))) {
		return normalizeSlashes(std::string(cwdBuf) + "/" + clean);
	}
#else
	if (getcwd(cwdBuf, sizeof(cwdBuf))) {
		return normalizeSlashes(std::string(cwdBuf) + "/" + clean);
	}
#endif
	return clean;
}

uint16_t readLE16(const uint8_t* p) {
	return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

uint32_t readLE32(const uint8_t* p) {
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

std::string shellQuote(const std::string& s) {
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
	std::string out = "\"";
	for (char c : s) {
		if (c == '"') out += "\\\"";
		else out += c;
	}
	out += "\"";
	return out;
#else
	std::string out = "'";
	for (char c : s) {
		if (c == '\'') out += "'\\''";
		else out += c;
	}
	out += "'";
	return out;
#endif
}

bool readFileBytes(const std::string& path, std::vector<uint8_t>& out) {
	FILE* f = fopen(path.c_str(), "rb");
	if (!f) return false;
	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	if (sz < 0) sz = 0;
	fseek(f, 0, SEEK_SET);
	out.resize((size_t)sz);
	if (sz > 0) {
		size_t r = fread(out.data(), 1, (size_t)sz, f);
		out.resize(r);
	}
	fclose(f);
	return true;
}

bool writeFileIfDifferent(const std::string& dst, const uint8_t* data, size_t len, bool* outChanged = nullptr) {
	std::vector<uint8_t> existing;
	if (isRegularFile(dst) && readFileBytes(dst, existing)) {
		if (existing.size() == len && (len == 0 || memcmp(existing.data(), data, len) == 0)) {
			if (outChanged) *outChanged = false;
			return true;
		}
	}
	ensureDirRecursiveImpl(parentDir(dst));
	FILE* f = fopen(dst.c_str(), "wb");
	if (!f) return false;
	if (len > 0) {
		size_t w = fwrite(data, 1, len, f);
		if (w != len) {
			fclose(f);
			return false;
		}
	}
	fclose(f);
	if (outChanged) *outChanged = true;
	return true;
}

bool copyFileIfDifferent(const std::string& src, const std::string& dst, std::string& errOut) {
	std::vector<uint8_t> buf;
	if (!readFileBytes(src, buf)) {
		errOut = "Failed to read file '" + src + "': " + strerror(errno);
		return false;
	}
	if (!writeFileIfDifferent(dst, buf.data(), buf.size())) {
		errOut = "Failed to write file '" + dst + "': " + strerror(errno);
		return false;
	}
	return true;
}

void removePathRecursive(const std::string& path) {
	if (!pathExists(path)) {
#if !defined(_WIN32) && !defined(__WIN32__) && !defined(WIN32)
		struct stat lst;
		if (lstat(path.c_str(), &lst) == 0 && S_ISLNK(lst.st_mode)) {
			unlink(path.c_str());
		}
#endif
		return;
	}
	if (!isDirectory(path)) {
		remove(path.c_str());
		return;
	}
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA((path + "\\*").c_str(), &fd);
	if (h != INVALID_HANDLE_VALUE) {
		do {
			std::string name = fd.cFileName;
			if (name == "." || name == "..") continue;
			removePathRecursive(path + "/" + name);
		} while (FindNextFileA(h, &fd));
		FindClose(h);
	}
	_rmdir(path.c_str());
#else
	DIR* d = opendir(path.c_str());
	if (d) {
		struct dirent* ent;
		while ((ent = readdir(d)) != nullptr) {
			std::string name = ent->d_name;
			if (name == "." || name == "..") continue;
			std::string child = path + "/" + name;
			struct stat lst;
			if (lstat(child.c_str(), &lst) == 0 && S_ISLNK(lst.st_mode)) {
				unlink(child.c_str());
			} else {
				removePathRecursive(child);
			}
		}
		closedir(d);
	}
	rmdir(path.c_str());
#endif
}

bool listDirEntries(const std::string& dir, std::vector<std::string>& outNames) {
	outNames.clear();
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA((dir + "\\*").c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE) return false;
	do {
		std::string n = fd.cFileName;
		if (n != "." && n != "..") outNames.push_back(n);
	} while (FindNextFileA(h, &fd));
	FindClose(h);
	return true;
#else
	DIR* d = opendir(dir.c_str());
	if (!d) return false;
	struct dirent* ent;
	while ((ent = readdir(d)) != nullptr) {
		std::string n = ent->d_name;
		if (n != "." && n != "..") outNames.push_back(n);
	}
	closedir(d);
	return true;
#endif
}

bool collectFilesRecursive(const std::string& root, const std::string& relPrefix, std::vector<std::string>& outRelPaths, bool skipAssetsAndGit = false) {
	std::vector<std::string> names;
	std::string curDir = relPrefix.empty() ? root : (root + "/" + relPrefix);
	if (!listDirEntries(curDir, names)) return false;
	std::sort(names.begin(), names.end());
	for (const std::string& name : names) {
		if (skipAssetsAndGit && (name == ".git" || (relPrefix.empty() && name == "assets"))) continue;
		std::string rel = relPrefix.empty() ? name : (relPrefix + "/" + name);
		std::string full = root + "/" + rel;
		if (isDirectory(full)) {
			collectFilesRecursive(root, rel, outRelPaths, false);
		} else if (isRegularFile(full)) {
			outRelPaths.push_back(rel);
		}
	}
	return true;
}

std::string getExtensionLower(const std::string& path) {
	size_t slash = path.find_last_of('/');
	size_t dot = path.find_last_of('.');
	if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) return "";
	return toLowerCopy(path.substr(dot));
}

bool isCodeFileRel(const std::string& rel) {
	std::string ext = getExtensionLower(rel);
	if (rel == "CMakeLists.txt") return true;
	if (ext == ".cpp" || ext == ".c" || ext == ".cc" || ext == ".cxx" || ext == ".S" || ext == ".s" ||
	    ext == ".hpp" || ext == ".h" || ext == ".hh" || ext == ".hxx" || ext == ".inl" || ext == ".inc") {
		return true;
	}
	if (startsWithStr(rel, "impl/") || startsWithStr(rel, "headers/")) {
		return true;
	}
	return false;
}

bool isNativeLibRel(const std::string& rel) {
	std::string ext = getExtensionLower(rel);
	return ext == ".so" || ext == ".dll" || ext == ".dylib";
}

bool isImageFileRel(const std::string& rel) {
	std::string ext = getExtensionLower(rel);
	return ext == ".png" || ext == ".tga" || ext == ".jpg" || ext == ".jpeg";
}

bool isJsonMetaFileRel(const std::string& rel) {
	std::string ext = getExtensionLower(rel);
	return ext == ".meta" || (ext == ".json" && rel != "patch.json");
}

bool isAssetFileRel(const std::string& rel) {
	if (isCodeFileRel(rel) || isNativeLibRel(rel) || rel == "patch.json") return false;
	if (startsWithStr(rel, "assets/") || startsWithStr(rel, "images/") || startsWithStr(rel, "textures/") ||
	    startsWithStr(rel, "font/") || startsWithStr(rel, "gui/") || startsWithStr(rel, "mob/") ||
	    startsWithStr(rel, "lang/") || startsWithStr(rel, "sound/")) {
		return true;
	}
	std::string ext = getExtensionLower(rel);
	return ext == ".png" || ext == ".tga" || ext == ".jpg" || ext == ".jpeg" ||
	       ext == ".meta" || ext == ".lang" || ext == ".json" || ext == ".txt" ||
	       ext == ".wav" || ext == ".ogg" || ext == ".pcm";
}

std::string resolveNormalizedPatchRoot(const std::string& baseDir) {
	std::string cur = baseDir;
	for (int depth = 0; depth < 2; ++depth) {
		std::vector<std::string> entries;
		if (!listDirEntries(cur, entries)) break;
		std::vector<std::string> nonMeta;
		for (const auto& e : entries) {
			if (!e.empty() && e[0] == '.') continue;
			nonMeta.push_back(e);
		}
		if (nonMeta.size() == 1 && isDirectory(cur + "/" + nonMeta[0])) {
			const std::string& only = nonMeta[0];
			if (only != "assets" && only != "impl" && only != "headers" &&
			    only != "images" && only != "textures" && only != "font" &&
			    only != "gui" && only != "mob" && only != "lang" && only != "sound") {
				std::string sub = cur + "/" + only;
				if (only == "modifiedeight-newadditions" ||
				    isDirectory(sub + "/assets") || isDirectory(sub + "/impl") ||
				    isDirectory(sub + "/headers") || isDirectory(sub + "/images") ||
				    isDirectory(sub + "/textures") || isDirectory(sub + "/modifiedeight-newadditions") ||
				    isRegularFile(sub + "/patch.json") || isRegularFile(sub + "/CMakeLists.txt")) {
					cur = sub;
					continue;
				}
			}
		}
		break;
	}
	return cur;
}

uint64_t fnv1a64(const void* data, size_t len, uint64_t seed = 14695981039346656037ULL) {
	uint64_t h = seed;
	const uint8_t* p = (const uint8_t*)data;
	for (size_t i = 0; i < len; ++i) {
		h ^= (uint64_t)p[i];
		h *= 1099511628211ULL;
	}
	return h;
}

bool runCommandCapture(const std::string& cmd, std::string& outLog, int& outExitCode,
                       std::mutex* statusMtx = nullptr, std::string* statusLine = nullptr) {
	outLog.clear();
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
	FILE* pipe = _popen(cmd.c_str(), "r");
#else
	FILE* pipe = popen(cmd.c_str(), "r");
#endif
	if (!pipe) {
		outLog = "Failed to execute command: " + cmd + " (" + strerror(errno) + ")";
		outExitCode = -1;
		return false;
	}
	char buf[1024];
	while (fgets(buf, sizeof(buf), pipe) != nullptr) {
		outLog += buf;
		if (statusMtx && statusLine) {
			std::string line = trimCopy(buf);
			if (!line.empty()) {
				std::lock_guard<std::mutex> lk(*statusMtx);
				*statusLine = PatchManager::stripAnsiCodes(line);
			}
		}
	}
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
	outExitCode = _pclose(pipe);
#else
	int raw = pclose(pipe);
	if (WIFEXITED(raw)) outExitCode = WEXITSTATUS(raw);
	else outExitCode = raw;
#endif
	return true;
}

std::vector<std::string> buildAssetCandidates(const std::string& path) {
	std::string clean = path;
	for (char& c : clean) {
		if (c == '\\') c = '/';
	}
	while (startsWithStr(clean, "./")) clean = clean.substr(2);
	while (!clean.empty() && clean[0] == '/') clean = clean.substr(1);

	std::string noAssets = clean;
	while (startsWithStr(noAssets, "assets/")) noAssets = noAssets.substr(7);

	std::string noImages = noAssets;
	if (startsWithStr(noImages, "images/")) noImages = noImages.substr(7);

	std::vector<std::string> out;
	auto add = [&](const std::string& s) {
		if (!s.empty() && std::find(out.begin(), out.end(), s) == out.end()) {
			out.push_back(s);
		}
	};

	add("assets/" + noAssets);
	add("assets/images/" + noImages);
	if (noAssets != noImages) {
		add("assets/images/" + noAssets);
	}
	add(noAssets);
	add("images/" + noImages);
	return out;
}

}

PatchManager& PatchManager::instance() {
	static PatchManager inst;
	return inst;
}

PatchManager::PatchManager() {}
PatchManager::~PatchManager() {}

void PatchManager::ensureDirRecursive(const std::string& dir) {
	ensureDirRecursiveImpl(dir);
}

std::string PatchManager::getExecutablePath() const {
	char buf[2048];
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
	DWORD len = GetModuleFileNameA(NULL, buf, sizeof(buf) - 1);
	if (len > 0 && len < sizeof(buf)) {
		buf[len] = '\0';
		return normalizeSlashes(buf);
	}
#elif !defined(ANDROID) && !defined(MCPE_IOS)
	ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
	if (len > 0 && (size_t)len < sizeof(buf)) {
		buf[len] = '\0';
		return normalizeSlashes(buf);
	}
#endif
	if (!this->exePath_.empty()) return this->exePath_;
	return "";
}

void PatchManager::handleCommandLine(int argc, char** argv) {
	std::lock_guard<std::mutex> lock(this->mutex_);
	if (argc > 0 && argv && argv[0]) {
		this->exePath_ = normalizeSlashes(argv[0]);
	}
	for (int i = 1; i < argc; ++i) {
		if (!argv[i]) continue;
		std::string arg = argv[i];
		if (arg == "--m8-patched") {
			this->relaunchFlag_ = true;
		} else if (arg == "--m8-base-bin" && i + 1 < argc && argv[i + 1]) {
			this->baseBinaryArg_ = normalizeSlashes(argv[++i]);
		}
	}
}

void PatchManager::setDataPath(const std::string& path) {
	std::lock_guard<std::mutex> lock(this->mutex_);
	this->dataPath_ = normalizeSlashes(path);
}

std::string PatchManager::getPatchesDir() {
	std::string exe = this->getExecutablePath();
	size_t cbPos = exe.find("/.build/cmake_build/");
	if (cbPos != std::string::npos) {
		std::string pdir = makeAbsolutePath(exe.substr(0, cbPos));
		ensureDirRecursiveImpl(pdir);
		return pdir;
	}
	if (!this->baseBinaryArg_.empty()) {
		std::string bdir = parentDir(makeAbsolutePath(this->baseBinaryArg_));
		if (!bdir.empty() && isDirectory(bdir + "/patches")) {
			return bdir + "/patches";
		}
	}
	std::string exeDir = parentDir(makeAbsolutePath(exe));
	if (!exeDir.empty() && isDirectory(exeDir + "/patches")) {
		return exeDir + "/patches";
	}
	if (isDirectory("patches")) {
		return makeAbsolutePath("patches");
	}
	if (!exeDir.empty() && (isDirectory(exeDir + "/skin_cache") || isDirectory(exeDir + "/capes") || isDirectory(exeDir + "/assets"))) {
		std::string dir = exeDir + "/patches";
		ensureDirRecursiveImpl(dir);
		return dir;
	}
	if (isDirectory("skin_cache") || isDirectory("capes") || isDirectory("texture_packs") || isDirectory("assets")) {
		std::string dir = makeAbsolutePath("patches");
		ensureDirRecursiveImpl(dir);
		return dir;
	}
#if defined(ANDROID) || defined(MCPE_IOS)
	if (!this->dataPath_.empty()) {
		std::string dir = this->dataPath_ + "/patches";
		ensureDirRecursiveImpl(dir);
		return dir;
	}
#endif
	std::string dir = makeAbsolutePath("patches");
	ensureDirRecursiveImpl(dir);
	return dir;
}

std::string PatchManager::getBaseBinaryPath() {
	if (!this->baseBinaryArg_.empty() && isRegularFile(this->baseBinaryArg_)) {
		return makeAbsolutePath(this->baseBinaryArg_);
	}
	if (!this->isPatchedBinary()) {
		std::string self = this->getExecutablePath();
		if (!self.empty() && isRegularFile(self)) return makeAbsolutePath(self);
	}
	std::string pdirParent = parentDir(this->getPatchesDir());
	if (!pdirParent.empty()) {
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
		std::string beside = pdirParent + "/modifiedeight-newadditions.exe";
#else
		std::string beside = pdirParent + "/modifiedeight-newadditions";
#endif
		if (isRegularFile(beside)) return beside;
	}
	std::string buildRoot = this->findBaseBuildRoot();
	if (!buildRoot.empty()) {
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
		std::string cand = buildRoot + "/modifiedeight-newadditions/build/modifiedeight-newadditions.exe";
#else
		std::string cand = buildRoot + "/modifiedeight-newadditions/build/modifiedeight-newadditions";
#endif
		if (isRegularFile(cand)) return cand;
	}
	return "modifiedeight-newadditions";
}

bool PatchManager::isPatchedBinary() const {
	if (strlen(MCPE_ACTIVE_CODE_FINGERPRINT) == 0) return false;
	std::string exe = this->getExecutablePath();
	if (!exe.empty() && exe.find("/.build/cmake_build/") == std::string::npos) return false;
	return true;
}

std::string PatchManager::getActiveCodeFingerprint() const {
	if (this->isPatchedBinary()) {
		return MCPE_ACTIVE_CODE_FINGERPRINT;
	}
	std::lock_guard<std::mutex> lock(this->mutex_);
	return this->activeDynamicPluginFp_;
}

std::string PatchManager::findBaseSourceRoot() const {
	auto isValidRoot = [](const std::string& s) {
		return !s.empty() && isRegularFile(s + "/CMakeLists.txt") && isRegularFile(s + "/modifiedeight-newadditions/CMakeLists.txt");
	};
	const char* envRoot = getenv("M8_SOURCE_ROOT");
	if (envRoot && envRoot[0]) {
		std::string s = makeAbsolutePath(envRoot);
		if (isValidRoot(s)) return s;
	}
	if (strlen(MCPE_BASE_SOURCE_ROOT) > 0) {
		std::string s = makeAbsolutePath(MCPE_BASE_SOURCE_ROOT);
		if (isValidRoot(s)) return s;
	}
	std::vector<std::string> starts;
	char cwdBuf[2048];
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
	if (_getcwd(cwdBuf, sizeof(cwdBuf))) {
		starts.push_back(normalizeSlashes(cwdBuf));
	}
#else
	if (getcwd(cwdBuf, sizeof(cwdBuf))) {
		starts.push_back(normalizeSlashes(cwdBuf));
	}
#endif
	std::string exePath = makeAbsolutePath(this->getExecutablePath());
	std::string exeDir = parentDir(exePath);
	if (!exeDir.empty()) starts.push_back(exeDir);
	if (!this->baseBinaryArg_.empty()) {
		std::string bdir = parentDir(makeAbsolutePath(this->baseBinaryArg_));
		if (!bdir.empty()) starts.push_back(bdir);
	}

	for (std::string cur : starts) {
		for (int depth = 0; depth < 6 && !cur.empty(); ++depth) {
			if (isValidRoot(cur)) return cur;
			std::string p = parentDir(cur);
			if (p == cur) break;
			cur = p;
		}
	}

#ifndef MCPE_HAS_EMBEDDED_SRC
	size_t cbPos = exePath.find("/.build/cmake_build/");
	if (cbPos != std::string::npos) {
		std::string cand = exePath.substr(0, cbPos) + "/.build/embedded_src";
		if (isValidRoot(cand)) return cand;
	}
	for (const std::string& st : starts) {
		std::string cand = st + "/patches/.build/embedded_src";
		if (isValidRoot(cand)) return cand;
	}
#endif
	return "";
}

std::string PatchManager::extractEmbeddedSourceRoot(std::string& outErr) {
#ifdef MCPE_HAS_EMBEDDED_SRC
	const uint8_t* start = m8_embedded_src_start;
	const uint8_t* end = m8_embedded_src_end;
	if (!start || !end || end <= start || (size_t)(end - start) < 12) {
		outErr = "Embedded client source bundle is missing or corrupted.";
		return "";
	}
	size_t blobLen = (size_t)(end - start);
	if (memcmp(start, "M8SB", 4) != 0) {
		outErr = "Embedded client source bundle has invalid magic header.";
		return "";
	}
	uint32_t rawLen = readLE32(start + 4);
	uint32_t fileCount = readLE32(start + 8);

	std::string pdir = makeAbsolutePath(this->getPatchesDir());
	std::string outRoot = pdir + "/.build/embedded_src";
	std::string stampPath = outRoot + "/.bundle_stamp";

	uint64_t h1 = fnv1a64(start, blobLen, 14695981039346656037ULL);
	uint64_t h2 = fnv1a64(start, blobLen, 1099511628211ULL);
	char stampBuf[64];
	snprintf(stampBuf, sizeof(stampBuf), "%016llx%016llx:%u:%u",
	         (unsigned long long)h1, (unsigned long long)h2, (unsigned)rawLen, (unsigned)fileCount);
	std::string expectedStamp(stampBuf);

	if (isRegularFile(outRoot + "/CMakeLists.txt") &&
	    isRegularFile(outRoot + "/modifiedeight-newadditions/CMakeLists.txt")) {
		std::ifstream sf(stampPath.c_str());
		std::string curStamp;
		if (sf.is_open() && std::getline(sf, curStamp) && trimCopy(curStamp) == expectedStamp) {
			return outRoot;
		}
	}

	{
		std::lock_guard<std::mutex> lk(this->mutex_);
		this->buildStatus_ = "Extracting embedded client sources...";
	}

	std::vector<uint8_t> raw(rawLen);
	uLongf destLen = (uLongf)rawLen;
	int zrc = uncompress(raw.data(), &destLen, start + 12, (uLong)(blobLen - 12));
	if (zrc != Z_OK || destLen != rawLen) {
		outErr = "Failed to decompress embedded client sources (zlib error " + std::to_string(zrc) + ").";
		return "";
	}

	removePathRecursive(outRoot);
	ensureDirRecursiveImpl(outRoot);

	size_t pos = 0;
	for (uint32_t i = 0; i < fileCount; ++i) {
		if (pos + 2 > raw.size()) {
			outErr = "Truncated embedded source bundle (path length).";
			return "";
		}
		uint16_t pathLen = readLE16(raw.data() + pos);
		pos += 2;
		if (pos + pathLen + 4 > raw.size()) {
			outErr = "Truncated embedded source bundle (entry header).";
			return "";
		}
		std::string rel((const char*)(raw.data() + pos), pathLen);
		pos += pathLen;
		uint32_t dataLen = readLE32(raw.data() + pos);
		pos += 4;
		if (pos + dataLen > raw.size()) {
			outErr = "Truncated embedded source bundle (file payload).";
			return "";
		}
		if (rel.empty() || rel[0] == '/' || rel == ".." || startsWithStr(rel, "../") ||
		    rel.find("/../") != std::string::npos || endsWithStr(rel, "/..")) {
			outErr = "Unsafe relative path in embedded source bundle: " + rel;
			return "";
		}
		std::string dst = outRoot + "/" + rel;
		if (!writeFileIfDifferent(dst, raw.data() + pos, dataLen)) {
			outErr = "Failed to extract embedded source file '" + dst + "': " + strerror(errno);
			return "";
		}
		pos += dataLen;
	}

	if (!writeFileIfDifferent(stampPath, (const uint8_t*)expectedStamp.data(), expectedStamp.size())) {
		outErr = "Failed to write embedded source stamp.";
		return "";
	}
	return outRoot;
#else
	(void)outErr;
	return "";
#endif
}

bool PatchManager::areEnabledCodePatchesPureAdditive(const std::string& baseSourceRoot) const {
	std::lock_guard<std::mutex> lock(this->mutex_);
	bool hasAnyCodePatch = false;
	bool hasAnyCpp = false;
	for (const auto& p : this->patches_) {
		if (!p.enabled || !p.hasCode) continue;
		hasAnyCodePatch = true;
		for (const std::string& rel : p.files) {
			if (!isCodeFileRel(rel)) continue;
			if (rel == "CMakeLists.txt") return false;
			std::string ext = getExtensionLower(rel);
			if (ext == ".s" || ext == ".c") return false;
			if (isRegularFile(baseSourceRoot + "/modifiedeight-newadditions/" + rel)) {
				return false;
			}
			if (ext == ".cpp" || ext == ".cc" || ext == ".cxx") {
				hasAnyCpp = true;
			}
		}
	}
	return hasAnyCodePatch && hasAnyCpp;
}

bool PatchManager::buildAdditiveDynamicPlugin(const std::string& baseSourceRoot, const std::string& wantedFp, std::string& outPluginPath, std::string& outErr) {
#if !defined(_WIN32) && !defined(__WIN32__) && !defined(WIN32) && !defined(ANDROID) && !defined(MCPE_IOS) && !defined(__APPLE__)
	std::string cxx;
	const char* compilers[] = {"c++", "g++", "clang++"};
	for (int i = 0; i < 3; ++i) {
		std::string probe = std::string("command -v ") + compilers[i] + " >/dev/null 2>&1";
		if (system(probe.c_str()) == 0) {
			cxx = compilers[i];
			break;
		}
	}
	if (cxx.empty()) {
		outErr = "No C++ compiler (c++, g++, or clang++) found in PATH to compile code patches.";
		return false;
	}

	std::vector<PatchEntry> enabledCodePatches;
	{
		std::lock_guard<std::mutex> lk(this->mutex_);
		for (const auto& p : this->patches_) {
			if (p.enabled && p.hasCode) enabledCodePatches.push_back(p);
		}
	}
	std::map<std::string, std::string> relToSrc;
	std::ostringstream namesSS;
	for (size_t i = 0; i < enabledCodePatches.size(); ++i) {
		if (i > 0) namesSS << ", ";
		namesSS << enabledCodePatches[i].displayName;
	}
	for (auto it = enabledCodePatches.rbegin(); it != enabledCodePatches.rend(); ++it) {
		for (const std::string& rel : it->files) {
			std::string ext = getExtensionLower(rel);
			if (ext == ".cpp" || ext == ".cc" || ext == ".cxx") {
				relToSrc[rel] = it->rootDir + "/" + rel;
			}
		}
	}
	if (relToSrc.empty()) {
		outErr = "No C++ source files found in enabled code patches.";
		return false;
	}

	std::string pdir = makeAbsolutePath(this->getPatchesDir());
	std::string pluginsDir = pdir + "/.build/plugins";
	ensureDirRecursiveImpl(pluginsDir);
	outPluginPath = pluginsDir + "/m8_patches_" + wantedFp + ".so";
	std::string tmpPath = outPluginPath + ".tmp";

	{
		std::lock_guard<std::mutex> lk(this->mutex_);
		this->buildStatus_ = "Compiling patch plugin (" + namesSS.str() + ")...";
	}

	std::ostringstream cmd;
	cmd << cxx
	    << " -shared -fPIC -O2 -std=gnu++11"
	    << " -fpermissive -Wno-out-of-line-declaration -Wno-write-strings"
	    << " -DDIFFERENTTWEAKS -DIMTOOLAZYTOFIX16ERRS -DPCTWEAKS"
	    << " -I" << shellQuote(baseSourceRoot + "/modifiedeight-newadditions/headers")
	    << " -I" << shellQuote(baseSourceRoot + "/stb")
	    << " -I" << shellQuote(baseSourceRoot + "/glm")
	    << " -I" << shellQuote(baseSourceRoot + "/OpenAES/inc")
	    << " -I" << shellQuote(baseSourceRoot + "/RakNet/Source")
	    << " -I" << shellQuote(baseSourceRoot + "/jsoncpp/jsoncpp/include")
	    << " -I" << shellQuote(baseSourceRoot + "/jsoncpp/jsoncpp/src/lib_json")
	    << " -I" << shellQuote(baseSourceRoot + "/utf8proc");
	if (isDirectory("/usr/include/SDL")) {
		cmd << " -I/usr/include/SDL";
	} else if (isDirectory("/run/host/usr/include/SDL")) {
		cmd << " -I/run/host/usr/include/SDL";
	}
	for (const auto& p : enabledCodePatches) {
		if (isDirectory(p.rootDir + "/headers")) {
			cmd << " -I" << shellQuote(p.rootDir + "/headers");
		}
		cmd << " -I" << shellQuote(p.rootDir);
	}
	for (const auto& kv : relToSrc) {
		cmd << " " << shellQuote(kv.second);
	}
	cmd << " -o " << shellQuote(tmpPath) << " 2>&1";

	std::string log;
	int code = 0;
	runCommandCapture(cmd.str(), log, code, &this->mutex_, &this->buildStatus_);
	if (code != 0 || !isRegularFile(tmpPath)) {
		remove(tmpPath.c_str());
		outErr = "Failed to compile patch(es): " + namesSS.str() + "\n\n" + PatchManager::stripAnsiCodes(trimCopy(log));
		return false;
	}
	remove(outPluginPath.c_str());
	if (::rename(tmpPath.c_str(), outPluginPath.c_str()) != 0) {
		remove(tmpPath.c_str());
		outErr = std::string("Failed to finalize compiled patch plugin: ") + strerror(errno);
		return false;
	}
	return true;
#else
	(void)baseSourceRoot;
	(void)wantedFp;
	(void)outPluginPath;
	outErr = "Dynamic patch plugins are not supported on this platform.";
	return false;
#endif
}

bool PatchManager::loadDynamicPatchPlugin(const std::string& pluginPath, const std::string& fp, std::string& outErr) {
#if !defined(_WIN32) && !defined(__WIN32__) && !defined(WIN32) && !defined(ANDROID) && !defined(MCPE_IOS) && !defined(__APPLE__)
	dlerror();
	void* handle = dlopen(pluginPath.c_str(), RTLD_NOW | RTLD_GLOBAL);
	if (!handle) {
		const char* err = dlerror();
		outErr = "Failed to load compiled patch plugin '" + pluginPath + "':\n" + (err ? err : "unknown dlopen error");
		return false;
	}
	typedef const char* (*PatchInitFn)();
	PatchInitFn initFn = (PatchInitFn)dlsym(handle, "mcpe_patch_init");
	if (!initFn) initFn = (PatchInitFn)dlsym(handle, "patch_init");
	if (initFn) {
		const char* errStr = initFn();
		if (errStr && errStr[0]) {
			outErr = "Compiled patch plugin returned error:\n" + std::string(errStr);
			return false;
		}
	}
	{
		std::lock_guard<std::mutex> lock(this->mutex_);
		this->activeDynamicPluginFp_ = fp;
	}
	return true;
#else
	(void)pluginPath;
	(void)fp;
	outErr = "Dynamic patch plugins are not supported on this platform.";
	return false;
#endif
}

std::string PatchManager::findBaseBuildRoot() const {
	if (strlen(MCPE_BASE_BUILD_ROOT) > 0) {
		std::string b = normalizeSlashes(MCPE_BASE_BUILD_ROOT);
		if (isDirectory(b)) return b;
	}
	std::string srcRoot = this->findBaseSourceRoot();
	if (!srcRoot.empty() && isDirectory(srcRoot + "/build")) {
		return srcRoot + "/build";
	}
	return "";
}

void PatchManager::scanPatches() {
	std::lock_guard<std::mutex> lock(this->mutex_);
	this->patches_.clear();

	std::string dir = this->getPatchesDir();
	std::vector<std::string> enabledOrder;
	{
		std::ifstream ef((dir + "/enabled.txt").c_str());
		std::string line;
		while (ef.is_open() && std::getline(ef, line)) {
			line = trimCopy(line);
			if (!line.empty() && line[0] != '#') {
				if (std::find(enabledOrder.begin(), enabledOrder.end(), line) == enabledOrder.end()) {
					enabledOrder.push_back(line);
				}
			}
		}
	}

	std::vector<std::string> names;
	if (!listDirEntries(dir, names)) return;
	std::sort(names.begin(), names.end());

	std::set<std::string> seenIds;
	std::vector<PatchEntry> found;

	for (const std::string& name : names) {
		if (name.empty() || name[0] == '.') continue;
		std::string full = dir + "/" + name;
		if (!isDirectory(full)) continue;
		seenIds.insert(name);
		PatchEntry entry;
		entry.id = name;
		entry.displayName = name;
		entry.sourcePath = full;
		entry.rootDir = resolveNormalizedPatchRoot(full);
		entry.isZip = false;
		found.push_back(entry);
	}

	for (const std::string& name : names) {
		if (name.empty() || name[0] == '.') continue;
		std::string full = dir + "/" + name;
		if (!isRegularFile(full)) continue;
		std::string lower = toLowerCopy(name);
		if (!endsWithStr(lower, ".zip") || name.size() <= 4) continue;
		std::string id = name.substr(0, name.size() - 4);
		if (seenIds.count(id)) continue;
		seenIds.insert(id);

		PatchEntry entry;
		entry.id = id;
		entry.displayName = id;
		entry.sourcePath = full;
		entry.isZip = true;

		std::string zipCache = dir + "/.zips/" + id;
		long long zSize = 0, zMtime = 0;
		getFileSizeAndMtime(full, zSize, zMtime);
		std::ostringstream stampSS;
		stampSS << zSize << ":" << zMtime;
		std::string expectedStamp = stampSS.str();

		bool needExtract = true;
		{
			std::ifstream sf((zipCache + "/.zip_stamp").c_str());
			std::string curStamp;
			if (sf.is_open() && std::getline(sf, curStamp) && trimCopy(curStamp) == expectedStamp) {
				needExtract = !isDirectory(zipCache + "/content");
			}
		}

		std::string contentDir = zipCache + "/content";
		if (needExtract) {
			removePathRecursive(contentDir);
			ensureDirRecursiveImpl(contentDir);
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
			std::string cmd = "tar -xf " + shellQuote(full) + " -C " + shellQuote(contentDir) + " 2>&1";
#else
			std::string cmd = "unzip -o -q " + shellQuote(full) + " -d " + shellQuote(contentDir) + " 2>&1";
#endif
			std::string log;
			int code = 0;
			if (!runCommandCapture(cmd, log, code) || code != 0) {
				entry.loadError = "Failed to extract zip patch '" + name + "':\n" + trimCopy(log);
			} else {
				std::ofstream sf((zipCache + "/.zip_stamp").c_str());
				if (sf.is_open()) sf << expectedStamp;
			}
		}

		if (entry.loadError.empty()) {
			entry.rootDir = resolveNormalizedPatchRoot(contentDir);
		}
		found.push_back(entry);
	}

	for (PatchEntry& entry : found) {
		auto itEn = std::find(enabledOrder.begin(), enabledOrder.end(), entry.id);
		entry.enabled = (itEn != enabledOrder.end());

		if (!entry.loadError.empty()) continue;

		std::vector<std::string> rels;
		if (!collectFilesRecursive(entry.rootDir, "", rels)) {
			entry.loadError = "Unable to read patch directory '" + entry.rootDir + "': " + strerror(errno);
			continue;
		}

		for (const std::string& r : rels) {
			if (r == ".DS_Store" || r == "Thumbs.db") continue;
			if (r.find("..") != std::string::npos && (r == ".." || startsWithStr(r, "../") || r.find("/../") != std::string::npos || endsWithStr(r, "/.."))) {
				entry.loadError = "Unsafe relative path '..' in patch '" + entry.id + "': " + r;
				break;
			}
			entry.files.push_back(r);
			if (r == "patch.json") {
				std::vector<uint8_t> jbytes;
				if (!readFileBytes(entry.rootDir + "/" + r, jbytes)) {
					entry.loadError = "Failed to read patch.json in patch '" + entry.id + "'";
					break;
				}
				std::string jstr((const char*)jbytes.data(), jbytes.size());
				Json::Reader reader;
				Json::Value root;
				if (!reader.parse(jstr, root, false)) {
					entry.loadError = "Malformed patch.json in patch '" + entry.id + "':\n" + reader.getFormattedErrorMessages();
					break;
				}
				if (root.isMember("name") && root["name"].isString() && !root["name"].asString().empty()) {
					entry.displayName = root["name"].asString();
				}
				if (root.isMember("version") && root["version"].isString()) {
					entry.version = root["version"].asString();
				}
				if (root.isMember("author") && root["author"].isString()) {
					entry.author = root["author"].asString();
				}
				if (root.isMember("description") && root["description"].isString()) {
					entry.description = root["description"].asString();
				}
			} else if (isNativeLibRel(r)) {
				entry.hasNativeLib = true;
			} else if (isCodeFileRel(r)) {
				entry.hasCode = true;
			} else if (isAssetFileRel(r)) {
				entry.hasAssets = true;
			}
		}

		entry.fileCount = (int)entry.files.size();
		if (entry.loadError.empty() && entry.files.empty()) {
			entry.loadError = "Patch '" + entry.id + "' is empty (contains no files).";
		}
	}

	std::sort(found.begin(), found.end(), [&](const PatchEntry& a, const PatchEntry& b) {
		if (a.enabled != b.enabled) return a.enabled > b.enabled;
		if (a.enabled) {
			size_t ia = (size_t)(std::find(enabledOrder.begin(), enabledOrder.end(), a.id) - enabledOrder.begin());
			size_t ib = (size_t)(std::find(enabledOrder.begin(), enabledOrder.end(), b.id) - enabledOrder.begin());
			if (ia != ib) return ia < ib;
		}
		return a.displayName < b.displayName;
	});

	this->patches_ = std::move(found);
}

std::vector<PatchEntry>& PatchManager::getPatches() { return this->patches_; }
const std::vector<PatchEntry>& PatchManager::getPatches() const { return this->patches_; }

PatchEntry* PatchManager::findPatch(const std::string& id) {
	for (auto& p : this->patches_) {
		if (p.id == id) return &p;
	}
	return nullptr;
}

bool PatchManager::setPatchEnabled(const std::string& id, bool enabled) {
	std::lock_guard<std::mutex> lock(this->mutex_);
	PatchEntry* target = this->findPatch(id);
	if (!target) return false;
	target->enabled = enabled;
	std::stable_partition(this->patches_.begin(), this->patches_.end(), [](const PatchEntry& p) {
		return p.enabled;
	});
	return true;
}

bool PatchManager::movePatchPriority(const std::string& id, int delta) {
	std::lock_guard<std::mutex> lock(this->mutex_);
	for (size_t i = 0; i < this->patches_.size(); ++i) {
		if (this->patches_[i].id == id && this->patches_[i].enabled) {
			int dst = (int)i + delta;
			if (dst < 0 || dst >= (int)this->patches_.size()) return false;
			if (!this->patches_[dst].enabled) return false;
			std::swap(this->patches_[i], this->patches_[dst]);
			return true;
		}
	}
	return false;
}

bool PatchManager::renamePatch(const std::string& id, const std::string& newNameRaw, std::string& outErr) {
	std::string newName = trimCopy(newNameRaw);
	if (newName.empty()) {
		outErr = "Patch name cannot be empty.";
		return false;
	}
	for (char c : newName) {
		if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
			outErr = "Patch name contains invalid path characters.";
			return false;
		}
	}
	if (newName == "." || newName == ".." || newName[0] == '.') {
		outErr = "Patch name cannot start with a dot.";
		return false;
	}

	std::string dir = this->getPatchesDir();
	std::vector<std::string> enabledIds = this->getEnabledIdsSnapshot();

	PatchEntry* target = this->findPatch(id);
	if (!target) {
		outErr = "Patch '" + id + "' not found.";
		return false;
	}

	bool isZip = target->isZip;
	std::string oldSource = target->sourcePath;
	std::string oldRoot = target->rootDir;
	std::string newId = newName;
	std::string newSource = dir + "/" + (isZip ? (newId + ".zip") : newId);

	if (newId != id && (pathExists(newSource) || this->findPatch(newId) != nullptr)) {
		outErr = "A patch named '" + newId + "' already exists.";
		return false;
	}

	if (!isZip && isRegularFile(oldRoot + "/patch.json")) {
		std::vector<uint8_t> jbytes;
		if (readFileBytes(oldRoot + "/patch.json", jbytes)) {
			std::string jstr((const char*)jbytes.data(), jbytes.size());
			Json::Reader reader;
			Json::Value root;
			if (reader.parse(jstr, root, false) && root.isObject()) {
				root["name"] = newName;
				std::string serialized = root.toStyledString();
				writeFileIfDifferent(oldRoot + "/patch.json", (const uint8_t*)serialized.data(), serialized.size());
			}
		}
	}

	if (oldSource != newSource) {
		if (::rename(oldSource.c_str(), newSource.c_str()) != 0) {
			outErr = std::string("Failed to rename patch on disk: ") + strerror(errno);
			return false;
		}
		if (isZip) {
			removePathRecursive(dir + "/.zips/" + id);
		}
	}

	for (auto& eid : enabledIds) {
		if (eid == id) eid = newId;
	}
	{
		std::ofstream ef((dir + "/enabled.txt").c_str());
		if (ef.is_open()) {
			for (const auto& eid : enabledIds) {
				ef << eid << "\n";
			}
		}
	}

	this->scanPatches();
	return true;
}

bool PatchManager::deletePatch(const std::string& id, std::string& outErr) {
	PatchEntry* target = this->findPatch(id);
	if (!target) {
		outErr = "Patch '" + id + "' not found.";
		return false;
	}

	std::string dir = this->getPatchesDir();
	std::string srcPath = target->sourcePath;
	bool isZip = target->isZip;

	if (isZip) {
		if (::remove(srcPath.c_str()) != 0 && pathExists(srcPath)) {
			outErr = std::string("Failed to delete zip patch: ") + strerror(errno);
			return false;
		}
		removePathRecursive(dir + "/.zips/" + id);
	} else {
		removePathRecursive(srcPath);
		if (pathExists(srcPath)) {
			outErr = "Failed to delete patch folder '" + srcPath + "'.";
			return false;
		}
	}

	std::vector<std::string> enabledIds = this->getEnabledIdsSnapshot();
	enabledIds.erase(std::remove(enabledIds.begin(), enabledIds.end(), id), enabledIds.end());
	{
		std::ofstream ef((dir + "/enabled.txt").c_str());
		if (ef.is_open()) {
			for (const auto& eid : enabledIds) {
				ef << eid << "\n";
			}
		}
	}

	this->scanPatches();
	return true;
}

bool PatchManager::saveEnabledConfig() {
	std::lock_guard<std::mutex> lock(this->mutex_);
	std::string dir = this->getPatchesDir();
	std::ofstream ef((dir + "/enabled.txt").c_str());
	if (!ef.is_open()) return false;
	for (const auto& p : this->patches_) {
		if (p.enabled) ef << p.id << "\n";
	}
	return true;
}

std::vector<std::string> PatchManager::getEnabledIdsSnapshot() const {
	std::lock_guard<std::mutex> lock(this->mutex_);
	std::vector<std::string> out;
	for (const auto& p : this->patches_) {
		if (p.enabled) out.push_back(p.id);
	}
	return out;
}

void PatchManager::restoreEnabledIds(const std::vector<std::string>& ids) {
	std::lock_guard<std::mutex> lock(this->mutex_);
	for (auto& p : this->patches_) {
		p.enabled = (std::find(ids.begin(), ids.end(), p.id) != ids.end());
	}
	std::sort(this->patches_.begin(), this->patches_.end(), [&](const PatchEntry& a, const PatchEntry& b) {
		if (a.enabled != b.enabled) return a.enabled > b.enabled;
		if (a.enabled) {
			size_t ia = (size_t)(std::find(ids.begin(), ids.end(), a.id) - ids.begin());
			size_t ib = (size_t)(std::find(ids.begin(), ids.end(), b.id) - ids.begin());
			if (ia != ib) return ia < ib;
		}
		return a.displayName < b.displayName;
	});
}

int PatchManager::getEnabledCount() const {
	std::lock_guard<std::mutex> lock(this->mutex_);
	int c = 0;
	for (const auto& p : this->patches_) {
		if (p.enabled) ++c;
	}
	return c;
}

bool PatchManager::validateEnabledPatches(std::string& outErr) {
	std::vector<PatchEntry> enabledCopy;
	{
		std::lock_guard<std::mutex> lock(this->mutex_);
		for (const auto& p : this->patches_) {
			if (p.enabled) enabledCopy.push_back(p);
		}
	}

	for (const PatchEntry& p : enabledCopy) {
		if (!p.loadError.empty()) {
			outErr = p.loadError;
			return false;
		}
		if (!isDirectory(p.rootDir)) {
			outErr = "Patch directory missing for '" + p.displayName + "': " + p.rootDir;
			return false;
		}
		for (const std::string& rel : p.files) {
			std::string full = p.rootDir + "/" + rel;
			if (!isRegularFile(full)) {
				outErr = "Missing file '" + rel + "' inside patch '" + p.displayName + "'";
				return false;
			}
			if (isImageFileRel(rel)) {
				std::vector<uint8_t> bytes;
				if (!readFileBytes(full, bytes) || bytes.empty()) {
					outErr = "Failed to read image '" + rel + "' in patch '" + p.displayName + "'";
					return false;
				}
				int w = 0, h = 0, ch = 0;
				unsigned char* px = stbi_load_from_memory(bytes.data(), (int)bytes.size(), &w, &h, &ch, STBI_rgb_alpha);
				if (!px || w <= 0 || h <= 0) {
					if (px) stbi_image_free(px);
					const char* reason = stbi_failure_reason();
					outErr = "Failed to decode image '" + rel + "' in patch '" + p.displayName + "': " +
					         (reason ? reason : "corrupted or unsupported image data");
					return false;
				}
				stbi_image_free(px);
			} else if (isJsonMetaFileRel(rel)) {
				std::vector<uint8_t> bytes;
				if (!readFileBytes(full, bytes)) {
					outErr = "Failed to read metadata file '" + rel + "' in patch '" + p.displayName + "'";
					return false;
				}
				std::string txt((const char*)bytes.data(), bytes.size());
				Json::Reader reader;
				Json::Value val;
				if (!reader.parse(txt, val, false)) {
					outErr = "Failed to parse JSON/meta file '" + rel + "' in patch '" + p.displayName + "':\n" +
					         reader.getFormattedErrorMessages();
					return false;
				}
			}
		}
	}
	return true;
}

bool PatchManager::loadNativeLibrariesForActivePatches(std::string& outErr) {
	std::vector<PatchEntry> enabledCopy;
	{
		std::lock_guard<std::mutex> lock(this->mutex_);
		for (const auto& p : this->patches_) {
			if (p.enabled && p.hasNativeLib) enabledCopy.push_back(p);
		}
	}

	for (const PatchEntry& p : enabledCopy) {
		for (const std::string& rel : p.files) {
			if (!isNativeLibRel(rel)) continue;
			std::string full = p.rootDir + "/" + rel;
			{
				std::lock_guard<std::mutex> lock(this->mutex_);
				if (std::find(this->loadedNativeLibs_.begin(), this->loadedNativeLibs_.end(), full) != this->loadedNativeLibs_.end()) {
					continue;
				}
			}
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
			HMODULE mod = LoadLibraryA(full.c_str());
			if (!mod) {
				DWORD err = GetLastError();
				char msgBuf[512] = {0};
				FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, 0, msgBuf, sizeof(msgBuf) - 1, NULL);
				outErr = "Failed to load native patch library '" + rel + "' from patch '" + p.displayName + "': " + trimCopy(msgBuf);
				return false;
			}
			typedef const char* (*PatchInitFn)();
			PatchInitFn initFn = (PatchInitFn)GetProcAddress(mod, "mcpe_patch_init");
			if (!initFn) initFn = (PatchInitFn)GetProcAddress(mod, "patch_init");
			if (initFn) {
				const char* errStr = initFn();
				if (errStr && errStr[0]) {
					outErr = "Native patch library '" + rel + "' in patch '" + p.displayName + "' returned error:\n" + errStr;
					return false;
				}
			}
#else
			dlerror();
			void* handle = dlopen(full.c_str(), RTLD_NOW | RTLD_GLOBAL);
			if (!handle) {
				const char* err = dlerror();
				outErr = "Failed to load native patch library '" + rel + "' from patch '" + p.displayName + "':\n" + (err ? err : "unknown dlopen error");
				return false;
			}
			typedef const char* (*PatchInitFn)();
			PatchInitFn initFn = (PatchInitFn)dlsym(handle, "mcpe_patch_init");
			if (!initFn) initFn = (PatchInitFn)dlsym(handle, "patch_init");
			if (initFn) {
				const char* errStr = initFn();
				if (errStr && errStr[0]) {
					outErr = "Native patch library '" + rel + "' in patch '" + p.displayName + "' returned error:\n" + errStr;
					return false;
				}
			}
#endif
			std::lock_guard<std::mutex> lock(this->mutex_);
			this->loadedNativeLibs_.push_back(full);
		}
	}
	return true;
}

bool PatchManager::initializeOnStartup() {
	this->scanPatches();
	std::string err;
	if (!this->validateEnabledPatches(err)) {
		this->reportError(err);
		return false;
	}
	if (!this->loadNativeLibrariesForActivePatches(err)) {
		this->reportError(err);
		return false;
	}
#if !defined(_WIN32) && !defined(__WIN32__) && !defined(WIN32) && !defined(ANDROID) && !defined(MCPE_IOS) && !defined(__APPLE__)
	if (!this->isPatchedBinary()) {
		std::string wantedFp = this->computeWantedCodeFingerprint();
		if (!wantedFp.empty() && this->getActiveCodeFingerprint() != wantedFp) {
			std::string pdir = this->getPatchesDir();
			std::string dynSo = pdir + "/.build/plugins/m8_patches_" + wantedFp + ".so";
			if (isRegularFile(dynSo)) {
				if (!this->loadDynamicPatchPlugin(dynSo, wantedFp, err)) {
					remove(dynSo.c_str());
					this->reportError(err);
					return false;
				}
			}
		}
	}
#endif
	return true;
}

bool PatchManager::maybeRelaunchIntoPatchedOnBoot() {
#if defined(ANDROID) || defined(MCPE_IOS)
	return false;
#else
	if (this->relaunchFlag_) return false;
	this->scanPatches();
	std::string wantedFp = this->computeWantedCodeFingerprint();
	if (wantedFp.empty()) return false;
	if (this->getActiveCodeFingerprint() == wantedFp) return false;

	std::string pdir = this->getPatchesDir();
#if !defined(_WIN32) && !defined(__WIN32__) && !defined(WIN32) && !defined(__APPLE__)
	std::string dynSo = pdir + "/.build/plugins/m8_patches_" + wantedFp + ".so";
	if (!this->isPatchedBinary() && isRegularFile(dynSo)) {
		return false;
	}
#endif
	std::string stampFile = pdir + "/.build/built_fingerprint.txt";
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
	std::string patchedExe = pdir + "/.build/cmake_build/modifiedeight-newadditions/build/modifiedeight-newadditions.exe";
#else
	std::string patchedExe = pdir + "/.build/cmake_build/modifiedeight-newadditions/build/modifiedeight-newadditions";
#endif
	if (!isRegularFile(patchedExe)) return false;
	std::string selfExe = this->getExecutablePath();
	if (!selfExe.empty() && normalizeSlashes(selfExe) == normalizeSlashes(patchedExe)) return false;

	std::ifstream sf(stampFile.c_str());
	std::string builtFp;
	if (sf.is_open() && std::getline(sf, builtFp)) {
		builtFp = trimCopy(builtFp);
		if (!builtFp.empty() && builtFp != wantedFp) return false;
	}

	this->pendingRelaunchExe_ = patchedExe;
	return this->relaunchPendingBinary();
#endif
}

AssetFile PatchManager::readAssetFile(const std::string& path, std::string* outPatchId, std::string* outDiskPath) {
	std::vector<PatchEntry> snapshot;
	{
		std::lock_guard<std::mutex> lock(this->mutex_);
		for (const auto& p : this->patches_) {
			if (p.enabled && p.loadError.empty()) {
				snapshot.push_back(p);
			}
		}
	}
	if (snapshot.empty()) return AssetFile(0, -1);

	std::vector<std::string> cands = buildAssetCandidates(path);
	for (const PatchEntry& p : snapshot) {
		for (const std::string& rel : cands) {
			std::string full = p.rootDir + "/" + rel;
			if (!isRegularFile(full)) continue;
			std::vector<uint8_t> bytes;
			if (!readFileBytes(full, bytes)) continue;
			uint8_t* arr = new uint8_t[bytes.size()];
			if (!bytes.empty()) {
				memcpy(arr, bytes.data(), bytes.size());
			}
			if (outPatchId) *outPatchId = p.displayName;
			if (outDiskPath) *outDiskPath = full;
			return AssetFile(arr, (int32_t)bytes.size());
		}
	}
	return AssetFile(0, -1);
}

bool PatchManager::hasEnabledAssetOverrides() const {
	std::lock_guard<std::mutex> lock(this->mutex_);
	for (const auto& p : this->patches_) {
		if (p.enabled && p.hasAssets && p.loadError.empty()) return true;
	}
	return false;
}

std::vector<std::string> PatchManager::collectTextureNamesFromActivePatches(bool isTerrain) {
	std::vector<PatchEntry> snapshot;
	{
		std::lock_guard<std::mutex> lock(this->mutex_);
		for (const auto& p : this->patches_) {
			if (p.enabled && p.loadError.empty()) snapshot.push_back(p);
		}
	}

	std::vector<std::string> prefixes = isTerrain
		? std::vector<std::string>{"assets/textures/blocks/", "assets/images/textures/blocks/", "textures/blocks/", "images/textures/blocks/"}
		: std::vector<std::string>{"assets/textures/items/", "assets/images/textures/items/", "textures/items/", "images/textures/items/"};

	std::vector<std::string> out;
	std::set<std::string> seen;
	for (const PatchEntry& p : snapshot) {
		for (const std::string& rel : p.files) {
			if (getExtensionLower(rel) != ".png") continue;
			for (const std::string& pre : prefixes) {
				if (startsWithStr(rel, pre)) {
					std::string rest = rel.substr(pre.size());
					if (rest.find('/') == std::string::npos && rest.size() > 4) {
						std::string stem = rest.substr(0, rest.size() - 4);
						if (!stem.empty() && !seen.count(stem)) {
							seen.insert(stem);
							out.push_back(stem);
						}
					}
					break;
				}
			}
		}
	}
	return out;
}

bool PatchManager::applyRuntimeAssetReload(NinecraftApp* app) {
	if (!app) return true;
	this->clearError();

	std::string err;
	if (!this->validateEnabledPatches(err)) {
		this->reportError(err);
		return false;
	}
	if (!this->loadNativeLibrariesForActivePatches(err)) {
		this->reportError(err);
		return false;
	}

	AppPlatform::clearAssetCache();
	if (NinecraftApp::_terrainTextureAtlas) {
		NinecraftApp::_terrainTextureAtlas->load(app);
	}
	if (NinecraftApp::_itemsTextureAtlas) {
		NinecraftApp::_itemsTextureAtlas->load(app);
	}
	if (app->texturesPtr) {
		app->texturesPtr->reloadAll();
	}
	I18n::loadLanguage(app->platform(), "en_US");
	if (app->texturesPtr) {
		app->gui.texturesLoaded(app->texturesPtr);
	}
	if (app->levelRenderer) {
		app->levelRenderer->allChanged();
	}

	if (this->hasError()) {
		return false;
	}
	return true;
}

std::string PatchManager::computeWantedCodeFingerprint() const {
	std::lock_guard<std::mutex> lock(this->mutex_);
	std::vector<const PatchEntry*> codePatches;
	for (const auto& p : this->patches_) {
		if (!p.enabled) continue;
		if (p.hasCode) codePatches.push_back(&p);
	}
	if (codePatches.empty()) return "";

	std::ostringstream ss;
	for (const PatchEntry* p : codePatches) {
		ss << p->id << "|";
		for (const std::string& rel : p->files) {
			if (!isCodeFileRel(rel)) continue;
			std::string full = p->rootDir + "/" + rel;
			std::vector<uint8_t> bytes;
			uint64_t h = 0;
			if (readFileBytes(full, bytes)) {
				h = fnv1a64(bytes.data(), bytes.size());
			}
			ss << rel << ":" << bytes.size() << ":" << h << ";";
		}
	}
	std::string payload = ss.str();
	uint64_t digest1 = fnv1a64(payload.data(), payload.size(), 14695981039346656037ULL);
	uint64_t digest2 = fnv1a64(payload.data(), payload.size(), 1099511628211ULL);
	char hex[40];
	snprintf(hex, sizeof(hex), "%016llx%016llx", (unsigned long long)digest1, (unsigned long long)digest2);
	return std::string(hex);
}

bool PatchManager::needsCodeRebuildOrRelaunch() const {
	if (this->computeWantedCodeFingerprint() != this->getActiveCodeFingerprint()) {
		return true;
	}
	std::lock_guard<std::mutex> lock(this->mutex_);
	if (!this->loadedNativeLibs_.empty()) {
		std::set<std::string> wantedLibs;
		for (const auto& p : this->patches_) {
			if (!p.enabled || !p.hasNativeLib) continue;
			for (const std::string& rel : p.files) {
				if (isNativeLibRel(rel)) wantedLibs.insert(p.rootDir + "/" + rel);
			}
		}
		for (const auto& loaded : this->loadedNativeLibs_) {
			if (!wantedLibs.count(loaded)) return true;
		}
	}
	return false;
}

bool PatchManager::startApplyPatchesAsync(const std::vector<std::string>& previousEnabled) {
	if (this->buildRunning_.load()) return false;

	this->clearError();
	{
		std::lock_guard<std::mutex> lock(this->mutex_);
		this->pendingRollbackIds_ = previousEnabled;
		this->asyncDone_ = false;
		this->asyncSuccess_ = false;
		this->asyncWillRelaunch_ = false;
		this->asyncError_.clear();
		this->pendingRelaunchExe_.clear();
		this->pendingDynamicPluginPath_.clear();
		this->pendingDynamicPluginFp_.clear();
		this->buildStatus_ = "Validating enabled patches...";
	}
	this->buildRunning_ = true;

	std::thread([this]() {
		auto finishFail = [&](const std::string& errMsg) {
			this->restoreEnabledIds(this->pendingRollbackIds_);
			this->saveEnabledConfig();
			this->reportError(errMsg);
			std::lock_guard<std::mutex> lk(this->mutex_);
			this->asyncError_ = errMsg;
			this->asyncSuccess_ = false;
			this->asyncWillRelaunch_ = false;
			this->asyncDone_ = true;
			this->buildRunning_ = false;
		};

		std::string valErr;
		if (!this->validateEnabledPatches(valErr)) {
			finishFail(valErr);
			return;
		}

		std::string wantedFp = this->computeWantedCodeFingerprint();
		std::string activeFp = this->getActiveCodeFingerprint();

		if (wantedFp.empty() && (this->isPatchedBinary() || !activeFp.empty() || !this->loadedNativeLibs_.empty())) {
#if defined(ANDROID) || defined(MCPE_IOS)
			finishFail("Disabling native/code patches on this platform requires restarting the app.");
			return;
#else
			std::string baseExe = this->getBaseBinaryPath();
			if (baseExe.empty() || !isRegularFile(baseExe)) {
				finishFail("Could not locate base client executable to restore unpatched mode: " + baseExe);
				return;
			}
			this->saveEnabledConfig();
			std::lock_guard<std::mutex> lk(this->mutex_);
			this->pendingRelaunchExe_ = baseExe;
			this->asyncSuccess_ = true;
			this->asyncWillRelaunch_ = true;
			this->asyncDone_ = true;
			this->buildRunning_ = false;
			return;
#endif
		}

		if (wantedFp == activeFp && !this->needsCodeRebuildOrRelaunch()) {
			this->saveEnabledConfig();
			std::lock_guard<std::mutex> lk(this->mutex_);
			this->asyncSuccess_ = true;
			this->asyncWillRelaunch_ = false;
			this->asyncDone_ = true;
			this->buildRunning_ = false;
			return;
		}

#if defined(ANDROID) || defined(MCPE_IOS)
		finishFail("Source-level C++ code patches cannot be compiled on mobile runtime; compile natively or use texture/asset patches.");
		return;
#else
		std::string baseSourceRoot = this->findBaseSourceRoot();
		if (baseSourceRoot.empty()) {
			std::string extractErr;
			baseSourceRoot = this->extractEmbeddedSourceRoot(extractErr);
			if (baseSourceRoot.empty()) {
				std::string msg = "Unable to locate client source root (modifiedeight-newadditions/CMakeLists.txt) to compile code patches.";
				if (!extractErr.empty()) msg += "\n" + extractErr;
				finishFail(msg);
				return;
			}
		}
		std::string baseBuildRoot = this->findBaseBuildRoot();
		std::string pdir = makeAbsolutePath(this->getPatchesDir());
		std::string buildBase = pdir + "/.build";

#if !defined(_WIN32) && !defined(__WIN32__) && !defined(WIN32) && !defined(__APPLE__)
		if (this->areEnabledCodePatchesPureAdditive(baseSourceRoot)) {
			std::string pluginPath = buildBase + "/plugins/m8_patches_" + wantedFp + ".so";
			bool havePlugin = isRegularFile(pluginPath);
			std::string pluginErr;
			if (!havePlugin) {
				havePlugin = this->buildAdditiveDynamicPlugin(baseSourceRoot, wantedFp, pluginPath, pluginErr);
			}
			if (havePlugin) {
				bool canHotLoad = false;
				{
					std::lock_guard<std::mutex> lk(this->mutex_);
					canHotLoad = !this->isPatchedBinary() && this->activeDynamicPluginFp_.empty() && this->loadedNativeLibs_.empty();
				}
				if (canHotLoad) {
					this->saveEnabledConfig();
					std::lock_guard<std::mutex> lk(this->mutex_);
					this->pendingDynamicPluginPath_ = pluginPath;
					this->pendingDynamicPluginFp_ = wantedFp;
					this->asyncSuccess_ = true;
					this->asyncWillRelaunch_ = false;
					this->asyncDone_ = true;
					this->buildRunning_ = false;
					return;
				}
				std::string baseExe = this->getBaseBinaryPath();
				if (!baseExe.empty() && isRegularFile(baseExe)) {
					this->saveEnabledConfig();
					std::lock_guard<std::mutex> lk(this->mutex_);
					this->pendingRelaunchExe_ = baseExe;
					this->asyncSuccess_ = true;
					this->asyncWillRelaunch_ = true;
					this->asyncDone_ = true;
					this->buildRunning_ = false;
					return;
				}
			} else if (system("command -v cmake >/dev/null 2>&1") != 0) {
				finishFail(pluginErr);
				return;
			}
		}
#endif

		std::string wsRoot = buildBase + "/workspace";
		std::string wsClient = wsRoot + "/modifiedeight-newadditions";
		std::string cmakeBuildDir = buildBase + "/cmake_build";
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
		std::string outExe = cmakeBuildDir + "/modifiedeight-newadditions/build/modifiedeight-newadditions.exe";
#else
		std::string outExe = cmakeBuildDir + "/modifiedeight-newadditions/build/modifiedeight-newadditions";
#endif

		{
			std::ifstream bf((buildBase + "/built_fingerprint.txt").c_str());
			std::string builtFp;
			if (bf.is_open() && std::getline(bf, builtFp) && trimCopy(builtFp) == wantedFp && isRegularFile(outExe)) {
				this->saveEnabledConfig();
				std::lock_guard<std::mutex> lk(this->mutex_);
				this->pendingRelaunchExe_ = outExe;
				this->asyncSuccess_ = true;
				this->asyncWillRelaunch_ = true;
				this->asyncDone_ = true;
				this->buildRunning_ = false;
				return;
			}
		}

		{
			std::lock_guard<std::mutex> lk(this->mutex_);
			this->buildStatus_ = "Staging patched source workspace...";
		}

		ensureDirRecursiveImpl(wsRoot);
#if !defined(_WIN32) && !defined(__WIN32__) && !defined(WIN32)
		{
			struct stat lst;
			if (lstat(wsClient.c_str(), &lst) == 0 && S_ISLNK(lst.st_mode)) {
				unlink(wsClient.c_str());
			}
		}
#endif
		ensureDirRecursiveImpl(wsClient);
		ensureDirRecursiveImpl(cmakeBuildDir);

		const char* siblingEntries[] = {"CMakeLists.txt", "stb", "glm", "OpenAES", "RakNet", "jsoncpp", "utf8proc", "platforms"};
		for (const char* item : siblingEntries) {
			std::string srcItem = baseSourceRoot + "/" + item;
			std::string dstItem = wsRoot + "/" + item;
			if (!pathExists(srcItem)) continue;
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
			if (!pathExists(dstItem)) {
				std::string cmd = isDirectory(srcItem)
					? ("mklink /J " + shellQuote(dstItem) + " " + shellQuote(srcItem) + " >nul 2>&1 || xcopy /E /I /Q /Y " + shellQuote(srcItem) + " " + shellQuote(dstItem) + " >nul")
					: ("copy /Y " + shellQuote(srcItem) + " " + shellQuote(dstItem) + " >nul");
				(void)system(cmd.c_str());
			}
#else
			struct stat lst;
			if (lstat(dstItem.c_str(), &lst) == 0) {
				char linkBuf[2048];
				ssize_t n = S_ISLNK(lst.st_mode) ? readlink(dstItem.c_str(), linkBuf, sizeof(linkBuf) - 1) : -1;
				if (n > 0) {
					linkBuf[n] = '\0';
					if (std::string(linkBuf) == srcItem) continue;
				}
				removePathRecursive(dstItem);
			}
			if (symlink(srcItem.c_str(), dstItem.c_str()) != 0) {
				finishFail("Failed to symlink '" + srcItem + "' to '" + dstItem + "': " + strerror(errno));
				return;
			}
#endif
		}

		{
			std::string srcAssets;
			std::vector<std::string> assetCands;
			assetCands.push_back(baseSourceRoot + "/modifiedeight-newadditions/assets");
			std::string baseBinDir = parentDir(this->getBaseBinaryPath());
			if (!baseBinDir.empty()) assetCands.push_back(baseBinDir + "/assets");
			std::string pdirParent = parentDir(pdir);
			if (!pdirParent.empty()) assetCands.push_back(pdirParent + "/assets");
			std::string selfDir = parentDir(this->getExecutablePath());
			if (!selfDir.empty()) assetCands.push_back(selfDir + "/assets");
			assetCands.push_back(makeAbsolutePath("assets"));
			for (const std::string& cand : assetCands) {
				std::string absCand = makeAbsolutePath(cand);
				if (!absCand.empty() && isDirectory(absCand)) {
					srcAssets = absCand;
					break;
				}
			}
			std::string dstAssets = wsClient + "/assets";
			if (!srcAssets.empty() && pathExists(srcAssets)) {
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
				if (!pathExists(dstAssets)) {
					std::string cmd = "mklink /J " + shellQuote(dstAssets) + " " + shellQuote(srcAssets) + " >nul 2>&1 || xcopy /E /I /Q /Y " + shellQuote(srcAssets) + " " + shellQuote(dstAssets) + " >nul";
					(void)system(cmd.c_str());
				}
#else
				struct stat lst;
				bool linkMatches = false;
				if (lstat(dstAssets.c_str(), &lst) == 0) {
					char linkBuf[2048];
					ssize_t n = S_ISLNK(lst.st_mode) ? readlink(dstAssets.c_str(), linkBuf, sizeof(linkBuf) - 1) : -1;
					if (n > 0) {
						linkBuf[n] = '\0';
						if (std::string(linkBuf) == srcAssets) linkMatches = true;
					}
					if (!linkMatches) removePathRecursive(dstAssets);
				}
				if (!linkMatches && symlink(srcAssets.c_str(), dstAssets.c_str()) != 0) {
					finishFail("Failed to symlink '" + srcAssets + "' to '" + dstAssets + "': " + strerror(errno));
					return;
				}
#endif
			}
		}

		std::vector<std::string> baseFiles;
		if (!collectFilesRecursive(baseSourceRoot + "/modifiedeight-newadditions", "", baseFiles, true)) {
			finishFail("Failed to scan base client sources in: " + baseSourceRoot + "/modifiedeight-newadditions");
			return;
		}
		std::map<std::string, std::string> expectedFiles;
		for (const std::string& rel : baseFiles) {
			expectedFiles[rel] = baseSourceRoot + "/modifiedeight-newadditions/" + rel;
		}

		std::vector<PatchEntry> enabledCodePatches;
		{
			std::lock_guard<std::mutex> lk(this->mutex_);
			for (const auto& p : this->patches_) {
				if (p.enabled && p.hasCode) enabledCodePatches.push_back(p);
			}
		}
		std::ostringstream patchNamesSS;
		for (size_t i = 0; i < enabledCodePatches.size(); ++i) {
			if (i > 0) patchNamesSS << ", ";
			patchNamesSS << enabledCodePatches[i].displayName;
		}
		for (auto it = enabledCodePatches.rbegin(); it != enabledCodePatches.rend(); ++it) {
			for (const std::string& rel : it->files) {
				if (!isCodeFileRel(rel)) continue;
				expectedFiles[rel] = it->rootDir + "/" + rel;
			}
		}

		std::vector<std::string> existingStaged;
		collectFilesRecursive(wsClient, "", existingStaged, true);
		for (const std::string& rel : existingStaged) {
			if (rel == "headers/patch/PatchedBuildStamp.hpp") continue;
			if (!expectedFiles.count(rel)) {
				remove((wsClient + "/" + rel).c_str());
			}
		}

		std::ostringstream cmakeSourcesSig;
		for (const auto& kv : expectedFiles) {
			const std::string& rel = kv.first;
			const std::string& src = kv.second;
			if (rel == "headers/patch/PatchedBuildStamp.hpp") continue;
			std::string dst = wsClient + "/" + rel;
			std::string cpErr;
			if (!copyFileIfDifferent(src, dst, cpErr)) {
				finishFail(cpErr);
				return;
			}
			std::string ext = getExtensionLower(rel);
			if (rel == "CMakeLists.txt" || ext == ".cpp" || ext == ".c" || ext == ".cc" || ext == ".cxx" || ext == ".S" || ext == ".s") {
				cmakeSourcesSig << rel << ";";
				if (rel == "CMakeLists.txt") {
					std::vector<uint8_t> cm;
					readFileBytes(src, cm);
					cmakeSourcesSig << fnv1a64(cm.data(), cm.size()) << ";";
				}
			}
		}

		{
			std::string stampHeader = "#pragma once\n#define MCPE_ACTIVE_CODE_FINGERPRINT \"" + wantedFp + "\"\n";
			if (!writeFileIfDifferent(wsClient + "/headers/patch/PatchedBuildStamp.hpp",
			                          (const uint8_t*)stampHeader.data(), stampHeader.size())) {
				finishFail("Failed to write PatchedBuildStamp.hpp into staged workspace.");
				return;
			}
		}

		std::string sigStr = cmakeSourcesSig.str();
		std::string oldSig;
		{
			std::ifstream sf((cmakeBuildDir + "/staged_sources.sig").c_str());
			if (sf.is_open()) std::getline(sf, oldSig);
		}
		bool needConfigure = !isRegularFile(cmakeBuildDir + "/CMakeCache.txt") || (oldSig != sigStr);

		if (needConfigure) {
			{
				std::lock_guard<std::mutex> lk(this->mutex_);
				this->buildStatus_ = "Configuring CMake for code patches...";
			}
			std::ostringstream cfgCmd;
			cfgCmd << "cmake -S " << shellQuote(wsRoot)
			       << " -B " << shellQuote(cmakeBuildDir)
			       << " -DCMAKE_BUILD_TYPE=Release"
			       << " -DCMAKE_POLICY_VERSION_MINIMUM=3.5"
			       << " -DM8_PATCHED_ONLY=ON"
			       << " -DMCPE_PATCHED_BUILD=1"
			       << " -DMCPE_BASE_SOURCE_ROOT=" << shellQuote(baseSourceRoot);
			if (!isRegularFile("/usr/bin/gmake") && isRegularFile("/usr/bin/make")) {
				cfgCmd << " -DCMAKE_MAKE_PROGRAM=/usr/bin/make";
			}
			if (!isRegularFile("/usr/include/SDL/SDL.h") && isRegularFile("/run/host/usr/include/SDL/SDL.h")) {
				cfgCmd << " -DSDL_INCLUDE_DIR=/run/host/usr/include/SDL"
				       << " -DSDL_LIBRARY=/run/host/usr/lib64/libSDL.so";
			}
			if (!baseBuildRoot.empty()) {
				cfgCmd << " -DMCPE_BASE_BUILD_ROOT=" << shellQuote(baseBuildRoot);
			}
			cfgCmd << " 2>&1";

			std::string cfgLog;
			int cfgCode = 0;
			runCommandCapture(cfgCmd.str(), cfgLog, cfgCode, &this->mutex_, &this->buildStatus_);
			if (cfgCode != 0) {
				std::string cleanLog = PatchManager::stripAnsiCodes(trimCopy(cfgLog));
				finishFail("CMake configure failed while loading patch(es): " + patchNamesSS.str() + "\n\n" + cleanLog);
				return;
			}
			std::ofstream sf((cmakeBuildDir + "/staged_sources.sig").c_str());
			if (sf.is_open()) sf << sigStr;
		}

		{
			std::lock_guard<std::mutex> lk(this->mutex_);
			this->buildStatus_ = "Compiling patched client (" + patchNamesSS.str() + ")...";
		}
		unsigned int cores = std::thread::hardware_concurrency();
		if (cores < 1) cores = 2;
		std::ostringstream bldCmd;
		bldCmd << "cmake --build " << shellQuote(cmakeBuildDir)
		       << " --target modifiedeight-newadditions -j " << cores << " 2>&1";

		std::string bldLog;
		int bldCode = 0;
		runCommandCapture(bldCmd.str(), bldLog, bldCode, &this->mutex_, &this->buildStatus_);
		if (bldCode != 0) {
			std::string cleanLog = PatchManager::stripAnsiCodes(trimCopy(bldLog));
			finishFail("Failed to compile patch(es): " + patchNamesSS.str() + "\n\n" + cleanLog);
			return;
		}

		if (!isRegularFile(outExe)) {
			finishFail("Patched build finished but output binary was not found at:\n" + outExe);
			return;
		}

		{
			std::ofstream bf((buildBase + "/built_fingerprint.txt").c_str());
			if (bf.is_open()) bf << wantedFp << "\n";
		}

		this->saveEnabledConfig();
		std::lock_guard<std::mutex> lk(this->mutex_);
		this->pendingRelaunchExe_ = outExe;
		this->asyncSuccess_ = true;
		this->asyncWillRelaunch_ = true;
		this->asyncDone_ = true;
		this->buildRunning_ = false;
#endif
	}).detach();

	return true;
}

bool PatchManager::isBuildRunning() const {
	return this->buildRunning_.load();
}

std::string PatchManager::getBuildStatus() const {
	std::lock_guard<std::mutex> lock(this->mutex_);
	return this->buildStatus_;
}

bool PatchManager::pollAsyncResult(bool& outSuccess, bool& outWillRelaunch, std::string& outError) {
	if (!this->asyncDone_.load()) return false;
	std::string dynPath;
	std::string dynFp;
	std::vector<std::string> rollbackIds;
	{
		std::lock_guard<std::mutex> lock(this->mutex_);
		outSuccess = this->asyncSuccess_;
		outWillRelaunch = this->asyncWillRelaunch_;
		outError = this->asyncError_;
		dynPath = this->pendingDynamicPluginPath_;
		dynFp = this->pendingDynamicPluginFp_;
		rollbackIds = this->pendingRollbackIds_;
		this->pendingDynamicPluginPath_.clear();
		this->pendingDynamicPluginFp_.clear();
		this->asyncDone_ = false;
	}
	if (outSuccess && !outWillRelaunch && !dynPath.empty()) {
		std::string loadErr;
		if (!this->loadDynamicPatchPlugin(dynPath, dynFp, loadErr)) {
			remove(dynPath.c_str());
			this->restoreEnabledIds(rollbackIds);
			this->saveEnabledConfig();
			this->reportError(loadErr);
			outSuccess = false;
			outError = loadErr;
		}
	}
	return true;
}

bool PatchManager::relaunchPendingBinary() {
	std::string targetExe;
	std::string baseExe = this->getBaseBinaryPath();
	{
		std::lock_guard<std::mutex> lock(this->mutex_);
		targetExe = this->pendingRelaunchExe_;
	}
	if (targetExe.empty() || !isRegularFile(targetExe)) {
		this->reportError("Cannot relaunch: patched executable missing at '" + targetExe + "'");
		return false;
	}
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
	const char* args[5];
	int idx = 0;
	args[idx++] = targetExe.c_str();
	std::string baseCopy = baseExe;
	if (!baseCopy.empty()) {
		args[idx++] = "--m8-base-bin";
		args[idx++] = baseCopy.c_str();
	}
	if (targetExe != baseCopy) {
		args[idx++] = "--m8-patched";
	}
	args[idx] = nullptr;
	intptr_t res = _spawnv(_P_NOWAIT, targetExe.c_str(), args);
	if (res == -1) {
		this->reportError("Failed to launch binary '" + targetExe + "': " + strerror(errno));
		return false;
	}
	exit(0);
	return true;
#elif !defined(ANDROID) && !defined(MCPE_IOS)
	std::vector<std::string> argStrs;
	argStrs.push_back(targetExe);
	if (!baseExe.empty()) {
		argStrs.push_back("--m8-base-bin");
		argStrs.push_back(baseExe);
	}
	if (targetExe != baseExe) {
		argStrs.push_back("--m8-patched");
	}
	std::vector<char*> argv;
	for (auto& s : argStrs) argv.push_back(&s[0]);
	argv.push_back(nullptr);
	execv(targetExe.c_str(), argv.data());
	this->reportError("Failed to exec binary '" + targetExe + "': " + strerror(errno));
	return false;
#else
	return false;
#endif
}

void PatchManager::reportError(const std::string& msg) {
	std::lock_guard<std::mutex> lock(this->mutex_);
	this->lastError_ = msg;
	std::string dir = this->getPatchesDir();
	std::ofstream ef((dir + "/last_error.txt").c_str());
	if (ef.is_open()) ef << msg << "\n";
	fprintf(stderr, "[PatchManager ERROR] %s\n", msg.c_str());
}

void PatchManager::clearError() {
	std::lock_guard<std::mutex> lock(this->mutex_);
	this->lastError_.clear();
}

bool PatchManager::hasError() const {
	std::lock_guard<std::mutex> lock(this->mutex_);
	return !this->lastError_.empty();
}

std::string PatchManager::getLastError() const {
	std::lock_guard<std::mutex> lock(this->mutex_);
	return this->lastError_;
}

std::string PatchManager::stripAnsiCodes(const std::string& s) {
	std::string out;
	out.reserve(s.size());
	for (size_t i = 0; i < s.size(); ++i) {
		if ((unsigned char)s[i] == 0x1B && i + 1 < s.size() && s[i + 1] == '[') {
			size_t j = i + 2;
			while (j < s.size() && (unsigned char)s[j] < 0x40) ++j;
			if (j < s.size()) i = j;
			continue;
		}
		if (s[i] != '\r') out.push_back(s[i]);
	}
	return out;
}

bool PatchManager::copyTextToClipboard(const std::string& text) {
	if (text.empty()) return false;
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
	if (OpenClipboard(NULL)) {
		EmptyClipboard();
		HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
		if (hg) {
			void* ptr = GlobalLock(hg);
			if (ptr) {
				memcpy(ptr, text.c_str(), text.size() + 1);
				GlobalUnlock(hg);
				SetClipboardData(CF_TEXT, hg);
			} else {
				GlobalFree(hg);
			}
		}
		CloseClipboard();
		return true;
	}
	return false;
#elif defined(__APPLE__) && !defined(MCPE_IOS)
	FILE* p = popen("pbcopy", "w");
	if (p) {
		fwrite(text.data(), 1, text.size(), p);
		return pclose(p) == 0;
	}
	return false;
#elif !defined(ANDROID) && !defined(MCPE_IOS)
	bool copied = false;
	typedef int (*SDL_SetClipboardTextFn)(const char*);
	void* h2 = dlopen("libSDL2-2.0.so.0", RTLD_LAZY);
	void* sym = h2 ? dlsym(h2, "SDL_SetClipboardText") : nullptr;
	if (!sym) {
		void* hSelf = dlopen(nullptr, RTLD_LAZY);
		if (hSelf) sym = dlsym(hSelf, "SDL_SetClipboardText");
	}
	if (sym) {
		SDL_SetClipboardTextFn fn = (SDL_SetClipboardTextFn)sym;
		if (fn(text.c_str()) == 0) {
			copied = true;
		}
	}

	const char* tools[] = {"wl-copy", "xclip -selection clipboard", "xsel --clipboard --input"};
	const char* probes[] = {"command -v wl-copy >/dev/null 2>&1", "command -v xclip >/dev/null 2>&1", "command -v xsel >/dev/null 2>&1"};
	for (int i = 0; i < 3; ++i) {
		if (system(probes[i]) == 0) {
			FILE* p = popen(tools[i], "w");
			if (p) {
				fwrite(text.data(), 1, text.size(), p);
				if (pclose(p) == 0) copied = true;
			}
		}
	}
	return copied;
#else
	return false;
#endif
}

void PatchManager::openPatchesFolderInOS() {
	std::string dir = PatchManager::instance().getPatchesDir();
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
	std::string cmd = "explorer " + shellQuote(dir);
	(void)system(cmd.c_str());
#elif defined(__APPLE__) && !defined(MCPE_IOS)
	std::string cmd = "open " + shellQuote(dir) + " >/dev/null 2>&1 &";
	(void)system(cmd.c_str());
#elif !defined(ANDROID) && !defined(MCPE_IOS)
	std::string cmd = "xdg-open " + shellQuote(dir) + " >/dev/null 2>&1 &";
	(void)system(cmd.c_str());
#endif
}

namespace {
std::vector<PatchHudRenderFn>& getHudCallbacks() {
	static std::vector<PatchHudRenderFn> cbs;
	return cbs;
}
std::vector<PatchOptionsPopulateFn>& getOptionsCallbacks() {
	static std::vector<PatchOptionsPopulateFn> cbs;
	return cbs;
}
}

void PatchManager::registerHudRenderCallback(PatchHudRenderFn fn) {
	if (!fn) return;
	auto& cbs = getHudCallbacks();
	if (std::find(cbs.begin(), cbs.end(), fn) == cbs.end()) {
		cbs.push_back(fn);
	}
}

void PatchManager::runHudRenderCallbacks(Minecraft* mc, Gui* gui) {
	auto& cbs = getHudCallbacks();
	for (size_t i = 0; i < cbs.size(); ++i) {
		if (cbs[i]) cbs[i](mc, gui);
	}
}

void PatchManager::registerPatchOptionsCallback(PatchOptionsPopulateFn fn) {
	if (!fn) return;
	auto& cbs = getOptionsCallbacks();
	if (std::find(cbs.begin(), cbs.end(), fn) == cbs.end()) {
		cbs.push_back(fn);
	}
}

void PatchManager::runPatchOptionsCallbacks(OptionsPane* pane, Minecraft* mc) {
	auto& cbs = getOptionsCallbacks();
	for (size_t i = 0; i < cbs.size(); ++i) {
		if (cbs[i]) cbs[i](pane, mc);
	}
}
