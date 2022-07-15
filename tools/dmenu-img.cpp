#include <iostream>
#include <png.h>
#include <cstdint>
#include <string>
#include <stdexcept>
#include <vector>

class Png {
	std::vector<uint8_t> pixels;
	uint32_t w, h;
	uint8_t colorType, depth;
public:
	Png(const std::string& path) {
		FILE* fp = fopen(path.c_str(), "rb");
		uint8_t header[8];
		if (!fp) throw std::invalid_argument("cannot open file");
		fread(header, 8, 1, fp);
		if (png_sig_cmp(header, 0, 8)) throw std::invalid_argument("file is not a PNG");

		png_struct* png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if (!png) throw std::runtime_error("png_create_read_struct failed");
		png_info* info = png_create_info_struct(png);
		if (!info) throw std::runtime_error("png_create_info_struct failed");

		png_init_io(png, fp);
		png_set_sig_bytes(png, 8);
		
		png_read_info(png, info);
		w = png_get_image_width(png, info);
		h = png_get_image_height(png, info);
		colorType = png_get_color_type(png, info);
		depth = png_get_bit_depth(png, info);

		size_t rowbytes = png_get_rowbytes(png, info);
		pixels = std::vector<uint8_t>(h * rowbytes, 0);
		uint8_t* rows[h];
		for (uint32_t y = 0; y < h; y++) rows[y] = pixels.data() + rowbytes * y;
		png_read_image(png, rows);

		png_destroy_read_struct(&png, &info, nullptr);
		fclose(fp);
	}

	const std::vector<uint8_t>& getPixels() { return pixels; }
	uint32_t getWidth() { return w; }
	uint32_t getHeight() { return h; }
	uint8_t getDepth() { return depth; }
	uint8_t getColorType() { return colorType; }
};

std::string escape(const std::vector<uint8_t>& pixels) {
	std::string out;
	out.reserve(pixels.size());
	for (auto b : pixels)
		switch (b) {
		case '\n':
			out += "\\n";
			break;
		case '\\':
			out += "\\\\";
			break;
		case '\0':
			out += "\\0";
			break;
		default:
			out += b;
		}
	return out;
}

void usage() {
	std::cout << 
	"dmenu-img: <Entry> <Image Path | 0>...\n"
	"	Generate input for dmenu\n"
	"	For each entry write the entry text and the entry image\n"
	"	Write 0 in the path argument for no image\n";
	exit(1);
}

int main(int argc, char* argv[]) {
	if (argc < 3 || argc % 2 != 1) usage();
	int entries = (argc - 1) / 2;

	for (int i = 0; i < entries; i++) {
		std::cout << argv[i * 2 + 1];
		std::string imgPath = argv[i * 2 + 2];
		if (imgPath != "0") {
			std::cout.write("\0", 1);
			Png png(imgPath);
			std::string encodedPng = escape(png.getPixels());
			std::cout << encodedPng;
		}
		std::cout << '\n';
	}
}
