#ifndef GLPALETTEMANAGER_H
#define GLPALETTEMANAGER_H

#include <map>

namespace GemRB 
{
	class Palette;

	typedef std::pair<Palette*, unsigned int> PaletteKey;
	typedef std::pair<GLuint, unsigned int> PaletteValue;

	class GLPaletteManager
	{
		private:
			static std::map<PaletteKey, PaletteValue> textures;
		public:
			static GLuint CreatePaletteTexture(Palette* palette, unsigned int colorKey);
			static void RemovePaletteTexture(Palette* palette, unsigned int colorKey);
			static void Clear();
	};
}

#endif