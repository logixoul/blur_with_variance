#include "precompiled.h"
#include "util.h"
#include <sstream>
#include <algorithm>
#include "gpgpu.h"
#include <cinder/app/Renderer.h>
#include "stefanfw.h"
#include "stuff.h"

int wsx=1280,wsy=720;
int scale=6;
int sx=wsx/::scale;
int sy=wsy/::scale;
Array2D<float> img(sx, sy);
bool pause = false, pause2 = false;
typedef std::complex<float> Complex;
Array2D<float> varianceArr(sx, sy);

struct SApp : App {
	void setup()
	{
		createConsole();
		setWindowSize(wsx, wsy);
		enableDenormalFlushToZero(); 
		disableGLReadClamp();
		reset();

		stefanfw::eventHandler.subscribeToEvents(*this);
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
			float lerpAmount = cfg1::getOpt("lerpAmount", .0031f, []() { return keys['l']; },
				[&]() { return expRange(constrain(mouseX, 0.0f, 1.0f), .001f, 0.009f); });

			//float lerpAmount = fmod(getElapsedSeconds(), 20.0) < 10.0f ? 0.005 : 0.01;
			float f = s1(getElapsedSeconds()*.5);
			float blurAmount = lerp(3.0f, 7.0f, pow(f, 3.0f));
			
			//cout << lerpAmount << endl;
			img = separableConvolve<float, WrapModes::DefaultImpl>(img, getGaussianKernel(7, sigmaFromKsize(blurAmount)));
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
					varianceArr(p) = 1.0f;
				else
					varianceArr(p) = variance / avg;
			}
			forxy(img)
			{
				img(p) = lerp(img(p), varianceArr(p),
					lerpAmount
				);
			}
#if 0
			auto img2 = zeros_like(img);
			vec2 center(img.Size() / 2);
			forxy(img) {
				vec2 fromCenter = vec2(p) - center;
				vec2 v(fromCenter.y, -fromCenter.x);
				v *= .01f;
				v = safeNormalized(v) * pow(length(v), 2.0f);
				aaPoint(img2, vec2(p) + v, img(p));
			}
			img = img2;
#endif

			if (mouseDown_[0])
			{
				vec2 scaledm = vec2(mouseX * (float)sx, mouseY * (float)sy);
				Area a(scaledm, scaledm);
				int r = 5;
				a.expand(r, r);
				for (int x = a.x1; x <= a.x2; x++)
				{
					for (int y = a.y1; y <= a.y2; y++)
					{
						vec2 v = vec2(x, y) - scaledm;
						float w = max(0.0f, 1.0f - length(v) / r);
						w = max(0.0f, w);
						w = 3 * w * w - 2 * w * w * w;
						w = smoothstep(0.0, 0.5, w);
						img.wr(x, y) = lerp(img.wr(x, y), 1.0f, w);
					}
				}
			}
		} // pause2
		if(pause || pause2)
			Sleep(50);
	}
	void stefanDraw()
	{
		gl::clear(Color(0, 0, 0));
		//tex.setMagFilter(GL_NEAREST);
		Array2D<float> img3 = (keys['v']) ? varianceArr.clone() : img.clone();
#if 0
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
		if(0) forxy(img3)
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
#endif
		auto tex = gtex(img3);
		tex = shade2(tex,
			"float f = fetch1();"
			"float fw = fwidth(f);"
			"f = smoothstep(.5 - fw / 2, 0.5 + fw / 2, f);"
			"_out.r = f;"
			, ShadeOpts().scale(::scale)
		);
		tex = redToLuminance(tex);
		gl::setMatricesWindow(getWindowSize(), false);
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