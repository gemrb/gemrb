#ifdef USE_GL
#include <GL/glew.h>
#else
#include <GLES2/GL2.h>
#include <GLES2/GL2ext.h>
#endif

#include "GLPaletteManager.h"
#include "Palette.h"

using namespace GemRB;

std::map<PaletteKey, PaletteValue> GLPaletteManager::textures;

GLuint GLPaletteManager::CreatePaletteTexture(Palette* palette, unsigned int colorKey)
{
	PaletteKey key = std::make_pair(palette, colorKey);
	if (GLPaletteManager::textures.find(key) == GLPaletteManager::textures.end())
	{
		GLuint texture;
		// not found, we need to create it
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		Color* colors = new Color[256];
		memcpy(colors, palette->col, sizeof(Color)*256);
		for(unsigned int i=0; i<256; i++)
		{
			if(colors[i].a == 0)
			{
				colors[i].a = 0xFF;
			}
			if(i == colorKey) colors[i].a = 0;
		}
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*) colors);
		delete[] colors;
		PaletteValue value = std::make_pair(texture, 1);
		textures.insert(std::make_pair(key, value));
	}
	else
	{
		textures[key].second++;
	}
	return textures[key].first;
}

void GLPaletteManager::RemovePaletteTexture(Palette* palette, unsigned int colorKey)
{
	std::pair<Palette*, unsigned int> key = std::make_pair(palette, colorKey);
	if (textures.find(key) == textures.end())
	{
		// nothing found
	}
	else
	{
		PaletteValue value = textures[key];
		if (value.second > 1) value.second --;
		else
		{
			glDeleteTextures(1, &(value.first));
			textures.erase(key);
		}
	}

}

void GLPaletteManager::Clear()
{
	for(std::map<std::pair<Palette*, unsigned int>, PaletteValue>::iterator it = textures.begin(); it != textures.end(); ++it)
	{
		glDeleteTextures(1, &(textures[it->first].first));
	}
	textures.clear();
}




