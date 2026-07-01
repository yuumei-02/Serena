// Copyright (c) 2026 Yuumei (https://github.com/yuumei-02) All Rights Reserved.
// See the LICENSE file for more information.

#ifndef SERENA_HDR
#define SERENA_HDR

#include <stdint.h>
#include <stddef.h>

#ifndef DONT_INCLUDE_MEMORY_UNITS
#define KB 1000
#define MB 1000000
#define GB 1000000000

#define KiB 1024
#define MiB 1048576
#define GiB 1073741824
#endif

#ifndef CUSTOM_PAGE_SIZE
size_t G_page_size = 4096;
#endif

#ifndef nullable
#define nullable
#endif

size_t alignup(size_t byte_length, size_t alignment);
void debug_print_memory_region(void* region, size_t bytes);

typedef struct {
   void* memory;
   size_t length;
   size_t capacity;
} Arena;

/// [capacity] gets upsized to align with [G_page_size] which is typically [4096] bytes.
Arena Arena_new(size_t capacity);

void Arena_delete(nullable Arena* self);
void Arena_reset(nullable Arena* self);

/// Aborts on OOM.
void* Arena_push(Arena* self, size_t bytes, size_t alignment);
/// Returns NULL on OOM or when bytes is 0.
void* Arena_push_no_panic(Arena* self, size_t bytes, size_t alignment);
void Arena_pop(Arena* self, size_t bytes);

#endif

#ifdef SERENA_IMPL
#undef SERENA_IMPL

#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

size_t alignup(size_t byte_length, size_t alignment) {
   assert(alignment > 0 && alignment % 2 == 0);
   return (byte_length + alignment - 1) & ~(alignment - 1);
}

Arena Arena_new(size_t capacity) {
   capacity = alignup(capacity, G_page_size);
   void* memory = mmap(NULL, capacity, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
   if (memory == MAP_FAILED) {
      fprintf(stderr, "[!] Failed to map \"%zu\" bytes of memory in Arena_new", capacity);
      exit(1);
   }

   return (Arena) {
      .memory = memory,
      .capacity = capacity
   };
}

void Arena_delete(nullable Arena* self) {
   if (self == NULL) return;

   munmap(self->memory, self->capacity);
   *self = (Arena) {0};
}

void Arena_reset(nullable Arena* self) {
   if (self == NULL) return;

   self->length = 0;
}

static inline void* Arena_push_impl(Arena* self, size_t bytes, size_t alignment, bool panic) {
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

   void* ptr = (uint8_t*) self->memory + aligned_off;
   self->length = aligned_off + bytes;

   return ptr;
}

void* Arena_push(Arena* self, size_t bytes, size_t alignment) {
   return Arena_push_impl(self, bytes, alignment, true);
}

void* Arena_push_no_panic(Arena* self, size_t bytes, size_t alignment) {
   return Arena_push_impl(self, bytes, alignment, false);
}

void Arena_pop(Arena* self, size_t bytes) {
   assert(self != NULL);

   if (bytes >= self->length)
      self->length = 0;
   else   
      self->length -= bytes;
}

void debug_print_memory_region(void* region, size_t bytes) {
   assert(region != NULL);

   uint8_t* chunk = (uint8_t*) region;
   for (size_t i = 0; i < bytes; ++i) {
      if ((i + 1) % 16 == 1) putc('\n', stdout);
      printf("%02x ", (int) *chunk++);
   }

   putc('\n', stdout);
}

// faebdaed

#endif

