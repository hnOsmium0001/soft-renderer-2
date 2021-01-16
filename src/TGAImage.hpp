#pragma once

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string_view>

namespace SRender {

#pragma pack(push, 1)
struct TGAHeader {
	uint8_t idlength;
	uint8_t colormaptype;
	uint8_t datatypecode;
	int16_t colormaporigin;
	int16_t colormaplength;
	uint8_t colormapdepth;
	int16_t x_origin;
	int16_t y_origin;
	int16_t width;
	int16_t height;
	uint8_t bitsperpixel;
	uint8_t imagedescriptor;
};
#pragma pack(pop)

struct TGAColor {
	union {
		struct {
			uint8_t b;
			uint8_t g;
			uint8_t r;
			uint8_t a;
		};
		uint8_t raw[4];
		uint32_t val;
	};
	int32_t bytespp;

	TGAColor()
		: val{ 0 }, bytespp{ 1 } {}

	TGAColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
		: r{ r }, g{ g }, b{ b }, a{ a } {}

	TGAColor(uint32_t val, int32_t bytespp)
		: val{ val }, bytespp{ bytespp } {}

	TGAColor(const uint8_t* p, int32_t bytespp)
		: val{ 0 }, bytespp{ bytespp } {
		for (size_t i = 0; i < bytespp; ++i) {
			raw[i] = p[i];
		}
	}

	TGAColor(const TGAColor&) = default;
	TGAColor& operator=(const TGAColor&) = default;
	TGAColor(TGAColor&&) = default;
	TGAColor& operator=(TGAColor&&) = default;
};

class TGAImage {
protected:
	uint8_t* data;
	uint32_t width;
	uint32_t height;
	int32_t bytespp;

	auto LoadRLEData(std::ifstream& in) -> bool;
	auto UnloadRLEData(std::ofstream& out) -> bool;

public:
	enum Format {
		GRAYSCALE = 1,
		RGB = 3,
		RGBA = 4
	};

	TGAImage();
	TGAImage(uint32_t w, uint32_t h, int32_t bpp);
	~TGAImage();

	TGAImage(const TGAImage& img);
	TGAImage& operator=(const TGAImage& img);
	TGAImage(TGAImage&& img);
	TGAImage& operator=(TGAImage&& img);

	auto ReadTGAFile(std::string_view filename) -> bool;
	auto WriteTGAFile(std::string_view filename, bool rle = true) -> bool;
	auto Get(uint32_t x, uint32_t y) const -> TGAColor;
	auto Set(uint32_t x, uint32_t y, TGAColor c) -> bool;
	auto FlipHorizontally() -> bool;
	auto FlipVertically() -> bool;
	auto Scale(int32_t w, int32_t h) -> bool;
	auto Clear() -> void;

	auto GetWidth() const -> uint32_t { return width; }
	auto GetHeight() const -> uint32_t { return height; }
	auto GetBytesPP() const -> int32_t { return bytespp; }
	auto Buffer() const -> uint8_t* { return data; }
};

} // namespace SRender
