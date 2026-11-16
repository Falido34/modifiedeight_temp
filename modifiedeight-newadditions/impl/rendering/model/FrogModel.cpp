#include <rendering/model/FrogModel.hpp>
#include <rendering/model/MobAnimUtils.hpp>
#include <entity/Entity.hpp>
#include <math/Mth.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

FrogModel::FrogModel()
	: body(3, 1, 48, 48)
	, head(0, 13, 48, 48)
	, croakingBody(26, 5, 48, 48)
	, leftEye(0, 5, 48, 48)
	, rightEye(0, 0, 48, 48)
	, leftArm(0, 32, 48, 48)
	, rightArm(0, 38, 48, 48)
	, leftLeg(14, 25, 48, 48)
	, rightLeg(0, 25, 48, 48) {
	this->body.setModel(this);
	this->body.addBox(-3.5f, -2.0f, -8.0f, 7, 3, 9);
	this->body.setPos(0.0f, 22.0f, 4.0f);

	this->head.setModel(this);
	this->head.addBox(-3.5f, -2.0f, -7.0f, 7, 3, 9, 0.02f);
	this->head.setPos(0.0f, 20.0f, 3.0f);

	this->croakingBody.setModel(this);
	this->croakingBody.addBox(-3.5f, -0.1f, -2.9f, 7, 2, 3);
	this->croakingBody.setPos(0.0f, 21.0f, -1.0f);

	this->leftEye.setModel(this);
	this->leftEye.addBox(0.5f, -4.0f, -6.0f, 3, 2, 3);
	this->leftEye.setPos(0.0f, 20.0f, 3.0f);

	this->rightEye.setModel(this);
	this->rightEye.addBox(-3.5f, -4.0f, -6.0f, 3, 2, 3);
	this->rightEye.setPos(0.0f, 20.0f, 3.0f);

	this->leftArm.setModel(this);
	this->leftArm.addBox(-1.0f, 0.0f, -1.0f, 2, 3, 3);
	this->leftArm.setPos(4.0f, 21.0f, -2.5f);

	this->rightArm.setModel(this);
	this->rightArm.addBox(-1.0f, 0.0f, -1.0f, 2, 3, 3);
	this->rightArm.setPos(-4.0f, 21.0f, -2.5f);

	this->leftLeg.setModel(this);
	this->leftLeg.addBox(-1.0f, 0.0f, -2.0f, 3, 3, 4);
	this->leftLeg.setPos(3.5f, 21.0f, 4.0f);

	this->rightLeg.setModel(this);
	this->rightLeg.addBox(-2.0f, 0.0f, -2.0f, 3, 3, 4);
	this->rightLeg.setPos(-3.5f, 21.0f, 4.0f);
}

FrogModel::~FrogModel() {
}

void FrogModel::setupAnim(float a2, float a3, float a4, float a5, float a6, float a7) {
	float throatPulse = fabsf(animBurstWave(a4, 46.0f, 419U, 96U, 2.2f));
	this->head.yRotAngle = 0.0f;
	this->head.xRotAngle = 0.0f;
	this->head.zRotAngle = 0.0f;
	this->leftEye.yRotAngle = 0.0f;
	this->leftEye.xRotAngle = 0.0f;
	this->leftEye.zRotAngle = 0.0f;
	this->rightEye.yRotAngle = 0.0f;
	this->rightEye.xRotAngle = 0.0f;
	this->rightEye.zRotAngle = 0.0f;
	this->croakingBody.setPos(0.0f, 21.0f + throatPulse * 0.35f, -1.0f - throatPulse * 0.4f);
	this->croakingBody.xRotAngle = throatPulse * 0.12f;
	this->leftLeg.yRotAngle = 0.0f;
	this->rightLeg.yRotAngle = 0.0f;
	this->leftArm.yRotAngle = 0.0f;
	this->rightArm.yRotAngle = 0.0f;
	this->leftLeg.xRotAngle = Mth::cos(a2 * 0.6662f) * 0.8f * a3;
	this->rightLeg.xRotAngle = Mth::cos(a2 * 0.6662f + (float)M_PI) * 0.8f * a3;
	this->leftArm.xRotAngle = Mth::cos(a2 * 0.6662f + (float)M_PI) * 0.4f * a3;
	this->rightArm.xRotAngle = Mth::cos(a2 * 0.6662f) * 0.4f * a3;
}

void FrogModel::render(Entity* a2, float a3, float a4, float a5, float a6, float a7, float a8) {
	this->setupAnim(a3, a4, a5, a6, a7, a8);
	if (a2 && a2->isInWater()) {
		float swimCycle = a5 * 0.25f;
		float amp = 0.35f + Mth::clamp(a4, 0.0f, 1.0f) * 0.65f;
		float legKick = 0.55f + Mth::sin(swimCycle) * 0.65f * amp;
		this->leftLeg.xRotAngle = legKick;
		this->rightLeg.xRotAngle = legKick;
		this->leftLeg.yRotAngle = -0.18f - Mth::cos(swimCycle) * 0.22f * amp;
		this->rightLeg.yRotAngle = 0.18f + Mth::cos(swimCycle) * 0.22f * amp;
		this->leftArm.xRotAngle = 0.45f + Mth::cos(swimCycle) * 0.35f * amp;
		this->rightArm.xRotAngle = 0.45f + Mth::cos(swimCycle) * 0.35f * amp;
	}
	this->body.render(a8);
	if (fabsf(animBurstWave(a5, 46.0f, 419U, 96U, 2.2f)) > 0.05f) {
		this->croakingBody.render(a8);
	}
	this->head.render(a8);
	this->leftEye.render(a8);
	this->rightEye.render(a8);
	this->leftArm.render(a8);
	this->rightArm.render(a8);
	this->leftLeg.render(a8);
	this->rightLeg.render(a8);
}
