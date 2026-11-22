#include <entity/Animal.hpp>
#include <entity/Player.hpp>
#include <entity/player/Abilities.hpp>
#include <entity/ai/MoveControl.hpp>
#include <entity/path/Path.hpp>
#include <entity/path/PathNavigation.hpp>
#include <inventory/Inventory.hpp>
#include <item/Item.hpp>
#include <level/Level.hpp>
#include <nbt/CompoundTag.hpp>
#include <tile/Tile.hpp>
#include <math/Mth.hpp>
#include <cmath>
#include <cstdlib>

Animal::Animal(Level* a2)
	: AgableMob(a2) {
	this->inLove = 0;
	this->fedControlTicks = 0;
	this->familyParentId = 0;
	this->familyCoParentId = 0;
	this->familyRepathTicks = 0;
}
bool_t Animal::canMate(const Animal* a2) {
	int32_t v4; // r6

	if(a2 != this && (v4 = a2->getEntityTypeId(), v4 == this->getEntityTypeId()) && this->isInLove()) {
		return a2->isInLove();
	} else {
		return 0;
	}
}
bool_t Animal::isInLove() const {
	return this->inLove > 0;
}
void Animal::resetLove() {
	this->inLove = 0;
}

Animal::~Animal() {
}
bool_t Animal::interactWithPlayer(Player* a2) {
	if (!a2) {
		return Entity::interactWithPlayer(a2);
	}
	ItemInstance* sel = a2->inventory ? a2->inventory->getSelected() : 0;
	if (this->isSaddleable() && !this->isBaby() && !this->hasSaddle() && sel && Item::saddle && sel->itemClass == Item::saddle) {
		if (!this->level->isClientMaybe) {
			this->setSaddle(1);
			this->level->playSound(this, "step.cloth", 0.6f, 1.0f);
		}
		if (!a2->abilities.instabuild) {
			--sel->count;
		}
		return 1;
	}
	if (sel && this->isFood(sel) && !this->isBaby()) {
		bool_t canFeedForRide = this->isSaddleable();
		if (this->getAge() == 0 || canFeedForRide) {
			if (!a2->abilities.instabuild) {
				--sel->count;
			}
			if (this->getAge() == 0) {
				this->inLove = 600;
			}
			if (canFeedForRide) {
				this->fedControlTicks = 1200;
			}
			this->attackTarget = 0;
			int32_t v5 = 7;
			do {
				float g = this->random.nextGaussian();
				float g2 = this->random.nextGaussian();
				float v8 = this->random.nextGaussian();
				float posX = this->posX;
				Level* level = this->level;
				float f = this->random.nextFloat();
				float entityWidth = this->entityWidth;
				float v14 = posX + (float)((float)(f * entityWidth) + (float)(f * entityWidth));
				float posY = this->posY;
				float ff = this->random.nextFloat();
				float posZ = this->posZ;
				float v18 = (float)(posY + 0.5) + (float)(ff * this->entityHeight);
				float v19 = this->random.nextFloat();
				level->addParticle(PT_HEART, v14 - entityWidth, v18, (float)(posZ + (float)((float)(v19 * this->entityWidth) + (float)(v19 * this->entityWidth))) - this->entityWidth, g * 0.02, g2 * 0.02, v8 * 0.02, 0);
				--v5;
			} while (v5);
			return 1;
		}
	}
	if (this->isSaddleable() && this->hasSaddle() && !this->isBaby() && (!this->rider || this->rider == a2)) {
		if (!this->level->isClientMaybe) {
			if (this->rider != a2) {
				a2->isJumping = 0;
				a2->setSneaking(0);
				a2->ride(this);
				this->positionRider(0);
			}
		}
		return 1;
	}
	return Entity::interactWithPlayer(a2);
}

bool_t Animal::hurt(Entity* a2, int32_t a3) {
	this->inPanicTicksMaybe = 60;
	this->attackTarget = 0;
	this->inLove = 0;
	return Mob::hurt(a2, a3);
}
int32_t Animal::getCreatureBaseType() {
	return 2;
}
void Animal::readAdditionalSaveData(CompoundTag* a2) {
	AgableMob::readAdditionalSaveData(a2);
	this->inLove = a2->getInt("InLove");
	this->fedControlTicks = a2->getInt("FedControlTicks");
}
void Animal::addAdditonalSaveData(CompoundTag* a2) {
	AgableMob::addAdditonalSaveData(a2);
	a2->putInt("InLove", this->inLove);
	a2->putInt("FedControlTicks", this->fedControlTicks);
}
bool_t Animal::hasSaddle() {
	return 0;
}
void Animal::setSaddle(bool_t) {
}
bool_t Animal::isSaddleable() {
	return 0;
}
void Animal::newServerAiStep() {
	if (this->isSaddleable() && this->hasSaddle() && this->rider && this->rider->isPlayer() && (this->fedControlTicks > 0 || this->inLove > 0)) {
		this->moveForward = 0.0f;
		this->moveStrafe = 0.0f;
		this->isJumping = 0;
		return;
	}
	Mob::newServerAiStep();
}
void Animal::positionRider(bool_t isDead) {
	if (!this->rider) return;
	float offY = (this->isDead || isDead)
		? (this->rider->ridingHeight + 0.05f)
		: (this->getRideHeight() + this->rider->getRidingHeight());
	this->rider->setPos(this->posX, this->posY + offY, this->posZ);
	this->rider->prevX = this->prevX;
	this->rider->prevY = this->prevY + offY;
	this->rider->prevZ = this->prevZ;
	this->rider->prevPosX = this->prevX;
	this->rider->prevPosY = this->prevY + offY;
	this->rider->prevPosZ = this->prevZ;
	this->rider->motionX = 0.0f;
	this->rider->motionY = 0.0f;
	this->rider->motionZ = 0.0f;
	this->rider->fallDistance = 0.0f;
}
void Animal::travel(float strafe, float forward) {
	if (this->isSaddleable() && this->hasSaddle() && this->rider && this->rider->isPlayer() && (this->fedControlTicks > 0 || this->inLove > 0)) {
		if (!this->level->isClientMaybe) {
			Player* pRider = (Player*)this->rider;
			float yawDiff = Mth::wrapDegrees(pRider->yaw - this->yaw);
			this->yaw = Mth::wrapDegrees(this->yaw + Mth::clamp(yawDiff, -20.0f, 20.0f));
			this->field_124 = this->yaw;
			this->headYaw = this->yaw;
			this->pitch = Mth::clamp(pRider->pitch * 0.5f, -25.0f, 25.0f);
			this->stepHeight = 1.0f;
			if (this->isInWater() || this->isInLava()) {
				this->motionY += 0.04f;
			}
			float drive = (pRider->moveForward > 0.05f) ? Mth::clamp(pRider->moveForward, 0.0f, 1.0f) : 0.0f;
			float prevSpeed = this->getSpeed();
			this->setSpeed(this->getBaseSpeed() * 1.15f);
			Mob::travel(0.0f, drive);
			this->setSpeed(prevSpeed);
			this->positionRider(0);
			return;
		}
		this->positionRider(0);
		return;
	}
	if (this->stepHeight > 0.5f) {
		this->stepHeight = 0.5f;
	}
	Mob::travel(strafe, forward);
}
int32_t Animal::getAmbientSoundInterval() {
	return 240;
}
void Animal::aiStep() {
	int32_t inLove; // r3
	int32_t v3;		// r3
	float g;		// s20
	float v5;		// s19
	float v6;		// r0
	float posX;		// s16
	Level* level;	// r6
	float v9;		// s18
	float f;		// r0
	float v11;		// s17
	float v12;		// s16
	float v13;		// r0
	float posZ;		// s21
	float v15;		// s16
	float v16;		// r0

	if (!this->isAlive() && this->rider) {
		this->rider->ride(0);
	}
	if (this->fedControlTicks > 0 && !this->rider) {
		--this->fedControlTicks;
	}
	if (this->isSaddleable() && this->hasSaddle() && this->rider && this->rider->isPlayer() && (this->fedControlTicks > 0 || this->inLove > 0)) {
		if (this->getNavigation()) {
			this->getNavigation()->stop();
		}
		this->attackTarget = 0;
	}
	AgableMob::aiStep();
	if(this->getAge()) {
		this->inLove = 0;
	}
	inLove = this->inLove;
	if(inLove > 0) {
		v3 = inLove - 1;
		this->inLove = v3;
		if((v3 & 0xF) == 0) {
			g = this->random.nextGaussian();
			v5 = this->random.nextGaussian();
			v6 = this->random.nextGaussian();
			posX = this->posX;
			level = this->level;
			v9 = v6;
			f = this->random.nextFloat();
			v11 = (float)(posX + (float)((float)(f * this->entityWidth) + (float)(f * this->entityWidth))) - this->entityWidth;
			v12 = this->posY + 0.5;
			v13 = this->random.nextFloat();
			posZ = this->posZ;
			v15 = v12 + (float)(v13 * this->entityHeight);
			v16 = this->random.nextFloat();
			level->addParticle(PT_HEART, v11, v15, (float)(posZ + (float)((float)(v16 * this->entityWidth) + (float)(v16 * this->entityWidth))) - this->entityWidth, g * 0.02, v5 * 0.02, v9 * 0.02, 0);
		}
		if (!this->useNewAi() && this->level && !this->level->isClientMaybe) {
			std::vector<Entity*> candidates;
			AABB searchBox{this->boundingBox.minX - 8.0f, this->boundingBox.minY - 8.0f, this->boundingBox.minZ - 8.0f, this->boundingBox.maxX + 8.0f, this->boundingBox.maxY + 8.0f, this->boundingBox.maxZ + 8.0f};
			this->level->getEntitiesOfType(this->getEntityTypeId(), searchBox, candidates);
			Animal* mate = 0;
			float bestDistSq = 64.0f;
			for (size_t i = 0; i < candidates.size(); ++i) {
				Animal* other = (Animal*)candidates[i];
				if (other && this->canMate(other)) {
					float dSq = this->distanceToSqr(other);
					if (dSq < bestDistSq) {
						bestDistSq = dSq;
						mate = other;
					}
				}
			}
			if (mate) {
				this->attackTarget = 0;
				this->lookAt(mate, 30.0f, 30.0f);
				float breedDistSq = (this->entityWidth > 1.0f) ? 12.25f : 6.25f;
				if (bestDistSq > breedDistSq) {
					if (!this->path || this->path->isDone() || (this->random.genrand_int32() % 10) == 0) {
						this->setPath(this->level->findPath(this, mate, 16.0f, 1, 0, 0, 1));
					}
					if (!this->path) {
						float dx = mate->posX - this->posX;
						float dz = mate->posZ - this->posZ;
						float targetYaw = (atan2f(dz, dx) * 180.0f / 3.14159265f) - 90.0f;
						float diff = Mth::wrapDegrees(targetYaw - this->yaw);
						this->yaw = Mth::wrapDegrees(this->yaw + Mth::clamp(diff, -25.0f, 25.0f));
						this->moveForward = this->getSpeed();
					}
				} else {
					if (this->path) {
						this->setPath(0);
					}
					if (this->inLove <= 555 || mate->inLove <= 555) {
						AgableMob* baby = (AgableMob*)this->getBreedOffspring(mate);
						if (baby) {
							this->setAge(6000);
							mate->setAge(6000);
							this->resetLove();
							mate->resetLove();
							baby->setAge(-24000);
							((Animal*)baby)->familyParentId = this->entityId;
							((Animal*)baby)->familyCoParentId = mate->entityId;
							baby->moveTo(this->posX, this->posY, this->posZ, this->yaw, 0.0f);
							this->level->addEntity(baby);
							for (int32_t p = 0; p < 7; ++p) {
								float gx = this->random.nextGaussian() * 0.02f;
								float gy = this->random.nextGaussian() * 0.02f;
								float gz = this->random.nextGaussian() * 0.02f;
								float px = this->posX + (this->random.nextFloat() * this->entityWidth * 2.0f) - this->entityWidth;
								float py = this->posY + 0.5f + (this->random.nextFloat() * this->entityHeight);
								float pz = this->posZ + (this->random.nextFloat() * this->entityWidth * 2.0f) - this->entityWidth;
								this->level->addParticle(PT_HEART, px, py, pz, gx, gy, gz, 0);
							}
						} else {
							this->resetLove();
							mate->resetLove();
						}
					}
				}
			}
		}
	}
}
bool_t Animal::canSpawn() {
	int32_t v3 = (int32_t)floorf(this->posX);
	int32_t v6 = (int32_t)floorf(this->boundingBox.minY);
	int32_t v11 = (int32_t)floorf(this->posZ);
	int32_t tBelow = this->level->getTile(v3, v6 - 1, v11);
	return (tBelow == Tile::grass->blockID || tBelow == Tile::snow->blockID || tBelow == Tile::topSnow->blockID || tBelow == Tile::dirt->blockID || tBelow == Tile::sand->blockID || tBelow == Tile::leaves->blockID || tBelow == Tile::ice->blockID) && this->level->getRawBrightness(v3, v6, v11) >= 4 && Mob::canSpawn();
}
bool_t Animal::removeWhenFarAway() {
	if (this->isBaby()) {
		return 0;
	}
	if (this->findSeparatedBaby(40.0f, 0)) {
		return 0;
	}
	return 1;
}
float Animal::getWalkTargetValue(int32_t x, int32_t y, int32_t z) {
	int tCur = this->level->getTile(x, y, z);
	int tBelow = this->level->getTile(x, y - 1, z);
	if (tCur == 8 || tCur == 9 || tBelow == 8 || tBelow == 9) {
		return -100.0f;
	}
	if (tBelow == Tile::grass->blockID || tBelow == Tile::snow->blockID || tBelow == Tile::topSnow->blockID || tBelow == Tile::leaves->blockID || tBelow == Tile::dirt->blockID || tBelow == Tile::sand->blockID || tBelow == Tile::ice->blockID) {
		return 10.0f;
	} else {
		return this->level->getBrightness(x, y, z) - 0.5f;
	}
}
Entity* Animal::findAttackTarget() {
	return 0;
}
bool_t Animal::isSittingOrResting() {
	return 0;
}
bool_t Animal::isFollowingOwner() {
	return 0;
}
Animal* Animal::findFamilyParent(float maxRange) {
	if (!this->isBaby() || !this->level) {
		return 0;
	}
	float maxDistSq = maxRange * maxRange;
	int32_t myType = this->getEntityTypeId();
	Animal* validPrimary = 0;
	Animal* validSecondary = 0;
	int32_t ids[2] = {this->familyParentId, this->familyCoParentId};
	for (int32_t i = 0; i < 2; ++i) {
		if (ids[i] != 0) {
			Entity* e = this->level->getEntity(ids[i]);
			if (e && e != this && e->isAlive() && e->getEntityTypeId() == myType && e->getCreatureBaseType() == 2) {
				Animal* a = (Animal*)e;
				if (!a->isBaby() && this->distanceToSqr(a) <= maxDistSq) {
					if (!a->isSittingOrResting()) {
						validPrimary = a;
						break;
					}
					if (!validSecondary) {
						validSecondary = a;
					}
				}
			}
		}
	}
	if (validPrimary) {
		return validPrimary;
	}
	std::vector<Entity*> candidates;
	AABB box{this->posX - maxRange, this->posY - 16.0f, this->posZ - maxRange, this->posX + maxRange, this->posY + 16.0f, this->posZ + maxRange};
	this->level->getEntitiesOfType(myType, box, candidates);
	Animal* bestActive = 0;
	float bestActiveSq = maxDistSq;
	Animal* bestAny = validSecondary;
	float bestAnySq = validSecondary ? this->distanceToSqr(validSecondary) : maxDistSq;
	for (size_t i = 0; i < candidates.size(); ++i) {
		Entity* e = candidates[i];
		if (!e || e == this || !e->isAlive() || e->getCreatureBaseType() != 2) {
			continue;
		}
		Animal* cand = (Animal*)e;
		if (cand->isBaby()) {
			continue;
		}
		float dSq = this->distanceToSqr(cand);
		if (dSq > maxDistSq) {
			continue;
		}
		if (!cand->isSittingOrResting()) {
			if (!bestActive || dSq < bestActiveSq) {
				bestActive = cand;
				bestActiveSq = dSq;
			}
		} else if (!bestAny || dSq < bestAnySq) {
			bestAny = cand;
			bestAnySq = dSq;
		}
	}
	Animal* chosen = bestActive ? bestActive : bestAny;
	if (chosen && chosen != validSecondary) {
		this->familyParentId = chosen->entityId;
	}
	return chosen;
}
Animal* Animal::findSeparatedBaby(float maxRange, float* outDistSq) {
	if (this->isBaby() || !this->level) {
		return 0;
	}
	float maxDistSq = maxRange * maxRange;
	int32_t myType = this->getEntityTypeId();
	std::vector<Entity*> candidates;
	AABB box{this->posX - maxRange, this->posY - 16.0f, this->posZ - maxRange, this->posX + maxRange, this->posY + 16.0f, this->posZ + maxRange};
	this->level->getEntitiesOfType(myType, box, candidates);
	Animal* bestOwn = 0;
	float bestOwnDistSq = -1.0f;
	Animal* bestWild = 0;
	float bestWildDistSq = maxDistSq;
	for (size_t i = 0; i < candidates.size(); ++i) {
		Entity* e = candidates[i];
		if (!e || e == this || !e->isAlive() || e->getCreatureBaseType() != 2) {
			continue;
		}
		Animal* cand = (Animal*)e;
		if (!cand->isBaby() || cand->isSittingOrResting()) {
			continue;
		}
		float dSq = this->distanceToSqr(cand);
		if (dSq > maxDistSq) {
			continue;
		}
		if (cand->familyParentId == this->entityId || cand->familyCoParentId == this->entityId) {
			if (!bestOwn || dSq > bestOwnDistSq) {
				bestOwn = cand;
				bestOwnDistSq = dSq;
			}
		} else if (cand->familyParentId == 0 && dSq <= 400.0f) {
			if (!bestWild || dSq < bestWildDistSq) {
				bestWild = cand;
				bestWildDistSq = dSq;
			}
		}
	}
	if (bestOwn) {
		if (outDistSq) *outDistSq = bestOwnDistSq;
		return bestOwn;
	}
	if (bestWild) {
		if (outDistSq) *outDistSq = bestWildDistSq;
		return bestWild;
	}
	return 0;
}
Animal* Animal::getFamilyFollowTarget(float* outStartDistSq, float* outStopDistSq, float* outTeleportDistSq) {
	if (!this->level || !this->isAlive() || this->isSittingOrResting()) {
		return 0;
	}
	if (this->isBaby()) {
		Animal* parent = this->findFamilyParent(48.0f);
		if (!parent) {
			return 0;
		}
		if (outStartDistSq) *outStartDistSq = 5.76f;
		if (outStopDistSq) *outStopDistSq = 4.0f;
		if (outTeleportDistSq) *outTeleportDistSq = parent->isSittingOrResting() ? 1024.0f : 784.0f;
		return parent;
	}
	if (this->isFollowingOwner() || this->isInLove()) {
		return 0;
	}
	Animal* baby = this->findSeparatedBaby(40.0f, 0);
	if (!baby) {
		return 0;
	}
	if (outStartDistSq) *outStartDistSq = 42.25f;
	if (outStopDistSq) *outStopDistSq = 20.25f;
	if (outTeleportDistSq) *outTeleportDistSq = 0.0f;
	return baby;
}
bool_t Animal::isFamilyStrollAllowed(float tx, float ty, float tz) {
	(void)ty;
	if (!this->level) {
		return 1;
	}
	if (this->isBaby()) {
		Animal* parent = this->findFamilyParent(48.0f);
		if (parent) {
			float dx = tx - parent->posX;
			float dz = tz - parent->posZ;
			return (dx * dx + dz * dz) <= 20.25f;
		}
	} else {
		Animal* baby = this->findSeparatedBaby(36.0f, 0);
		if (baby) {
			float dx = tx - baby->posX;
			float dz = tz - baby->posZ;
			return (dx * dx + dz * dz) <= 42.25f;
		}
	}
	return 1;
}
bool_t Animal::navigateToEntity(Entity* target, float speedMult) {
	if (!this->level || !target) {
		return 0;
	}
	float dx = target->posX - this->posX;
	float dy = target->posY - this->posY;
	float dz = target->posZ - this->posZ;
	float distSq = dx * dx + dy * dy + dz * dz;
	float dist = sqrtf(distSq);
	float stepX = target->posX;
	float stepY = target->posY;
	float stepZ = target->posZ;
	if (dist > 14.0f) {
		float scale = 12.0f / dist;
		stepX = this->posX + dx * scale;
		stepY = this->posY + dy * scale;
		stepZ = this->posZ + dz * scale;
	}
	if (this->useNewAi()) {
		PathNavigation* nav = this->getNavigation();
		if (nav) {
			if (dist <= 14.0f && target->isMob() && nav->moveTo((Mob*)target, speedMult)) {
				return 1;
			}
			if (nav->moveTo(stepX, stepY, stepZ, speedMult)) {
				return 1;
			}
		}
		if (this->getMoveControl()) {
			this->getMoveControl()->setWantedPosition(target->posX, target->posY, target->posZ, speedMult);
		}
		this->lookAt(target, 30.0f, 30.0f);
		return 0;
	}
	Path* p = (dist <= 14.0f) ? this->level->findPath(this, target, 16.0f, 1, 0, 0, 1) : this->level->findPath(this, Mth::floor(stepX), Mth::floor(stepY), Mth::floor(stepZ), 16.0f, 1, 0, 0, 1);
	this->setPath(p);
	return p != 0;
}
void Animal::steerTowardsEntity(Entity* target, float speedMult, float turnRate) {
	if (!target) {
		return;
	}
	float dx = target->posX - this->posX;
	float dz = target->posZ - this->posZ;
	float desiredYaw = (atan2f(dz, dx) * (180.0f / 3.14159265f)) - 90.0f;
	float diff = Mth::wrapDegrees(desiredYaw - this->yaw);
	if (diff > turnRate) diff = turnRate;
	if (diff < -turnRate) diff = -turnRate;
	this->yaw = Mth::wrapDegrees(this->yaw + diff);
	this->headYaw = this->yaw;
	this->lookAt(target, 30.0f, 30.0f);
	this->moveForward = this->getSpeed() * speedMult;
	if (this->isCollidedHorizontally && (this->onGround || this->isInWater())) {
		this->isJumping = 1;
	}
}
void Animal::tryTeleportNear(Entity* target) {
	if (!this->level || !target) {
		return;
	}
	int32_t tx = Mth::floor(target->posX);
	int32_t ty = Mth::floor(target->boundingBox.minY + 0.25f);
	int32_t tz = Mth::floor(target->posZ);
	bool_t allowWater = this->isWaterMob() || target->isInWater();
	for (int32_t r = 1; r <= 3; ++r) {
		for (int32_t dx = -r; dx <= r; ++dx) {
			for (int32_t dz = -r; dz <= r; ++dz) {
				if (abs(dx) < r && abs(dz) < r) {
					continue;
				}
				for (int32_t dy = -1; dy <= 2; ++dy) {
					int32_t bx = tx + dx;
					int32_t by = ty + dy;
					int32_t bz = tz + dz;
					if (by <= 1 || by >= 126) {
						continue;
					}
					int32_t tBelow = this->level->getTile(bx, by - 1, bz);
					int32_t tAt = this->level->getTile(bx, by, bz);
					int32_t tAbove = this->level->getTile(bx, by + 1, bz);
					bool_t solidBelow = (tBelow > 0 && Tile::solid[tBelow]);
					bool_t waterBelow = allowWater && (tBelow == Tile::water->blockID || tBelow == Tile::calmWater->blockID);
					if (!solidBelow && !waterBelow) {
						continue;
					}
					if ((tAt > 0 && Tile::solid[tAt]) || (tAbove > 0 && Tile::solid[tAbove])) {
						continue;
					}
					if (tAt == Tile::lava->blockID || tAt == Tile::calmLava->blockID || tAbove == Tile::lava->blockID || tAbove == Tile::calmLava->blockID) {
						continue;
					}
					if (!allowWater && (tAt == Tile::water->blockID || tAt == Tile::calmWater->blockID || tBelow == Tile::water->blockID || tBelow == Tile::calmWater->blockID)) {
						continue;
					}
					this->moveTo((float)bx + 0.5f, (float)by, (float)bz + 0.5f, this->yaw, this->pitch);
					this->fallDistance = 0.0f;
					this->setPath(0);
					if (this->useNewAi() && this->getNavigation()) {
						this->getNavigation()->stop();
					}
					return;
				}
			}
		}
	}
}
void Animal::findRandomStrollLocation() {
	float startDistSq = 0.0f;
	Animal* familyTgt = this->getFamilyFollowTarget(&startDistSq, 0, 0);
	if (familyTgt && this->distanceToSqr(familyTgt) > startDistSq) {
		return;
	}
	float bestScore = -99999.0f;
	int32_t bestX = -1;
	int32_t bestY = -1;
	int32_t bestZ = -1;
	bool_t found = 0;
	for (int32_t tries = 0; tries < 10; ++tries) {
		int32_t cx = Mth::floor((this->posX + (float)(this->random.genrand_int32() % 13)) - 6.0f);
		int32_t cy = Mth::floor((this->posY + (float)(this->random.genrand_int32() % 7)) - 3.0f);
		int32_t cz = Mth::floor((this->posZ + (float)(this->random.genrand_int32() % 13)) - 6.0f);
		if (!this->isFamilyStrollAllowed((float)cx + 0.5f, (float)cy, (float)cz + 0.5f)) {
			continue;
		}
		float score = this->getWalkTargetValue(cx, cy, cz);
		if (score > bestScore) {
			bestScore = score;
			bestX = cx;
			bestY = cy;
			bestZ = cz;
			found = 1;
		}
	}
	if (found) {
		this->setPath(this->level->findPath(this, bestX, bestY, bestZ, 10.0f, 1, 0, 0, 1));
	}
}
bool_t Animal::applyFamilyFollowAi() {
	if (!this->level || this->useNewAi() || this->attackTarget != 0 || this->inPanicTicksMaybe > 0 || this->isSittingOrResting()) {
		return 0;
	}
	float startDistSq = 0.0f;
	float stopDistSq = 0.0f;
	float teleportDistSq = 0.0f;
	Animal* tgt = this->getFamilyFollowTarget(&startDistSq, &stopDistSq, &teleportDistSq);
	if (!tgt) {
		return 0;
	}
	float distSq = this->distanceToSqr(tgt);
	if (teleportDistSq > 0.0f && distSq > teleportDistSq) {
		this->tryTeleportNear(tgt);
		return 1;
	}
	bool_t shouldMove = (distSq > startDistSq) || (this->path != 0 && distSq > stopDistSq);
	if (!shouldMove) {
		if (this->path) {
			this->setPath(0);
		}
		return 0;
	}
	if (--this->familyRepathTicks <= 0 || !this->path) {
		this->familyRepathTicks = 5;
		this->navigateToEntity(tgt, distSq > 49.0f ? 1.2f : 1.0f);
	}
	if (!this->path || distSq > 196.0f) {
		this->steerTowardsEntity(tgt, distSq > 49.0f ? 1.15f : 1.0f, 24.0f);
		return 1;
	}
	return 0;
}
bool_t Animal::applyWaterFamilySwim(float* swimAngle, float* swimSpeed, float* swimTargetVy, int32_t* swimTimer, bool_t preferSurface) {
	if (!this->level || this->isSittingOrResting()) {
		return 0;
	}
	float startDistSq = 0.0f;
	float stopDistSq = 0.0f;
	float teleportDistSq = 0.0f;
	Animal* tgt = this->getFamilyFollowTarget(&startDistSq, &stopDistSq, &teleportDistSq);
	if (!tgt) {
		return 0;
	}
	float distSq = this->distanceToSqr(tgt);
	if (teleportDistSq > 0.0f && distSq > teleportDistSq) {
		this->tryTeleportNear(tgt);
		return 1;
	}
	if (distSq <= startDistSq) {
		return 0;
	}
	float dx = tgt->posX - this->posX;
	float dz = tgt->posZ - this->posZ;
	*swimAngle = (atan2f(dz, dx) * (180.0f / 3.14159265f)) - 90.0f;
	*swimSpeed = (distSq > 49.0f) ? 0.115f : 0.088f;
	float dy = (tgt->posY + (preferSurface ? 0.2f : 0.0f)) - this->posY;
	*swimTargetVy = Mth::clamp(dy * 0.04f, -0.045f, 0.045f);
	*swimTimer = 8;
	if (this->isCollidedHorizontally && !tgt->isInWater()) {
		this->motionY = 0.24f;
	}
	return 1;
}
bool_t Animal::isFood(const ItemInstance* a2) {
	return a2->getId() == Item::wheat->itemID;
}
