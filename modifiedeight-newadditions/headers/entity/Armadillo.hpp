#pragma once
#include <entity/Animal.hpp>

struct Armadillo: Animal {
	int32_t rollTicks;
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
	virtual Mob* getBreedOffspring(Animal*);
	bool_t isRolledUp() const;
};