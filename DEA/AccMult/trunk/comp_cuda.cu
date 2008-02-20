#include <stdint.h>
#include "comp_cuda.h"
#include "jobs.h"
#include "parameters.h"

//#define MATA(x,y)	(datamatA[(x)+(y)*widthA])
//#define MATB(x,y)	(datamatB[(x)+(y)*widthB])
//#define MATC(x,y)	(datamatC[(x)+(y)*widthC])

#define MATA(x,y)	(datamatA[(x)+__mul24((y),widthA)])
#define MATB(x,y)	(datamatB[(x)+__mul24((y),widthB)])
#define MATC(x,y)	(datamatC[(x)+__mul24((y),widthC)])

//#define DEBUG
#define CHUNCKDEPTH	16 

#define MAXHEIGTH	(GRAIN/GRIDDIMY)
#define MAXWIDTH	(GRAIN/GRIDDIMX)

__shared__ uint32_t Achunk[CHUNCKDEPTH*MAXHEIGTH];
__shared__ uint32_t Bchunk[MAXWIDTH*CHUNCKDEPTH];
__shared__ uint32_t Cchunk[MAXWIDTH*MAXHEIGTH];

extern "C"
__global__ void 
cuda_mult
(
	int * datamatA, unsigned widthA, unsigned xaA, unsigned xbA, unsigned yaA, unsigned ybA,
	uint32_t *datamatB, unsigned widthB, unsigned xaB, unsigned xbB, unsigned yaB, unsigned ybB,
	uint32_t *datamatC, unsigned widthC, unsigned xaC, unsigned xbC, unsigned yaC, unsigned ybC
#ifdef DEBUG
	,int *toto
#endif
)
{	

	unsigned x,y;//,z;
	unsigned localx, localy, localz;
	unsigned nextz;

	int blockwidth = UPDIV( xbC - xaC , gridDim.x );
	int blockheigth = UPDIV( ybC - yaC , gridDim.y );

	int startx = MIN(xaC + blockIdx.x * blockwidth,  xbC);
	int endx   = MIN(xaC + (blockIdx.x+1) * blockwidth,  xbC);
	
	int starty = MIN(yaC + blockIdx.y * blockheigth, ybC);
	int endy   = MIN(yaC + (blockIdx.y+1) * blockheigth, ybC);


	int actual_width = (endx - startx);
	int actual_heigth = (endy - starty);

	/* zero the Cchunk ... */
	int i;
	for (i = threadIdx.x; i < actual_width*actual_heigth; i+= blockDim.x)
	{
		Cchunk[i] = 0;
	}

	__syncthreads();


	/* perform the actual computation */
	for (localz = 0 ; localz < ybB-yaB ; localz += CHUNCKDEPTH)
	{

		/* assert : ybB - yaB == xbA - xaA */
		nextz = MIN(localz+CHUNCKDEPTH, ybB-yaB);

		/* copy local A chunk */
		for (y = starty + threadIdx.y, localy = threadIdx.y;
		     y < endy ;
		     y += blockDim.y, localy += blockDim.y)
		{
			for (x = xaA + localz + threadIdx.x, localx = threadIdx.x; 
			     x < xaA + nextz;
			     x += blockDim.x, localx += blockDim.x)
			{
				//Achunk[localx + localy * CHUNCKDEPTH] = MATA(x, y);
				Achunk[localx + __mul24(localy, CHUNCKDEPTH)] = MATA(x, y);
			}
		}

		/* copy local B chunk */
		for (y = yaB + localz + threadIdx.y, localy = threadIdx.y;
		     y < yaB + nextz;
		     y += blockDim.y, localy += blockDim.y)
		{
			for (x = startx + threadIdx.x, localx = threadIdx.x;
			     x < endx ;
			     x += blockDim.x, localx += blockDim.x)
			{
				//Bchunk[localx + localy*MAXWIDTH] = MATB(x, y);
				Bchunk[localx + __mul24(localy, MAXWIDTH)] = MATB(x, y);
			}
		}

		__syncthreads();

		/* multiply both chunks */
		int index;
		for (localy = threadIdx.y; localy < actual_heigth ; localy += blockDim.y)
		{
			for (localx = threadIdx.x ; localx < actual_width ; localx += blockDim.x) 
			{
				for (index = 0; index < (nextz - localz) ; index++) 
				{
					Cchunk[localx + __umul24(localy, MAXWIDTH)] += 
								__mul24(Achunk[index + localy * CHUNCKDEPTH],
									Bchunk[localx + index * MAXWIDTH]);
				//	Cchunk[localx + localy * MAXWIDTH] +=  
				//		Achunk[index + localy * CHUNCKDEPTH] 
				//		* Bchunk[localx + index * MAXWIDTH];
				}	
			}
		}
		__syncthreads();
	}


	/* put Cchunk back into device memory */
	for (localy = threadIdx.y ; localy < actual_heigth ; localy += blockDim.y)
	{
		for (localx = threadIdx.x; localx < actual_width; localx += blockDim.x)
		{
			MATC(startx + localx, starty + localy) = Cchunk[localx + localy * MAXWIDTH];
		}
	}

	return;
}


