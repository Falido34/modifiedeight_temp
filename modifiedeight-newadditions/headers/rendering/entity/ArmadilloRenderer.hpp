#pragma once
#include <rendering/entity/MobRenderer.hpp>
#include <rendering/model/ArmadilloModel.hpp>

struct ArmadilloRenderer: MobRenderer {
	ArmadilloRenderer(ArmadilloModel*, float);
	virtual ~ArmadilloRenderer();
};