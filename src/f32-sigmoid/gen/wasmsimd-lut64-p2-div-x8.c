// Auto-generated file. Do not edit!
//   Template: src/f32-sigmoid/wasmsimd-lut64-p2-div.c.in
//   Generator: tools/xngen
//
// Copyright 2020 Google LLC
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#include <assert.h>

#include <wasm_simd128.h>

#include <xnnpack/common.h>
#include <xnnpack/vunary.h>


extern XNN_INTERNAL const float xnn_table_exp2minus_k_over_64[64];

void xnn_f32_sigmoid_ukernel__wasmsimd_lut64_p2_div_x8(
    size_t n,
    const float* x,
    float* y,
    const void* params) XNN_DISABLE_TSAN
{
  assert(n % sizeof(float) == 0);

  const v128_t vmagic_bias = wasm_f32x4_splat(0x1.800000p17f);
  const v128_t vminus_log2e = wasm_f32x4_splat(-0x1.715476p0f);
  const v128_t vindex_mask = wasm_i32x4_splat(INT32_C(0x3F));
  const v128_t vln2_hi = wasm_f32x4_splat(0x1.630000p-1f);
  const v128_t vln2_lo = wasm_f32x4_splat(-0x1.BD0106p-13f);
  const v128_t vc2 = wasm_f32x4_splat(0x1.FFFF0Ap-2f);
  const v128_t vone = wasm_f32x4_splat(1.0f);
  const v128_t vdenorm_cutoff = wasm_f32x4_splat(0x1.5D589Ep+6f);

  for (; n >= 8 * sizeof(float); n -= 8 * sizeof(float)) {
    const v128_t vx0123 = wasm_v128_load(x);
    const v128_t vx4567 = wasm_v128_load(x + 4);
    x += 8;

    const v128_t vz0123 = wasm_f32x4_abs(vx0123);
    const v128_t vz4567 = wasm_f32x4_abs(vx4567);

    v128_t vn0123 = wasm_f32x4_add(vmagic_bias, wasm_f32x4_mul(vz0123, vminus_log2e));
    v128_t vn4567 = wasm_f32x4_add(vmagic_bias, wasm_f32x4_mul(vz4567, vminus_log2e));

    const v128_t ve0123 = wasm_i32x4_shl(vn0123, 17);
    const v128_t ve4567 = wasm_i32x4_shl(vn4567, 17);

    const v128_t vidx0123 = wasm_i32x4_shl(wasm_v128_and(vn0123, vindex_mask), 2);
    const v128_t vidx4567 = wasm_i32x4_shl(wasm_v128_and(vn4567, vindex_mask), 2);

    const uint64_t vidx01 = wasm_i64x2_extract_lane(vidx0123, 0);
    const uint64_t vidx23 = wasm_i64x2_extract_lane(vidx0123, 1);
    const float vl0   = *((const float*) ((uintptr_t) xnn_table_exp2minus_k_over_64 + (uint32_t) vidx01));
    const float vl1 = *((const float*) ((uintptr_t) xnn_table_exp2minus_k_over_64 + (uint32_t) (vidx01 >> 32)));
    const float vl2 = *((const float*) ((uintptr_t) xnn_table_exp2minus_k_over_64 + (uint32_t) vidx23));
    const float vl3 = *((const float*) ((uintptr_t) xnn_table_exp2minus_k_over_64 + (uint32_t) (vidx23 >> 32)));
    const v128_t vl0123 = wasm_f32x4_make(vl0, vl1, vl2, vl3);
    const uint64_t vidx45 = wasm_i64x2_extract_lane(vidx4567, 0);
    const uint64_t vidx67 = wasm_i64x2_extract_lane(vidx4567, 1);
    const float vl4   = *((const float*) ((uintptr_t) xnn_table_exp2minus_k_over_64 + (uint32_t) vidx45));
    const float vl5 = *((const float*) ((uintptr_t) xnn_table_exp2minus_k_over_64 + (uint32_t) (vidx45 >> 32)));
    const float vl6 = *((const float*) ((uintptr_t) xnn_table_exp2minus_k_over_64 + (uint32_t) vidx67));
    const float vl7 = *((const float*) ((uintptr_t) xnn_table_exp2minus_k_over_64 + (uint32_t) (vidx67 >> 32)));
    const v128_t vl4567 = wasm_f32x4_make(vl4, vl5, vl6, vl7);

    const v128_t vs0123 = wasm_i32x4_add(vl0123, ve0123);
    const v128_t vs4567 = wasm_i32x4_add(vl4567, ve4567);

    vn0123 = wasm_f32x4_sub(vn0123, vmagic_bias);
    vn4567 = wasm_f32x4_sub(vn4567, vmagic_bias);

    v128_t vt0123 = wasm_f32x4_add(vz0123, wasm_f32x4_mul(vn0123, vln2_hi));
    v128_t vt4567 = wasm_f32x4_add(vz4567, wasm_f32x4_mul(vn4567, vln2_hi));

    vt0123 = wasm_f32x4_add(vt0123, wasm_f32x4_mul(vn0123, vln2_lo));
    vt4567 = wasm_f32x4_add(vt4567, wasm_f32x4_mul(vn4567, vln2_lo));

    v128_t vp0123 = wasm_f32x4_mul(vt0123, vc2);
    v128_t vp4567 = wasm_f32x4_mul(vt4567, vc2);

    vp0123 = wasm_f32x4_sub(vt0123, wasm_f32x4_mul(vp0123, vt0123));
    vp4567 = wasm_f32x4_sub(vt4567, wasm_f32x4_mul(vp4567, vt4567));

    const v128_t vy0123 = wasm_f32x4_sub(vs0123, wasm_f32x4_mul(vs0123, vp0123));
    const v128_t vy4567 = wasm_f32x4_sub(vs4567, wasm_f32x4_mul(vs4567, vp4567));

    const v128_t vd0123 = wasm_f32x4_add(vy0123, vone);
    const v128_t vd4567 = wasm_f32x4_add(vy4567, vone);

    v128_t vf0123 = wasm_f32x4_div(vy0123, vd0123);
    v128_t vf4567 = wasm_f32x4_div(vy4567, vd4567);

    vf0123 = wasm_v128_andnot(vf0123, wasm_f32x4_gt(vz0123, vdenorm_cutoff));
    vf4567 = wasm_v128_andnot(vf4567, wasm_f32x4_gt(vz4567, vdenorm_cutoff));

    vf0123 = __builtin_wasm_signselect_i32x4(vf0123, wasm_f32x4_sub(vone, vf0123), vx0123);
    vf4567 = __builtin_wasm_signselect_i32x4(vf4567, wasm_f32x4_sub(vone, vf4567), vx4567);

    wasm_v128_store(y, vf0123);
    wasm_v128_store(y + 4, vf4567);
    y += 8;
  }
  for (; n >= 4 * sizeof(float); n -= 4 * sizeof(float)) {
    const v128_t vx = wasm_v128_load(x);
    x += 4;

    const v128_t vz = wasm_f32x4_abs(vx);

    v128_t vn = wasm_f32x4_add(vmagic_bias, wasm_f32x4_mul(vz, vminus_log2e));
    const v128_t ve = wasm_i32x4_shl(vn, 17);

    const v128_t vidx = wasm_i32x4_shl(wasm_v128_and(vn, vindex_mask), 2);
    const uint64_t vidx_lo = wasm_i64x2_extract_lane(vidx, 0);
    const uint64_t vidx_hi = wasm_i64x2_extract_lane(vidx, 1);
    const float vl0 = *((const float*) ((uintptr_t) xnn_table_exp2minus_k_over_64 + (uint32_t) vidx_lo));
    const float vl1 = *((const float*) ((uintptr_t) xnn_table_exp2minus_k_over_64 + (uint32_t) (vidx_lo >> 32)));
    const float vl2 = *((const float*) ((uintptr_t) xnn_table_exp2minus_k_over_64 + (uint32_t) vidx_hi));
    const float vl3 = *((const float*) ((uintptr_t) xnn_table_exp2minus_k_over_64 + (uint32_t) (vidx_hi >> 32)));
    const v128_t vl = wasm_f32x4_make(vl0, vl1, vl2, vl3);

    const v128_t vs = wasm_i32x4_add(vl, ve);
    vn = wasm_f32x4_sub(vn, vmagic_bias);

    v128_t vt = wasm_f32x4_add(vz, wasm_f32x4_mul(vn, vln2_hi));
    vt = wasm_f32x4_add(vt, wasm_f32x4_mul(vn, vln2_lo));

    v128_t vp = wasm_f32x4_mul(vt, vc2);
    vp = wasm_f32x4_sub(vt, wasm_f32x4_mul(vp, vt));

    const v128_t vy = wasm_f32x4_sub(vs, wasm_f32x4_mul(vs, vp));
    const v128_t vd = wasm_f32x4_add(vy, vone);

    v128_t vf = wasm_f32x4_div(vy, vd);
    vf = wasm_v128_andnot(vf, wasm_f32x4_gt(vz, vdenorm_cutoff));
    vf = __builtin_wasm_signselect_i32x4(vf, wasm_f32x4_sub(vone, vf), vx);

    wasm_v128_store(y, vf);
    y += 4;
  }
  if XNN_UNLIKELY(n != 0) {
    const v128_t vx = wasm_v128_load(x);

    const v128_t vz = wasm_f32x4_abs(vx);

    v128_t vn = wasm_f32x4_add(vmagic_bias, wasm_f32x4_mul(vz, vminus_log2e));
    const v128_t ve = wasm_i32x4_shl(vn, 17);

    const v128_t vidx = wasm_i32x4_shl(wasm_v128_and(vn, vindex_mask), 2);
    const uint64_t vidx_lo = wasm_i64x2_extract_lane(vidx, 0);
    const uint64_t vidx_hi = wasm_i64x2_extract_lane(vidx, 1);
    const float vl0 = *((const float*) ((uintptr_t) xnn_table_exp2minus_k_over_64 + (uint32_t) vidx_lo));
    const float vl1 = *((const float*) ((uintptr_t) xnn_table_exp2minus_k_over_64 + (uint32_t) (vidx_lo >> 32)));
    const float vl2 = *((const float*) ((uintptr_t) xnn_table_exp2minus_k_over_64 + (uint32_t) vidx_hi));
    const float vl3 = *((const float*) ((uintptr_t) xnn_table_exp2minus_k_over_64 + (uint32_t) (vidx_hi >> 32)));
    const v128_t vl = wasm_f32x4_make(vl0, vl1, vl2, vl3);

    const v128_t vs = wasm_i32x4_add(vl, ve);
    vn = wasm_f32x4_sub(vn, vmagic_bias);

    v128_t vt = wasm_f32x4_add(vz, wasm_f32x4_mul(vn, vln2_hi));
    vt = wasm_f32x4_add(vt, wasm_f32x4_mul(vn, vln2_lo));

    v128_t vp = wasm_f32x4_mul(vt, vc2);
    vp = wasm_f32x4_sub(vt, wasm_f32x4_mul(vp, vt));

    const v128_t vy = wasm_f32x4_sub(vs, wasm_f32x4_mul(vs, vp));
    const v128_t vd = wasm_f32x4_add(vy, vone);

    v128_t vf = wasm_f32x4_div(vy, vd);
    vf = wasm_v128_andnot(vf, wasm_f32x4_gt(vz, vdenorm_cutoff));
    vf = __builtin_wasm_signselect_i32x4(vf, wasm_f32x4_sub(vone, vf), vx);

    if (n & (2 * sizeof(float))) {
      *((double*) y) = wasm_f64x2_extract_lane(vf, 0);
      vf = wasm_v32x4_shuffle(vf, vf, 2, 3, 2, 3);
      y += 2;
    }
    if (n & (1 * sizeof(float))) {
      *y = wasm_f32x4_extract_lane(vf, 0);
    }
  }
}
