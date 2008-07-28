#ifndef __EAGER_CENTRAL_POLICY_H__
#define __EAGER_CENTRAL_POLICY_H__

#include <core/workers.h>
#include <core/mechanisms/deque_queues.h>

void initialize_eager_center_policy(struct machine_config_s *config, struct sched_policy_s *policy);
//void set_local_queue_eager(struct jobq_s *jobq);
struct jobq_s *get_local_queue_eager(struct sched_policy_s *policy);

#endif // __EAGER_CENTRAL_POLICY_H__
