#include "../Core/ImageWriter.h"

class BMPWriter : public ImageWriter {
public:
	BMPWriter(void);
	~BMPWriter(void);

	void PutImage(DataStream *output, Sprite2D *sprite);
public:
	void release()
	{
		delete this;
	}
};
