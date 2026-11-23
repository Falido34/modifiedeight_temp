#include <entity/Armadillo.hpp>
#include <entity/Player.hpp>
#include <item/Item.hpp>
#include <entity/ai/goals/FloatGoal.hpp>
#include <entity/ai/goals/PanicGoal.hpp>
#include <entity/ai/goals/RandomStrollGoal.hpp>
#include <entity/ai/goals/LookAtPlayerGoal.hpp>
#include <entity/ai/goals/RandomLookAroundGoal.hpp>
#include <entity/path/PathNavigation.hpp>
#include <level/Level.hpp>
#include <nbt/CompoundTag.hpp>
#include <math/Mth.hpp>

Armadillo::Armadillo(Level* level)
	: Animal(level) {
	this->rollTicks = 0;
	this->threatScanTicks = 0;
	this->scuteTicks = 6000 + (level ? (level->random.genrand_int32() % 6001) : 0);
	this->entityRenderId = ARMADILLO;
	this->skin = "mob/armadillo.png";
	this->setSize(0.7f, 0.65f);
	this->stepHeight = 0.5f;
	this->getNavigation()->setAvoidWater(1);
	this->goalSelector.addGoal(0, new FloatGoal(this), 1);
	this->goalSelector.addGoal(1, new PanicGoal(this, 1.35f), 1);
	this->goalSelector.addGoal(2, new RandomStrollGoal(this, 0.7f), 1);
	this->goalSelector.addGoal(3, new LookAtPlayerGoal(this, 6.0f), 1);
	this->goalSelector.addGoal(4, new RandomLookAroundGoal(this), 1);
}
Armadillo::~Armadillo() {}
int32_t Armadillo::getEntityTypeId() const { return 42; }
int32_t Armadillo::getMaxHealth() { return 12; }
float Armadillo::getBaseSpeed() { return 0.14f; }
const char_t* Armadillo::getAmbientSound() { return "mob.armadillo.idle"; }
std::string Armadillo::getHurtSound() { return "mob.armadillo.hurt"; }
std::string Armadillo::getDeathSound() { return "mob.armadillo.death"; }

bool_t Armadillo::isFood(const ItemInstance* item) {
	// Vanilla item ID 375 is the spider eye in the legacy item registry.
	return item && item->itemClass && Item::spiderEye && item->itemClass == Item::spiderEye;
}

void Armadillo::startRolling() {
	if (this->isInWater() || !this->onGround || this->isDead) return;
	if (this->rollTicks < 60) {
		this->rollTicks = 60;
		if (this->level) this->level->playSound(this, "mob.armadillo.roll", 1.0f, 1.0f);
	}
}

bool_t Armadillo::hasNearbyThreat() {
	if (!this->level) return 0;
	AABB scanBox{
		this->boundingBox.minX - 7.0f, this->boundingBox.minY - 2.0f, this->boundingBox.minZ - 7.0f,
		this->boundingBox.maxX + 7.0f, this->boundingBox.maxY + 2.0f, this->boundingBox.maxZ + 7.0f
	};
	std::vector<Entity*>* nearby = this->level->getEntities(this, scanBox);
	if (!nearby) return 0;
	for (size_t i = 0; i < nearby->size(); ++i) {
		Entity* entity = nearby->at(i);
		if (!entity || !entity->isAlive()) continue;
		int32_t type = entity->getEntityTypeId();
		// Undead mobs: zombie, skeleton, zombie pigman and zombie villager.
		if (type == 32 || type == 34 || type == 36 || type == 44 || type == 121) return 1;
		if (entity->isPlayer()) {
			Player* player = (Player*)entity;
			// Entity shared flag 3 is the sprinting flag in this client.
			if (player->getSharedFlag(3) || player->isRiding()) return 1;
		}
	}
	return 0;
}

bool_t Armadillo::hurt(Entity* attacker, int32_t damage) {
	if (this->isRolledUp() && damage > 1) {
		damage = (damage - 1) / 2;
		if (damage < 1) damage = 1;
	}
	bool_t damaged = Mob::hurt(attacker, damage);
	if (damaged) this->resetLove();
	if (damaged) this->startRolling();
	return damaged;
}

void Armadillo::tick() {
	Animal::tick();
	if (this->rollTicks > 0) --this->rollTicks;
	if (!this->level || this->level->isClientMaybe || this->isBaby()) return;
	if (this->scuteTicks > 0) --this->scuteTicks;
	if (this->scuteTicks <= 0) {
		if (Item::armadilloScute) this->spawnAtLocation(Item::armadilloScute->itemID, 1);
		this->scuteTicks = 6000 + (this->random.genrand_int32() % 6001);
	}
}

void Armadillo::aiStep() {
	Animal::aiStep();
	if (!this->level) return;
	if (this->level->isClientMaybe) return;

	if (this->threatScanTicks > 0) --this->threatScanTicks;
	if (this->threatScanTicks <= 0) {
		this->threatScanTicks = 4;
		if (this->hasNearbyThreat()) this->startRolling();
	}

	if (this->isInWater() || !this->onGround) this->rollTicks = 0;
	if (this->isRolledUp()) {
		this->moveForward = 0.0f;
		this->moveStrafe = 0.0f;
		this->isJumping = 0;
		if (this->getNavigation()) this->getNavigation()->stop();
	}
}

bool_t Armadillo::interactWithPlayer(Player* player) {
	if (!player || !player->inventory) return Animal::interactWithPlayer(player);
	ItemInstance* selected = player->inventory->getSelected();
	if (!this->isBaby() && !this->isRolledUp() && selected && selected->itemClass && Item::brush && selected->itemClass == Item::brush) {
		if (!this->level->isClientMaybe) {
			if (Item::armadilloScute) this->spawnAtLocation(Item::armadilloScute->itemID, 1);
			this->level->playSound(this, "mob.armadillo.brush", 0.9f, 1.0f);
		}
		selected->hurtAndBreak(1, this);
		return 1;
	}
	if (this->isBaby() && this->isFood(selected) && !this->isRolledUp()) {
		if (!player->abilities.instabuild) {
			--selected->count;
			if (selected->count <= 0) player->inventory->clearSlot(player->inventory->selectedSlot);
		}
		if (!this->level->isClientMaybe) {
			int32_t age = this->getAge();
			if (age < 0) this->setAge(age - age / 10);
			this->level->playSound(this, "random.eat", 0.8f, 1.0f);
		}
		return 1;
	}
	if (this->isRolledUp()) return 0;
	return Animal::interactWithPlayer(player);
}

bool_t Armadillo::isRolledUp() const { return this->rollTicks > 0; }

void Armadillo::readAdditionalSaveData(CompoundTag* tag) {
	Animal::readAdditionalSaveData(tag);
	this->rollTicks = tag->getInt("ArmadilloRollTicks");
	this->scuteTicks = tag->getInt("ArmadilloScuteTicks");
	if (this->rollTicks < 0) this->rollTicks = 0;
	if (this->rollTicks > 60) this->rollTicks = 60;
	if (this->scuteTicks <= 0) this->scuteTicks = 6000;
}

void Armadillo::addAdditonalSaveData(CompoundTag* tag) {
	Animal::addAdditonalSaveData(tag);
	tag->putInt("ArmadilloRollTicks", this->rollTicks);
	tag->putInt("ArmadilloScuteTicks", this->scuteTicks);
}

Mob* Armadillo::getBreedOffspring(Animal*) { return new Armadillo(this->level); }
