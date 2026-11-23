#include <rendering/entity/ArmadilloRenderer.hpp>
#include <unigl.h>

ArmadilloRenderer::ArmadilloRenderer(ArmadilloModel* model, float shadowRadius)
	: MobRenderer(model, shadowRadius) {}
ArmadilloRenderer::~ArmadilloRenderer() {}

void ArmadilloRenderer::render(Entity* entity, float x, float y, float z, float rot, float partialTick) {
	// Keep the fixed-function texture state explicit on GLES 1.x. The shared
	// renderer binds the entity's mob/armadillo texture immediately afterwards.
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.1f);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	MobRenderer::render(entity, x, y, z, rot, partialTick);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glAlphaFunc(GL_GREATER, 0.5f);
	glDisable(GL_ALPHA_TEST);
}
