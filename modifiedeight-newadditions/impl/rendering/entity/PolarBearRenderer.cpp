#include <rendering/entity/PolarBearRenderer.hpp>
#include <entity/Mob.hpp>
#include <unigl.h>

PolarBearRenderer::PolarBearRenderer(PolarBearModel* model, float shadowRadius)
	: MobRenderer(model, shadowRadius) {
}

PolarBearRenderer::~PolarBearRenderer() {
}

void PolarBearRenderer::scale(Mob* mob, float partialTicks) {
	if (mob && !mob->isBaby()) {
		glScalef(1.4f, 1.4f, 1.4f);
	}
}
