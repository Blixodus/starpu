#include "dw_sparse_cg.h"

/*
 *	Algorithm :
 *		
 *		i = 0
 *		r = b - A x
 *			( d = A x ; r = r - d )
 *		d = r
 *		delta_new = trans(r) r
 *		delta_0 = delta_new
 *
 * 		while (i < i_max && delta_new > eps^2 delta_0) 
 * 		{
 *			q = A d
 *			alpha = delta_new / ( trans(d) q )
 *			x = x + alpha d
 *			if ( i is divisible by 50 )
 *				r = b - A x
 *			else 
 *				r = r - alpha q
 *			delta_old = delta_new
 *			delta_new = trans(r) r
 *			beta = delta_new / delta_old
 *			d = r + beta d
 *			i = i + 1
 * 		}
 */


/*
 *	compute r = b - A x
 *
 *		descr[0] = A, descr[1] = x, descr [2] = r, descr[3] = b
 */

void core_codelet_func_1(data_interface_t *descr, __attribute__((unused)) void *arg)
{
	float *nzval = (float *)descr[0].csr.nzval;
	uint32_t *colind = descr[0].csr.colind;
	uint32_t *rowptr = descr[0].csr.rowptr;

	float *vecx = (float *)descr[1].blas.ptr;
	float *vecr = (float *)descr[2].blas.ptr;
	float *vecb = (float *)descr[3].blas.ptr;

	uint32_t nnz;
	uint32_t nrow;

	nnz = descr[0].csr.nnz;
	nrow = descr[0].csr.nrow;

	unsigned row;
	for (row = 0; row < nrow; row++)
	{
		float tmp = 0.0f;
		unsigned index;

		unsigned firstindex = rowptr[row];
		unsigned lastindex = (row < nrow - 1)?rowptr[row+1]:nnz;

		for (index = firstindex; index < lastindex; index++)
		{
			unsigned col;

			col = colind[index];
			tmp += nzval[index]*vecx[col];
		}

		vecr[row] = vecb[row] - tmp;
	}

	printf("vecr : %f %f %f %f \n", vecr[0], vecr[1], vecr[2], vecr[3]);
}

/*
 *	compute d = r
 *		descr[0] = d, descr[1] = r
 */
void core_codelet_func_2(data_interface_t *descr, __attribute__((unused)) void *arg)
{
	/* simply copy r into d */
	uint32_t nx = descr[0].blas.nx;
	size_t elemsize = descr[0].blas.elemsize;

	ASSERT(descr[0].blas.nx == descr[1].blas.nx);
	ASSERT(descr[0].blas.elemsize == descr[1].blas.elemsize);

	float *src = (float *)descr[1].blas.ptr;
	float *dst = (float *)descr[0].blas.ptr;

	memcpy(dst, src, nx*elemsize);

	printf("src : %f %f %f %f \n", src[0], src[1], src[2], src[3]);
	printf("dst : %f %f %f %f \n", dst[0], dst[1], dst[2], dst[3]);
}

/*
 *	compute delta_new = trans(r) r
 *		delta_0   = delta_new
 *
 *		args = &delta_new, &delta_0
 */

void core_codelet_func_3(data_interface_t *descr, void *arg)
{
	struct cg_problem *pb = arg;
	float dot;
	float *vec;
	uint32_t size;
	
	/* get the vector */
	vec = (float *)descr[0].blas.ptr;
	size = descr[0].blas.nx;

	dot = cblas_sdot (size, vec, 1, vec, 1);

	printf("func 3 : deltanew = %f\n", dot);

	pb->delta_new = dot;
	pb->delta_0 = dot;
}

#if defined (USE_CUBLAS) || defined (USE_CUDA)
void cublas_codelet_func_3(data_interface_t *descr, void *arg)
{
	struct cg_problem *pb = arg;
	float dot;
	float *vec;
	uint32_t size;
	
	/* get the vector */
	vec = (float *)descr[0].blas.ptr;
	size = descr[0].blas.nx;

	dot = cublasSdot (size, vec, 1, vec, 1);

	printf("func 3 : deltanew = %f\n", dot);

	pb->delta_new = dot;
	pb->delta_0 = dot;
}
#endif


/*
 *	compute q with : q = A d
 *
 *		descr[0] = A, descr[1] = d, descr [2] = q
 */

void core_codelet_func_4(data_interface_t *descr, __attribute__((unused)) void *arg)
{
	float *nzval = (float *)descr[0].csr.nzval;
	uint32_t *colind = descr[0].csr.colind;
	uint32_t *rowptr = descr[0].csr.rowptr;

	float *vecd = (float *)descr[1].blas.ptr;
	float *vecq = (float *)descr[2].blas.ptr;

	uint32_t nnz;
	uint32_t nrow;

	nnz = descr[0].csr.nnz;
	nrow = descr[0].csr.nrow;

	unsigned row;
	for (row = 0; row < nrow; row++)
	{
		float tmp = 0.0f;
		unsigned index;

		unsigned firstindex = rowptr[row];
		unsigned lastindex = (row < nrow - 1)?rowptr[row+1]:nnz;

		for (index = firstindex; index < lastindex; index++)
		{
			unsigned col;

			col = colind[index];
			tmp += nzval[index]*vecd[col];
		}

		vecq[row] = tmp;
	}

}

/* 
 *	compute alpha = delta_new / ( trans(d) q )
 *
 * 		descr[0] = d, descr[1] = q
 *		args = &alpha, &delta_new
 */

void core_codelet_func_5(data_interface_t *descr, void *arg)
{
	float dot;
	struct cg_problem *pb = arg;
	float *vecd, *vecq;
	uint32_t size;
	
	/* get the vector */
	vecd = (float *)descr[0].blas.ptr;
	vecq = (float *)descr[1].blas.ptr;

	ASSERT(descr[1].blas.nx == descr[0].blas.nx);
	size = descr[0].blas.nx;

	dot = cblas_sdot(size, vecd, 1, vecq, 1);

	pb->alpha = pb->delta_new / dot;
}

#if defined (USE_CUBLAS) || defined (USE_CUDA)
void cublas_codelet_func_5(data_interface_t *descr, void *arg)
{
	float dot;
	struct cg_problem *pb = arg;
	float *vecd, *vecq;
	uint32_t size;
	
	/* get the vector */
	vecd = (float *)descr[0].blas.ptr;
	vecq = (float *)descr[1].blas.ptr;

	ASSERT(descr[1].blas.nx == descr[0].blas.nx);
	size = descr[0].blas.nx;

	dot = cublasSdot (size, vecd, 1, vecq, 1);

	pb->alpha = pb->delta_new / dot;
}
#endif



/*
 *	compute x = x + alpha d
 *
 * 		descr[0] : x, descr[1] : d
 *		args = &alpha
 */

void core_codelet_func_6(data_interface_t *descr, void *arg)
{
	struct cg_problem *pb = arg;
	float *vecx, *vecd;
	uint32_t size;
	
	/* get the vector */
	vecx = (float *)descr[0].blas.ptr;
	vecd = (float *)descr[1].blas.ptr;

	size = descr[0].blas.nx;

	cblas_saxpy(size, pb->alpha, vecd, 1, vecx, 1);
}

#if defined (USE_CUBLAS) || defined (USE_CUDA)
void cublas_codelet_func_6(data_interface_t *descr, void *arg)
{
	struct cg_problem *pb = arg;
	float *vecx, *vecd;
	uint32_t size;
	
	/* get the vector */
	vecx = (float *)descr[0].blas.ptr;
	vecd = (float *)descr[1].blas.ptr;

	size = descr[0].blas.nx;

	cublasSaxpy (size, pb->alpha, vecd, 1, vecx, 1);
}
#endif

/*
 *	compute r = r - alpha q
 *
 * 		descr[0] : r, descr[1] : q
 *		args = &alpha
 */

void core_codelet_func_7(data_interface_t *descr, void *arg)
{
	struct cg_problem *pb = arg;
	float *vecr, *vecq;
	uint32_t size;
	
	/* get the vector */
	vecr = (float *)descr[0].blas.ptr;
	vecq = (float *)descr[1].blas.ptr;

	size = descr[0].blas.nx;

	cblas_saxpy(size, -pb->alpha, vecq, 1, vecr, 1);
}

#if defined (USE_CUBLAS) || defined (USE_CUDA)
void cublas_codelet_func_7(data_interface_t *descr, void *arg)
{
	struct cg_problem *pb = arg;
	float *vecr, *vecq;
	uint32_t size;
	
	/* get the vector */
	vecr = (float *)descr[0].blas.ptr;
	vecq = (float *)descr[1].blas.ptr;

	size = descr[0].blas.nx;

	cublasSaxpy (size, -pb->alpha, vecq, 1, vecr, 1);
}
#endif

/*
 *	compute delta_old = delta_new
 *		delta_new = trans(r) r
 *		beta = delta_new / delta_old
 *
 * 		descr[0] = r
 *		args = &delta_old, &delta_new, &beta
 */

void core_codelet_func_8(data_interface_t *descr, void *arg)
{
	float dot;
	struct cg_problem *pb = arg;
	float *vecr;
	uint32_t size;
	
	/* get the vector */
	vecr = (float *)descr[0].blas.ptr;
	size = descr[0].blas.nx;

	dot = cblas_sdot(size, vecr, 1, vecr, 1);

	pb->delta_old = pb->delta_new;
	pb->delta_new = dot;
	pb->beta = pb->delta_new/pb->delta_old;

	printf("func 8 : delta old %f new %f\n", pb->delta_old, pb->delta_new);
}

#if defined (USE_CUBLAS) || defined (USE_CUDA)
void cublas_codelet_func_8(data_interface_t *descr, void *arg)
{
	float dot;
	struct cg_problem *pb = arg;
	float *vecr;
	uint32_t size;
	
	/* get the vector */
	vecr = (float *)descr[0].blas.ptr;
	size = descr[0].blas.nx;

	dot = cublasSdot (size, vecr, 1, vecr, 1);

	pb->delta_old = pb->delta_new;
	pb->delta_new = dot;
	pb->beta = pb->delta_new/pb->delta_old;

	printf("func 8 : delta old %f new %f\n", pb->delta_old, pb->delta_new);
}
#endif

/*
 *	compute d = r + beta d
 *
 * 		descr[0] : d, descr[1] : r
 *		args = &beta
 *
 */

void core_codelet_func_9(data_interface_t *descr, void *arg)
{
	struct cg_problem *pb = arg;
	float *vecd, *vecr;
	uint32_t size;
	
	/* get the vector */
	vecd = (float *)descr[0].blas.ptr;
	vecr = (float *)descr[1].blas.ptr;

	size = descr[0].blas.nx;

	/* d = beta d */
	cblas_sscal(size, pb->beta, vecd, 1);

	/* d = r + d */
	cblas_saxpy (size, 1.0f, vecr, 1, vecd, 1);
}

#if defined (USE_CUBLAS) || defined (USE_CUDA)
void cublas_codelet_func_9(data_interface_t *descr, void *arg)
{
	struct cg_problem *pb = arg;
	float *vecd, *vecr;
	uint32_t size;
	
	/* get the vector */
	vecd = (float *)descr[0].blas.ptr;
	vecr = (float *)descr[1].blas.ptr;

	size = descr[0].blas.nx;

	/* d = beta d */
	cublasSscal(size, pb->beta, vecd, 1);

	/* d = r + d */
	cublasSaxpy (size, 1.0f, vecr, 1, vecd, 1);
}
#endif


