/* StarPU --- Runtime system for heterogeneous multicore architectures.
 *
 * Copyright (C) 2013 Corentin Salingue
 * Copyright (C) 2015, 2016 CNRS
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

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdint.h>
#ifdef HAVE_AIO_H
#include <aio.h>
#endif
#include <errno.h>

#include <common/config.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include <starpu.h>
#include <core/disk.h>
#include <core/perfmodel/perfmodel.h>
#include <core/disk_ops/unistd/disk_unistd_global.h>
#include <datawizard/copy_driver.h>
#include <datawizard/memory_manager.h>
#include <starpu_parameters.h>

#ifdef STARPU_HAVE_WINDOWS
#  include <io.h>
#endif

#define NITER	_starpu_calibration_minimum

#ifdef O_DIRECT
#  define MEM_SIZE getpagesize()
#else
#  define MEM_SIZE 1
#endif

#define MAX_OPEN_FILES 64
#define TEMP_HIERARCHY_DEPTH 2

/* TODO: on Linux, use io_submit */

static int starpu_unistd_opened_files;

#ifdef HAVE_AIO_H
struct starpu_unistd_aiocb {
	struct aiocb aiocb;
	struct starpu_unistd_global_obj *obj;
};
#endif

/* ------------------- use UNISTD to write on disk -------------------  */

static void _starpu_unistd_init(struct starpu_unistd_global_obj *obj, int descriptor, char *path, size_t size)
{
	STARPU_HG_DISABLE_CHECKING(starpu_unistd_opened_files);
	if (starpu_unistd_opened_files >= MAX_OPEN_FILES)
	{
		/* Too many opened files, avoid keeping this one opened */
		close(descriptor);
		descriptor = -1;
	}
	else
		(void) STARPU_ATOMIC_ADD(&starpu_unistd_opened_files, 1);

	STARPU_PTHREAD_MUTEX_INIT(&obj->mutex, NULL);

	obj->descriptor = descriptor;
	obj->path = path;
	obj->size = size;
}

static int _starpu_unistd_reopen(struct starpu_unistd_global_obj *obj)
{
	int id = open(obj->path, obj->flags);
	STARPU_ASSERT(id >= 0);
	return id;
}

static void _starpu_unistd_reclose(int id)
{
	close(id);
}

static void _starpu_unistd_close(struct starpu_unistd_global_obj *obj)
{
	if (obj->descriptor < 0)
		return;

	if (starpu_unistd_opened_files < MAX_OPEN_FILES)
		(void) STARPU_ATOMIC_ADD(&starpu_unistd_opened_files, -1);

	close(obj->descriptor);
}

static void _starpu_unistd_fini(struct starpu_unistd_global_obj *obj)
{
	STARPU_PTHREAD_MUTEX_DESTROY(&obj->mutex);

	free(obj->path);
	free(obj);
}

/* allocation memory on disk */
void *starpu_unistd_global_alloc(struct starpu_unistd_global_obj *obj, void *base, size_t size)
{
	int id;
	char *baseCpy = _starpu_mktemp_many(base, TEMP_HIERARCHY_DEPTH, obj->flags, &id);

	/* fail */
	if (!baseCpy)
	{
		free(obj);
		return NULL;
	}

	int val = ftruncate(id,size);
	/* fail */
	if (val < 0)
	{
		_STARPU_DISP("Could not truncate file, ftruncate failed with error '%s'\n", strerror(errno));
		close(id);
		unlink(baseCpy);
		free(baseCpy);
		free(obj);
		return NULL;
	}

	_starpu_unistd_init(obj, id, baseCpy, size);

	return obj;
}

/* free memory on disk */
void starpu_unistd_global_free(void *base STARPU_ATTRIBUTE_UNUSED, void *obj, size_t size STARPU_ATTRIBUTE_UNUSED)
{
	struct starpu_unistd_global_obj *tmp = (struct starpu_unistd_global_obj *) obj;

	_starpu_unistd_close(tmp);
	unlink(tmp->path);
	_starpu_rmtemp_many(tmp->path, TEMP_HIERARCHY_DEPTH);
	_starpu_unistd_fini(tmp);
}

/* open an existing memory on disk */
void *starpu_unistd_global_open(struct starpu_unistd_global_obj *obj, void *base, void *pos, size_t size)
{
	/* create template */
	char *baseCpy = malloc(strlen(base)+1+strlen(pos)+1);
	STARPU_ASSERT(baseCpy != NULL);
	strcpy(baseCpy,(char *) base);
	strcat(baseCpy,(char *) "/");
	strcat(baseCpy,(char *) pos);

	int id = open(baseCpy, obj->flags);
	if (id < 0)
	{
		free(obj);
		free(baseCpy);
		return NULL;
	}

	_starpu_unistd_init(obj, id, baseCpy, size);

	return obj;
}

/* free memory without delete it */
void starpu_unistd_global_close(void *base STARPU_ATTRIBUTE_UNUSED, void *obj, size_t size STARPU_ATTRIBUTE_UNUSED)
{
	struct starpu_unistd_global_obj *tmp = (struct starpu_unistd_global_obj *) obj;

	_starpu_unistd_close(tmp);
	_starpu_unistd_fini(tmp);
}

/* read the memory disk */
int starpu_unistd_global_read(void *base STARPU_ATTRIBUTE_UNUSED, void *obj, void *buf, off_t offset, size_t size)
{
	struct starpu_unistd_global_obj *tmp = (struct starpu_unistd_global_obj *) obj;
	starpu_ssize_t nb;
	int fd = tmp->descriptor;

#ifdef HAVE_PREAD
	if (fd >= 0)
		nb = pread(fd, buf, size, offset);
	else
#endif
	{
		if (fd >= 0)
			STARPU_PTHREAD_MUTEX_LOCK(&tmp->mutex);
		else
			fd = _starpu_unistd_reopen(obj);

		int res = lseek(fd, offset, SEEK_SET);
		STARPU_ASSERT_MSG(res >= 0, "Starpu Disk unistd lseek for read failed: offset %lu got errno %d", (unsigned long) offset, errno);

		nb = read(fd, buf, size);

		if (tmp->descriptor >= 0)
			STARPU_PTHREAD_MUTEX_UNLOCK(&tmp->mutex);
		else
			_starpu_unistd_reclose(fd);

	}

	STARPU_ASSERT_MSG(nb >= 0, "Starpu Disk unistd read failed: size %lu got errno %d", (unsigned long) size, errno);

	return nb;
}

#ifdef HAVE_AIO_H
void *starpu_unistd_global_async_read(void *base STARPU_ATTRIBUTE_UNUSED, void *obj, void *buf, off_t offset, size_t size)
{
        struct starpu_unistd_global_obj *tmp = obj;
        struct starpu_unistd_aiocb *starpu_aiocb = calloc(1,sizeof(*starpu_aiocb));
        struct aiocb *aiocb = &starpu_aiocb->aiocb;
        starpu_aiocb->obj = obj;
        int fd = tmp->descriptor;

        if (fd < 0)
                fd = _starpu_unistd_reopen(obj);

        aiocb->aio_fildes = fd;
        aiocb->aio_offset = offset;
        aiocb->aio_nbytes = size;
        aiocb->aio_buf = buf;
        aiocb->aio_reqprio = 0;
        aiocb->aio_lio_opcode = LIO_NOP;

        if (aio_read(aiocb) < 0)
        {
                free(aiocb);
                if (tmp->descriptor < 0)
                        _starpu_unistd_reclose(fd);
                aiocb = NULL;
        }

        return aiocb;
}
#endif

int starpu_unistd_global_full_read(void *base STARPU_ATTRIBUTE_UNUSED, void *obj, void **ptr, size_t *size)
{
        struct starpu_unistd_global_obj *tmp = (struct starpu_unistd_global_obj *) obj;
	int fd = tmp->descriptor;

	if (fd < 0)
		fd = _starpu_unistd_reopen(obj);
#ifdef STARPU_HAVE_WINDOWS
	*size = _filelength(fd);
#else
	struct stat st;
	fstat(fd, &st);

	*size = st.st_size;
#endif
	if (tmp->descriptor < 0)
		_starpu_unistd_reclose(fd);

	/* Allocated aligned buffer */
	starpu_malloc_flags(ptr, *size, 0);
	return starpu_unistd_global_read(base, obj, *ptr, 0, *size);
}

/* write on the memory disk */
int starpu_unistd_global_write(void *base STARPU_ATTRIBUTE_UNUSED, void *obj, const void *buf, off_t offset, size_t size)
{
	struct starpu_unistd_global_obj *tmp = (struct starpu_unistd_global_obj *) obj;
	int res;
	int fd = tmp->descriptor;

#ifdef HAVE_PWRITE
	if (fd >= 0)
		res = pwrite(fd, buf, size, offset);
	else
#endif
	{
		if (fd >= 0)
			STARPU_PTHREAD_MUTEX_LOCK(&tmp->mutex);
		else
			fd = _starpu_unistd_reopen(obj);

		res = lseek(fd, offset, SEEK_SET);
		STARPU_ASSERT_MSG(res >= 0, "Starpu Disk unistd lseek for write failed: offset %lu got errno %d", (unsigned long) offset, errno);

		res = write(fd, buf, size);

		if (tmp->descriptor >= 0)
			STARPU_PTHREAD_MUTEX_UNLOCK(&tmp->mutex);
		else
			_starpu_unistd_reclose(fd);
	}

	STARPU_ASSERT_MSG(res >= 0, "Starpu Disk unistd write failed: size %lu got errno %d", (unsigned long) size, errno);
	return 0;
}

#ifdef HAVE_AIO_H
void *starpu_unistd_global_async_write(void *base STARPU_ATTRIBUTE_UNUSED, void *obj, void *buf, off_t offset, size_t size)
{
        struct starpu_unistd_global_obj *tmp = obj;
        struct starpu_unistd_aiocb *starpu_aiocb = calloc(1,sizeof(*starpu_aiocb));
        struct aiocb *aiocb = &starpu_aiocb->aiocb;
        starpu_aiocb->obj = obj;
        int fd = tmp->descriptor;

        if (fd < 0)
                fd = _starpu_unistd_reopen(obj);

        aiocb->aio_fildes = fd;
        aiocb->aio_offset = offset;
        aiocb->aio_nbytes = size;
        aiocb->aio_buf = buf;
        aiocb->aio_reqprio = 0;
        aiocb->aio_lio_opcode = LIO_NOP;

        if (aio_write(aiocb) < 0)
        {
                free(aiocb);
                if (tmp->descriptor < 0)
                        _starpu_unistd_reclose(fd);
                aiocb = NULL;
        }

        return aiocb;
}
#endif

int starpu_unistd_global_full_write(void *base STARPU_ATTRIBUTE_UNUSED, void *obj, void *ptr, size_t size)
{
        struct starpu_unistd_global_obj *tmp = (struct starpu_unistd_global_obj *) obj;

        /* update file size to realise the next good full_read */
        if(size != tmp->size)
        {
		int fd = tmp->descriptor;

		if (fd < 0)
			fd = _starpu_unistd_reopen(obj);
		int val = ftruncate(fd,size);
		if (tmp->descriptor < 0)
			_starpu_unistd_reclose(fd);
		STARPU_ASSERT(val == 0);
		tmp->size = size;
        }

	return starpu_unistd_global_write(base, obj, ptr, 0, size);
}

/* create a new copy of parameter == base */
void *starpu_unistd_global_plug(void *parameter, starpu_ssize_t size STARPU_ATTRIBUTE_UNUSED)
{
	char *tmp = malloc(sizeof(char)*(strlen(parameter)+1));
	STARPU_ASSERT(tmp != NULL);
	strcpy(tmp,(char *) parameter);

	{
		struct stat buf;
		if (!(stat(tmp, &buf) == 0 && S_ISDIR(buf.st_mode)))
		{
			_STARPU_ERROR("Directory '%s' does not exist\n", tmp);
		}
	}

	return (void *) tmp;
}

/* free memory allocated for the base */
void starpu_unistd_global_unplug(void *base)
{
	free(base);
}

int get_unistd_global_bandwidth_between_disk_and_main_ram(unsigned node)
{
	int res;
	unsigned iter;
	double timing_slowness, timing_latency;
	double start;
	double end;

	srand(time(NULL));
	char *buf;
	starpu_malloc_flags((void *) &buf, SIZE_DISK_MIN, 0);
	STARPU_ASSERT(buf != NULL);
	memset(buf, 0, SIZE_DISK_MIN);

	/* allocate memory */
	void *mem = _starpu_disk_alloc(node, SIZE_DISK_MIN);
	/* fail to alloc */
	if (mem == NULL)
		return 0;

	struct starpu_unistd_global_obj *tmp = (struct starpu_unistd_global_obj *) mem;

	/* Measure upload slowness */
	start = starpu_timing_now();
	for (iter = 0; iter < NITER; ++iter)
	{
		int fd = tmp->descriptor;

		_starpu_disk_write(STARPU_MAIN_RAM, node, mem, buf, 0, SIZE_DISK_MIN, NULL);

		if (fd < 0)
			fd = _starpu_unistd_reopen(tmp);
#ifdef STARPU_HAVE_WINDOWS
		res = _commit(fd);
#else
		res = fsync(fd);
#endif
		if (tmp->descriptor < 0)
			_starpu_unistd_reclose(fd);

		STARPU_ASSERT_MSG(res == 0, "bandwidth computation failed");
	}
	end = starpu_timing_now();
	timing_slowness = end - start;

	/* free memory */
	starpu_free_flags(buf, SIZE_DISK_MIN, 0);

	starpu_malloc_flags((void *) &buf, MEM_SIZE, 0);
	STARPU_ASSERT(buf != NULL);

	memset(buf, 0, MEM_SIZE);

	/* Measure latency */
	start = starpu_timing_now();
	for (iter = 0; iter < NITER; ++iter)
	{
		int fd = tmp->descriptor;

		_starpu_disk_write(STARPU_MAIN_RAM, node, mem, buf, (rand() % (SIZE_DISK_MIN/MEM_SIZE)) * MEM_SIZE, MEM_SIZE, NULL);

		if (fd < 0)
			fd = _starpu_unistd_reopen(tmp);
#ifdef STARPU_HAVE_WINDOWS
		res = _commit(fd);
#else
		res = fsync(fd);
#endif
		if (tmp->descriptor < 0)
			_starpu_unistd_reclose(fd);

		STARPU_ASSERT_MSG(res == 0, "Latency computation failed");
	}
	end = starpu_timing_now();
	timing_latency = end - start;

	_starpu_disk_free(node, mem, SIZE_DISK_MIN);
	starpu_free_flags(buf, MEM_SIZE, 0);

	_starpu_save_bandwidth_and_latency_disk((NITER/timing_slowness)*1000000, (NITER/timing_slowness)*1000000,
					       timing_latency/NITER, timing_latency/NITER, node);
	return 1;
}

#ifdef HAVE_AIO_H
void starpu_unistd_global_wait_request(void *async_channel)
{
        const struct aiocb *aiocb = async_channel;
        int values = -1;
        int ret, myerrno = EAGAIN;
        while(values < 0 && (myerrno == EAGAIN || myerrno == EINTR))
        {
                /* Wait the answer of the request TIMESTAMP IS NULL */
                values = aio_suspend(&aiocb, 1, NULL);
                myerrno = errno;
        }
        ret = aio_error(aiocb);
        STARPU_ASSERT_MSG(!ret, "aio_error returned %d", ret);
}

int starpu_unistd_global_test_request(void *async_channel)
{
        const struct aiocb *aiocb = async_channel;
        int ret;

#if defined(__GLIBC__) && (__GLIBC__ < 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ < 22))
        /* glibc's aio_error was not threadsafe before glibc 2.22 */
        struct timespec ts = { .tv_sec = 0, .tv_nsec = 0 };
        ret = aio_suspend(&aiocb, 1, &ts);
        if (ret < 0 && (errno == EAGAIN || errno == EINTR))
                return 0;
        STARPU_ASSERT_MSG(!ret, "aio_suspend returned %d %d\n", ret, errno);
#endif
        /* Test the answer of the request */
        ret = aio_error(aiocb);
        if (ret == 0)
                /* request is finished */
                return 1;
        if (ret == EINPROGRESS || ret == EAGAIN)
                return 0;
        /* an error occured */
        STARPU_ABORT_MSG("aio_error returned %d", ret);
}

void starpu_unistd_global_free_request(void *async_channel)
{
        struct starpu_unistd_aiocb *starpu_aiocb = async_channel;
        struct aiocb *aiocb = &starpu_aiocb->aiocb;
        if (starpu_aiocb->obj->descriptor < 0)
                _starpu_unistd_reclose(aiocb->aio_fildes);
        aio_return(aiocb);
        free(starpu_aiocb);
}
#endif
