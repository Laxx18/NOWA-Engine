/*
 * FractalNoiseGenerator.h
 *
 *  Created on: Apr 28, 2011
 *      Author: _1nsane
 */

#ifndef FRACTALNOISEGENERATOR_H_
#define FRACTALNOISEGENERATOR_H_

class FractalNoiseGenerator {
public:
	/*
	 * generate 2D fractal noise using square diamond algorithm
	 * @return float[(1<<iResolution)+1][(1<<iResolution)+1] array with noise values
	 */
	static float** Generate(int iResolution, float fRoughness);
};

#endif /* FRACTALNOISEGENERATOR_H_ */
