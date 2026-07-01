// Copyright (c) 2026 Yuumei (https://github.com/yuumei-02) All Rights Reserved.
// See the LICENSE file for more information.

#ifndef SERENA_HDR
#define SERENA_HDR

#include <stdint.h>
#include <stddef.h>

/// Conditionally upsizes [byte_length] to include padding in order to satisfy memory alignment requirements.
/// This works for all types with alignment requirements up to the alignment of max_align_t which is typically [8 bytes].
/// Use alignup_ex to specify what alignment to alignup from.
size_t alignup(size_t byte_length);
size_t alignup_ex(size_t byte_length, size_t alignment);

#endif

#ifdef SERENA_IMPL
#undef SERENA_IMPL

#include <stdalign.h>

const size_t C_platform_alignment = alignof(max_align_t);

size_t alignup(size_t byte_length) {
   return (byte_length + C_platform_alignment - 1) & ~(C_platform_alignment - 1);
}

size_t alignup_ex(size_t byte_length, size_t alignment) {
   return (byte_length + alignment - 1) & ~(alignment - 1);
}

#endif

