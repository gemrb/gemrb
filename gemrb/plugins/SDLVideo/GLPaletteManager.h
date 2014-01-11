#ifndef GLPALETTEMANAGER_H
#define GLPALETTEMANAGER_H

#include <map>

namespace GemRB 
{
	class Palette;

	struct PaletteKey
	{
		Palette* palette;
		unsigned int colorKey;
		bool operator () (const PaletteKey& lhs, const PaletteKey& rhs) const 
		{ 
			if (lhs.palette < rhs.palette) return true;
			else
			if (rhs.palette < lhs.palette) return false;
			if (lhs.colorKey < rhs.colorKey) return true;
			else
			if (rhs.colorKey < lhs.colorKey) return false;
			return false;
		}
		PaletteKey(Palette* pal, unsigned int key) { colorKey = key; palette = pal; }
		PaletteKey() {}
	};

	typedef std::pair<GLuint, unsigned int> PaletteValue;

	class GLPaletteManager
	{
		private:
			std::map<PaletteKey, PaletteValue, PaletteKey> textures;
			std::map<GLuint, PaletteKey> indexes;
		public:
			GLuint CreatePaletteTexture(Palette* palette, unsigned int colorKey);
			void RemovePaletteTexture(Palette* palette, unsigned int colorKey);
			void RemovePaletteTexture(GLuint texture);
			void Clear();
			~GLPaletteManager();
			GLPaletteManager();
	};
}

#endif