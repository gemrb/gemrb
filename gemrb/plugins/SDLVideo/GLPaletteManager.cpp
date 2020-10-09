
#include "OpenGLEnv.h"

#include <cstring>

#include "GLPaletteManager.h"
#include "Palette.h"

using namespace GemRB;

GLuint GLPaletteManager::CreatePaletteTexture(PaletteHolder palette, unsigned int colorKey, bool attached)
{
	const PaletteKey key(palette, colorKey);
	std::map<PaletteKey, GLuint, PaletteKey> *currentTextures;
	std::map<GLuint, PaletteKey> *currentIndexes;
	if (attached)
	{
		currentTextures = &a_textures;
		currentIndexes = &a_indexes;
	}
	else
	{
		currentTextures = &textures;
		currentIndexes = &indexes;
	}

	if (currentTextures->find(key) == currentTextures->end())
	{
		// not found, we need to create it
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		Color* colors = new Color[256];
		memcpy(colors, palette->col, sizeof(Color)*256);
		if (!palette->alpha)
		{
			for (unsigned int i=0; i<256; i++)
			{
				colors[i].a = 0xFF;
			}
		}
		if (PALETTE_INVALID_INDEX != colorKey) {
			colors[colorKey].a = 0;
		}
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
#ifdef USE_GL
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*) colors);
		delete[] colors;
		currentTextures->insert(std::make_pair(key, texture));
		currentIndexes->insert(std::make_pair(texture, key));
	}
	return currentTextures->at(key);
}

void GLPaletteManager::RemovePaletteTexture(PaletteHolder palette, unsigned int colorKey, bool attached)
{
	const PaletteKey key(palette, colorKey);

	std::map<PaletteKey, GLuint, PaletteKey> *currentTextures;
	std::map<GLuint, PaletteKey> *currentIndexes;
	if (attached)
	{
		currentTextures = &a_textures;
		currentIndexes = &a_indexes;
	}
	else
	{
		currentTextures = &textures;
		currentIndexes = &indexes;
	}

	if (currentTextures->find(key) == currentTextures->end())
	{
		// nothing found
	}
	else
	{
		if (!palette->IsShared())
		{
			currentIndexes->erase(currentTextures->at(key));
			glDeleteTextures(1, &(currentTextures->at(key)));
			currentTextures->erase(key);
		}
	}
}

void GLPaletteManager::RemovePaletteTexture(GLuint texture, bool attached)
{
	std::map<PaletteKey, GLuint, PaletteKey> *currentTextures;
	std::map<GLuint, PaletteKey> *currentIndexes;
	if (attached)
	{
		currentTextures = &a_textures;
		currentIndexes = &a_indexes;
	}
	else
	{
		currentTextures = &textures;
		currentIndexes = &indexes;
	}

	if (currentIndexes->find(texture) == currentIndexes->end())
	{
		// nothing found
	}
	else
	{
		PaletteKey key = currentIndexes->at(texture);
		if (!key.palette->IsShared())
		{
			currentIndexes->erase(texture);
			glDeleteTextures(1, &texture);
			currentTextures->erase(key);
		}
	}
}

void GLPaletteManager::ClearUnused(bool attached)
{
	std::map<PaletteKey, GLuint, PaletteKey> *currentTextures;
	std::map<GLuint, PaletteKey> *currentIndexes;
	if (attached)
	{
		currentTextures = &a_textures;
		currentIndexes = &a_indexes;
	}
	else
	{
		currentTextures = &textures;
		currentIndexes = &indexes;
	}
	std::map<PaletteKey, GLuint, PaletteKey>::iterator it = currentTextures->begin();
	while(it != currentTextures->end())
	{
		if (!it->first.palette->IsShared())
		{
			glDeleteTextures(1, &(currentTextures->at(it->first)));
			currentIndexes->erase(it->second);
			currentTextures->erase(it++);
		}
		else
		{
			++it;
		}
	}
}

void GLPaletteManager::Clear()
{
	std::map<PaletteKey, GLuint, PaletteKey>::iterator it = textures.begin();
	while (it != textures.end())
	{
		glDeleteTextures(1, &(textures[it->first]));
		it = textures.erase(it);
	}
	indexes.clear();

	it = a_textures.begin();
	while (it != a_textures.end())
	{
		glDeleteTextures(1, &(a_textures[it->first]));
		it = a_textures.erase(it);
	}
	a_indexes.clear();
}


GLPaletteManager::GLPaletteManager()
{
}

GLPaletteManager::~GLPaletteManager()
{
	Clear();
}
