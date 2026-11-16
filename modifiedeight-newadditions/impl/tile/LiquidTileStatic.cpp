#include <tile/LiquidTileStatic.hpp>
#include <level/Level.hpp>
#include <level/LightLayer.hpp>
#include <level/biome/Biome.hpp>
#include <tile/Tile.hpp>
#include <tile/material/Material.hpp>

LiquidTileStatic::LiquidTileStatic(int32_t a2, const struct Material* a3, const std::string& a4, const std::string& a5)
	: LiquidTile(a2, a3, a4, a5) {
	this->setTicking(0);
	if(a3 == Material::lava || a3 == Material::water) this->setTicking(1);
}
void LiquidTileStatic::setDynamic(Level* level, int32_t x, int32_t y, int32_t z) {
	int32_t data;
	int32_t id;
	int32_t v12;

	data = level->getData(x, y, z);
	level->setTileAndDataNoUpdate(x, y, z, this->blockID - 1, data);
	level->setTilesDirty(x, y, z, x, y, z);
	id = this->blockID - 1;
	v12 = this->getTickDelay();
	level->addToTickNextTick(x, y, z, id, v12);
}

LiquidTileStatic::~LiquidTileStatic() {
}
void LiquidTileStatic::onPlace(Level* level, int32_t x, int32_t y, int32_t z) {
	LiquidTile::onPlace(level, x, y, z);
	if(!level->isClientMaybe && this->material == Material::water && level->getBiome(x, z) == Biome::tundra) {
		level->addToTickNextTick(x, y, z, this->blockID, 15 + (level->random.genrand_int32() % 15));
	}
}
void LiquidTileStatic::tick(Level* level, int32_t x, int32_t y, int32_t z, Random* random) {
	if(this->material == Material::lava) {
		this->_trySpreadFire(level, x, y, z, random);
	} else if(this->material == Material::water && !level->isClientMaybe) {
		Biome* b = level->getBiome(x, z);
		if((b == Biome::tundra || b == Biome::icePeaks || b == Biome::iceDesert) && level->getData(x, y, z) == 0 && level->getBrightness(LightLayer::Block, x, y, z) < 10) {
			int32_t above = (y + 1 < 128) ? level->getTile(x, y + 1, z) : 0;
			if((y + 1 >= 128 || above == 0 || (Tile::topSnow && above == Tile::topSnow->blockID)) && (y + 1 >= 128 || level->canSeeSky(x, y + 1, z))) {
				level->setTileAndData(x, y, z, Tile::ice->blockID, 0, 3);
				if(Tile::topSnow && above == Tile::topSnow->blockID) {
					level->setTileAndData(x, y + 1, z, 0, 0, 3);
				}
			}
		}
	}
}
void LiquidTileStatic::neighborChanged(Level* level, int32_t x, int32_t y, int32_t z, int32_t, int32_t, int32_t, int32_t) {
	this->updateLiquid(level, x, y, z);
	if(level->getTile(x, y, z) == this->blockID) {
		this->setDynamic(level, x, y, z);
	}
}
