/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2011-2024  University of Bordeaux, CNRS (LaBRI UMR 5800), Inria
 * Copyright (C) 2013-2013  Thibaut Lambert
 * Copyright (C) 2011-2011  Télécom Sud Paris
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

#include <common/config.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <limits.h>
#ifdef STARPU_USE_FXT
#include <common/fxt.h>
#endif
#include <common/utils.h>

#include <starpu.h>
#include <core/perfmodel/perfmodel.h> // we need to browse the list associated to history-based models

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#endif

#define PROGNAME "starpu_perfmodel_plot"

struct _perfmodel_plot_options
{
	/* display all available models */
	int list;
	/* display directory */
	int directory;
	/* what kernel ? */
	char *symbol;
	/* what energy model ? */
	char *energy_symbol;
	/* which combination */
	int comb_is_set;
	int comb;
	/* display all available combinations of a specific model */
	int list_combs;
	int gflops;
	int energy;
	/* Unless a FxT file is specified, we just display the model */
	int with_fxt_file;

	char avg_file_name[256];

#ifdef STARPU_USE_FXT
	struct starpu_fxt_codelet_event *dumped_codelets;
	struct starpu_fxt_options fxt_options;
	char data_file_name[256];
#endif
};

static void usage()
{
	fprintf(stderr, "Draw a graph corresponding to the execution time of a given perfmodel\n");
	fprintf(stderr, "Usage: %s [ options ]\n", PROGNAME);
	fprintf(stderr, "\n");
	fprintf(stderr, "One must specify a symbol with the -s option or use -l or -d\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "   -d			display the directory storing performance models\n");
	fprintf(stderr, "   -l			display all available models\n");
	fprintf(stderr, "   -s <symbol>		specify the symbol\n");
	fprintf(stderr, "   -e			display perfmodel as energy instead of time\n");
	fprintf(stderr, "   -se <time_symbol> <energy_symbol> \n");
	fprintf(stderr, "			specify both a time symbol and an energy symbol\n");
	fprintf(stderr, "   -f			draw GFlop/s instead of time\n");
	fprintf(stderr, "   -i <Fxt files>	input FxT files generated by StarPU\n");
	fprintf(stderr, "   -lc			display all combinations of a given model\n");
	fprintf(stderr, "   -c <combination>	specify the combination (use the option -lc to list all combinations of a given model)\n");
	fprintf(stderr, "   -o <directory>	specify directory in which to create output files (current directory by default)\n");
	fprintf(stderr, "   -h, --help		display this help and exit\n");
	fprintf(stderr, "   -v, --version	output version information and exit\n\n");
	fprintf(stderr, "Report bugs to <%s>.", PACKAGE_BUGREPORT);
	fprintf(stderr, "\n");
}

static void parse_args(int argc, char **argv, struct _perfmodel_plot_options *options, char **directory)
{
	int correct_usage = 0;
	memset(options, 0, sizeof(struct _perfmodel_plot_options));

#ifdef STARPU_USE_FXT
	/* Default options */
	starpu_fxt_options_init(&options->fxt_options);

	free(options->fxt_options.out_paje_path);
	options->fxt_options.out_paje_path = NULL;
	free(options->fxt_options.activity_path);
	options->fxt_options.activity_path = NULL;
	free(options->fxt_options.distrib_time_path);
	options->fxt_options.distrib_time_path = NULL;
	free(options->fxt_options.dag_path);
	options->fxt_options.dag_path = NULL;

	options->fxt_options.dumped_codelets = &options->dumped_codelets;
#endif

	/* We want to support arguments such as "-i trace_*" */
	unsigned reading_input_filenames = 0;

	int i;
	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-s") == 0)
		{
			if (i >= argc-1)
			{
				fprintf(stderr,"-s requires an argument\n");
				usage();
				exit(EXIT_FAILURE);
			}
			options->symbol = argv[++i];
			correct_usage = 1;
			continue;
		}

		if (strcmp(argv[i], "-se") == 0)
		{
			if (i >= argc-2)
			{
				fprintf(stderr,"-se requires two arguments\n");
				usage();
				exit(EXIT_FAILURE);
			}
			options->symbol = argv[++i];
			options->energy_symbol = argv[++i];
			correct_usage = 1;
			continue;
		}

		if (strcmp(argv[i], "-o") == 0)
		{
			free(*directory);
			*directory = strdup(argv[++i]);
#ifdef STARPU_USE_FXT
			options->fxt_options.dir = strdup(*directory);
#endif
			continue;
		}

		if (strcmp(argv[i], "-i") == 0)
		{
			if (i >= argc-1)
			{
				fprintf(stderr,"-i requires an argument\n");
				usage();
				exit(EXIT_FAILURE);
			}
			reading_input_filenames = 1;
#ifdef STARPU_USE_FXT
			options->fxt_options.filenames[options->fxt_options.ninputfiles++] = argv[++i];
			options->with_fxt_file = 1;
#else
			fprintf(stderr, "Warning: FxT support was not enabled in StarPU: FxT traces will thus be ignored!\n");
#endif
			continue;
		}

		if (strcmp(argv[i], "-l") == 0)
		{
			options->list = 1;
			correct_usage = 1;
			continue;
		}

		if (strcmp(argv[i], "-lc") == 0)
		{
			options->list_combs = 1;
			continue;
		}

		if (strcmp(argv[i], "-f") == 0)
		{
			options->gflops = 1;
			continue;
		}

		if (strcmp(argv[i], "-e") == 0)
		{
			options->energy = 1;
			continue;
		}

		if (strcmp(argv[i], "-c") == 0)
		{
			if (i >= argc-1)
			{
				fprintf(stderr,"-c requires an argument\n");
				usage();
				exit(EXIT_FAILURE);
			}
			options->comb_is_set = 1;
			options->comb = atoi(argv[++i]);
			continue;
		}

		if (strcmp(argv[i], "-d") == 0)
		{
			options->directory = 1;
			correct_usage = 1;
			continue;
		}

		if (strcmp(argv[i], "-h") == 0 ||
		    strcmp(argv[i], "--help") == 0)
		{
			usage();
			exit(EXIT_SUCCESS);
		}

		if (strcmp(argv[i], "-v") == 0 ||
		    strcmp(argv[i], "--version") == 0)
		{
			fputs(PROGNAME " (" PACKAGE_NAME ") " PACKAGE_VERSION "\n", stderr);
			exit(EXIT_SUCCESS);
		}

		/* If the reading_input_filenames flag is set, and that the
		 * argument does not match an option, we assume this may be
		 * another filename */
		if (reading_input_filenames)
		{
#ifdef STARPU_USE_FXT
			options->fxt_options.filenames[options->fxt_options.ninputfiles++] = argv[i];
#endif
			continue;
		}
	}

	if (correct_usage == 0)
	{
		fprintf(stderr, "Incorrect usage, aborting\n");
		usage();
		exit(-1);
	}
}

static char *replace_char(char *str, char old, char new)
{
	char *p = strdup(str);
	char *ptr = p;
	while (*ptr)
	{
		if (*ptr == old) *ptr = new;
		ptr ++;
	}
	return p;
}

static void print_comma(FILE *gnuplot_file, int *first)
{
	if (*first)
	{
		*first = 0;
	}
	else
	{
		fprintf(gnuplot_file, ",\\\n\t");
	}
}

static void display_perf_model(FILE *gnuplot_file, struct starpu_perfmodel_arch* arch, struct starpu_perfmodel_per_arch *arch_model, int impl, int *first, struct _perfmodel_plot_options *options)
{
	char arch_name[256];
	const char *factor;

	if (options->energy)
		factor = "";
	else
		factor = "0.001 * ";

	starpu_perfmodel_get_arch_name(arch, arch_name, 256, impl);

#ifdef STARPU_USE_FXT
	if (options->with_fxt_file && impl == 0)
	{
		if (options->gflops)
		{
			_STARPU_DISP("gflops unit selected, ignoring fxt trace\n");
		}
		else
		{
			char *arch_name2 = replace_char(arch_name, '_', '-');
			print_comma(gnuplot_file, first);
			fprintf(gnuplot_file, "\"< grep '^%s' %s\" using 3:4 title \"Profiling %s\"", arch_name, options->data_file_name, arch_name2);
			free(arch_name2);
		}
	}
#endif

	/* Only display the regression model if we could actually build a model */
	if (!options->gflops && arch_model->regression.valid && !arch_model->regression.nl_valid)
	{
		print_comma(gnuplot_file, first);

		fprintf(stderr, "\tLinear: y = alpha size ^ beta\n");
		fprintf(stderr, "\t\talpha = %e\n", arch_model->regression.alpha * 0.001);
		fprintf(stderr, "\t\tbeta = %e\n", arch_model->regression.beta);

		fprintf(gnuplot_file, "%s%g * x ** %g title \"Linear Regression %s\"", factor,
			arch_model->regression.alpha, arch_model->regression.beta, arch_name);
	}

	if (!options->gflops && arch_model->regression.nl_valid)
	{
		print_comma(gnuplot_file, first);

		fprintf(stderr, "\tNon-Linear: y = a size ^b + c\n");
		fprintf(stderr, "\t\ta = %e\n", arch_model->regression.a * 0.001);
		fprintf(stderr, "\t\tb = %e\n", arch_model->regression.b);
		fprintf(stderr, "\t\tc = %e\n", arch_model->regression.c * 0.001);

		fprintf(gnuplot_file, "%s%g * x ** %g + %s%g title \"Non-Linear Regression %s\"", factor,
			arch_model->regression.a, arch_model->regression.b, factor, arch_model->regression.c, arch_name);
	}
}

static void display_history_based_perf_models(FILE *gnuplot_file, struct starpu_perfmodel *model, struct starpu_perfmodel *energy_model, int *first, struct _perfmodel_plot_options *options)
{
	FILE *datafile;
	struct starpu_perfmodel_history_list *ptr;
	char arch_name[32];
	int col;
	unsigned long minimum = 0;

	datafile = fopen(options->avg_file_name, "w");
	col = 2;

	int i;
	for(i = 0; i < model->state->ncombs; i++)
	{
		int comb = model->state->combs[i];
		if (options->comb_is_set == 0 || options->comb == comb)
		{
			struct starpu_perfmodel_arch *arch;
			int impl;

			arch = starpu_perfmodel_arch_comb_fetch(comb);
			for(impl = 0; impl < model->state->nimpls[comb]; impl++)
			{
				struct starpu_perfmodel_per_arch *arch_model = &model->state->per_arch[comb][impl];
				starpu_perfmodel_get_arch_name(arch, arch_name, 32, impl);

				if (arch_model->list)
				{
					char *arch_name2 = replace_char(arch_name, '_', '-');
					print_comma(gnuplot_file, first);
					fprintf(gnuplot_file, "\"%s\" using 1:%d:%d with errorlines title \"Average %s\"", options->avg_file_name, col, col+1, arch_name2);
					col += 2;
					free(arch_name2);
				}
			}
		}
	}

	/* Dump entries in size order */
	while (1)
	{
		unsigned long last = minimum;

		minimum = ULONG_MAX;
		/* Get the next minimum */
		for(i = 0; i < model->state->ncombs; i++)
		{
			int comb = model->state->combs[i];
			if (options->comb_is_set == 0 || options->comb == comb)
			{
				int impl;
				for(impl = 0; impl < model->state->nimpls[comb]; impl++)
				{
					struct starpu_perfmodel_per_arch *arch_model = &model->state->per_arch[comb][impl];
					for (ptr = arch_model->list; ptr; ptr = ptr->next)
					{
						unsigned long size = ptr->entry->size;
						if (size > last && size < minimum)
							minimum = size;
					}
				}
			}
		}
		if (minimum == ULONG_MAX)
			break;

		fprintf(stderr, "%lu ", minimum);
		fprintf(datafile, "%-15lu ", minimum);
		/* Find that minimum */
		for(i = 0; i < model->state->ncombs; i++)
		{
			int comb = model->state->combs[i];
			if (options->comb_is_set == 0 || options->comb == comb)
			{
				int impl;

				for(impl = 0; impl < model->state->nimpls[comb]; impl++)
				{
					int found = 0;
					struct starpu_perfmodel_per_arch *arch_model = &model->state->per_arch[comb][impl];
					for (ptr = arch_model->list; ptr; ptr = ptr->next)
					{
						struct starpu_perfmodel_history_entry *entry = ptr->entry;
						if (entry->size == minimum)
						{
							if (options->energy_symbol)
							{
								/* Look for the same in the energy model */

								if (impl >= energy_model->state->nimpls[comb])
									/* Doesn't have measurements for this impl */
									break;

								struct starpu_perfmodel_per_arch *arch_model2 = &energy_model->state->per_arch[comb][impl];
								struct starpu_perfmodel_history_list *ptr2;
								for (ptr2 = arch_model2->list; ptr2; ptr2 = ptr2->next)
								{
									struct starpu_perfmodel_history_entry *entry2 = ptr2->entry;
									if (entry2->size == minimum)
									{
										/* Found the same size, can print */

										double rel_delta = sqrt(
											(entry2->deviation * entry2->deviation) / (entry2->mean * entry2->mean)
											+ (entry->deviation * entry->deviation) / (entry->mean * entry->mean));

										fprintf(datafile, "\t%-15le\t%-15le", entry2->mean / (entry->mean / 1000000),
												entry2->mean / (entry->mean / 1000000) * rel_delta);
										found = 1;
										break;
									}
								}

							}
							else
							{
								if (options->gflops)
									if (options->energy)
										fprintf(datafile, "\t%-15le\t%-15le", entry->flops / entry->mean / 1000000000,
											entry->flops * entry->deviation / (entry->mean * entry->mean) / 1000000000
											);
									else
										fprintf(datafile, "\t%-15le\t%-15le", entry->flops / (entry->mean * 1000),
											entry->flops * entry->deviation / (entry->mean * entry->mean * 1000)
											);
								else
									if (options->energy)
										fprintf(datafile, "\t%-15le\t%-15le", entry->mean, entry->deviation);
									else
										fprintf(datafile, "\t%-15le\t%-15le", 0.001*entry->mean, 0.001*entry->deviation);
								found = 1;
							}
							break;
						}
					}
					if (!found && arch_model->list)
						/* No value for this arch. */
						fprintf(datafile, "\t\"\"\t\"\"");
				}
			}
		}
		fprintf(datafile, "\n");
	}
	fprintf(stderr, "\n");

	fclose(datafile);
}

static void display_all_perf_models(FILE *gnuplot_file, struct starpu_perfmodel *model, int *first, struct _perfmodel_plot_options *options)
{
	int i;
	for(i = 0; i < model->state->ncombs; i++)
	{
		int comb = model->state->combs[i];
		if (options->comb_is_set == 0 || options->comb == comb)
		{
			struct starpu_perfmodel_arch *arch;
			int impl;

			arch = starpu_perfmodel_arch_comb_fetch(comb);
			for(impl = 0; impl < model->state->nimpls[comb]; impl++)
			{
				struct starpu_perfmodel_per_arch *archmodel = &model->state->per_arch[comb][impl];
				display_perf_model(gnuplot_file, arch, archmodel, impl, first, options);
			}
		}
	}
}

#ifdef STARPU_USE_FXT
static void dump_data_file(FILE *data_file, struct _perfmodel_plot_options *options)
{
	int i;
	for (i = 0; i < options->fxt_options.dumped_codelets_count; i++)
	{
		/* Dump only if the codelet symbol matches user's request (with or without the machine name) */
		char *tmp = strdup(options->symbol);
		char *dot = strchr(tmp, '.');
		if (dot) tmp[strlen(tmp)-strlen(dot)] = '\0';
		if ((strncmp(options->dumped_codelets[i].symbol, options->symbol, (FXT_MAX_PARAMS - 4)*sizeof(unsigned long)-1) == 0)
		    || (strncmp(options->dumped_codelets[i].symbol, tmp, (FXT_MAX_PARAMS - 4)*sizeof(unsigned long)-1) == 0))
		{
			char *archname = options->dumped_codelets[i].perfmodel_archname;
			size_t size = options->dumped_codelets[i].size;
			float time = options->dumped_codelets[i].time;

			fprintf(data_file, "%s	%f	%f\n", archname, (float)size, time);
		}
		free(tmp);
	}
	free(options->dumped_codelets);
}
#endif

static void display_selected_models(FILE *gnuplot_file, struct starpu_perfmodel *model, struct starpu_perfmodel *energy_model, struct _perfmodel_plot_options *options)
{
	char hostname[64];
	char *symbol = replace_char(options->symbol, '_', '-');

	_starpu_gethostname(hostname, sizeof(hostname));
	fprintf(gnuplot_file, "#!/usr/bin/gnuplot -persist\n");
	fprintf(gnuplot_file, "\n");
	fprintf(gnuplot_file, "set term postscript eps enhanced color\n");
	fprintf(gnuplot_file, "set output \"starpu_%s%s.eps\"\n", options->energy_symbol?"power_":options->gflops?"gflops_":"", options->symbol);
	fprintf(gnuplot_file, "set title \"Model for codelet %s on %s\"\n", symbol, hostname);
	fprintf(gnuplot_file, "set xlabel \"Total data size\"\n");
	if (options->energy_symbol)
		fprintf(gnuplot_file, "set ylabel \"Power (W)\"\n");
	else if (options->gflops)
		if (options->energy)
			fprintf(gnuplot_file, "set ylabel \"GFlop/s/W\"\n");
		else
			fprintf(gnuplot_file, "set ylabel \"GFlop/s\"\n");
	else
		if (options->energy)
			fprintf(gnuplot_file, "set ylabel \"Energy (J)\"\n");
		else
			fprintf(gnuplot_file, "set ylabel \"Time (ms)\"\n");
	fprintf(gnuplot_file, "\n");
	fprintf(gnuplot_file, "set key top left\n");
	fprintf(gnuplot_file, "set logscale x\n");
	fprintf(gnuplot_file, "set logscale y\n");
	fprintf(gnuplot_file, "\n");

	/* If no input data is given to gnuplot, we at least need to specify an
	 * arbitrary range. */
	if (options->with_fxt_file == 0 || options->gflops)
		fprintf(gnuplot_file, "set xrange [1 < * < 10**9 : 1 < * < 10**9]\n\n");

	int first = 1;
	fprintf(gnuplot_file, "plot\t");

	/* display all or selected combinations */
	if (!options->energy_symbol)
		display_all_perf_models(gnuplot_file, model, &first, options);
	display_history_based_perf_models(gnuplot_file, model, energy_model, &first, options);

	fprintf(gnuplot_file, "\nset term png\n");
	fprintf(gnuplot_file, "set output \"starpu_%s%s.png\"\n", options->energy_symbol?"power_":options->gflops?"gflops_":"", options->symbol);
	fprintf(gnuplot_file, "replot\n");
	free(symbol);
}

int main(int argc, char **argv)
{
	int ret = 0;
	struct starpu_perfmodel model = { .type = STARPU_PERFMODEL_INVALID };
	struct starpu_perfmodel energy_model = { .type = STARPU_PERFMODEL_INVALID };
	char gnuplot_file_name[256];
	struct _perfmodel_plot_options options;
	char *directory = strdup("./");

#if defined(_WIN32) && !defined(__CYGWIN__)
	WSADATA wsadata;
	WSAStartup(MAKEWORD(1,0), &wsadata);
#endif

	parse_args(argc, argv, &options, &directory);
	starpu_drivers_preinit();
	starpu_perfmodel_initialize();

	if (options.directory)
	{
		starpu_perfmodel_directory(stdout);
	}
	else if (options.list)
	{
		ret = starpu_perfmodel_list(stdout);
		if (ret)
		{
			_STARPU_DISP("The performance model directory is invalid\n");
		}
	}
	else
	{
		/* Load the performance model associated to the symbol */
		ret = starpu_perfmodel_load_symbol(options.symbol, &model);
		if (options.energy_symbol)
			ret = starpu_perfmodel_load_symbol(options.energy_symbol, &energy_model);
		if (ret)
		{
			_STARPU_DISP("The performance model for the symbol <%s> could not be loaded\n", options.symbol);
		}
		else if (options.list_combs)
		{
			ret = starpu_perfmodel_list_combs(stdout, &model);
			if (ret)
			{
					fprintf(stderr, "Error when listing combinations for model <%s>\n", options.symbol);
				}
		}
		else
		{
			/* If some FxT input was specified, we put the points on the graph */
#ifdef STARPU_USE_FXT
			if (options.with_fxt_file)
			{
				starpu_fxt_generate_trace(&options.fxt_options);

				snprintf(options.data_file_name, sizeof(options.data_file_name), "%s/starpu_%s.data", directory, options.symbol);

				FILE *data_file = fopen(options.data_file_name, "w+");
				STARPU_ASSERT(data_file);
				dump_data_file(data_file, &options);
				fclose(data_file);
			}
#endif

			if (options.energy_symbol)
			{
				snprintf(gnuplot_file_name, sizeof(gnuplot_file_name), "%s/starpu_power_%s.gp", directory, options.symbol);
				snprintf(options.avg_file_name, sizeof(options.avg_file_name), "%s/starpu_power_%s_avg.data", directory, options.symbol);
			}
			else if (options.gflops)
			{
				snprintf(gnuplot_file_name, sizeof(gnuplot_file_name), "%s/starpu_gflops_%s.gp", directory, options.symbol);
				snprintf(options.avg_file_name, sizeof(options.avg_file_name), "%s/starpu_gflops_%s_avg.data", directory, options.symbol);
			}
			else
			{
				snprintf(gnuplot_file_name, sizeof(gnuplot_file_name), "%s/starpu_%s.gp", directory, options.symbol);
				snprintf(options.avg_file_name, sizeof(options.avg_file_name), "%s/starpu_%s_avg.data", directory, options.symbol);
			}

			FILE *gnuplot_file = fopen(gnuplot_file_name, "w+");
			STARPU_ASSERT_MSG(gnuplot_file, "Cannot create file <%s>\n", gnuplot_file_name);
			display_selected_models(gnuplot_file, &model, &energy_model, &options);
			fprintf(gnuplot_file,"\n");
			fclose(gnuplot_file);

			/* Retrieve the current mode of the gnuplot executable */
			struct stat sb;
			ret = stat(gnuplot_file_name, &sb);
			if (ret)
			{
				perror("stat");
				STARPU_ABORT();
			}

			/* Make the gnuplot script executable */
			ret = chmod(gnuplot_file_name, sb.st_mode|S_IXUSR
#ifdef S_IXGRP
								 |S_IXGRP
#endif
#ifdef S_IXOTH
								 |S_IXOTH
#endif
								 );
			if (ret)
			{
				perror("chmod");
				STARPU_ABORT();
			}
			_STARPU_DISP("Gnuplot file <%s> generated\n", gnuplot_file_name);
		}
		starpu_perfmodel_unload_model(&model);
		if (options.energy_symbol)
			starpu_perfmodel_unload_model(&energy_model);
	}
	starpu_perfmodel_free_sampling();
	free(directory);
#ifdef STARPU_USE_FXT
	free(options.fxt_options.dir);
	starpu_fxt_options_shutdown(&options.fxt_options);
#endif
	return ret;
}
