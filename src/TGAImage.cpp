#include "TGAImage.hpp"

#include <cmath>
#include <cstring>
#include <ctime>
#include <iostream>

using namespace SRender;

TGAImage::TGAImage()
	: data{ nullptr }, width{ 0 }, height{ 0 }, bytespp{ 0 } {}

TGAImage::TGAImage(uint32_t w, uint32_t h, int32_t bpp)
	: data{ nullptr }
	, width{ w }
	, height{ h }
	, bytespp{ bpp } {
	uint64_t nbytes = width * height * bytespp;
	data = new uint8_t[nbytes];
	memset(data, 0, nbytes);
}

TGAImage::~TGAImage() {
	if (data) {
		delete[] data;
	}
}

TGAImage::TGAImage(const TGAImage& img)
	: width{ img.width }, height{ img.height }, bytespp{ img.bytespp } {
	uint64_t nbytes = width * height * bytespp;
	data = new uint8_t[nbytes];
	memcpy(data, img.data, nbytes);
}

auto TGAImage::operator=(const TGAImage& img) -> TGAImage& {
	if (data) {
		delete[] data;
	}
	width = img.width;
	height = img.height;
	bytespp = img.bytespp;
	uint64_t nbytes = width * height * bytespp;
	data = new uint8_t[nbytes];
	memcpy(data, img.data, nbytes);
	return *this;
}

TGAImage::TGAImage(TGAImage&& img)
	: width{ img.width }, height{ img.height }, bytespp{ img.bytespp }, data{ img.data } {
	img.data = nullptr;
}

auto TGAImage::operator=(TGAImage&& img) -> TGAImage& {
	if (data) {
		delete[] data;
	}
	width = img.width;
	height = img.height;
	bytespp = img.bytespp;
	data = img.data;
	img.data = nullptr;
	return *this;
}

auto TGAImage::ReadTGAFile(std::string_view filename) -> bool {
	if (data) {
		delete[] data;
	}

	data = nullptr;
	std::ifstream in;
	in.open(filename.data(), std::ios::binary);
	if (!in.is_open()) {
		std::cerr << "can't open file " << filename << "\n";
		in.close();
		return false;
	}
	TGAHeader header;
	in.read(reinterpret_cast<char*>(&header), sizeof(header));
	if (!in.good()) {
		in.close();
		std::cerr << "an error occured while reading the header\n";
		return false;
	}
	width = header.width;
	height = header.height;
	bytespp = header.bitsperpixel >> 3;
	if (width <= 0 || height <= 0 ||
		(bytespp != GRAYSCALE && bytespp != RGB && bytespp != RGBA)) {
		in.close();
		std::cerr << "bad bpp (or width/height) value\n";
		return false;
	}

	uint64_t nbytes = bytespp * width * height;
	data = new uint8_t[nbytes];
	if (3 == header.datatypecode || 2 == header.datatypecode) {
		in.read(reinterpret_cast<char*>(data), nbytes);
		if (!in.good()) {
			in.close();
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
	} else if (10 == header.datatypecode || 11 == header.datatypecode) {
		if (!LoadRLEData(in)) {
			in.close();
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
	} else {
		in.close();
		std::cerr << "unknown file format " << static_cast<int32_t>(header.datatypecode) << "\n";
		return false;
	}
	if (!(header.imagedescriptor & 0x20)) {
		FlipVertically();
	}
	if (header.imagedescriptor & 0x10) {
		FlipHorizontally();
	}
	std::cerr << width << "x" << height << "/" << bytespp * 8 << "\n";
	in.close();
	return true;
}

auto TGAImage::LoadRLEData(std::ifstream& in) -> bool {
	uint64_t pixelcount = width * height;
	uint64_t currentpixel = 0;
	uint64_t currentbyte = 0;
	TGAColor colorbuffer;
	do {
		uint8_t chunkheader = 0;
		chunkheader = in.get();
		if (!in.good()) {
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
		if (chunkheader < 128) {
			chunkheader++;
			for (int32_t i = 0; i < chunkheader; i++) {
				in.read(reinterpret_cast<char*>(colorbuffer.raw), bytespp);
				if (!in.good()) {
					std::cerr << "an error occured while reading the header\n";
					return false;
				}
				for (int32_t t = 0; t < bytespp; t++)
					data[currentbyte++] = colorbuffer.raw[t];
				currentpixel++;
				if (currentpixel > pixelcount) {
					std::cerr << "Too many pixels read\n";
					return false;
				}
			}
		} else {
			chunkheader -= 127;
			in.read(reinterpret_cast<char*>(colorbuffer.raw), bytespp);
			if (!in.good()) {
				std::cerr << "an error occured while reading the header\n";
				return false;
			}
			for (int32_t i = 0; i < chunkheader; i++) {
				for (int32_t t = 0; t < bytespp; t++)
					data[currentbyte++] = colorbuffer.raw[t];
				currentpixel++;
				if (currentpixel > pixelcount) {
					std::cerr << "Too many pixels read\n";
					return false;
				}
			}
		}
	} while (currentpixel < pixelcount);
	return true;
}

auto TGAImage::WriteTGAFile(std::string_view filename, bool rle) -> bool {
	uint8_t developerAreaRef[4] = { 0, 0, 0, 0 };
	uint8_t extensionAreaRef[4] = { 0, 0, 0, 0 };
	uint8_t footer[18] = { 'T', 'R', 'U', 'E', 'V', 'I', 'S', 'I', 'O', 'N', '-', 'X', 'F', 'I', 'L', 'E', '.', '\0' };
	std::ofstream out;
	out.open(filename.data(), std::ios::binary);
	if (!out.is_open()) {
		std::cerr << "can't open file " << filename << "\n";
		out.close();
		return false;
	}
	TGAHeader header;
	memset(reinterpret_cast<void*>(&header), 0, sizeof(header));
	header.bitsperpixel = bytespp << 3;
	header.width = width;
	header.height = height;
	header.datatypecode = bytespp == GRAYSCALE ? (rle ? 11 : 3) : (rle ? 10 : 2);
	header.imagedescriptor = 0x20; // top-left origin
	out.write(reinterpret_cast<char*>(&header), sizeof(header));
	if (!out.good()) {
		out.close();
		std::cerr << "can't dump the tga file\n";
		return false;
	}
	if (!rle) {
		out.write(reinterpret_cast<char*>(data), width * height * bytespp);
		if (!out.good()) {
			std::cerr << "can't unload raw data\n";
			out.close();
			return false;
		}
	} else {
		if (!UnloadRLEData(out)) {
			out.close();
			std::cerr << "can't unload rle data\n";
			return false;
		}
	}
	out.write(reinterpret_cast<char*>(developerAreaRef), sizeof(developerAreaRef));
	if (!out.good()) {
		std::cerr << "can't dump the tga file\n";
		out.close();
		return false;
	}
	out.write(reinterpret_cast<char*>(extensionAreaRef), sizeof(extensionAreaRef));
	if (!out.good()) {
		std::cerr << "can't dump the tga file\n";
		out.close();
		return false;
	}
	out.write(reinterpret_cast<char*>(footer), sizeof(footer));
	if (!out.good()) {
		std::cerr << "can't dump the tga file\n";
		out.close();
		return false;
	}
	out.close();
	return true;
}

auto TGAImage::UnloadRLEData(std::ofstream& out) -> bool {
	const uint8_t max_chunk_length = 128;
	uint64_t npixels = width * height;
	uint64_t curpix = 0;
	while (curpix < npixels) {
		uint64_t chunkstart = curpix * bytespp;
		uint64_t curbyte = curpix * bytespp;
		uint8_t run_length = 1;
		auto raw = true;
		while (curpix + run_length < npixels && run_length < max_chunk_length) {
			auto succ_eq = true;
			for (uint32_t t = 0; succ_eq && t < bytespp; t++) {
				succ_eq = (data[curbyte + t] == data[curbyte + t + bytespp]);
			}
			curbyte += bytespp;
			if (1 == run_length) {
				raw = !succ_eq;
			}
			if (raw && succ_eq) {
				run_length--;
				break;
			}
			if (!raw && !succ_eq) {
				break;
			}
			run_length++;
		}
		curpix += run_length;
		out.put(raw ? run_length - 1 : run_length + 127);
		if (!out.good()) {
			std::cerr << "can't dump the tga file\n";
			return false;
		}
		out.write(reinterpret_cast<char*>(data + chunkstart),
			(raw ? run_length * bytespp : bytespp));
		if (!out.good()) {
			std::cerr << "can't dump the tga file\n";
			return false;
		}
	}
	return true;
}

auto TGAImage::Get(uint32_t x, uint32_t y) const -> TGAColor {
	if (!data || x < 0 || y < 0 || x >= width || y >= height) {
		return TGAColor{};
	}
	return TGAColor{ data + (x + y * width) * bytespp, bytespp };
}

auto TGAImage::Set(uint32_t x, uint32_t y, TGAColor c) -> bool {
	if (!data || x < 0 || y < 0 || x >= width || y >= height) {
		return false;
	}
	memcpy(data + (x + y * width) * bytespp, c.raw, bytespp);
	return true;
}

auto TGAImage::FlipHorizontally() -> bool {
	if (!data) {
		return false;
	}
	int32_t half = width >> 1;
	for (int32_t i = 0; i < half; i++) {
		for (int32_t j = 0; j < height; j++) {
			auto c1 = Get(i, j);
			auto c2 = Get(width - 1 - i, j);
			Set(i, j, c2);
			Set(width - 1 - i, j, c1);
		}
	}
	return true;
}

auto TGAImage::FlipVertically() -> bool {
	if (!data) {
		return false;
	}
	uint64_t bytes_per_line = width * bytespp;
	uint8_t* line = new unsigned char[bytes_per_line];
	int32_t half = height >> 1;
	for (int32_t j = 0; j < half; j++) {
		uint64_t l1 = j * bytes_per_line;
		uint64_t l2 = (height - 1 - j) * bytes_per_line;
		memmove(reinterpret_cast<void*>(line), reinterpret_cast<void*>(data + l1), bytes_per_line);
		memmove(reinterpret_cast<void*>(data + l1), reinterpret_cast<void*>(data + l2), bytes_per_line);
		memmove(reinterpret_cast<void*>(data + l2), reinterpret_cast<void*>(line), bytes_per_line);
	}
	delete[] line;
	return true;
}

auto TGAImage::Scale(int32_t w, int32_t h) -> bool {
	if (w <= 0 || h <= 0 || !data) {
		return false;
	}
	uint8_t* tdata = new uint8_t[w * h * bytespp];
	int32_t nscanline = 0;
	int32_t oscanline = 0;
	int32_t erry = 0;
	uint64_t nlinebytes = w * bytespp;
	uint64_t olinebytes = width * bytespp;
	for (int32_t j = 0; j < height; j++) {
		int32_t errx = width - w;
		int32_t nx = -bytespp;
		int32_t ox = -bytespp;
		for (int32_t i = 0; i < width; i++) {
			ox += bytespp;
			errx += w;
			while (errx >= static_cast<int32_t>(width)) {
				errx -= width;
				nx += bytespp;
				memcpy(tdata + nscanline + nx, data + oscanline + ox, bytespp);
			}
		}
		erry += h;
		oscanline += olinebytes;
		while (erry >= static_cast<int32_t>(height)) {
			if (erry >= static_cast<int32_t>(height) << 1) // it means we jump over a scanline
				memcpy(tdata + nscanline + nlinebytes, tdata + nscanline, nlinebytes);
			erry -= height;
			nscanline += nlinebytes;
		}
	}
	delete[] data;
	data = tdata;
	width = w;
	height = h;
	return true;
}

auto TGAImage::Clear() -> void {
	memset(reinterpret_cast<void*>(data), 0, width * height * bytespp);
}
