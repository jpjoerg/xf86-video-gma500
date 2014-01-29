#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <xf86drm.h>
#include <xf86.h>
#include <uapi/drm/gma_drm.h>
#include "libgma.h"

struct gma_bo *gma_bo_create(int fd, uint32_t size, uint32_t type, uint32_t flags)
{
	struct drm_gma_gem_create args;
	struct gma_bo *bo;
	int ret;

	bo = calloc(1, sizeof(struct gma_bo));
	if (!bo)
		return NULL;

	memset(&args, 0, sizeof(args));

	args.size = size;
	args.type = type;
	args.flags = flags;

	ret = drmIoctl(fd, DRM_IOCTL_GMA_GEM_CREATE, &args);
	if (ret)
		goto free;

	bo->handle = args.handle;
	bo->size = args.size;
	bo->type = args.type;
	return bo;
free:
	free(bo);
	return NULL;
}

struct gma_bo *gma_bo_create_surface(int fd, uint32_t width, uint32_t height,
				     uint32_t bpp, uint32_t type,
				     uint32_t flags)
{
	struct gma_bo *bo;
	uint32_t pitch = ALIGN(width * ((bpp + 7) / 8), 64);
	uint32_t size = pitch * height;

	bo = gma_bo_create(fd, size, type, flags);
	if (!bo)
		return NULL;

	bo->pitch = pitch;

	return bo;
}

int gma_bo_mmap(int fd, struct gma_bo *bo)
{
	struct drm_gma_gem_mmap args;
	int ret;
	void *map;

	memset(&args, 0, sizeof(args));

	args.handle = bo->handle;

	ret = drmIoctl(fd, DRM_IOCTL_GMA_GEM_MMAP, &args);
	if (ret)
		return ret;

	map = mmap(0, bo->size, PROT_READ | PROT_WRITE, MAP_SHARED,
		   fd, args.offset);

	if (map == MAP_FAILED)
		return -errno;

	bo->ptr = map;

	return 0;
}

int gma_bo_destroy(int fd, struct gma_bo *bo)
{
	struct drm_mode_destroy_dumb args;
	int ret;

	if (bo->ptr) {
		munmap(bo->ptr, bo->size);
		bo->ptr = NULL;
	}

	memset(&args, 0, sizeof(args));

	args.handle = bo->handle;
	ret = drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &args);
	if (ret)
		return -errno;

	free(bo);

	return 0;
}
