#include <rendering/entity/ArmadilloRenderer.hpp>
ArmadilloRenderer::ArmadilloRenderer(ArmadilloModel* model, float shadowRadius)
	: MobRenderer(model, shadowRadius) {}
ArmadilloRenderer::~ArmadilloRenderer() {}
