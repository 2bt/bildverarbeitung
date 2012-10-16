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


class FloatImage {
	vector<float> data;
	int width;
	int height;
	int channels;

public:
	int getHeight() const { return height; }
	int getWidth() const { return width; }
	int getChannels() const { return channels; }

	float getValue(int x, int y, int c=0) const {
		return data[(y * width + x) * channels + c];
	}
	void setValue(float v, int x, int y, int c=0) {
		data[(y * width + x) * channels + c] = v;
	}

	FloatImage(QImage& img) {
		width = img.width();
		height = img.height();
		channels = 3;
		data.resize(width * height * channels);
		for(int y = 0; y < height; y++) {
			for(int x = 0; x < width; x++) {
				QRgb color = img.pixel(x, y);
				data[(y * width + x) * 3 + 0] = qRed(color) / 255.0;
				data[(y * width + x) * 3 + 1] = qGreen(color) / 255.0;
				data[(y * width + x) * 3 + 2] = qBlue(color) / 255.0;
			}
		}
	}

	FloatImage(int w, int h, int c=3) : data(w * h * c) {
		width = w;
		height = h;
		channels = c;
	}

	QImage convert() const {
		QImage img(width, height, QImage::Format_RGB32);
		for(int y = 0; y < height; y++) {
			for(int x = 0; x < width; x++) {
				QRgb color;
				if(channels == 3) {
					color = qRgb(	data[(y * width + x) * 3 + 0] * 255,
									data[(y * width + x) * 3 + 1] * 255,
									data[(y * width + x) * 3 + 2] * 255);
				} else {
					color = qRgb(	data[y * width + x] * 255,
									data[y * width + x] * 255,
									data[y * width + x] * 255);
				}
				img.setPixel(x, y, color);
			}
		}
		return img;
	}

	void makeGrey() {
		if(channels == 1) return;
		channels = 1;
		vector<float> new_data(width * height);
		for(int y = 0; y < height; y++) {
			for(int x = 0; x < width; x++) {
				float s =	data[(y * width + x) * 3 + 0] +
							data[(y * width + x) * 3 + 1] +
							data[(y * width + x) * 3 + 2];
				new_data[y * width + x] = s / 3;
			}
		}
		data = new_data;
	}

	void saturate() {
		for(float& x: data) x = (x > 1) ? 1 : (x < 0) ? 0 : x;
	}

	void square() {
		for(float& x: data) x *= x;
	}
	void squareRoot() {
		for(float& x: data) x = sqrt(x);
	}
	void sign() {
		for(float& x: data) x = (x > 0) - (x < 0);
	}



	FloatImage& operator*=(const FloatImage& other) {
		assert(data.size() == other.data.size());
		for(unsigned int i = 0; i < data.size(); i++) data[i] *= other.data[i];
		return *this;
	}
	FloatImage& operator*=(float f) {
		for(unsigned int i = 0; i < data.size(); i++) data[i] *= f;
		return *this;
	}
	FloatImage& operator+=(const FloatImage& other) {
		assert(data.size() == other.data.size());
		for(unsigned int i = 0; i < data.size(); i++) data[i] += other.data[i];
		return *this;
	}

};




class LinearMask {
	vector<float> mask;
	int	offset;

	LinearMask(vector<float> m, int o) {
		mask = m;
		offset = o;
	}
public:
	void normalize() {
		double sum = 0;
		for(float x: mask) sum += x;
		for(float& x: mask) x /= sum;
	}

	float apply(const FloatImage& img, int x, int y, int c, bool isVertical) {
		float ans = 0;
		int xx = x;
		int yy = y;
		for(unsigned int i = 0; i < mask.size(); i++) {
			if(isVertical) {
				yy = y + i + offset;
				if(yy < 0) yy = 0;
				if(yy >= img.getHeight()) yy = img.getHeight() - 1;
			} else {
				xx = x + i + offset;
				if(xx < 0) xx = 0;
				if(xx >= img.getWidth()) xx = img.getWidth() - 1;
			}
			ans += img.getValue(xx, yy, c) * mask[i];
		}
		return ans;
	}

	static LinearMask createNabla() {
		vector<float> m(2);
		m[0] = -1;
		m[1] = 1;
		return LinearMask(m, -1);
	}

	static LinearMask createLaplace() {
		vector<float> m(3);
		m[0] = 1;
		m[1] = -2;
		m[2] = 1;
		return LinearMask(m, -1);
	}

	static LinearMask createGaussianMask(float sigma) {
		int ll = 4 * sigma;
		int mask_len = ll * 2 + 1;
		vector<float> mask(mask_len);
		for(int x = 0; x <= ll; x++) {
			float h = exp(-x * x / (2 * sigma * sigma)) / (sigma * sqrt(2 * M_PI));
			mask[ll + x] = h;
			mask[ll - x] = h;
		}
		LinearMask g(mask, -ll);
		g.normalize();
		return g;
	}

};







class RectMask {
	vector<float> mask;
	int width;
	int	offset_x;
	int	offset_y;

	RectMask(vector<float> m, int w, int ox, int oy) {
		width = w;
		mask = m;
		offset_x = ox;
		offset_y = oy;
	}
public:
	void normalize() {
		double sum = 0;
		for(float x: mask) sum += x;
		for(float& x: mask) x /= sum;
	}

	float apply(const FloatImage& img, int x, int y, int c) {
		int height = mask.size() / width;
		float ans = 0;

		for(int iy = 0; iy < height; iy++) {

			int yy = iy + y + offset_y;
			if(yy < 0) yy = 0;
			if(yy >= img.getHeight()) yy = img.getHeight() - 1;

			for(int ix = 0; ix < width; ix++) {

				int xx = ix + x + offset_x;
				if(xx < 0) xx = 0;
				if(xx >= img.getWidth()) xx = img.getWidth() - 1;


				ans += img.getValue(xx, yy, c) * mask[iy * width + ix];
			}
		}
		return ans;
	}

	static RectMask createLXYMask() {
		vector<float> m(4);
		m[0] = 1;
		m[1] = -1;
		m[2] = -1;
		m[3] = 1;
		return RectMask(m, 2, -1, -1);
	}

	static RectMask createLaplaceMask() {
		vector<float> m(9);
		m[0] = 0;
		m[1] = 1;
		m[2] = 0;
		m[3] = 1;
		m[4] = -4;
		m[5] = 1;
		m[6] = 0;
		m[7] = 1;
		m[8] = 0;
		return RectMask(m, 3, -1, -1);
	}

	static RectMask createImprovedLaplaceMask() {
		vector<float> m(9);
		m[0] = 1;
		m[1] = 1;
		m[2] = 1;
		m[3] = 1;
		m[4] = -8;
		m[5] = 1;
		m[6] = 1;
		m[7] = 1;
		m[8] = 1;
		return RectMask(m, 3, -1, -1);
	}

};



class Filter {
public:
	virtual FloatImage apply(const FloatImage&) = 0;
};


class SimpleLinearMaskFilter : public Filter {
	bool isVertical;
	LinearMask mask;

public:
	SimpleLinearMaskFilter(LinearMask m, bool v) : mask(m) { isVertical = v; }
	void flipIsVertical() { isVertical = !isVertical; }

	virtual FloatImage apply(const FloatImage& src_img) {
		FloatImage img(src_img.getWidth(), src_img.getHeight(), src_img.getChannels());
		for(int y = 0; y < img.getHeight(); y++) {
			for(int x = 0; x < img.getWidth(); x++) {
				for(int c = 0; c < img.getChannels(); c++) {
					img.setValue(mask.apply(src_img, x, y, c, isVertical), x, y, c);
				}
			}
		}
		return img;
	}

	float apply(const FloatImage& src_img, int x, int y, int c=0) {
		return mask.apply(src_img, x, y, c, isVertical);
	}
};





class SimpleRectMaskFilter : public Filter {
	RectMask mask;

public:
	SimpleRectMaskFilter(RectMask m) : mask(m) {}

	virtual FloatImage apply(const FloatImage& src_img) {
		FloatImage img(src_img.getWidth(), src_img.getHeight(), src_img.getChannels());
		for(int y = 0; y < img.getHeight(); y++) {
			for(int x = 0; x < img.getWidth(); x++) {
				for(int c = 0; c < img.getChannels(); c++) {
					img.setValue(mask.apply(src_img, x, y, c), x, y, c);
				}
			}
		}
		return img;
	}

	float apply(const FloatImage& src_img, int x, int y, int c=0) {
		return mask.apply(src_img, x, y, c);
	}
};





class SimpleLinearSeperableMaskFilter : public Filter {
	SimpleLinearMaskFilter filter;
public:
	SimpleLinearSeperableMaskFilter(LinearMask mask) : filter(mask, true) {}
	virtual FloatImage apply(const FloatImage& src_img) {
		FloatImage img = filter.apply(src_img);
		filter.flipIsVertical();
		img = filter.apply(img);
		return img;
	}
};



class SimpleShockFilter : public Filter {
	unsigned int iterations;
	SimpleLinearMaskFilter nxf;
	SimpleLinearMaskFilter nyf;
	SimpleRectMaskFilter lf;
	SimpleLinearSeperableMaskFilter gsf;
	SimpleLinearSeperableMaskFilter tmpGauss;

	void iteration(FloatImage& img) {
		img = tmpGauss.apply(img);

		FloatImage nabs = nxf.apply(img);
		nabs.square();

		FloatImage temp = nyf.apply(img);
		temp.square();

		nabs += temp;
		nabs *= 0.5;
		nabs.squareRoot();

		FloatImage laplace = lf.apply(img);
		laplace.sign();
		laplace *= -1;
		laplace *= nabs;
		img += laplace;
		img.saturate();
	}

public:
	SimpleShockFilter(float s, unsigned int i)
	: nxf(LinearMask::createNabla(), false)
	, nyf(LinearMask::createNabla(), true)
	, lf(RectMask::createImprovedLaplaceMask())
	, gsf(LinearMask::createGaussianMask(s))
	, tmpGauss(LinearMask::createGaussianMask(1)) { iterations = i; }

	virtual FloatImage apply(const FloatImage& src_img) {
		FloatImage img = gsf.apply(src_img);
		for(unsigned int i = 0; i < iterations; i++) iteration(img);
		return img;
	}
};



class ImprovedShockFilter : public Filter {
	float alpha;
	float sigma;
	float roh;
	float omikron;
	unsigned int iterations;
	SimpleLinearMaskFilter	nxf;
	SimpleLinearMaskFilter	nyf;
	SimpleLinearMaskFilter	lxxf;
	SimpleRectMaskFilter	lxyf;
	SimpleLinearMaskFilter	lyyf;
	SimpleLinearSeperableMaskFilter gsf;
	SimpleLinearSeperableMaskFilter grf;
	SimpleLinearSeperableMaskFilter gof;
	SimpleLinearSeperableMaskFilter tmpGauss;

	void iteration(FloatImage& img) {
		img = tmpGauss.apply(img);
		FloatImage input = img;

		// calc nabla
		FloatImage grey_img = input;
		grey_img.makeGrey();
		FloatImage nabla_x = nxf.apply(grey_img);
		FloatImage nabla_y = nyf.apply(grey_img);

		// calc laplace
		grey_img = gsf.apply(grey_img);
		FloatImage dxx = lxxf.apply(grey_img);
		FloatImage dxy = lxyf.apply(grey_img);
		FloatImage dyy = lyyf.apply(grey_img);

		// calc matrix
		FloatImage mat_a = nabla_x;
		FloatImage mat_d = nabla_y;
		FloatImage mat_c = nabla_x;
		mat_c *= nabla_y;
		mat_a.square();
		mat_d.square();

		mat_a = grf.apply(mat_a);
		mat_d = grf.apply(mat_d);
		mat_c = grf.apply(mat_c);

		float sin_alpha = sin(alpha);
		float cos_alpha = cos(alpha);

		for(int y = 0; y < input.getHeight(); y++) {
			for(int x = 0; x < input.getWidth(); x++) {
				float ma = mat_a.getValue(x, y);
				float mc = mat_c.getValue(x, y);
				float md = mat_d.getValue(x, y);

				float a, b; // eigen-vector
				if(mc == 0) b = !(a = (ma > md));
				else {
					float tmp = (ma + md) / 2;
					float eigen = tmp + sqrt(tmp * tmp - ma * md + mc * mc);
					b = (eigen - ma) / mc;
					float scale = sqrt(1 + b * b);
					a = 1 / scale;
					b /= scale;
				}

				float u = a;
				float v = b;
				a = u * cos_alpha - v * sin_alpha;
				b = u * sin_alpha + v * cos_alpha;


				float sign = -(a * a * dxx.getValue(x, y) +
								2 * a * b * dxy.getValue(x, y) +
								b * b * dyy.getValue(x, y));
				sign = (sign > 0) - (sign < 0);

				float nx = nabla_x.getValue(x, y);
				float ny = nabla_y.getValue(x, y);

				float e = sign * sqrt(nx * nx + ny * ny);
				for(int c = 0; c < 3; c++) {
					img.setValue(e + input.getValue(x, y, c), x, y, c);
				}

			}
		}
		img.saturate();
	}

public:
	ImprovedShockFilter(float s, float r, float o, unsigned int i, float alpha=0)
	: alpha(alpha)
	, nxf(LinearMask::createNabla(), false)
	, nyf(LinearMask::createNabla(), true)
	, lxxf(LinearMask::createLaplace(), false)
	, lxyf(RectMask::createLXYMask())
	, lyyf(LinearMask::createLaplace(), true)
	, gsf(LinearMask::createGaussianMask(s))
	, grf(LinearMask::createGaussianMask(r))
	, gof(LinearMask::createGaussianMask(o))
	, tmpGauss(LinearMask::createGaussianMask(1))
	{ iterations = i; }

	virtual FloatImage apply(const FloatImage& src_img) {
		FloatImage img = gof.apply(src_img);
		for(unsigned int i = 0; i < iterations; i++) iteration(img);
		return img;
	}
};



int main(int argc, char* argv[]) {

	// parse options
	string input_file;
	string output_file;
	float alpha;
	float sigma;
	float roh;
	float omikron;
	unsigned int iter;

	po::variables_map vm;
	try {
		po::options_description desc("allowed options");
		desc.add_options()
			("help,h", "produce help message")
			("input,i", po::value<string>(&input_file)->default_value("input.png"), "input file")
			("output,o", po::value<string>(&output_file)->default_value("output.png"), "output file")
			("sigma,s", po::value<float>(&sigma)->default_value(2.0), "sigma")
			("roh,r", po::value<float>(&roh)->default_value(5.0), "roh")
			("omikron,k", po::value<float>(&omikron)->default_value(0.2), "omikron")
			("iter,x", po::value<unsigned int>(&iter)->default_value(5), "iteration count")
			("alpha,a", po::value<float>(&alpha)->default_value(0), "alpha")
			("simple", "just use the simple shock filter instead")
		;
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
	if(vm.count("simple")) {
		SimpleShockFilter(sigma, iter).apply(FloatImage(src_img)).convert().save(output_file.c_str());
	}
	else {
		ImprovedShockFilter(sigma, roh, omikron, iter, alpha / 180 * M_PI).apply(FloatImage(src_img)).convert().save(output_file.c_str());
	}

	return 0;
}

