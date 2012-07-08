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

	string input_file;
	string output_file;
	float sigma;

	try {
		po::options_description desc("allowed options");
		desc.add_options()
			("help,h", "produce help message")
			("input,i", po::value<string>(&input_file)->default_value("input.png"), "input file")
			("output,o", po::value<string>(&output_file)->default_value("output.png"), "output file")
			("sigma,s", po::value<float>(&sigma)->default_value(1.0), "set sigma")
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


	// gaussian mask
	int ll = sqrt(-2 * sigma * sigma * log(1.0 / 512 * sigma * sqrt(2 * M_PI))) + 0.5;
	int mask_len = ll * 2 + 1;
	vector<float> mask(mask_len);
	for(int x = 0; x <= ll; x++) {
		float h = exp(-x * x / (2 * sigma * sigma)) / (sigma * sqrt(2 * M_PI));
		mask[ll + x] = h;
		mask[ll - x] = h;
	}
	float sum = 0;
	for(vector<float>::iterator i = mask.begin(); i != mask.end(); ++i) sum += *i;
	for(vector<float>::iterator i = mask.begin(); i != mask.end(); ++i) *i /= sum;


	QImage src_img(input_file.c_str());
	int width = src_img.width();
	int height = src_img.height();
	QImage dst_img(width, height, QImage::Format_RGB32);


	// apply mask
	for(int x = 0; x < width; x++) {
		for(int y = 0; y < height; y++) {
			float acc_r = 0;
			float acc_g = 0;
			float acc_b = 0;
			for(int i = 0; i < mask_len; i++) {

				int yy = y - mask_len / 2 + i;
				if(yy < 0) yy = 0;
				if(yy > height - 1) yy = height - 1;

				unsigned int color = src_img.pixel(x, yy);
				acc_r += mask[i] * qRed(color);
				acc_g += mask[i] * qGreen(color);
				acc_b += mask[i] * qBlue(color);
			}
			dst_img.setPixel(x, y, qRgb(acc_r, acc_g, acc_b));
		}
	}
	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			float acc_r = 0;
			float acc_g = 0;
			float acc_b = 0;
			for(int i = 0; i < mask_len; i++) {

				int xx = x - mask_len / 2 + i;
				if(xx < 0) xx = 0;
				if(xx > width - 1) xx = width - 1;

				unsigned int color = dst_img.pixel(xx, y);
				acc_r += mask[i] * qRed(color);
				acc_g += mask[i] * qGreen(color);
				acc_b += mask[i] * qBlue(color);
			}
			src_img.setPixel(x, y, qRgb(acc_r, acc_g, acc_b));
		}
	}

	src_img.save(output_file.c_str());

	return 0;
}

