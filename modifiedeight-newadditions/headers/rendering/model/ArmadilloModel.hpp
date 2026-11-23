#pragma once
#include <rendering/model/Model.hpp>
#include <rendering/model/ModelPart.hpp>

struct ArmadilloModel: Model {
	ModelPart head;
	ModelPart shell;
	ModelPart body;
	ModelPart leg0;
	ModelPart leg1;
	ModelPart leg2;
	ModelPart leg3;
	ArmadilloModel();
	virtual ~ArmadilloModel();
	virtual void render(Entity*, float, float, float, float, float, float);
	virtual void setupAnim(float, float, float, float, float, float);
};