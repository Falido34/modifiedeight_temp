#include <rendering/model/SnowGolemModel.hpp>
#include <math/Mth.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SnowGolemModel::SnowGolemModel()
	: piece1(0, 16, 64, 64)
	, piece2(0, 36, 64, 64)
	, head(0, 0, 64, 64)
	, arm1(32, 0, 64, 64)
	, arm2(32, 0, 64, 64) {
	this->head.setModel(this);
	this->head.addBox(-4.0f, -8.0f, -4.0f, 8, 8, 8, -0.5f);
	this->head.setPos(0.0f, 4.0f, 0.0f);

	this->arm1.setModel(this);
	this->arm1.addBox(-1.0f, 0.0f, -1.0f, 12, 2, 2, -0.5f);
	this->arm1.setPos(5.0f, 6.0f, 0.0f);
	this->arm1.zRotAngle = 1.0f;

	this->arm2.setModel(this);
	this->arm2.addBox(-11.0f, 0.0f, -1.0f, 12, 2, 2, -0.5f);
	this->arm2.setPos(-5.0f, 6.0f, 0.0f);
	this->arm2.zRotAngle = -1.0f;

	this->piece1.setModel(this);
	this->piece1.addBox(-5.0f, -10.0f, -5.0f, 10, 10, 10, -0.5f);
	this->piece1.setPos(0.0f, 13.0f, 0.0f);

	this->piece2.setModel(this);
	this->piece2.addBox(-6.0f, -12.0f, -6.0f, 12, 12, 12, -0.5f);
	this->piece2.setPos(0.0f, 24.0f, 0.0f);
}

SnowGolemModel::~SnowGolemModel() {
}

void SnowGolemModel::setupAnim(float a2, float a3, float a4, float a5, float a6, float a7) {
	this->head.xRotAngle = 0.0f;
	this->head.yRotAngle = 0.0f;
	this->head.zRotAngle = 0.0f;
	this->piece1.xRotAngle = 0.0f;
	this->piece1.yRotAngle = 0.0f;
	this->piece1.zRotAngle = 0.0f;
	this->piece2.xRotAngle = 0.0f;
	this->piece2.yRotAngle = 0.0f;
	this->piece2.zRotAngle = 0.0f;
	this->arm1.xRotAngle = 0.0f;
	this->arm1.yRotAngle = 0.0f;
	this->arm1.zRotAngle = 1.0f;
	this->arm1.setPos(5.0f, 6.0f, 0.0f);
	this->arm2.xRotAngle = 0.0f;
	this->arm2.yRotAngle = 0.0f;
	this->arm2.zRotAngle = -1.0f;
	this->arm2.setPos(-5.0f, 6.0f, 0.0f);
}

void SnowGolemModel::render(Entity* a2, float a3, float a4, float a5, float a6, float a7, float a8) {
	this->setupAnim(a3, a4, a5, a6, a7, a8);
	this->piece1.render(a8);
	this->piece2.render(a8);
	this->head.render(a8);
	this->arm1.render(a8);
	this->arm2.render(a8);
}
