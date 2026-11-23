#pragma once
#include <rendering/entity/MobRenderer.hpp>
#include <rendering/model/ArmadilloModel.hpp>

struct ArmadilloRenderer: MobRenderer {
	ArmadilloRenderer(ArmadilloModel*, float);
	virtual ~ArmadilloRenderer();
	virtual void render(Entity*, float, float, float, float, float);
};
