/*
*
* Copyright © 2017 NXP
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice (including the next
* paragraph) shall be included in all copies or substantial portions of the
* Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

#ifndef HEADERS_EXPOSURE_COMPENSATOR_HPP_
#define HEADERS_EXPOSURE_COMPENSATOR_HPP_

/*******************************************************************************************
 * Includes
 *******************************************************************************************/
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <errno.h>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "cameracalibrator.h"
#include "lines.hpp"
#define CAMERA_HPP_EXIST


using namespace std;
using namespace cv;

/*******************************************************************************************
 * Macros
 *******************************************************************************************/
#define MASK_VAL 255 // Color of mask
#define PREV(x, max)  ((x > 0) ? (x - 1) : (max)) // Previous index
#define NEXT(x, max)  ((x < max) ? (x + 1) : (0)) // Next index

#ifndef Rect2f
	typedef Rect_<float> Rect2f;
#endif

/*******************************************************************************************
 * Global functions
 *******************************************************************************************/
/**************************************************************************************************************
 *
 * @brief  			Search the circumscribed rectangle of white region of mask
 *
 * @param  	in		Mat &mask - mask
 *
 * @return 			Rect - the circumscribed rectangles of white region of input mask
 *
 * @remarks 		The function searches the circumscribed rectangles of input mask white region. It filters
 * 					impulse noise of mask image and detects the first and the last white pixels in X, Y axis.
 *
 **************************************************************************************************************/
extern Rect getRect(Mat &mask);

/*******************************************************************************************
 * Types
 *******************************************************************************************/
struct CompensatorInfo {
	vector<Rect2f> roi;
	double radius;
	Mat mask;
};

/*******************************************************************************************
 * Classes
 *******************************************************************************************/
/* Compensator class */
class Compensator {
	public:
		/**************************************************************************************************************
		 *
		 * @brief  			Compensator class constructor.
		 *
		 * @param  in 		Size mask_size	-	size of compensator mask (screen size)
		 *
		 * @return 			The function create the Compensator object.
		 *
		 * @remarks 		The function sets cinf.mask property of new Compensator object.
		 *
		 **************************************************************************************************************/
		Compensator(Size mask_size);
		
		/**************************************************************************************************************
		 *
		 * @brief  			Get coordinates of rectangle reflection across the x-axis.
		 *
		 * @param  in 		uint index	-	rectangle index
		 *
		 * @return 			Rect - rectangle reflection across the x-axis.
		 *
		 * @remarks 		The function reflect rectangle across the x-axis.
		 *
		 **************************************************************************************************************/	
		Rect getFlipROI(uint index);

#ifdef CAMERA_HPP_EXIST
		/**************************************************************************************************************
		 *
		 * @brief  			Fill the CompensatorInfo vector private property of Compensator object.
		 *
		 * @param  	in		vector<Camera*> cameras - vector of Camera objects
		 *
		 * @return 			-
		 *
		 * @remarks 		The function fills CompensatorInfo property of Compensator. It calculates mask which
		 * 					defines 4 overlap regions and circumscribed rectangles of each region.
		 *
		 **************************************************************************************************************/
        void feed(vector<CameraCalibrator*> &cameras, vector< vector<Point3f> > &seam_points);
#endif
		/**************************************************************************************************************
		 *
		 * @brief  			Save compensator info
		 *
		 * @param  	in		char* path - path name
		 *
		 * @return 			-
		 *
		 * @remarks 		The function generates grids only for overlap regions which lays on flat bowl bottom for each
		 * 					camera and saves the grids into compensator folder  1 grid per camera. Each grid contained
		 * 					description of two overlap regions: left and right.
		 *					Also the application save the file with texel coordinates of circumscribed rectangle for each
		 *					overlap regions. The texture mapping application uses this information to copy only overlap
		 *					regions from frame buffer when it calculates exposure correction coefficients.
		 *
		 **************************************************************************************************************/
        int save(const char *path);
		/**************************************************************************************************************
		 *
		 * @brief  			Load compensator info
		 *
		 * @param  	in		char* path - path name
		 *
		 * @return 			-
		 *
		 * @remarks 		The function loads texel coordinates of circumscribed rectangle for each overlap region.
		 * 					The texture mapping application uses this information to copy only overlap regions from
		 * 					frame buffer when it calculates exposure correction coefficients.
		 *
		 **************************************************************************************************************/
        int load(const char* path);

	private:
		CompensatorInfo cinf;
};
#endif /* HEADERS_EXPOSURE_COMPENSATOR_HPP_ */

