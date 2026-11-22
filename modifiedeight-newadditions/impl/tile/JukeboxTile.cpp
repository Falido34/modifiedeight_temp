#include <tile/JukeboxTile.hpp>
#include <tile/material/Material.hpp>
#include <level/Level.hpp>
#include <entity/Player.hpp>
#include <entity/LocalPlayer.hpp>
#include <Minecraft.hpp>
#include <gui/screens/JukeboxScreen.hpp>
#include <rendering/TextureAtlasTextureItem.hpp>

JukeboxTile::JukeboxTile(int32_t id)
	: Tile(id, "jukebox", Material::wood) {
	this->setShape(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}

JukeboxTile::~JukeboxTile() {
}

TextureUVCoordinateSet* JukeboxTile::getTexture(int32_t face, int32_t data) {
	if (face == 1) {
		TextureAtlasTextureItem* top = this->getTextureItem("jukebox_top");
		if (top) return top->getUV(0);
	}
	TextureAtlasTextureItem* side = this->getTextureItem("jukebox_side");
	if (side) return side->getUV(0);
	return &this->textureUV;
}

TextureUVCoordinateSet* JukeboxTile::getCarriedTexture(int32_t face, int32_t data) {
	return this->getTexture(face, data);
}

bool_t JukeboxTile::use(Level* level, int32_t x, int32_t y, int32_t z, Player* player) {
	if (player && player->isPlayer()) {
		LocalPlayer* lp = (LocalPlayer*)player;
		if (lp->minecraft) {
			lp->minecraft->setScreen(new JukeboxScreen(x, y, z));
			return 1;
		}
	}
	return 0;
}
