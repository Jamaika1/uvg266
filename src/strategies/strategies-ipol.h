#ifndef STRATEGIES_IPOL_H_
#define STRATEGIES_IPOL_H_
/*****************************************************************************
 * This file is part of uvg266 VVC encoder.
 *
 * Copyright (c) 2021, Tampere University, ITU/ISO/IEC, project contributors
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 * 
 * * Neither the name of the Tampere University or ITU/ISO/IEC nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * INCLUDING NEGLIGENCE OR OTHERWISE ARISING IN ANY WAY OUT OF THE USE OF THIS
 ****************************************************************************/

/**
 * \ingroup Optimization
 * \file
 * Interface for subpixel interpolation functions.
 */

#include "encoder.h"
#include "global.h" // IWYU pragma: keep
#include "uvg266.h"
#include "search_inter.h"

 // AVX2 implementation of horizontal filter reads and
 // writes two rows for luma and four for chroma at a time.
 // Extra vertical padding is added to prevent segfaults.
 // Needs one extra byte for input buffer to prevent ASAN
 // error because AVX2 reads one extra byte in the end.
#define UVG_IPOL_MAX_INPUT_SIZE_LUMA_SIMD   ((UVG_EXT_BLOCK_W_LUMA   + 1) * UVG_EXT_BLOCK_W_LUMA   + 1)
#define UVG_IPOL_MAX_INPUT_SIZE_CHROMA_SIMD ((UVG_EXT_BLOCK_W_CHROMA + 3) * UVG_EXT_BLOCK_W_CHROMA + 1)
#define UVG_IPOL_MAX_IM_SIZE_LUMA_SIMD      ((UVG_EXT_BLOCK_W_LUMA   + 1) * LCU_WIDTH)
#define UVG_IPOL_MAX_IM_SIZE_CHROMA_SIMD    ((UVG_EXT_BLOCK_W_CHROMA + 3) * LCU_WIDTH_C)

// On top of basic interpolation, FME needs one extra
// column and row for ME (left and up). Needs one extra
// byte to prevent ASAN error because AVX2 reads one extra byte.
#define UVG_FME_MAX_INPUT_SIZE_SIMD ((UVG_EXT_BLOCK_W_LUMA + 1) * (UVG_EXT_BLOCK_W_LUMA + 1) + 1)

typedef struct { uvg_pixel *buffer; uvg_pixel *orig_topleft; unsigned stride; unsigned malloc_used; } uvg_extended_block;

typedef void(ipol_blocks_func)(const encoder_control_t * encoder, uvg_pixel *src, int16_t src_stride, int width, int height,
  uvg_pixel filtered[4][LCU_LUMA_SIZE], int16_t hor_intermediate[5][UVG_IPOL_MAX_IM_SIZE_LUMA_SIMD], int8_t fme_level, int16_t hor_first_cols[5][UVG_EXT_BLOCK_W_LUMA + 1],
  int8_t sample_off_x, int8_t sample_off_y);

typedef struct {
  // Source samples
  uvg_pixel *src; // Top-left sample
  int src_w; // Width
  int src_h; // Height
  int src_s; // Stride

  // Requested sampling position, base dimensions, and padding
  int blk_x;
  int blk_y;
  int blk_w; // Width
  int blk_h; // Height
  int pad_l; // Left
  int pad_r; // Right
  int pad_t; // Top
  int pad_b; // Bottom
  int pad_b_simd; // "Don't care" rows in the end. Zeroed out.

  // Buffer for possible extrapolation. Free memory provided by the caller.
  uvg_pixel *buf;

  // Extended block data. These are set by the function.
  uvg_pixel **ext; // Top-left sample with padding
  uvg_pixel **ext_origin; // Top-left sample without padding
  int *ext_s; // Stride
} uvg_epol_args;

typedef void(epol_func)(uvg_epol_args *args);


typedef void(uvg_sample_quarterpel_luma_func)(const encoder_control_t * const encoder, uvg_pixel *src, int16_t src_stride, int width, int height, uvg_pixel *dst, int16_t dst_stride, int8_t hor_flag, int8_t ver_flag, const mv_t mv[2]);
typedef void(uvg_sample_octpel_chroma_func)(const encoder_control_t * const encoder, uvg_pixel *src, int16_t src_stride, int width, int height, uvg_pixel *dst, int16_t dst_stride, int8_t hor_flag, int8_t ver_flag, const mv_t mv[2]);

typedef void(uvg_sample_quarterpel_luma_hi_func)(const encoder_control_t * const encoder, uvg_pixel *src, int16_t src_stride, int width, int height, int16_t *dst, int16_t dst_stride, int8_t hor_flag, int8_t ver_flag, const mv_t mv[2]);
typedef void(uvg_sample_octpel_chroma_hi_func)(const encoder_control_t * const encoder, uvg_pixel *src, int16_t src_stride, int width, int height, int16_t *dst, int16_t dst_stride, int8_t hor_flag, int8_t ver_flag, const mv_t mv[2]);

typedef void(uvg_sample_14bit_quarterpel_luma_func)(const encoder_control_t * const encoder,
  uvg_pixel *src,
  int16_t src_stride,
  int width,
  int height,
  int16_t *dst,
  int16_t dst_stride,
  int8_t hor_flag,
  int8_t ver_flag,
  const mv_t mv[2]);

typedef void(uvg_sample_14bit_octpel_chroma_func)(const encoder_control_t *const encoder,
  uvg_pixel *src,
  int16_t src_stride,
  int width,
  int height,
  int16_t *dst,
  int16_t dst_stride,
  int8_t hor_flag,
  int8_t ver_flag,
  const mv_t mv[2]);

// Declare function pointers.
extern ipol_blocks_func * uvg_filter_hpel_blocks_hor_ver_luma;
extern ipol_blocks_func * uvg_filter_hpel_blocks_diag_luma;
extern ipol_blocks_func * uvg_filter_qpel_blocks_hor_ver_luma;
extern ipol_blocks_func * uvg_filter_qpel_blocks_diag_luma;
extern epol_func * uvg_get_extended_block;
extern epol_func * uvg_get_extended_block_wraparound;
extern uvg_sample_quarterpel_luma_func * uvg_sample_quarterpel_luma;
extern uvg_sample_octpel_chroma_func * uvg_sample_octpel_chroma;
extern uvg_sample_quarterpel_luma_hi_func * uvg_sample_quarterpel_luma_hi;
extern uvg_sample_octpel_chroma_hi_func * uvg_sample_octpel_chroma_hi;


int uvg_strategy_register_ipol(void* opaque, uint8_t bitdepth);


#define STRATEGIES_IPOL_EXPORTS \
  {"filter_hpel_blocks_hor_ver_luma", (void**) &uvg_filter_hpel_blocks_hor_ver_luma}, \
  {"filter_hpel_blocks_diag_luma",    (void**) &uvg_filter_hpel_blocks_diag_luma}, \
  {"filter_qpel_blocks_hor_ver_luma", (void**) &uvg_filter_qpel_blocks_hor_ver_luma}, \
  {"filter_qpel_blocks_diag_luma",    (void**) &uvg_filter_qpel_blocks_diag_luma}, \
  {"sample_quarterpel_luma", (void**) &uvg_sample_quarterpel_luma}, \
  {"sample_octpel_chroma", (void**) &uvg_sample_octpel_chroma}, \
  {"sample_quarterpel_luma_hi", (void**) &uvg_sample_quarterpel_luma_hi}, \
  {"sample_octpel_chroma_hi", (void**) &uvg_sample_octpel_chroma_hi}, \
  {"get_extended_block", (void**) &uvg_get_extended_block}, \
  {"get_extended_block_wraparound", (void**) &uvg_get_extended_block_wraparound}, \



#endif //STRATEGIES_IPOL_H_
