#pragma once
#include <entity/Animal.hpp>

struct Armadillo: Animal {
	int32_t rollTicks;
	int32_t threatScanTicks;
	int32_t scuteTicks;
	Armadillo(Level*);
	virtual ~Armadillo();
	virtual int32_t getEntityTypeId() const;
	virtual int32_t getMaxHealth();
	virtual float getBaseSpeed();
	virtual const char_t* getAmbientSound();
	virtual std::string getHurtSound();
	virtual std::string getDeathSound();
	virtual void tick();
	virtual void aiStep();
	virtual bool_t hurt(Entity*, int32_t);
	virtual bool_t isFood(const ItemInstance*);
	virtual void readAdditionalSaveData(CompoundTag*);
	virtual void addAdditonalSaveData(CompoundTag*);
	virtual Mob* getBreedOffspring(Animal*);
	bool_t isRolledUp() const;
	bool_t hasNearbyThreat();
	void startRolling();
};
