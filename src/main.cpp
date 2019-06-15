#include "precompiled.h"
#include "util.h"
#include <sstream>
#include <algorithm>
#include "gpgpu.h"
#include <cinder/app/Renderer.h>
#include "stefanfw.h"
#include "stuff.h"
#include "simplexnoise.h"

int wsx=1280,wsy=720;
int scale=6;
int sx=wsx/::scale;
int sy=wsy/::scale;
Array2D<float> img(sx, sy);
bool pause = false, pause2 = false;
typedef std::complex<float> Complex;
Array2D<float> varianceArr(sx, sy);
float noiseTimeDim = 0;

struct SApp : App {
	void setup()
	{
		createConsole();
		setWindowSize(wsx, wsy);
		enableDenormalFlushToZero(); 
		disableGLReadClamp();
		reset();
	}
	void update()
	{
		stefanfw::beginFrame();
		stefanUpdate();
		stefanDraw();
		stefanfw::endFrame();
	}
	void reset()
	{
		forxy(img)
		{
			img(p) = ci::randFloat();
		}
	}
	void keyDown(KeyEvent e)
	{
		if(e.getChar() == 'r')
		{
			reset();
		}
		if(e.getChar() == 'p')
		{
			pause = !pause;
		}
		if(e.getChar() == '2')
		{
			pause2 = !pause2;
		}
	}
	void stefanUpdate() {
		if(!pause2) {
			noiseTimeDim++;

			//float lerpAmount = cfg1::getOpt("lerpAmount", .5f, []() { return true; },
			//	[&]() { return expRange(constrain(mouseX, 0.0f, 1.0f), .0001f, 1.0f); });
			//float lerpAmount = fmod(getElapsedSeconds(), 20.0) < 10.0f ? 0.005 : 0.01;
			//float lerpAmount = lerp(0.001, 0.009, pow(s1(getElapsedSeconds() * 0.6f), 3.0f));
			//float lerpAmount = 0.009f;
			//cout << lerpAmount << endl;
			img = separableConvolve<float, WrapModes::DefaultImpl>(img, getGaussianKernel(3, sigmaFromKsize(2.0f)));
			float sum = std::accumulate(img.begin(), img.end(), 0.0f);
			float avg = sum / (float)img.area;
			forxy(img)
			{
				float f = img(p);
				f += .5f - avg;
				f -= .5f;
				f *= 2.0f; // change to *= 1.1f for another effect
				f += .5f;
				f = constrain(f, 0.0f, 1.0f);
				img(p) = f;
			}
			float sum_ = std::accumulate(img.begin(), img.end(), 0.0f);
			float avg_ = sum_ / (float)img.area;
			int r = 5;
			Array2D<float> weights(r*2+1, r*2+1);
			float sumw = 0.0f;
			forxy(weights) {
				weights(p) = smoothstep(r, r-1, distance(vec2(p), vec2(r, r)));
				sumw += weights(p);
			}
			forxy(varianceArr)
			{
				float sum = 0.0f;
				for(int i = -r; i <= r; i++)
				{
					for(int j = -r; j <= r; j++)
					{
						float w = weights(i + r, j + r);
						sum += img.wr(p.x + i, p.y + j) * w;
					}
				}
				float avg = sum / sumw;
				float variance = 0.0f;
				for(int i = -r; i <= r; i++)
				{
					for(int j = -r; j <= r; j++)
					{
						float w = weights(i + r, j + r);
						variance += w * abs(img.wr(p.x + i, p.y + j) - avg);
					}
				}
				if(avg == 0.0f)
					varianceArr(p) = 0.0f;
				else
					varianceArr(p) = variance / avg;
			}
			float nscale = 4 / (float)img.w;
			forxy(img)
			{
				float lerpAmount = scaled_raw_noise_3d(0.001, 0.009, p.x * nscale, p.y * nscale, noiseTimeDim * .01f);
				img(p) = lerp(img(p), varianceArr(p),
					lerpAmount
				);
			}
			auto img2 = zeros_like(img);
			vec2 center(img.Size() / 2);
			forxy(img) {
				vec2 fromCenter = vec2(p) - center;
				vec2 v(fromCenter.y, -fromCenter.x);
				v *= .01f;
				v = safeNormalized(v) * dot(v, v);//pow(length(v), 2.0f);
				aaPoint(img2, vec2(p) + v, img(p));
			}
			img = img2;
		} // pause2
		if(pause || pause2)
			Sleep(50);
	}
	void stefanDraw()
	{
		gl::clear(Color(0, 0, 0));
		//tex.setMagFilter(GL_NEAREST);
		Array2D<float> img3 = (keys['v']) ? varianceArr.clone() : img.clone();
		float min_=*std::min_element(img3.begin(), img3.end());
		float max_=*std::max_element(img3.begin(), img3.end());
		if(min_==max_)
			max_++;
		vector<int> histogram(65535, 0.0f);
		forxy(img3)
		{
			img3(p) = lmap(img3(p), min_, max_, 0.0f, 1.0f);
		}
		forxy(img3)
		{
			int i = (histogram.size() - 1) * img3(p);
			histogram[i]++;
		}
		int i0, i1;
		for(i0 = 0; i0 < histogram.size(); i0++)
		{
			if(histogram[i0] > img3.area * .1f)
				break;
		}
		for(i1 = histogram.size() - 1; i1 >= 0; i1--)
		{
			if(histogram[i1] > img3.area * .1f)
				break;
		}
		float i0_f = lmap((float)i0, 0.0f, (float)(histogram.size() - 1), 0.0f, 1.0f);
		float i1_f = lmap((float)i1, 0.0f, (float)(histogram.size() - 1), 0.0f, 1.0f);
		if(i1_f == i0_f)
			i1_f++;
		forxy(img3)
		{
			float f = lmap(img3(p), i0_f, i1_f, 0.0f, 1.0f);
			//f *= 2.0f * .5f * exp(lmap(mouseX, 0.0f, 1.0f, log(.001f), log(100.0f)));
			//f /= f + 1.0f;
			float x = f;
			//f = x/(x-100.0f*(x-1.0f));
			//f = pow(f, 2.0f);
			img3(p) = f;

			if(0)img3(p) = 100.0f*exp(
				-pow(mouseX*distance(vec2(p),vec2(sx,sy)/2.0f), 2.0f)
				);
		}

		auto tex = gtex(img3);
		tex = shade2(tex,
			"float f = fetch1();"
			"float fw = fwidth(f);"
			"f = smoothstep(.5 - fw / 2, 0.5 + fw / 2, f);"
			"_out.r = f;"
			, ShadeOpts().scale(::scale)
		);
		tex = redToLuminance(tex);
		gl::draw(tex, getWindowBounds());
	}
	float s1(float f) { return .5f + .5f * sin(f); }
	float expRange(float f, float val0, float val1) { return exp(lmap(f, 0.0f, 1.0f, log(val0), log(val1))); }
	void to01(Array2D<float> data)
	{
		float min_=*std::min_element(data.begin(), data.end());
		float max_=*std::max_element(data.begin(), data.end());
		forxy(data)
		{
			data(p) = lmap(data(p), min_, max_, 0.0f, 1.0f);
		}
	}
};

CINDER_APP(SApp, RendererGl)