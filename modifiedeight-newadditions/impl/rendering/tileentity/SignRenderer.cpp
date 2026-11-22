#include <rendering/tileentity/SignRenderer.hpp>
#include <rendering/Font.hpp>
#include <tile/Tile.hpp>
#include <tile/entity/SignTileEntity.hpp>
#include <util/Color4.hpp>
#include <utils.h>

SignRenderer::SignRenderer() : TileEntityRenderer(){

}
SignRenderer::~SignRenderer() {
}
void SignRenderer::render(TileEntity* tee, float x, float y, float z, float a6, bool_t a7) {
	SignTileEntity* te = (SignTileEntity*)tee;
	Tile* tile;	 // r8
	int8_t v13;	 // r3
	int32_t v14; // r0
	float v15;	 // r8
	Font* font;	 // r5
	float v19;	 // s17
	int32_t v20; // r6
	int32_t v21; // r10
	int32_t v26; // r0
	int32_t v27; // r0

	tile = te->getTile();
	te->setupLighting(a7, a6);
	glPushMatrix();
	if(tile == Tile::sign) {
		glTranslatef(x + 0.5, y + 0.5, z + 0.5);
		glRotatef(-vcvts_n_f32_s32(360 * te->getData(), 4u), 0.0, 1.0, 0.0);
		v13 = 1;
	} else {
		v14 = te->getData();
		switch(v14) {
			case 2:
				v15 = 180.0;
				break;
			case 4:
				v15 = 90.0;
				break;
			case 5:
				v15 = -90.0;
				break;
			default:
				v15 = 0.0;
				break;
		}
		glTranslatef(x + 0.5, y + 0.5, z + 0.5);
		glRotatef(-v15, 0.0, 1.0, 0.0);
		glTranslatef(0.0, -0.3125, -0.4375);
		v13 = 0;
	}
	this->model.field_9C.field_1D = v13;
	this->bindTexture("item/sign.png");
	glPushMatrix();
	glScalef(0.66667, -0.66667, -0.66667);
	this->model.render();
	glPopMatrix();
	font = this->getFont();
	glTranslatef(0.0f, 0.33333f, 0.055f);
	glScalef(0.011111f, -0.011111f, 0.011111f);
	GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);
	GLboolean lightingWasEnabled = glIsEnabled(GL_LIGHTING);
	GLboolean alphaTestWasEnabled = glIsEnabled(GL_ALPHA_TEST);
	GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
	GLboolean vertWasEnabled = glIsEnabled(GL_VERTEX_ARRAY);
	GLboolean texCoordWasEnabled = glIsEnabled(GL_TEXTURE_COORD_ARRAY);
	GLboolean colorWasEnabled = glIsEnabled(GL_COLOR_ARRAY);
	GLboolean normalWasEnabled = glIsEnabled(GL_NORMAL_ARRAY);
	glDisable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
	glDisable(GL_ALPHA_TEST);
	glDisableClientState(GL_NORMAL_ARRAY);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(-8.0f, -8.0f);
	glDepthFunc(GL_LEQUAL);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glDepthMask(0);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	v19 = -20.0;
	v20 = 0;
	static const uint32_t DYE_COLORS[16] = {
		0xFF000000, // 0 Black
		0xFFB02E26, // 1 Red
		0xFF5E7C16, // 2 Green
		0xFF835432, // 3 Brown
		0xFF3C44AA, // 4 Blue
		0xFF8932B8, // 5 Purple
		0xFF169C9C, // 6 Cyan
		0xFF9D9D97, // 7 Light Gray
		0xFF474F52, // 8 Gray
		0xFFF38BAA, // 9 Pink
		0xFF80C71F, // 10 Lime
		0xFFFED83D, // 11 Yellow
		0xFF3AB3DA, // 12 Light Blue
		0xFFC74EBD, // 13 Magenta
		0xFFF9801D, // 14 Orange
		0xFFFFFFFF  // 15 White
	};
	v21 = DYE_COLORS[te->textColor & 15];
	do {
		font->field_4 = 1;
		if(v20 == te->field_6C) {
			std::string a2 = "> " + te->textLines[v20] + " <";
			v26 = font->width(a2);
			font->draw(a2, (float)-v26 * 0.5, v19, v21);
		} else {
			v27 = font->width(te->textLines[v20]);
			font->draw(te->textLines[v20], (float)-v27 * 0.5, v19, v21);
		}
		++v20;
		v19 = v19 + 10.0;
	} while(v20 != 4);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glDepthMask(1u);
	if(normalWasEnabled) {
		glEnableClientState(GL_NORMAL_ARRAY);
	}
	if(!colorWasEnabled) {
		glDisableClientState(GL_COLOR_ARRAY);
	}
	if(!texCoordWasEnabled) {
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	if(!vertWasEnabled) {
		glDisableClientState(GL_VERTEX_ARRAY);
	}
	if(!blendWasEnabled) {
		glDisable(GL_BLEND);
	}
	if(alphaTestWasEnabled) {
		glEnable(GL_ALPHA_TEST);
	}
	if(lightingWasEnabled) {
		glEnable(GL_LIGHTING);
	}
	if(cullWasEnabled) {
		glEnable(GL_CULL_FACE);
	}
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glPopMatrix();
}
