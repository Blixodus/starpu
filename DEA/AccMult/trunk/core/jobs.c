#include "jobs.h"

job_t job_create(void)
{
	job_t job;

	job = job_new();
	job->type = CODELET;

	job->where = 0;
	job->cb = NULL;
	job->cl = NULL;
	job->argcb = NULL;
	job->use_tag = 0;
	job->nbuffers = 0;
	job->priority = DEFAULT_PRIO;

	job->cost_model = NULL;
	job->cuda_cost_model = NULL;
	job->core_cost_model = NULL;

	return job;
}
