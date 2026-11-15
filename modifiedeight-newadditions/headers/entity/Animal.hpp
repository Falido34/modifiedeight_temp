#pragma once
#include <entity/AgableMob.hpp>

struct Animal: AgableMob
{
	int32_t inLove;
	int32_t familyParentId;
	int32_t familyCoParentId;
	int32_t familyRepathTicks;

	Animal(Level*);
	bool_t canMate(const Animal*);
	bool_t isInLove() const;
	void resetLove();
	Animal* findFamilyParent(float maxRange);
	Animal* findSeparatedBaby(float maxRange, float* outDistSq);
	Animal* getFamilyFollowTarget(float* outStartDistSq, float* outStopDistSq, float* outTeleportDistSq);
	bool_t isFamilyStrollAllowed(float tx, float ty, float tz);
	bool_t navigateToEntity(Entity* target, float speedMult);
	void steerTowardsEntity(Entity* target, float speedMult, float turnRate);
	void tryTeleportNear(Entity* target);
	bool_t applyFamilyFollowAi();
	bool_t applyWaterFamilySwim(float* swimAngle, float* swimSpeed, float* swimTargetVy, int32_t* swimTimer, bool_t preferSurface);

	virtual ~Animal();
	virtual bool_t interactWithPlayer(Player*);
	virtual bool_t hurt(Entity*, int32_t);
	virtual int32_t getCreatureBaseType();
	virtual void readAdditionalSaveData(CompoundTag*);
	virtual void addAdditonalSaveData(CompoundTag*);
	virtual int32_t getAmbientSoundInterval();
	virtual void aiStep();
	virtual bool_t canSpawn();
	virtual bool_t removeWhenFarAway();
	virtual float getWalkTargetValue(int32_t, int32_t, int32_t);
	virtual Entity* findAttackTarget();
	virtual void findRandomStrollLocation();
	virtual bool_t isSittingOrResting();
	virtual bool_t isFollowingOwner();
	virtual bool_t isFood(const ItemInstance*);
	virtual Mob* getBreedOffspring(Animal*) = 0;
};
