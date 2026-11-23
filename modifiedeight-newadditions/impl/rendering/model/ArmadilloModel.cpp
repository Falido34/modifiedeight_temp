#include <rendering/model/ArmadilloModel.hpp>
#include <rendering/model/MobAnimUtils.hpp>
#include <entity/Armadillo.hpp>
#include <entity/Entity.hpp>
#include <math/Mth.hpp>

ArmadilloModel::ArmadilloModel()
	: head(0, 0, 64, 64), shell(0, 20, 64, 64), body(0, 36, 64, 64),
	  leg0(32, 0, 64, 64), leg1(32, 0, 64, 64),
	  leg2(32, 0, 64, 64), leg3(32, 0, 64, 64) {
	this->head.setModel(this);
	this->head.addBox(-2.5f, -2.0f, -4.5f, 5, 4, 5);
	this->head.setPos(0.0f, 19.0f, -4.0f);
	this->body.setModel(this);
	this->body.addBox(-3.0f, -2.0f, -4.0f, 6, 5, 8);
	this->body.setPos(0.0f, 20.0f, 1.0f);
	this->shell.setModel(this);
	this->shell.addBox(-3.5f, -3.0f, -4.5f, 7, 6, 9);
	this->shell.setPos(0.0f, 18.5f, 1.0f);
	this->leg0.setModel(this); this->leg0.addBox(-1.0f, 0.0f, -1.0f, 2, 3, 2); this->leg0.setPos(-2.0f, 21.0f, -2.5f);
	this->leg1.setModel(this); this->leg1.addBox(-1.0f, 0.0f, -1.0f, 2, 3, 2); this->leg1.setPos(2.0f, 21.0f, -2.5f);
	this->leg2.setModel(this); this->leg2.addBox(-1.0f, 0.0f, -1.0f, 2, 3, 2); this->leg2.setPos(-2.0f, 21.0f, 3.0f);
	this->leg3.setModel(this); this->leg3.addBox(-1.0f, 0.0f, -1.0f, 2, 3, 2); this->leg3.setPos(2.0f, 21.0f, 3.0f);
}
ArmadilloModel::~ArmadilloModel() {}
void ArmadilloModel::setupAnim(float a2, float a3, float a4, float a5, float a6, float) {
	this->head.xRotAngle = a6 * 0.0174532925f;
	this->head.yRotAngle = a5 * 0.0174532925f;
	this->leg0.xRotAngle = Mth::cos(a2 * 0.6662f) * 1.4f * a3;
	this->leg1.xRotAngle = Mth::cos(a2 * 0.6662f + 3.14159265f) * 1.4f * a3;
	this->leg2.xRotAngle = Mth::cos(a2 * 0.6662f + 3.14159265f) * 1.4f * a3;
	this->leg3.xRotAngle = Mth::cos(a2 * 0.6662f) * 1.4f * a3;
}
void ArmadilloModel::render(Entity* entity, float a2, float a3, float a4, float a5, float a6, float a7) {
	this->setupAnim(a2, a3, a4, a5, a6, a7);
	Armadillo* mob = (Armadillo*)entity;
	if (mob && mob->isRolledUp()) {
		this->head.xRotAngle = 0.35f;
		this->head.setPos(0.0f, 20.5f, -1.5f);
	} else {
		this->head.setPos(0.0f, 19.0f, -4.0f);
	}
	this->body.render(a7); this->shell.render(a7); this->head.render(a7);
	if (!mob || !mob->isRolledUp()) {
		this->leg0.render(a7); this->leg1.render(a7); this->leg2.render(a7); this->leg3.render(a7);
	}
}