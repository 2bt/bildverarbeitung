#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <iostream>
#include <QImage>
#include <vector>
#include <math.h>
#include <boost/program_options.hpp>

using namespace std;
namespace po = boost::program_options;

int main(int argc, char* argv[]) {

	// parse options
	string input_file;
	string output_file;
	int area;
	try {
		po::options_description desc("allowed options");
		desc.add_options()
			("help,h", "produce help message")
			("input,i", po::value<string>(&input_file)->default_value("input.png"), "input file")
			("output,o", po::value<string>(&output_file)->default_value("output.png"), "output file")
			("area,a", po::value<int>(&area)->default_value(23), "square size for mean evaluation")
		;
		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
		po::notify(vm);
		if(vm.count("help")) {
			cout << desc;
			return 0;
		}
	}
	catch(exception& e) {
		cout << e.what() << "\n";
		return 1;
	}


	QImage src_img(input_file.c_str());
	int width = src_img.width();
	int height = src_img.height();


	vector<unsigned int> summed_area_table(width * height * 3, 0);
	#define PIXEL(x, y, c) summed_area_table[((y) * width + (x)) * 3 + (c)]


	// calculate summed area table
	for(int x = 0; x < width; x++) {
		unsigned int color = src_img.pixel(x, 0);
		PIXEL(x, 0, 0) = qRed(color);
		PIXEL(x, 0, 1) = qGreen(color);
		PIXEL(x, 0, 2) = qBlue(color);
		for(int y = 1; y < height; y++) {
			unsigned int color = src_img.pixel(x, y);
			PIXEL(x, y, 0) = PIXEL(x, y - 1, 0) + qRed(color);
			PIXEL(x, y, 1) = PIXEL(x, y - 1, 1) + qGreen(color);
			PIXEL(x, y, 2) = PIXEL(x, y - 1, 2) + qBlue(color);
		}
	}
	for(int y = 0; y < height; y++) {
		for(int x = 1; x < width; x++) {
			PIXEL(x, y, 0) += PIXEL(x - 1, y, 0);
			PIXEL(x, y, 1) += PIXEL(x - 1, y, 1);
			PIXEL(x, y, 2) += PIXEL(x - 1, y, 2);
		}
	}

/*
	// slower but maybe clearer
	for(int x = 0; x < width; x++) {
		for(int y = 0; y < height; y++) {
			unsigned int color = src_img.pixel(x, y);
			PIXEL(x, y, 0) = qRed(color);
			PIXEL(x, y, 1) = qGreen(color);
			PIXEL(x, y, 2) = qBlue(color);
			if(x > 0) {
				PIXEL(x, y, 0) += PIXEL(x - 1, y, 0);
				PIXEL(x, y, 1) += PIXEL(x - 1, y, 1);
				PIXEL(x, y, 2) += PIXEL(x - 1, y, 2);
			}
			if(y > 0) {
				PIXEL(x, y, 0) += PIXEL(x, y - 1, 0);
				PIXEL(x, y, 1) += PIXEL(x, y - 1, 1);
				PIXEL(x, y, 2) += PIXEL(x, y - 1, 2);
			}
			if(x > 0 && y > 0) {
				PIXEL(x, y, 0) -= PIXEL(x - 1, y - 1, 0);
				PIXEL(x, y, 1) -= PIXEL(x - 1, y - 1, 1);
				PIXEL(x, y, 2) -= PIXEL(x - 1, y - 1, 2);
			}
		}
	}
*/

	// mean filter
	for(int x = 0; x < width; x++) {
		int x1 = x - area / 2;
		int x2 = x1 + area;
		if(x1 < 0) x1 = 0;
		if(x2 > width - 1) x2 = width - 1;

		for(int y = 0; y < height; y++) {
			int y1 = y - area / 2;
			int y2 = y1 + area;
			if(y1 < 0) y1 = 0;
			if(y2 > height - 1) y2 = height - 1;

			int r = PIXEL(x1, y1, 0) + PIXEL(x2, y2, 0) - PIXEL(x1, y2, 0) - PIXEL(x2, y1, 0);
			int g = PIXEL(x1, y1, 1) + PIXEL(x2, y2, 1) - PIXEL(x1, y2, 1) - PIXEL(x2, y1, 1);
			int b = PIXEL(x1, y1, 2) + PIXEL(x2, y2, 2) - PIXEL(x1, y2, 2) - PIXEL(x2, y1, 2);
			int s = (x2 - x1) * (y2 - y1);
			src_img.setPixel(x, y, qRgb(r / s, g / s, b / s));
		}
	}
	#undef PIXEL

	src_img.save(output_file.c_str());
	return 0;
}
