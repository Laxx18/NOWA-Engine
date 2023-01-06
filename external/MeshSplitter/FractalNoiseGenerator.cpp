/*
 * FractalNoiseGenerator.cpp
 *
 *  Created on: Apr 28, 2011
 *      Author: _1nsane
 */

#include "FractalNoiseGenerator.h"
#include "OgreMath.h"
#include <stdlib.h>

float**
FractalNoiseGenerator::Generate(int iResolution, float fRoughness)
{
	int IMGSIZE=(1<<iResolution);
	float** ppfFNA=new float*[IMGSIZE+1];
	for(int i=0; i<IMGSIZE+1; i++)
		ppfFNA[i]=new float[IMGSIZE+1];

	ppfFNA[0][0]=0;
	ppfFNA[IMGSIZE][0]=0;
	ppfFNA[0][IMGSIZE]=0;
	ppfFNA[IMGSIZE][IMGSIZE]=0;

	int iStep=IMGSIZE;
	float fCurrRoughness=1;
	while(iStep>1)
	{
		fCurrRoughness*=fRoughness;
		for(int x=0; x<IMGSIZE; x+=iStep)
		{
			for(int y=0; y<IMGSIZE; y+=iStep)
			{
				//square step
				float fOffset=Ogre::Math::RangeRandom(-1.0f, 1.0f)*0.5f*fCurrRoughness*(float)iStep/(float)IMGSIZE;
				//calc average of surrounding square
				float fSum=0;
				fSum+=ppfFNA[x][y];
				fSum+=ppfFNA[x+iStep][y];
				fSum+=ppfFNA[x][y+iStep];
				fSum+=ppfFNA[x+iStep][y+iStep];
				//set center point of square
				ppfFNA[x+(iStep>>1)][y+(iStep>>1)]=fOffset+fSum/4.0f;
				//a little hack right here: since this generator is used to create a cutting plane
				//and this very point is the origin of that plane, we thereby make sure,
				//that the cutting plane will not exceed the original geometry
				if(iStep==IMGSIZE)
					ppfFNA[x+(iStep>>1)][y+(iStep>>1)]=0;

				//diamond step
				ppfFNA[x+(iStep>>1)][y]=(ppfFNA[x][y]+ppfFNA[x+iStep][y])/2.0+
					Ogre::Math::RangeRandom(-1.0f, 1.0f)*0.5f*fCurrRoughness*(float)iStep/(float)IMGSIZE;
				ppfFNA[x][y+(iStep>>1)]=(ppfFNA[x][y]+ppfFNA[x][y+iStep])/2.0+
					Ogre::Math::RangeRandom(-1.0f, 1.0f)*0.5f*fCurrRoughness*(float)iStep/(float)IMGSIZE;
				ppfFNA[x+(iStep>>1)][y+iStep]=(ppfFNA[x][y+iStep]+ppfFNA[x+iStep][y+iStep])/2.0+
					Ogre::Math::RangeRandom(-1.0f, 1.0f)*0.5f*fCurrRoughness*(float)iStep/(float)IMGSIZE;
				ppfFNA[x+iStep][y+(iStep>>1)]=(ppfFNA[x+iStep][y]+ppfFNA[x+iStep][y+iStep])/2.0+
					Ogre::Math::RangeRandom(-1.0f, 1.0f)*0.5f*fCurrRoughness*(float)iStep/(float)IMGSIZE;
			}
		}
		iStep>>=1;
	}
	return ppfFNA;
}
