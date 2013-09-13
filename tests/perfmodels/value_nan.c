/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2013  Centre National de la Recherche Scientifique
 *
 * StarPU is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * StarPU is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU Lesser General Public License in COPYING.LGPL for more details.
 */

#include <config.h>
#include <core/perfmodel/perfmodel.h>
#include "../helper.h"

#define STRING "booh"

int _check_number(double val, int nan)
{
	char *filename = tmpnam(NULL);

	/* write the double value in the file followed by a predefined string */
	FILE *f = fopen(filename, "w");
	fprintf(f, "%lf %s\n", val, STRING);
	fclose(f);

	/* read the double value and the string back from the file */
	f = fopen(filename, "r");
	double lat;
	char str[10];
	int x = _starpu_read_double(f, "%lf", &lat);
	int y = fscanf(f, "%s", str);
	fclose(f);

	/* check that what has been read is identical to what has been written */
	int pass;
	pass = (x == 1) && (y == 1);
	pass = pass && strcmp(str, STRING) == 0;
	if (nan)
		pass = pass && isnan(val) && isnan(lat);
	else
		pass = pass && lat == val;
	return pass;
}

int main(int argc, char **argv)
{
	int ret;

	ret = _check_number(42.0, 0);
	FPRINTF(stderr, "%s when reading %lf\n", ret?"Success":"Error", 42.0);

	if (ret)
	{
	     ret = _check_number(NAN, 1);
	     FPRINTF(stderr, "%s when reading %lf\n", ret?"Success":"Error", NAN);
	}

	return ret;
}
