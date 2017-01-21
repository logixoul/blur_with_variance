#include <fftw3.h>
#include <cmath>
#include <numeric>
#include <iostream>
#include <string>
#include <cinder/Vector.h>
#include <cinder/gl/GlslProg.h>
#include <cinder/gl/Texture.h>
#include <cinder/app/AppBasic.h>
#include <cinder/Rand.h>
#include <cinder/ip/Resize.h>
#include <boost/foreach.hpp>
#include <complex>

#define foreach BOOST_FOREACH
using namespace ci;
using namespace std;
using namespace ci::app;
