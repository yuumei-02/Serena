// Copyright (c) 2026 Yuumei (https://github.com/yuumei-02) All Rights Reserved.
// See the LICENSE file for more information.

#ifndef SERENA_HDR
#define SERENA_HDR

#include <stdint.h>
#include <stddef.h>

#ifndef SERENA_DEF
#define SERENA_DEF
#endif

#ifndef DONT_INCLUDE_MEMORY_UNITS
#define KB 1000
#define MB 1000000
#define GB 1000000000

#define KiB 1024
#define MiB 1048576
#define GiB 1073741824
#endif

extern size_t G_page_size;

#ifndef nullable
#define nullable
#endif

typedef enum : int {
   MP_Read  = 2,
   MP_Write = 4,
   MP_Exec  = 6,
   MP_None  = 0,
} MemoryProtection;

typedef struct {
   void* buffer;
   size_t length;
   size_t capacity;
} Arena;

SERENA_DEF size_t alignup(size_t bytes, size_t alignment);
SERENA_DEF size_t alignup_fast_power_of_2(size_t bytes, size_t alignment);
SERENA_DEF void debug_print_memory_region(void* region, size_t bytes);

/// [capacity] gets upsized to align with [G_page_size] which is typically [4096] bytes.
SERENA_DEF Arena Arena_new(size_t capacity);
SERENA_DEF Arena Arena_new_ex(size_t capacity, MemoryProtection protection);
SERENA_DEF Arena Arena_from_parent_allocator(void* buffer, size_t capacity);

/// Does not handle the deletion of arena's derived from other allocators.
SERENA_DEF void Arena_delete(nullable Arena* self);
SERENA_DEF void Arena_reset(nullable Arena* self);

/// Aborts on OOM.
SERENA_DEF void* Arena_push(Arena* self, size_t bytes, size_t alignment);
/// Returns NULL on OOM or when bytes is 0.
SERENA_DEF void* Arena_push_no_panic(Arena* self, size_t bytes, size_t alignment);
SERENA_DEF void Arena_pop(Arena* self, size_t bytes);

#endif

#ifdef SERENA_IMPL
#undef SERENA_IMPL

#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#ifndef CUSTOM_PAGE_SIZE
size_t G_page_size = 4096;
#endif

SERENA_DEF size_t alignup(size_t bytes, size_t alignment) {
   if (alignment == 0) return bytes;

   size_t remainder = bytes % alignment;

   if (remainder == 0)
      return bytes;
   else
      return bytes + (alignment - remainder);
}

SERENA_DEF size_t alignup_fast_power_of_2(size_t bytes, size_t alignment) {
   assert(alignment > 0 && alignment % 2 == 0);
   return (bytes + alignment - 1) & ~(alignment - 1);
}

SERENA_DEF Arena Arena_new(size_t capacity) {
   return Arena_new_ex(capacity, MP_Read | MP_Write);
}

SERENA_DEF Arena Arena_new_ex(size_t capacity, MemoryProtection protection) {
   capacity = alignup(capacity, G_page_size);

   int prot = 0;
   if (protection & MP_None) {
      prot = PROT_NONE;
   } else {
      prot = protection & MP_Exec  ? (prot | PROT_EXEC)  : prot;
      prot = protection & MP_Read  ? (prot | PROT_READ)  : prot;
      prot = protection & MP_Write ? (prot | PROT_WRITE) : prot;
   }
   
   void* buffer = mmap(NULL, capacity, protection, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
   if (buffer == MAP_FAILED) {
      fprintf(stderr, "[!] Failed to map \"%zu\" bytes of memory in Arena_new", capacity);
      exit(1);
   }

   return (Arena) {
      .buffer = buffer,
      .capacity = capacity
   };
}

SERENA_DEF Arena Arena_from_parent_allocator(void* buffer, size_t capacity) {
   assert(capacity > 0);

   return (Arena) {
      .buffer = buffer,
      .capacity = capacity,
   };
}

SERENA_DEF void Arena_delete(nullable Arena* self) {
   if (self == NULL) return;

   munmap(self->buffer, self->capacity);
   *self = (Arena) {0};
}

SERENA_DEF void Arena_reset(nullable Arena* self) {
   if (self == NULL) return;

   self->length = 0;
}

SERENA_DEF void* Arena_push_impl(Arena* self, size_t bytes, size_t alignment, bool panic) {
   assert(self != NULL);
   if (panic) assert(bytes > 0);
   if (bytes == 0) return NULL;

   uintptr_t aligned_off = alignup(self->length, alignment);

   if (aligned_off + bytes >= self->capacity) {
      if (panic) {
         fprintf(stderr, "[!] OOM, failed to allocate \"%zu\" bytes from arena", bytes);
         exit(1);
      } else {
         return NULL;
      }
   }

   void* ptr = (uint8_t*) self->buffer + aligned_off;
   self->length = aligned_off + bytes;

   return ptr;
}

SERENA_DEF void* Arena_push(Arena* self, size_t bytes, size_t alignment) {
   return Arena_push_impl(self, bytes, alignment, true);
}

SERENA_DEF void* Arena_push_no_panic(Arena* self, size_t bytes, size_t alignment) {
   return Arena_push_impl(self, bytes, alignment, false);
}

SERENA_DEF void Arena_pop(Arena* self, size_t bytes) {
   assert(self != NULL);

   if (bytes >= self->length)
      self->length = 0;
   else   
      self->length -= bytes;
}

SERENA_DEF void debug_print_memory_region(void* region, size_t bytes) {
   assert(region != NULL);

   uint8_t* chunk = (uint8_t*) region;
   for (size_t i = 0; i < bytes; ++i) {
      printf("%02x ", (int) *chunk++);
      if ((i + 1) % 16 == 0) putc('\n', stdout);
   }

   if (bytes == 0 || bytes % 16)
      putc('\n', stdout);
}

#endif

