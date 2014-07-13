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

	class GLPaletteManager
	{
		private:

			// sprite-owned palettes
			std::map<PaletteKey, GLuint, PaletteKey> textures;
			std::map<GLuint, PaletteKey> indexes;

			// attached palettes
			std::map<PaletteKey, GLuint, PaletteKey> a_textures;
			std::map<GLuint, PaletteKey> a_indexes;

		public:
			GLuint CreatePaletteTexture(Palette* palette, unsigned int colorKey, bool attached = false);
			void RemovePaletteTexture(Palette* palette, unsigned int colorKey, bool attached = false);
			void RemovePaletteTexture(GLuint texture, bool attached = false);
			void ClearUnused(bool attached = false);
			void Clear();
			~GLPaletteManager();
			GLPaletteManager();
	};
}

#endif