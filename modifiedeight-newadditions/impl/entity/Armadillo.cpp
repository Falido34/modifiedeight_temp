#include <entity/Armadillo.hpp>
#include <entity/ai/goals/FloatGoal.hpp>
#include <entity/ai/goals/PanicGoal.hpp>
#include <entity/ai/goals/RandomStrollGoal.hpp>
#include <entity/ai/goals/LookAtPlayerGoal.hpp>
#include <entity/ai/goals/RandomLookAroundGoal.hpp>
#include <entity/path/PathNavigation.hpp>

Armadillo::Armadillo(Level* level)
	: Animal(level) {
	this->rollTicks = 0;
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
void Armadillo::tick() {
	Animal::tick();
	if (this->rollTicks > 0) --this->rollTicks;
}
void Armadillo::aiStep() {
	Animal::aiStep();
	if (this->isHurt()) this->rollTicks = 40;
}
bool_t Armadillo::isRolledUp() const { return this->rollTicks > 0; }
Mob* Armadillo::getBreedOffspring(Animal*) { return new Armadillo(this->level); }
