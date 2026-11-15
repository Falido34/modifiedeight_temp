#include <entity/ai/goals/FollowParentGoal.hpp>
#include <entity/Animal.hpp>
#include <entity/path/PathNavigation.hpp>
#include <float.h>
#include <level/Level.hpp>

FollowParentGoal::FollowParentGoal(Animal* a2, float a3)
	: Goal() {
	this->holder = a2;
	this->speedMultiplier = a3;
	this->ticksFollowing = 0;
	this->setRequiredControlFlags(1);
}
FollowParentGoal::~FollowParentGoal() {
}
bool_t FollowParentGoal::canUse() {
	if (this->holder->attackTarget != 0 || this->holder->inPanicTicksMaybe > 0) {
		return 0;
	}
	float startDistSq = 0.0f;
	float stopDistSq = 0.0f;
	float teleportDistSq = 0.0f;
	Animal* tgt = this->holder->getFamilyFollowTarget(&startDistSq, &stopDistSq, &teleportDistSq);
	if (!tgt) {
		return 0;
	}
	float distSq = this->holder->distanceToSqr(tgt);
	if (distSq <= startDistSq) {
		return 0;
	}
	if (tgt != this->parent.entity) {
		this->parent.entityId = tgt->entityId;
		Level* lvl = tgt->level;
		this->parent.level = lvl;
		lvl->registerTemporaryPointer(&this->parent);
		this->parent.locked = 0;
	}
	return 1;
}
bool_t FollowParentGoal::canContinueToUse() {
	Animal* tgt = this->parent.lock();
	if (!tgt || !tgt->isAlive() || this->holder->isSittingOrResting() || this->holder->attackTarget != 0 || this->holder->inPanicTicksMaybe > 0) {
		return 0;
	}
	float distSq = this->holder->distanceToSqr(tgt);
	if (this->holder->isBaby()) {
		return !tgt->isBaby() && distSq >= 4.0f && distSq <= 2304.0f;
	}
	return !this->holder->isFollowingOwner() && !this->holder->isInLove() && tgt->isBaby() && !tgt->isSittingOrResting() && distSq >= 20.25f && distSq <= 1600.0f;
}
void FollowParentGoal::start() {
	this->ticksFollowing = 0;
}
void FollowParentGoal::stop() {
	if(this->parent.entity) {
		this->parent.unset();
	}
}
void FollowParentGoal::tick() {
	Animal* tgt = this->parent.lock();
	if (!tgt || !tgt->isAlive()) {
		return;
	}
	float distSq = this->holder->distanceToSqr(tgt);
	float teleportDistSq = this->holder->isBaby() ? (tgt->isSittingOrResting() ? 1024.0f : 784.0f) : 0.0f;
	if (teleportDistSq > 0.0f && distSq > teleportDistSq) {
		this->holder->tryTeleportNear(tgt);
		this->ticksFollowing = 0;
		return;
	}
	if (--this->ticksFollowing <= 0) {
		this->ticksFollowing = 5;
		float spd = this->speedMultiplier * (distSq > 49.0f ? 1.2f : 1.0f);
		this->holder->navigateToEntity(tgt, spd);
	}
}
