#pragma once
#include <rendering/entity/MobRenderer.hpp>
#include <rendering/model/PolarBearModel.hpp>

struct PolarBearRenderer: MobRenderer {
	PolarBearRenderer(PolarBearModel*, float);
	virtual ~PolarBearRenderer();
	virtual void scale(Mob*, float);
};
