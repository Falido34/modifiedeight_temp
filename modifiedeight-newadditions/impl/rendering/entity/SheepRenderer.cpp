#include <rendering/entity/SheepRenderer.hpp>
#include <rendering/model/Model.hpp>
#include <rendering/model/ModelPart.hpp>
#include <entity/Sheep.hpp>
#include <unigl.h>

struct SheepSaddleModel : Model {
	ModelPart body;
	SheepSaddleModel() : body(28, 8, 64, 32) {
		this->body.setModel(this);
		this->body.addBox(-5.0f, -10.0f, -7.0f, 10, 16, 8, 0.5f);
		this->body.setPos(0.0f, 5.0f, 2.0f);
	}
	virtual void render(Entity* e, float a3, float a4, float a5, float a6, float a7, float a8) {
		if (!e || ((Mob*)e)->isBaby()) return;
		this->body.xRotAngle = 90.0f / (float)(180.0 / 3.1416);
		bool sheared = ((Sheep*)e)->isSheared() != 0;
		glPushMatrix();
		glTranslatef(0.0f, 5.0f * a8, 2.0f * a8);
		if (sheared) {
			glScalef(0.84f, 0.84f, 1.02f);
		} else {
			glScalef(1.16f, 1.14f, 1.14f);
		}
		glTranslatef(0.0f, -5.0f * a8, -2.0f * a8);
		this->body.render(a8);
		glPopMatrix();
	}
};

SheepRenderer::SheepRenderer(Model* a2, Model* a3, float a4)
	: MobRenderer(a2, a4) {
	this->shearedModel = a3;
	this->furredModel = a2;
	this->setArmor(new SheepSaddleModel());
}

SheepRenderer::~SheepRenderer() {
	this->model = 0;

	if(this->furredModel) delete this->furredModel;
	this->furredModel = 0;
	if(this->shearedModel) delete this->shearedModel;
	this->shearedModel = 0;
	Model* arm = this->getArmor();
	if(arm) delete arm;
}
void SheepRenderer::render(Entity* a2, float a3, float a4, float a5, float a6, float a7) {
	if(((Sheep*)a2)->isSheared()) {
		this->model = this->shearedModel;
	} else {
		this->model = this->furredModel;
	}

	MobRenderer::render(a2, a3, a4, a5, a6, a7);
}
int32_t SheepRenderer::prepareArmor(Mob* a2, int32_t a3, float) {
	if(!a2 || !((Sheep*)a2)->hasSaddle() || a3 > 0) return -1;
	this->bindTexture("mob/saddle.png");
	return 1;
}
