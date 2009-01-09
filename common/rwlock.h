#ifndef __RWLOCKS_H__
#define __RWLOCKS_H__

#include <stdint.h>
#include "util.h"

typedef struct rw_lock_t {
	uint32_t busy;
	uint8_t writer;
	uint16_t readercnt;
} rw_lock;

void init_rw_lock(rw_lock *lock);
void take_rw_lock_write(rw_lock *lock);
void take_rw_lock_read(rw_lock *lock);
int take_rw_lock_write_try(rw_lock *lock);
int take_rw_lock_read_try(rw_lock *lock);
void release_rw_lock(rw_lock *lock);

///* make sure to have the lock before using that function */
//inline uint8_t rw_lock_is_writer(rw_lock *lock);
//unsigned is_rw_lock_referenced(rw_lock *lock);

#endif
