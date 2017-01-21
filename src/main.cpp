#include "precompiled.h"
#include "util.h"
#include <sstream>
#include <algorithm>
#include <cinder/app/Renderer.h>
#include "stuff.h"

int wsx=800,wsy=600;
int scale=4;
int sx=wsx/scale;
int sy=wsy/scale;
Array2D<float> img(sx, sy);
gl::Texture tex;
float mouseX, mouseY;
bool pause = false, pause2 = false;
typedef std::complex<float> Complex;
bool keys[256];
stringstream out;
//typedef double N

struct SApp : AppBasic {
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
		keys[e.getChar()]=true;
	}
	void keyUp(KeyEvent e)
	{
		keys[e.getChar()]=false;
	}
	void mouseDown(MouseEvent e)
	{
	}
	void draw()
	{
		out.str("");

		wsx = getWindowWidth();
		wsy = getWindowHeight();
		mouseX =(float)getMousePos().x / (float)wsx;
		mouseY =(float)getMousePos().y / (float)wsy;
		gl::clear(Color(0, 0, 0));
		static Array2D<float> img2(sx, sy);

		if(!pause2) {
			if(!pause)
			{
				img = gaussianBlur(img, 3);
				float sum = std::accumulate(img.begin(), img.end(), 0.0f);
				float avg = sum / (float)img.area;
				forxy(img)
				{
					float f = img(p);
					f += .5f - avg;
					f = lmap(f, 0.0f, 1.0f, -1.0f, 2.0f);
					f = constrain(f, 0.0f, 1.0f);
					img(p) = f;
				}
			}
			float sum_ = std::accumulate(img.begin(), img.end(), 0.0f);
			float avg_ = sum_ / (float)img.area;
			cout << avg_ << endl;
			forxy(img2)
			{
				int r = 5;
				float sum = 0.0f;
				float sumw = 0.0f;
				for(int i = -r; i <= r; i++)
				{
					for(int j = -r; j <= r; j++)
					{
						
						sum += img.wr(p.x + i, p.y + j);
					}
				}
				float avg = sum / pow(2.0f * (float)r + 1.0f, 2.0f);
				float variance = 0.0f;
				for(int i = -r; i <= r; i++)
				{
					for(int j = -r; j <= r; j++)
					{
						variance += abs(img.wr(p.x + i, p.y + j) - avg);
					}
				}
				if(avg == 0.0f)
					img2(p) = 0.0f;
				else
					img2(p) = /*.1f * mouseX * */ variance / avg;
			}
			forxy(img)
			{
				//img(p) += img2(p);
				//img(p) *= .5f;
				img(p) = lerp(img(p), img2(p),
					exp(lmap(constrain(mouseX, 0.0f, 1.0f), 0.0f, 1.0f, log(.0001f), log(1.0f)))
					);//
			}
		} // pause2
		//tex.setMagFilter(GL_NEAREST);
		Array2D<float> img3 = (keys['v']) ? img2.clone() : img.clone();
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
