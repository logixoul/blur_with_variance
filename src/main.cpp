#include "precompiled.h"
#include "util.h"
#include <sstream>
#include <algorithm>
#include <cinder/app/Renderer.h>
#include "stefanapp.h"
#include "stuff.h"

int wsx=800,wsy=600;
int scale=4;
int sx=wsx/scale;
int sy=wsy/scale;
Array2D<float> img(sx, sy);
gl::Texture tex;
bool pause = false, pause2 = false;
typedef std::complex<float> Complex;
stringstream out;
Array2D<float> varianceArr(sx, sy);

//typedef double N

struct SApp : StefanApp {
	void setup()
	{
		createConsole();
		setWindowSize(wsx, wsy);
		gl::Texture::Format fmt;
		fmt.setInternalFormat(GL_RGBA16F);
		tex = gl::Texture(sx, sy, fmt);
		glClampColor(GL_CLAMP_VERTEX_COLOR, GL_FALSE);
		glClampColor(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE);
		glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
		reset();

		cout.rdbuf(out.rdbuf());
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
		StefanApp::keyDown(e);
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
			img = separableConvolve<float, WrapModes::DefaultImpl>(img, getGaussianKernel(3, sigmaFromKsize(3.0f)));
			float sum = std::accumulate(img.begin(), img.end(), 0.0f);
			float avg = sum / (float)img.area;
			forxy(img)
			{
				float f = img(p);
				f += .5f - avg;
				f -= .5f;
				f *= 1.1f; // change to *= 2.0f for another effect
				f += .5f;
				f = constrain(f, 0.0f, 1.0f);
				img(p) = f;
			}
			float sum_ = std::accumulate(img.begin(), img.end(), 0.0f);
			float avg_ = sum_ / (float)img.area;
			int r = 5;
			Array2D<float> weights(r*2+1, r*2+1);
			forxy(weights) {
				weights(p) = smoothstep(r, r-1, p.distance(Vec2f(r, r)));
			}
			forxy(varianceArr)
			{
				float sum = 0.0f;
				float sumw = 0.0f;
				for(int i = -r; i <= r; i++)
				{
					for(int j = -r; j <= r; j++)
					{
						float w = weights(i + r, j + r);
						sum += img.wr(p.x + i, p.y + j) * w;
						sumw += w;
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
			forxy(img)
			{
				//img(p) += varianceArr(p)*::niceExpRangeX(mouseX, 0.01, 1.0);
				//img(p) *= .9f;
				img(p) = lerp(img(p), varianceArr(p),
					exp(lmap(constrain(mouseX, 0.0f, 1.0f), 0.0f, 1.0f, log(.0001f), log(1.0f)))
					);//
			}
		} // pause2
		if(pause || pause2)
			Sleep(50);
	}
	void stefanDraw()
	{
		out.str("");

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
				-pow(mouseX*Vec2f(p).distance(Vec2f(sx,sy)/2.0f), 2.0f)
				);
		}

		static gl::Texture tex2(img3.w, img3.h);
		tex2.bind();
		//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, sx, sy, GL_RGB, GL_FLOAT, imgf_rgb.data);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, img3.w, img3.h, GL_LUMINANCE, GL_FLOAT, img3.data);

		gl::draw(tex2, getWindowBounds());

		gl::drawString(out.str(), Vec2f(0, 20));
		//Sleep(constrain(mouseX, 0.0f, 1.0f) * 1000.0f);
	}
	template<class T>
	Array2D<T> switch_rows_cols(Array2D<T> a) {
		auto a2=Array2D<T>(a.h, a.w); forxy(a2) { a2(p) = a(p.y, p.x); }
		return a2;
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

//CINDER_APP_BASIC(SApp, RendererGl)
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow) {	
	try{
		cinder::app::AppBasic::prepareLaunch();														
		cinder::app::AppBasic *app = new SApp;														
		cinder::app::Renderer *ren = new RendererGl;													
		cinder::app::AppBasic::executeLaunch( app, ren, "SApp" );										
		cinder::app::AppBasic::cleanupLaunch();														
		return 0;																					
	}catch(ci::gl::GlslProgCompileExc const& e) {
		cout << "caught: " << endl << e.what() << endl;
		//int dummy;cin>>dummy;
		system("pause");



	}
}
