#include "bcsr_filters.h"
#include "bcsr_interface.h"
#include "blas_filters.h"
#include "blas_interface.h"

extern size_t allocate_blas_buffer_on_node(data_state *state, uint32_t dst_node);
extern void liberate_blas_buffer_on_node(data_state *state, uint32_t node);
extern void do_copy_blas_buffer_1_to_1(data_state *state, uint32_t src_node, uint32_t dst_node);
extern size_t dump_blas_interface(data_interface_t *interface, void *buffer);


unsigned canonical_block_filter_bcsr(filter *f __attribute__((unused)), data_state *root_data)
{
	unsigned nchunks;

	uint32_t nnz = root_data->interface[0].bcsr.nnz;

	size_t elemsize = root_data->interface[0].bcsr.elemsize;
	uint32_t firstentry = root_data->interface[0].bcsr.firstentry;

	/* size of the tiles */
	uint32_t r = root_data->interface[0].bcsr.r;
	uint32_t c = root_data->interface[0].bcsr.c;

	/* we create as many subdata as there are blocks ... */
	nchunks = nnz;
	
	/* first allocate the children data_state */
	root_data->children = calloc(nchunks, sizeof(data_state));
	ASSERT(root_data->children);

	/* actually create all the chunks */

	/* XXX */
	ASSERT(root_data->per_node[0].allocated);

	/* each chunk becomes a small dense matrix */
	unsigned chunk;
	for (chunk = 0; chunk < nchunks; chunk++)
	{
		uint32_t ptr_offset = c*r*chunk*elemsize;

		unsigned node;
		for (node = 0; node < MAXNODES; node++)
		{
			blas_interface_t *local = &root_data->children[chunk].interface[node].blas;

			local->nx = c;
			local->ny = r;
			local->ld = c;
			local->elemsize = elemsize;

			if (root_data->per_node[node].allocated) {
				uint8_t *nzval = (uint8_t *)(root_data->interface[node].bcsr.nzval);
				local->ptr = (uintptr_t)&nzval[firstentry + ptr_offset];
			}
		}

		struct data_state_t *state = &root_data->children[chunk];
		state->interfaceid = BLAS_INTERFACE;

		state->allocation_method = &allocate_blas_buffer_on_node;
		state->deallocation_method = &liberate_blas_buffer_on_node;
		state->copy_1_to_1_method = &do_copy_blas_buffer_1_to_1;
		state->dump_interface = &dump_blas_interface;

		ASSERT(state->allocation_method);
	}

	return nchunks;

}
