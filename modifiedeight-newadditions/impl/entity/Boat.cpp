#include <entity/Boat.hpp>
#include <entity/Player.hpp>
#include <level/Level.hpp>
#include <tile/material/Material.hpp>
#include <item/Item.hpp>
#include <item/ItemInstance.hpp>
#include <nbt/CompoundTag.hpp>
#include <cmath>

Boat::Boat(Level* level)
	: Entity(level) {
	this->entityRenderId = BOAT;
	this->setSize(1.375f, 0.5625f);
	this->damageTime = 0;
	this->damageDir = 1;
	this->damageTaken = 0.0f;
	this->rowingTime[0] = 0.0f;
	this->rowingTime[1] = 0.0f;
	this->deltaRotation = 0.0f;
	this->ridingHeight = 0.0f;

	this->synchedEntityData.define<int32_t>(17, 0);
	this->synchedEntityData.define<int32_t>(18, 1);
	this->synchedEntityData.define<float>(19, 0.0f);
	this->synchedEntityData.define<int32_t>(20, 0);
}

Boat::Boat(Level* level, float x, float y, float z)
	: Boat(level) {
	this->setPos(x, y + 0.3f, z);
	this->motionX = 0.0f;
	this->motionY = 0.0f;
	this->motionZ = 0.0f;
	this->prevX = x;
	this->prevY = y + 0.3f;
	this->prevZ = z;
}

Boat::~Boat() {
}

bool_t Boat::isPickable() {
	return !this->isDead;
}

bool_t Boat::isPushable() {
	return 1;
}

int32_t Boat::getEntityTypeId() const {
	return 40;
}

int32_t Boat::getBoatType() const {
	return const_cast<SynchedEntityData&>(this->synchedEntityData).getInt(20);
}

void Boat::setBoatType(int32_t type) {
	this->synchedEntityData.set(20, type);
}

float Boat::getRowingTime(int32_t paddle, float partialTicks) const {
	return this->rowingTime[paddle];
}

bool_t Boat::hurt(Entity* attacker, int32_t damage) {
	if (this->isDead) return 0;
	this->damageDir = -this->damageDir;
	this->damageTime = 10;
	this->damageTaken += (float)damage * 10.0f;
	this->synchedEntityData.set(17, this->damageTime);
	this->synchedEntityData.set(18, this->damageDir);
	this->synchedEntityData.set(19, this->damageTaken);

	if (attacker && attacker->isPlayer()) {
		Player* p = (Player*)attacker;
		if (p->abilities.instabuild) {
			this->remove();
			return 1;
		}
	}

	if (this->damageTaken > 40.0f) {
		if (this->rider) {
			this->rider->ride(0);
		}
		if (Item::boat) {
			this->spawnAtLocation(Item::boat->itemID, 1, this->getBoatType());
		}
		this->remove();
	}
	return 1;
}

bool_t Boat::interactWithPlayer(Player* player) {
	if (!player) return 0;
	if (this->rider && this->rider != player) {
		return 0;
	}
	if (!this->level->isClientMaybe) {
		if (this->rider != player) {
			player->isJumping = 0;
			player->setSneaking(0);
			player->ride(this);
			this->positionRider(0);
		}
	}
	return 1;
}

float Boat::getRideHeight() {
	return 1.48f;
}

void Boat::positionRider(bool_t isDead) {
	if (!this->rider) return;
	float h = this->getRideHeight();
	float ry = this->posY + h;
	this->rider->setPos(this->posX, ry, this->posZ);
	this->rider->prevX = this->prevX;
	this->rider->prevY = this->prevY + h;
	this->rider->prevZ = this->prevZ;
	this->rider->prevPosX = this->prevX;
	this->rider->prevPosY = this->prevY + h;
	this->rider->prevPosZ = this->prevZ;
	this->rider->motionX = 0.0f;
	this->rider->motionY = 0.0f;
	this->rider->motionZ = 0.0f;
	this->rider->fallDistance = 0.0f;
}

void Boat::tick() {
	this->prevX = this->posX;
	this->prevY = this->posY;
	this->prevZ = this->posZ;
	this->prevYaw = this->yaw;
	this->prevPitch = this->pitch;

	if (this->damageTime > 0) {
		this->damageTime--;
		this->synchedEntityData.set(17, this->damageTime);
	}
	if (this->damageTaken > 0.0f) {
		this->damageTaken -= 0.1f;
		if (this->damageTaken < 0.0f) this->damageTaken = 0.0f;
		this->synchedEntityData.set(19, this->damageTaken);
	}

	bool_t hasWater = 0;
	float waterSurfaceY = -1000.0f;
	int32_t minBX = (int32_t)std::floor(this->boundingBox.minX + 0.05f);
	int32_t maxBX = (int32_t)std::floor(this->boundingBox.maxX - 0.05f);
	int32_t minBZ = (int32_t)std::floor(this->boundingBox.minZ + 0.05f);
	int32_t maxBZ = (int32_t)std::floor(this->boundingBox.maxZ - 0.05f);
	int32_t minBY = (int32_t)std::floor(this->boundingBox.minY - 0.15f);
	int32_t maxBY = (int32_t)std::floor(this->boundingBox.maxY + 0.5f);
	for (int32_t bx = minBX; bx <= maxBX; ++bx) {
		for (int32_t bz = minBZ; bz <= maxBZ; ++bz) {
			for (int32_t by = maxBY; by >= minBY; --by) {
				if (this->level->getMaterial(bx, by, bz) == Material::water) {
					float surf = (float)(by + 1) - 0.125f;
					if (!hasWater || surf > waterSurfaceY) {
						waterSurfaceY = surf;
						hasWater = 1;
					}
					break;
				}
			}
		}
	}

	bool_t inWater = hasWater && (this->boundingBox.minY <= waterSurfaceY + 0.15f);
	if (inWater) {
		float targetMinY = waterSurfaceY - 0.12f;
		float dy = targetMinY - this->boundingBox.minY;
		this->motionY = this->motionY * 0.55f + dy * 0.35f;
		if (this->motionY > 0.12f) this->motionY = 0.12f;
		if (this->motionY < -0.12f) this->motionY = -0.12f;
		if (fabsf(dy) < 0.004f && fabsf(this->motionY) < 0.004f) {
			this->motionY = 0.0f;
		}
		this->motionX *= 0.94f;
		this->motionZ *= 0.94f;
	} else {
		this->motionY -= 0.04f;
		this->motionX *= 0.82f;
		this->motionZ *= 0.82f;
		this->motionY *= 0.98f;
	}

	if (this->rider && this->rider->isPlayer()) {
		Player* p = (Player*)this->rider;
		if (p->isSneaking()) {
			p->ride(0);
		} else {
			float forward = p->moveForward;
			if (forward > 1.0f) forward = 1.0f;
			if (forward < -1.0f) forward = -1.0f;
			if (p->moveStrafe > 0.25f) {
				this->yaw -= 3.0f;
			} else if (p->moveStrafe < -0.25f) {
				this->yaw += 3.0f;
			} else if (fabsf(forward) > 0.05f) {
				float diff = p->yaw - this->yaw;
				while (diff < -180.0f) diff += 360.0f;
				while (diff >= 180.0f) diff -= 360.0f;
				if (diff > 6.0f) diff = 6.0f;
				if (diff < -6.0f) diff = -6.0f;
				this->yaw += diff * 0.5f;
			}
			while (this->yaw - this->prevYaw < -180.0f) this->prevYaw -= 360.0f;
			while (this->yaw - this->prevYaw >= 180.0f) this->prevYaw += 360.0f;
			if (fabsf(forward) > 0.05f) {
				float speed = inWater ? 0.08f : 0.015f;
				float rad = (this->yaw * 3.14159265f) / 180.0f;
				this->motionX += -std::sin(rad) * speed * forward;
				this->motionZ += std::cos(rad) * speed * forward;
				this->rowingTime[0] += 0.25f;
				this->rowingTime[1] += 0.25f;
			} else if (fabsf(p->moveStrafe) > 0.25f) {
				this->rowingTime[0] += 0.15f;
				this->rowingTime[1] += 0.15f;
			}
		}
	}

	float maxSpeed = inWater ? 0.55f : 0.15f;
	float speedH = std::sqrt(this->motionX * this->motionX + this->motionZ * this->motionZ);
	if (speedH > maxSpeed) {
		this->motionX = (this->motionX / speedH) * maxSpeed;
		this->motionZ = (this->motionZ / speedH) * maxSpeed;
	}

	this->move(this->motionX, this->motionY, this->motionZ);
	this->positionRider(0);
}

void Boat::readAdditionalSaveData(CompoundTag* tag) {
	if (tag->contains("BoatType")) {
		this->setBoatType(tag->getInt("BoatType"));
	}
}

void Boat::addAdditonalSaveData(CompoundTag* tag) {
	tag->putInt("BoatType", this->getBoatType());
}
