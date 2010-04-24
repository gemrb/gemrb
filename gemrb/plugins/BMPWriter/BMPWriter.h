#include "ImageWriter.h"

class BMPWriter : public ImageWriter {
public:
	BMPWriter(void);
	~BMPWriter(void);

	void PutImage(DataStream *output, Sprite2D *sprite);
};
