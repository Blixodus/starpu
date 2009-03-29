#ifndef __HEAT_H__
#define __HEAT_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#ifdef OPENGL_RENDER
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif

#include <starpu_config.h>
#include <starpu.h>

#include <common/timing.h>

#define X	0
#define Y	1

#define DIM	ntheta*nthick

#define RMIN	(150.0f)
#define RMAX	(200.0f)

#define Pi	(3.141592f)

#define NODE_NUMBER(theta, thick)	((thick)+(theta)*nthick)
#define NODE_TO_THICK(n)		((n) % nthick)
#define NODE_TO_THETA(n)		((n) / nthick)

//#define USE_POSTSCRIPT	1

typedef struct point_t {
	float x;
	float y;
} point;

extern void dw_factoLU(float *matA, unsigned size, unsigned ld, unsigned nblocks, unsigned version);
extern void dw_factoLU_tag(float *matA, unsigned size, unsigned ld, unsigned nblocks);
extern void initialize_system(float **A, float **B, unsigned dim, unsigned pinned);

void display_stat_heat(void);

#ifdef OPENGL_RENDER
extern void opengl_render(unsigned _ntheta, unsigned _nthick, float *_result, point *_pmesh, int argc_, char **argv_);
#endif

#endif // __HEAT_H__
