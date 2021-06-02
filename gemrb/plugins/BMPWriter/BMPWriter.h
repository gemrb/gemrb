#include "ImageWriter.h"

namespace GemRB {

class BMPWriter : public ImageWriter {
public:
	BMPWriter(void);
	~BMPWriter(void) override;

	void PutImage(DataStream *output, Holder<Sprite2D> sprite) override;
};

}
