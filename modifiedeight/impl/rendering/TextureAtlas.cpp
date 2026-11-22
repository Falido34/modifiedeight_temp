#include <rendering/TextureAtlas.hpp>
#include <NinecraftApp.hpp>
#include <_AssetFile.hpp>
#include <patch/PatchManager.hpp>
#include <json/reader.h>
#include <cstdio>
#include <rendering/TextureAtlasTextureItem.hpp>
#include <stb_image.h>
#include <stdlib.h>
#include <string.h>

extern uint8_t* g_terrainAtlasPixels;
extern int g_terrainAtlasWidth;
extern int g_terrainAtlasHeight;
extern uint8_t* g_itemsAtlasPixels;
extern int g_itemsAtlasWidth;
extern int g_itemsAtlasHeight;

uint8_t* g_terrainAtlasPixels = nullptr;
int g_terrainAtlasWidth = 1024;
int g_terrainAtlasHeight = 1024;
uint8_t* g_itemsAtlasPixels = nullptr;
int g_itemsAtlasWidth = 512;
int g_itemsAtlasHeight = 512;

static uint8_t* load_and_resize_png(struct NinecraftApp* mc, const std::string& path, int target_w, int target_h) {
	int w, h, channels;
	uint8_t* pixels = nullptr;
	std::string patchId;
	AssetFile patchFile = PatchManager::instance().readAssetFile(path, &patchId, nullptr);
	AssetFile file = (patchFile.bytes && patchFile.length > 0) ? patchFile : mc->platform()->readAssetFile(path);
	if (file.bytes && file.length > 0) {
		pixels = stbi_load_from_memory(file.bytes, file.length, &w, &h, &channels, STBI_rgb_alpha);
		delete[] file.bytes;
		if (!pixels && !patchId.empty()) {
			const char* reason = stbi_failure_reason();
			PatchManager::instance().reportError(
				"Failed to decode texture '" + path + "' in patch '" + patchId + "': " +
				(reason ? reason : "corrupt PNG/image data"));
			return nullptr;
		}
	}
	if (!pixels) {
		return nullptr;
	}
	if (w == target_w && h == target_h) {
		return pixels;
	}
	uint8_t* resized = (uint8_t*)malloc(target_w * target_h * 4);
	for (int y = 0; y < target_h; ++y) {
		for (int x = 0; x < target_w; ++x) {
			int src_x = x * w / target_w;
			int src_y = y * h / target_h;
			int dst_idx = (y * target_w + x) * 4;
			int src_idx = (src_y * w + src_x) * 4;
			resized[dst_idx + 0] = pixels[src_idx + 0];
			resized[dst_idx + 1] = pixels[src_idx + 1];
			resized[dst_idx + 2] = pixels[src_idx + 2];
			resized[dst_idx + 3] = pixels[src_idx + 3];
		}
	}
	stbi_image_free(pixels);
	return resized;
}

static uint8_t* slice_sub_image(const uint8_t* src, int src_w, int src_h, float u0, float v0, float u1, float v1, int target_w, int target_h) {
	if (!src || src_w <= 0 || src_h <= 0) return nullptr;
	int sx0 = (int)(u0 * src_w + 0.5f);
	int sy0 = (int)(v0 * src_h + 0.5f);
	int sx1 = (int)(u1 * src_w + 0.5f);
	int sy1 = (int)(v1 * src_h + 0.5f);
	int sw = sx1 - sx0;
	int sh = sy1 - sy0;
	if (sw <= 0 || sh <= 0) return nullptr;
	if (sx0 < 0) sx0 = 0;
	if (sy0 < 0) sy0 = 0;
	if (sx0 + sw > src_w) sw = src_w - sx0;
	if (sy0 + sh > src_h) sh = src_h - sy0;

	uint8_t* resized = (uint8_t*)malloc(target_w * target_h * 4);
	for (int y = 0; y < target_h; ++y) {
		for (int x = 0; x < target_w; ++x) {
			int src_x = sx0 + (x * sw / target_w);
			int src_y = sy0 + (y * sh / target_h);
			if (src_x >= src_w) src_x = src_w - 1;
			if (src_y >= src_h) src_y = src_h - 1;
			int dst_idx = (y * target_w + x) * 4;
			int src_idx = (src_y * src_w + src_x) * 4;
			resized[dst_idx + 0] = src[src_idx + 0];
			resized[dst_idx + 1] = src[src_idx + 1];
			resized[dst_idx + 2] = src[src_idx + 2];
			resized[dst_idx + 3] = src[src_idx + 3];
		}
	}
	return resized;
}

static void paste_texture(uint8_t* atlas_pixels, int atlas_w, int atlas_h, const uint8_t* tex_pixels, int grid_x, int grid_y, int tex_w, int tex_h) {
	if (!atlas_pixels || !tex_pixels) return;
	for (int y = 0; y < tex_h; ++y) {
		for (int x = 0; x < tex_w; ++x) {
			int dst_x = grid_x + x;
			int dst_y = grid_y + y;
			if (dst_x >= 0 && dst_x < atlas_w && dst_y >= 0 && dst_y < atlas_h) {
				int dst_idx = (dst_y * atlas_w + dst_x) * 4;
				int src_idx = (y * tex_w + x) * 4;
				atlas_pixels[dst_idx + 0] = tex_pixels[src_idx + 0];
				atlas_pixels[dst_idx + 1] = tex_pixels[src_idx + 1];
				atlas_pixels[dst_idx + 2] = tex_pixels[src_idx + 2];
				atlas_pixels[dst_idx + 3] = tex_pixels[src_idx + 3];
			}
		}
	}
}

TextureAtlas::TextureAtlas(const std::string& s){
	this->path = s;
}

TextureUVCoordinateSet TextureAtlas::_parseJSON(const Json::Value& v){
	float v17 = (v[2].asFloat() - v[0].asFloat()) * 0.002f;
	float v18 = (v[3].asFloat() - v[1].asFloat()) * 0.002f;

	TextureUVCoordinateSet t;
	t.width = v[4].asFloat();
	t.height = v[5].asFloat();
	t.minX = v[0].asFloat() + v17;
	t.maxX = v[2].asFloat() - v17;
	t.minY = v[1].asFloat() + v18;
	t.maxY = v[3].asFloat() - v18;
	return t;
}

TextureAtlasTextureItem* TextureAtlas::getTextureItem(const std::string& s){
	return &this->field_4[s];
}

void TextureAtlas::load(struct NinecraftApp* mc) {
	this->field_4.clear();
	bool is_terrain = (this->path.find("terrain.meta") != std::string::npos);

	std::string atlasPatchId;
	const char* primaryAtlas = is_terrain ? "images/terrain-atlas.tga" : "images/items-opaque.png";
	AssetFile f = PatchManager::instance().readAssetFile(primaryAtlas, &atlasPatchId, nullptr);
	if (!f.bytes) {
		f = mc->platform()->readAssetFile(primaryAtlas);
	}
	if (!f.bytes) {
		const char* alt1 = is_terrain ? "terrain-atlas.tga" : "items-opaque.png";
		atlasPatchId.clear();
		f = PatchManager::instance().readAssetFile(alt1, &atlasPatchId, nullptr);
		if (!f.bytes) f = mc->platform()->readAssetFile(alt1);
	}
	if (!f.bytes) {
		const char* alt2 = is_terrain ? "images/terrain.png" : "images/gui/items.png";
		atlasPatchId.clear();
		f = PatchManager::instance().readAssetFile(alt2, &atlasPatchId, nullptr);
		if (!f.bytes) f = mc->platform()->readAssetFile(alt2);
	}
	if (!f.bytes) {
		const char* alt3 = is_terrain ? "terrain.png" : "gui/items.png";
		atlasPatchId.clear();
		f = PatchManager::instance().readAssetFile(alt3, &atlasPatchId, nullptr);
		if (!f.bytes) f = mc->platform()->readAssetFile(alt3);
	}

	uint8_t* px = nullptr;
	int pack_w = 0, pack_h = 0, ch = 0;
	if (f.bytes && f.length > 0) {
		px = stbi_load_from_memory(f.bytes, f.length, &pack_w, &pack_h, &ch, STBI_rgb_alpha);
		delete[] f.bytes;
		if (!px && !atlasPatchId.empty()) {
			const char* reason = stbi_failure_reason();
			PatchManager::instance().reportError(
				"Failed to decode atlas '" + std::string(primaryAtlas) + "' from patch '" + atlasPatchId + "': " +
				(reason ? reason : "corrupt image"));
		}
	}

	Json::Reader reader;
	Json::Value root;
	std::string metaPatchId;
	AssetFile file = PatchManager::instance().readAssetFile(this->path, &metaPatchId, nullptr);
	if (!file.bytes || file.length <= 0) {
		file = mc->platform()->readAssetFile(this->path);
	}
	std::string data;
	if (file.bytes && file.length > 0) {
		data.assign((char_t*)file.bytes, file.length);
		delete[] file.bytes;
	}

	bool metaParsed = !data.empty() && reader.parse(data, root, 1);
	if (!metaParsed && !metaPatchId.empty()) {
		PatchManager::instance().reportError(
			"Failed to parse metadata '" + this->path + "' in patch '" + metaPatchId + "':\n" +
			reader.getFormattedErrorMessages());
	}

	std::vector<std::string> patchedExtraNames = PatchManager::instance().collectTextureNamesFromActivePatches(is_terrain);

	int estimatedSlots = 128 + (int)patchedExtraNames.size();
	if (metaParsed && root.isArray()) {
		estimatedSlots += (int)root.size() * 3;
	}

	int atlas_w = is_terrain ? (pack_w > 1024 ? pack_w : 1024) : (pack_w > 512 ? pack_w : 512);
	int atlas_base_w = px ? pack_w : (is_terrain ? 512 : 256);
	int atlas_base_h = px ? pack_h : (is_terrain ? 256 : 256);
	int slots_per_row = atlas_w / 16;
	int needed_extra_rows = (estimatedSlots + slots_per_row - 1) / slots_per_row;
	int min_needed_h = atlas_base_h + needed_extra_rows * 16;
	int atlas_h = is_terrain ? (pack_h > 512 ? pack_h * 2 : 1024) : (pack_h > 512 ? pack_h : 512);
	while (atlas_h < min_needed_h) {
		atlas_h *= 2;
	}

	uint8_t* atlas_pixels = (uint8_t*)malloc(atlas_w * atlas_h * 4);
	memset(atlas_pixels, 0, atlas_w * atlas_h * 4);

	if (px) {
		for (int y = 0; y < pack_h && y < atlas_h; ++y) {
			int copy_w = (pack_w < atlas_w) ? pack_w : atlas_w;
			memcpy(&atlas_pixels[y * atlas_w * 4], &px[y * pack_w * 4], copy_w * 4);
		}
		stbi_image_free(px);
	}

	if (is_terrain) {
		if (g_terrainAtlasPixels) free(g_terrainAtlasPixels);
		g_terrainAtlasPixels = atlas_pixels;
		g_terrainAtlasWidth = atlas_w;
		g_terrainAtlasHeight = atlas_h;
	} else {
		if (g_itemsAtlasPixels) free(g_itemsAtlasPixels);
		g_itemsAtlasPixels = atlas_pixels;
		g_itemsAtlasWidth = atlas_w;
		g_itemsAtlasHeight = atlas_h;
	}

	if (metaParsed) {
		int slot_idx = 0;
		auto tryPaintBaseCell = [&](const Json::Value& uvArr, const uint8_t* cellPx) {
			if (!cellPx || !uvArr.isArray() || uvArr.size() < 4 || uvArr[2].asFloat() <= 0.0001f) return;
			float iw = (uvArr.size() >= 5 && uvArr[4].asFloat() > 0.0f) ? uvArr[4].asFloat() : (float)atlas_base_w;
			float ih = (uvArr.size() >= 6 && uvArr[5].asFloat() > 0.0f) ? uvArr[5].asFloat() : (float)atlas_base_h;
			int sx = (int)(uvArr[0].asFloat() * iw + 0.5f);
			int sy = (int)(uvArr[1].asFloat() * ih + 0.5f);
			int sw = (int)((uvArr[2].asFloat() - uvArr[0].asFloat()) * iw + 0.5f);
			int sh = (int)((uvArr[3].asFloat() - uvArr[1].asFloat()) * ih + 0.5f);
			if (sw == 16 && sh == 16 && sx >= 0 && sy >= 0 && sx + 16 <= atlas_w && sy + 16 <= atlas_base_h) {
				paste_texture(atlas_pixels, atlas_w, atlas_h, cellPx, sx, sy, 16, 16);
			}
		};

		Json::Value::iterator v27 = root.begin();
		Json::Value::iterator v28 = root.end();

		while (v27 != v28) {
			Json::Value v32(*v27);
			std::string name = v32["name"].asString();
			std::vector<TextureUVCoordinateSet> v29;

			TextureUVCoordinateSet main_uv;
			bool has_valid_uv = (v32.isMember("uv") && v32["uv"].isArray() && v32["uv"].size() >= 6 && v32["uv"][2].asFloat() > 0.0001f);

			bool is_procedural = is_terrain && (name == "fire" || name == "still_water" || name == "flowing_water" || name == "still_lava" || name == "flowing_lava");
			std::string base_path = is_terrain ? "textures/blocks/" : "textures/items/";
			std::string main_img_path = base_path + name + ".png";
			uint8_t* main_pixels = is_procedural ? nullptr : load_and_resize_png(mc, main_img_path, 16, 16);
			if (!main_pixels && !is_procedural) {
				main_img_path = base_path + name + "_0.png";
				main_pixels = load_and_resize_png(mc, main_img_path, 16, 16);
			}

			if (main_pixels) {
				if (has_valid_uv) {
					tryPaintBaseCell(v32["uv"], main_pixels);
				}
				int main_x = (slot_idx % slots_per_row) * 16;
				int main_y = atlas_base_h + (slot_idx / slots_per_row) * 16;
				slot_idx++;

				paste_texture(atlas_pixels, atlas_w, atlas_h, main_pixels, main_x, main_y, 16, 16);
				free(main_pixels);

				float main_min_x = main_x / (float)atlas_w;
				float main_max_x = (main_x + 16) / (float)atlas_w;
				float main_min_y = main_y / (float)atlas_h;
				float main_max_y = (main_y + 16) / (float)atlas_h;
				float main_inset_x = (main_max_x - main_min_x) * 0.002f;
				float main_inset_y = (main_max_y - main_min_y) * 0.002f;

				main_uv.width = atlas_w;
				main_uv.height = atlas_h;
				main_uv.minX = main_min_x + main_inset_x;
				main_uv.maxX = main_max_x - main_inset_x;
				main_uv.minY = main_min_y + main_inset_y;
				main_uv.maxY = main_max_y - main_inset_y;
			} else if (has_valid_uv) {
				float v17 = (v32["uv"][2].asFloat() - v32["uv"][0].asFloat()) * 0.002f;
				float v18 = (v32["uv"][3].asFloat() - v32["uv"][1].asFloat()) * 0.002f;
				float item_w = (v32["uv"].size() >= 5 && v32["uv"][4].asFloat() > 0.0f) ? v32["uv"][4].asFloat() : (float)atlas_base_w;
				float item_h = (v32["uv"].size() >= 6 && v32["uv"][5].asFloat() > 0.0f) ? v32["uv"][5].asFloat() : (float)atlas_base_h;
				float factor_x = item_w / (float)atlas_w;
				float factor_y = item_h / (float)atlas_h;
				main_uv.width = atlas_w;
				main_uv.height = atlas_h;
				main_uv.minX = (v32["uv"][0].asFloat() + v17) * factor_x;
				main_uv.maxX = (v32["uv"][2].asFloat() - v17) * factor_x;
				main_uv.minY = (v32["uv"][1].asFloat() + v18) * factor_y;
				main_uv.maxY = (v32["uv"][3].asFloat() - v18) * factor_y;
			}

			if (v32.isMember("additonal_textures") && v32["additonal_textures"].isArray()) {
				Json::Value v6 = v32["additonal_textures"];
				int add_count = v6.size();
				for (int i = 0; i < add_count; ++i) {
					TextureUVCoordinateSet add_uv;
					bool has_valid_add_uv = (v6[i].isArray() && v6[i].size() >= 6 && v6[i][2].asFloat() > 0.0001f);

					char num_buf[16];
					sprintf(num_buf, "%d", i + 1);
					std::string base_path = is_terrain ? "textures/blocks/" : "textures/items/";
					std::string add_img_path = base_path + name + "_" + num_buf + ".png";
					uint8_t* add_pixels = is_procedural ? nullptr : load_and_resize_png(mc, add_img_path, 16, 16);
					if (!add_pixels && !is_procedural) {
						sprintf(num_buf, "%d", i);
						add_img_path = base_path + name + "_" + num_buf + ".png";
						add_pixels = load_and_resize_png(mc, add_img_path, 16, 16);
					}

					if (add_pixels) {
						if (has_valid_add_uv) {
							tryPaintBaseCell(v6[i], add_pixels);
						}
						int add_x = (slot_idx % slots_per_row) * 16;
						int add_y = atlas_base_h + (slot_idx / slots_per_row) * 16;
						slot_idx++;

						paste_texture(atlas_pixels, atlas_w, atlas_h, add_pixels, add_x, add_y, 16, 16);
						free(add_pixels);

						float add_min_x = add_x / (float)atlas_w;
						float add_max_x = (add_x + 16) / (float)atlas_w;
						float add_min_y = add_y / (float)atlas_h;
						float add_max_y = (add_y + 16) / (float)atlas_h;
						float add_inset_x = (add_max_x - add_min_x) * 0.002f;
						float add_inset_y = (add_max_y - add_min_y) * 0.002f;

						add_uv.width = atlas_w;
						add_uv.height = atlas_h;
						add_uv.minX = add_min_x + add_inset_x;
						add_uv.maxX = add_max_x - add_inset_x;
						add_uv.minY = add_min_y + add_inset_y;
						add_uv.maxY = add_max_y - add_inset_y;
					} else if (has_valid_add_uv) {
						float v17 = (v6[i][2].asFloat() - v6[i][0].asFloat()) * 0.002f;
						float v18 = (v6[i][3].asFloat() - v6[i][1].asFloat()) * 0.002f;
						float item_w = (v6[i].size() >= 5 && v6[i][4].asFloat() > 0.0f) ? v6[i][4].asFloat() : (float)atlas_base_w;
						float item_h = (v6[i].size() >= 6 && v6[i][5].asFloat() > 0.0f) ? v6[i][5].asFloat() : (float)atlas_base_h;
						float factor_x = item_w / (float)atlas_w;
						float factor_y = item_h / (float)atlas_h;
						add_uv.width = atlas_w;
						add_uv.height = atlas_h;
						add_uv.minX = (v6[i][0].asFloat() + v17) * factor_x;
						add_uv.maxX = (v6[i][2].asFloat() - v17) * factor_x;
						add_uv.minY = (v6[i][1].asFloat() + v18) * factor_y;
						add_uv.maxY = (v6[i][3].asFloat() - v18) * factor_y;
					}

					v29.emplace_back(add_uv);
				}
			}

			this->field_4[name] = TextureAtlasTextureItem(name, main_uv, v29);
			++v27;
		}

		if (is_terrain) {
			const char* customSlots[] = {
				"seagrass", "tall_seagrass_bottom", "tall_seagrass_top", "kelp", "kelp_plant"
			};
			for (const char* cs : customSlots) {
				std::string img_path = std::string("textures/blocks/") + cs + ".png";
				uint8_t* pixels = load_and_resize_png(mc, img_path, 16, 16);

				int sx = (slot_idx % slots_per_row) * 16;
				int sy = atlas_base_h + (slot_idx / slots_per_row) * 16;
				slot_idx++;

				if (pixels) {
					paste_texture(atlas_pixels, atlas_w, atlas_h, pixels, sx, sy, 16, 16);
					free(pixels);
				}

				float min_x = sx / (float)atlas_w;
				float max_x = (sx + 16) / (float)atlas_w;
				float min_y = sy / (float)atlas_h;
				float max_y = (sy + 16) / (float)atlas_h;
				float inset_x = (max_x - min_x) * 0.002f;
				float inset_y = (max_y - min_y) * 0.002f;

				TextureUVCoordinateSet uv;
				uv.width = atlas_w;
				uv.height = atlas_h;
				uv.minX = min_x + inset_x;
				uv.maxX = max_x - inset_x;
				uv.minY = min_y + inset_y;
				uv.maxY = max_y - inset_y;

				std::vector<TextureUVCoordinateSet> vec;
				vec.emplace_back(uv);
				this->field_4[cs] = TextureAtlasTextureItem(cs, uv, vec);
			}
		}

		std::vector<std::string> patchSlots = PatchManager::instance().collectTextureNamesFromActivePatches(is_terrain);
		std::string patchBasePath = is_terrain ? "textures/blocks/" : "textures/items/";
		for (const std::string& pName : patchSlots) {
			if (this->field_4.find(pName) != this->field_4.end()) continue;
			std::string img_path = patchBasePath + pName + ".png";
			uint8_t* pixels = load_and_resize_png(mc, img_path, 16, 16);
			if (!pixels) continue;

			int sx = (slot_idx % slots_per_row) * 16;
			int sy = atlas_base_h + (slot_idx / slots_per_row) * 16;
			if (sy + 16 > atlas_h) {
				free(pixels);
				break;
			}
			slot_idx++;

			paste_texture(atlas_pixels, atlas_w, atlas_h, pixels, sx, sy, 16, 16);
			free(pixels);

			float min_x = sx / (float)atlas_w;
			float max_x = (sx + 16) / (float)atlas_w;
			float min_y = sy / (float)atlas_h;
			float max_y = (sy + 16) / (float)atlas_h;
			float inset_x = (max_x - min_x) * 0.002f;
			float inset_y = (max_y - min_y) * 0.002f;

			TextureUVCoordinateSet uv;
			uv.width = atlas_w;
			uv.height = atlas_h;
			uv.minX = min_x + inset_x;
			uv.maxX = max_x - inset_x;
			uv.minY = min_y + inset_y;
			uv.maxY = max_y - inset_y;

			std::vector<TextureUVCoordinateSet> vec;
			vec.emplace_back(uv);
			this->field_4[pName] = TextureAtlasTextureItem(pName, uv, vec);
		}
	}
}
