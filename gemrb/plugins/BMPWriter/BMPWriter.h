#include "ImageWriter.h"

namespace GemRB {

class BMPWriter : public ImageWriter {
public:
	BMPWriter() noexcept = default;
	void PutImage(DataStream* output, Holder<Sprite2D> sprite) override;
};

}
