/*
 * Copyright (C) 2010-2019 (see AUTHORS file for a list of contributors)
 *
 * This file is part of GNSS-SDR.
 *
 * GNSS-SDR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GNSS-SDR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNSS-SDR. If not, see <https://www.gnu.org/licenses/>.
 */

#include "volk_gnsssdr/volk_gnsssdr_malloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * C11 features:
 * see: https://en.cppreference.com/w/c/memory/aligned_alloc
 *
 * MSVC is broken
 * see: https://docs.microsoft.com/en-us/cpp/overview/visual-cpp-language-conformance?view=vs-2019
 * This section:
 * C11 The Universal CRT implemented the parts of the
 * C11 Standard Library that are required by C++17,
 * with the exception of C99 strftime() E/O alternative
 * conversion specifiers, C11 fopen() exclusive mode,
 * and C11 aligned_alloc(). The latter is unlikely to
 * be implemented, because C11 specified aligned_alloc()
 * in a way that's incompatible with the Microsoft
 * implementation of free():
 * namely, that free() must be able to handle highly aligned allocations.
 *
 * We must work around this problem because MSVC is non-compliant!
 */

#if (__STDC_VERSION__ >= 200112L) && !__APPLE__

void *volk_gnsssdr_malloc(size_t size, size_t alignment)
{
#if defined(_MSC_VER)
    void *ptr = _aligned_malloc(size, alignment);
#else
    void *ptr = aligned_alloc(alignment, size);
#endif
    if (ptr == NULL)
        {
            fprintf(stderr, "VOLK_GNSSDR: Error allocating memory (aligned_alloc/_aligned_malloc)\n");
        }
    return ptr;
}


void volk_gnsssdr_free(void *ptr)
{
#if defined(_MSC_VER)
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}


#else

#if HAVE_POSIX_MEMALIGN

void *volk_gnsssdr_malloc(size_t size, size_t alignment)
{
    void *ptr;

    // quoting posix_memalign() man page:
    // "alignment must be a power of two and a multiple of sizeof(void *)"
    // volk_gnsssdr_get_alignment() could return 1 for some machines (e.g. generic_orc)
    if (alignment == 1)
        {
            return malloc(size);
        }

    int err = posix_memalign(&ptr, alignment, size);
    if (err == 0)
        {
            return ptr;
        }
    else
        {
            fprintf(stderr,
                "VOLK_GNSSSDR: Error allocating memory "
                "(posix_memalign: error %d: %s)\n",
                err, strerror(err));
            return NULL;
        }
}


void volk_gnsssdr_free(void *ptr)
{
    free(ptr);
}

#else

// No standard handlers; we'll do it ourselves.
struct block_info
{
    void *real;
};

void *volk_gnsssdr_malloc(size_t size, size_t alignment)
{
    void *real, *user;
    struct block_info *info;

    /* At least align to sizeof our struct */
    if (alignment < sizeof(struct block_info))
        alignment = sizeof(struct block_info);

    /* Alloc */
    real = malloc(size + (2 * alignment - 1));

    /* Get pointer to the various zones */
    user = (void *)((((uintptr_t)real) + sizeof(struct block_info) + alignment - 1) & ~(alignment - 1));
    info = (struct block_info *)(((uintptr_t)user) - sizeof(struct block_info));

    /* Store the info for the free */
    info->real = real;

    /* Return pointer to user */
    return user;
}


void volk_gnsssdr_free(void *ptr)
{
    struct block_info *info;

    /* Get the real pointer */
    info = (struct block_info *)(((uintptr_t)ptr) - sizeof(struct block_info));

    /* Release real pointer */
    free(info->real);
}

#endif /* HAVE_POSIX_MEMALIGN */

#endif /* _POSIX_C_SOURCE >= 200112L && !__APPLE__ */
