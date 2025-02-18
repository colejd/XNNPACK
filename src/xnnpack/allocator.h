// Copyright 2019 Google LLC
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
  #include <malloc.h>
#elif !defined(__GNUC__)
  #include <alloca.h>
#endif

#include <xnnpack.h>
#include <xnnpack/common.h>
#include <xnnpack/params.h>


#if XNN_ARCH_WASM
  #define XNN_ALLOCATION_ALIGNMENT 4
#elif XNN_ARCH_X86 || XNN_ARCH_X86_64
  #if XNN_PLATFORM_MOBILE
    #define XNN_ALLOCATION_ALIGNMENT 32
  #else
    #define XNN_ALLOCATION_ALIGNMENT 64
  #endif
#else
  #define XNN_ALLOCATION_ALIGNMENT 16
#endif

XNN_INTERNAL extern const struct xnn_allocator xnn_default_allocator;

inline static void* xnn_allocate_memory(size_t memory_size) {
  return xnn_params.allocator.allocate(xnn_params.allocator.context, memory_size);
}

inline static void* xnn_allocate_zero_memory(size_t memory_size) {
  void* memory_pointer = xnn_params.allocator.allocate(xnn_params.allocator.context, memory_size);
  if (memory_pointer != NULL) {
    memset(memory_pointer, 0, memory_size);
  }
  return memory_pointer;
}

inline static void* xnn_reallocate_memory(void* memory_pointer, size_t memory_size) {
  return xnn_params.allocator.reallocate(xnn_params.allocator.context, memory_pointer, memory_size);
}

inline static void xnn_release_memory(void* memory_pointer) {
  xnn_params.allocator.deallocate(xnn_params.allocator.context, memory_pointer);
}

inline static void* xnn_allocate_simd_memory(size_t memory_size) {
  return xnn_params.allocator.aligned_allocate(xnn_params.allocator.context, XNN_ALLOCATION_ALIGNMENT, memory_size);
}

inline static void* xnn_allocate_zero_simd_memory(size_t memory_size) {
  void* memory_pointer = xnn_params.allocator.aligned_allocate(
    xnn_params.allocator.context, XNN_ALLOCATION_ALIGNMENT, memory_size);
  if (memory_pointer != NULL) {
    memset(memory_pointer, 0, memory_size);
  }
  return memory_pointer;
}

inline static void xnn_release_simd_memory(void* memory_pointer) {
  xnn_params.allocator.aligned_deallocate(xnn_params.allocator.context, memory_pointer);
}

#if defined(__GNUC__) && defined(__BIGGEST_ALIGNMENT__) && (__BIGGEST_ALIGNMENT__ >= XNN_ALLOCATION_ALIGNMENT)
  #define XNN_SIMD_ALLOCA(size) __builtin_alloca((size))
#elif (defined(__clang_major__) && (__clang_major__ >= 4)) || \
    (defined(__GNUC__) && (__GNUC__ >= 5 || __GNUC__ == 4 && __GNUC_MINOR__ >= 7) && !defined(__INTEL_COMPILER))
  #define XNN_SIMD_ALLOCA(size) __builtin_alloca_with_align((size), XNN_ALLOCATION_ALIGNMENT)
#elif defined(__GNUC__)
  #define XNN_SIMD_ALLOCA(size) \
    ((void*) ((((uintptr_t) __builtin_alloca((size) + XNN_ALLOCATION_ALIGNMENT)) | (XNN_ALLOCATION_ALIGNMENT - 1)) + 1))
#elif defined(_MSC_VER)
  #define XNN_SIMD_ALLOCA(size) \
    ((void*) ((((uintptr_t) _alloca((size) + XNN_ALLOCATION_ALIGNMENT)) | (XNN_ALLOCATION_ALIGNMENT - 1)) + 1))
#else
  #define XNN_SIMD_ALLOCA(size) \
    ((void*) ((((uintptr_t) alloca((size) + XNN_ALLOCATION_ALIGNMENT)) | (XNN_ALLOCATION_ALIGNMENT - 1)) + 1))
#endif

#define XNN_DEFAULT_CODE_BUFFER_SIZE 16384  // 16kb.

#ifdef __cplusplus
extern "C" {
#endif

struct xnn_code_buffer {
  // Pointer to allocated, externally managed memory.
  void* code;
  // Actual size of instructions (bytes). It is only safe to access code within
  // this size.
  size_t size;
  // Maximum capacity of the buffer pointer to by `code`. This is the size of
  // the currently mapped memory.
  size_t capacity;
};

// Allocates a code region and associates it with `buf`.
enum xnn_status xnn_allocate_code_memory(struct xnn_code_buffer* buf, size_t size);
// Finalize buffer, users won't need to call this directly, called by Assembler.
enum xnn_status xnn_finalize_code_memory(struct xnn_code_buffer* buf);
// Grows the code memory owned by buf.
enum xnn_status xnn_grow_code_memory(struct xnn_code_buffer* buf);
// Free all memory associated with `buf`.
enum xnn_status xnn_release_code_memory(struct xnn_code_buffer* buf);

#ifdef __cplusplus
}  // extern "C"
#endif
