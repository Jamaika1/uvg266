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

#include "strategies/generic/intra-generic.h"

#include <stdlib.h>

#include "cu.h"
#include "intra.h"
#include "uvg266.h"
#include "strategyselector.h"
#include "uvg_math.h"


/**
 * \brief Generate angular predictions.
 * \param cu_loc        CU location and size data.
 * \param intra_mode    Angular mode in range 2..34.
 * \param channel_type  Color channel.
 * \param in_ref_above  Pointer to -1 index of above reference, length=width*2+1.
 * \param in_ref_left   Pointer to -1 index of left reference, length=height*2+1.
 * \param dst           Buffer of size width*width.
 * \param multi_ref_idx Multi reference line index for use with MRL.
 */
static void uvg_angular_pred_generic(
  const cu_loc_t* const cu_loc,
  const int_fast8_t intra_mode,
  const int_fast8_t channel_type,
  const uvg_pixel *const in_ref_above,
  const uvg_pixel *const in_ref_left,
  uvg_pixel *const dst,
  const uint8_t multi_ref_idx,
  const uint8_t isp_mode)
{
  int width  = channel_type == COLOR_Y ? cu_loc->width : cu_loc->chroma_width;
  int height = channel_type == COLOR_Y ? cu_loc->height : cu_loc->chroma_height;
  const int log2_width  = uvg_g_convert_to_log2[width];
  const int log2_height = uvg_g_convert_to_log2[height];
  
  assert((log2_width >= 2 && log2_width <= 5) &&  log2_height <= 5);
  // assert(intra_mode >= 2 && intra_mode <= 66);

  static const int16_t modedisp2sampledisp[32] = { 0,    1,    2,    3,    4,    6,     8,   10,   12,   14,   16,   18,   20,   23,   26,   29,   32,   35,   39,  45,  51,  57,  64,  73,  86, 102, 128, 171, 256, 341, 512, 1024 };
  static const int16_t modedisp2invsampledisp[32] = { 0, 16384, 8192, 5461, 4096, 2731, 2048, 1638, 1365, 1170, 1024, 910, 819, 712, 630, 565, 512, 468, 420, 364, 321, 287, 256, 224, 191, 161, 128, 96, 64, 48, 32, 16 }; // (512 * 32) / sampledisp
  static const int32_t pre_scale[] = { 8, 7, 6, 5, 5, 4, 4, 4, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 0, 0, 0, -1, -1, -2, -3 };
  
  static const int16_t cubic_filter[32][4] =
  {
    { 0, 64,  0,  0 },
    { -1, 63,  2,  0 },
    { -2, 62,  4,  0 },
    { -2, 60,  7, -1 },
    { -2, 58, 10, -2 },
    { -3, 57, 12, -2 },
    { -4, 56, 14, -2 },
    { -4, 55, 15, -2 },
    { -4, 54, 16, -2 },
    { -5, 53, 18, -2 },
    { -6, 52, 20, -2 },
    { -6, 49, 24, -3 },
    { -6, 46, 28, -4 },
    { -5, 44, 29, -4 },
    { -4, 42, 30, -4 },
    { -4, 39, 33, -4 },
    { -4, 36, 36, -4 },
    { -4, 33, 39, -4 },
    { -4, 30, 42, -4 },
    { -4, 29, 44, -5 },
    { -4, 28, 46, -6 },
    { -3, 24, 49, -6 },
    { -2, 20, 52, -6 },
    { -2, 18, 53, -5 },
    { -2, 16, 54, -4 },
    { -2, 15, 55, -4 },
    { -2, 14, 56, -4 },
    { -2, 12, 57, -3 },
    { -2, 10, 58, -2 },
    { -1,  7, 60, -2 },
    { 0,  4, 62, -2 },
    { 0,  2, 63, -1 },
  };

                                                    // Temporary buffer for modes 11-25.
                                                    // It only needs to be big enough to hold indices from -width to width-1.

  uvg_pixel temp_dst[TR_MAX_WIDTH * TR_MAX_WIDTH];


  // TODO: check the correct size for these arrays when MRL is used
  //uvg_pixel tmp_ref[2 * 128 + 3 + 33 * MAX_REF_LINE_IDX] = { 0 };
  uvg_pixel temp_above[2 * 128 + 3 + 33 * MAX_REF_LINE_IDX] = { 0 };
  uvg_pixel temp_left[2 * 128 + 3 + 33 * MAX_REF_LINE_IDX] = { 0 };

  uint32_t pred_mode = intra_mode; // ToDo: handle WAIP

  uint8_t multi_ref_index = multi_ref_idx;
  uint8_t isp = isp_mode;

  // Whether to swap references to always project on the left reference row.
  const bool vertical_mode = intra_mode >= 34;
  // Modes distance to horizontal or vertical mode.
  const int_fast8_t mode_disp = vertical_mode ? pred_mode - 50 : -((int32_t)pred_mode - 18);
  
  // Sample displacement per column in fractions of 32.
  const int_fast8_t sample_disp = (mode_disp < 0 ? -1 : 1) * modedisp2sampledisp[abs(mode_disp)];
  
  const int side_size = vertical_mode ? log2_height : log2_width;
  int scale = MIN(2, side_size - pre_scale[abs(mode_disp)]);

  // Pointer for the reference we are interpolating from.
  uvg_pixel *ref_main;
  // Pointer for the other reference.
  const uvg_pixel *ref_side;
  uvg_pixel* work = width == height || vertical_mode ? dst : temp_dst;

  const int cu_dim = MAX(width, height);
  const int top_ref_length  = isp_mode ? width + cu_dim  : width << 1;
  const int left_ref_length = isp_mode ? height + cu_dim : height << 1;

  // Set ref_main and ref_side such that, when indexed with 0, they point to
  // index 0 in block coordinates.
  if (sample_disp < 0) {
    memcpy(&temp_above[height], &in_ref_above[0], (width + 2 + multi_ref_index) * sizeof(uvg_pixel));
    memcpy(&temp_left[width], &in_ref_left[0], (height + 2 + multi_ref_index) * sizeof(uvg_pixel));

    ref_main = vertical_mode ? temp_above + height : temp_left + width;
    ref_side = vertical_mode ? temp_left + width : temp_above + height;

    int size_side = vertical_mode ? height : width;
    for (int i = -size_side; i <= -1; i++) {
      ref_main[i] = ref_side[MIN((-i * modedisp2invsampledisp[abs(mode_disp)] + 256) >> 9, size_side)];
    }
  }
  else {
    memcpy(&temp_above[0], &in_ref_above[0], (top_ref_length + 1 + multi_ref_index) * sizeof(uvg_pixel));
    memcpy(&temp_left[0], &in_ref_left[0], (left_ref_length + 1 + multi_ref_index) * sizeof(uvg_pixel));

    ref_main = vertical_mode ? temp_above : temp_left;
    ref_side = vertical_mode ? temp_left : temp_above;

    const int log2_ratio = log2_width - log2_height;
    const int s = MAX(0, vertical_mode ? log2_ratio : -log2_ratio);
    const int max_index = (multi_ref_index << s) + 2;
    int ref_length;
    if (isp_mode) {
      ref_length = vertical_mode ? top_ref_length : left_ref_length;
    }
    else {
      ref_length = vertical_mode ? width << 1 : height << 1;
    }
    const uvg_pixel val = ref_main[ref_length + multi_ref_index];
    for (int j = 1; j <= max_index; j++) {
      ref_main[ref_length + multi_ref_index +  j] = val;
    }
  }


  // compensate for line offset in reference line buffers
  ref_main += multi_ref_index;
  ref_side += multi_ref_index;
  if (!vertical_mode) { SWAP(width, height, int) }

  if (sample_disp != 0) {
    // The mode is not horizontal or vertical, we have to do interpolation.

    for (int_fast32_t y = 0, delta_pos = sample_disp * (1 + multi_ref_index); y < height; ++y, delta_pos += sample_disp) {
      int_fast32_t delta_int = delta_pos >> 5;
      int_fast32_t delta_fract = delta_pos & (32 - 1);

      if ((abs(sample_disp) & 0x1F) != 0) {
        
        // Luma Channel
        if (channel_type == 0) {
          int32_t ref_main_index = delta_int;
          uvg_pixel p[4];
          bool use_cubic = true; // Default to cubic filter
          static const int uvg_intra_hor_ver_dist_thres[8] = { 24, 24, 24, 14, 2, 0, 0, 0 };
          int filter_threshold = uvg_intra_hor_ver_dist_thres[(log2_width + log2_height) >> 1];
          int dist_from_vert_or_hor = MIN(abs((int32_t)pred_mode - 50), abs((int32_t)pred_mode - 18));
          if (dist_from_vert_or_hor > filter_threshold) {
            if ((abs(sample_disp) & 0x1F) != 0)
            {
              use_cubic = false;
            }
          }
          // Cubic must be used if ref line != 0 or if isp mode is != 0
          if (multi_ref_index || isp) {
            use_cubic = true;
          }
          const int16_t filter_coeff[4] = { 16 - (delta_fract >> 1), 32 - (delta_fract >> 1), 16 + (delta_fract >> 1), delta_fract >> 1 };
          int16_t const * const f = use_cubic ? cubic_filter[delta_fract] : filter_coeff;
          // Do 4-tap intra interpolation filtering
          for (int_fast32_t x = 0; x < width; x++, ref_main_index++) {
            p[0] = ref_main[ref_main_index];
            p[1] = ref_main[ref_main_index + 1];
            p[2] = ref_main[ref_main_index + 2];
            p[3] = ref_main[ref_main_index + 3];
         
            work[y * width + x] = CLIP_TO_PIXEL(((int32_t)(f[0] * p[0]) + (int32_t)(f[1] * p[1]) + (int32_t)(f[2] * p[2]) + (int32_t)(f[3] * p[3]) + 32) >> 6);

          }
        }
        else {
        
          // Do linear filtering
          for (int_fast32_t x = 0; x < width; ++x) {
            uvg_pixel ref1 = ref_main[x + delta_int + 1];
            uvg_pixel ref2 = ref_main[x + delta_int + 2];
            work[y * width + x] = ref1 + ((delta_fract * (ref2-ref1) + 16) >> 5);
          }
        }
      }
      else {
        // Just copy the integer samples
        for (int_fast32_t x = 0; x < width; x++) {
          work[y * width + x] = ref_main[x + delta_int + 1];
        }
      }

     
      // PDPC
      bool PDPC_filter = ((width >= TR_MIN_WIDTH && height >= TR_MIN_WIDTH)  || channel_type != 0) && multi_ref_index == 0;
      if (pred_mode > 1 && pred_mode < 67) {
        if (mode_disp < 0 || multi_ref_index) { // Cannot be used with MRL.
          PDPC_filter = false;
        }
        else if (mode_disp > 0) {
          PDPC_filter &= (scale >= 0);
        }
      }
      if(PDPC_filter) {
        int inv_angle_sum = 256;
        for (int x = 0; x < MIN(3 << scale, width); x++) {
          inv_angle_sum += modedisp2invsampledisp[abs(mode_disp)];

          int wL = 32 >> (2 * x >> scale);
          const uvg_pixel left = ref_side[y + (inv_angle_sum >> 9) + 1];
          work[y * width + x] = work[y * width + x] + ((wL * (left - work[y * width + x]) + 32) >> 6);
        }
      }
    }
  }
  else {
    // Mode is horizontal or vertical, just copy the pixels.
    
    // Do not apply PDPC if multi ref line index is other than 0
    // TODO: do not do PDPC if block is in BDPCM mode
    bool do_pdpc = (((width >= 4 && height >= 4) || channel_type != 0) && sample_disp >= 0 && multi_ref_index == 0 /*&& !bdpcm*/);

    if (do_pdpc) {
      int scale = (log2_width + log2_height - 2) >> 2;
      const uvg_pixel top_left = ref_main[0];
      for (int_fast32_t y = 0; y < height; ++y) {
        memcpy(&work[y * width], &ref_main[1], width * sizeof(uvg_pixel));
        const uvg_pixel left = ref_side[1 + y];
        for (int_fast32_t x = 0; x < MIN(3 << scale, width); ++x) {
          const int wL = 32 >> (2 * x >> scale);
          const uvg_pixel val = work[y * width + x];
          work[y * width + x] = CLIP_TO_PIXEL(val + ((wL * (left - top_left) + 32) >> 6));
        }
      }
    } else {
      for (int_fast32_t y = 0; y < height; ++y) {
        memcpy(&work[y * width], &ref_main[1], width * sizeof(uvg_pixel));
      }
    }
  }

  // Flip the block if this is was a horizontal mode.
  if (!vertical_mode) {
    if(width == height) {
      for (int_fast32_t y = 0; y < height - 1; ++y) {
        for (int_fast32_t x = y + 1; x < width; ++x) {
          SWAP(work[y * height + x], work[x * width + y], uvg_pixel);
        }
      }
    } else {
      for(int y = 0; y < width; ++y) {
        for(int x = 0; x < height; ++x) {
          dst[x + y * height] = work[y + x * width];
        }
      }
    }
  }
}


/**
 * \brief Generate planar prediction.
 * \param cu_loc        CU location and size data.
 * \param color         Color channel.
 * \param in_ref_above  Pointer to -1 index of above reference, length=width*2+1.
 * \param in_ref_left   Pointer to -1 index of left reference, length=width*2+1.
 * \param dst           Buffer of size width*width.
 */
static void uvg_intra_pred_planar_generic(
  const cu_loc_t* const cu_loc,
  color_t color,
  const uvg_pixel *const ref_top,
  const uvg_pixel *const ref_left,
  uvg_pixel *const dst)
{
  const int width = color == COLOR_Y ? cu_loc->width : cu_loc->chroma_width;
  const int height = color == COLOR_Y ? cu_loc->height : cu_loc->chroma_height;
  const int log2_width  = uvg_g_convert_to_log2[width];
  const int log2_height = uvg_g_convert_to_log2[height];

  const int offset = 1 << (log2_width + log2_height);
  const int final_shift = 1 + log2_width + log2_height;
  
  // If ISP is enabled log_dim 1 is possible (limit was previously 2)
  assert((log2_width >= 1 && log2_width <= 5) && (log2_height >= 1 && log2_height <= 5));

  const uvg_pixel top_right = ref_top[width + 1];
  const uvg_pixel bottom_left = ref_left[height + 1];

#if 0
  // Unoptimized version for reference.
  for (int y = 0; y < width; ++y) {
    for (int x = 0; x < width; ++x) {
      int_fast16_t hor = (width - 1 - x) * ref_left[y + 1] + (x + 1) * top_right;
      int_fast16_t ver = (width - 1 - y) * ref_top[x + 1] + (y + 1) * bottom_left;
      dst[y * width + x] = (ver + hor + width) >> (log2_width + 1);
    }
  }
#else
  // TODO: get rid of magic numbers. Make a define for this
  int_fast16_t top[64];
  int_fast16_t bottom[64];
  int_fast16_t left[64];
  int_fast16_t right[64];
  for (int i = 0; i < width; ++i) {
    bottom[i] = bottom_left - ref_top[i + 1];
    top[i] = ref_top[i + 1] << log2_height;
  }

  for (int j = 0; j < height; ++j) {
    right[j] = top_right - ref_left[j + 1];
    left[j] = ref_left[j + 1] << log2_width;
  }

  for (int y = 0; y < height; ++y) {
    int_fast16_t hor = left[y];
    for (int x = 0; x < width; ++x) {
      hor += right[y];
      top[x] += bottom[x];
      dst[y * width + x] = ((hor << log2_height) + (top[x] << log2_width) + offset) >> final_shift;
    }
  }
#endif
}

/**
* \brief Generate intra DC prediction with post filtering applied.
* \param log2_width    Log2 of width, range 2..5.
* \param in_ref_above  Pointer to -1 index of above reference, length=width*2+1.
* \param in_ref_left   Pointer to -1 index of left reference, length=width*2+1.
* \param dst           Buffer of size width*width.
* \param multi_ref_idx Reference line index. May be non-zero when MRL is used.
*/
static void uvg_intra_pred_filtered_dc_generic(
  const int_fast8_t log2_width,
  const uvg_pixel *const ref_top,
  const uvg_pixel *const ref_left,
  uvg_pixel *const out_block,
  const uint8_t multi_ref_idx)
{
  assert(log2_width >= 2 && log2_width <= 5);

  // TODO: height for non-square block sizes
  const int_fast8_t width = 1 << log2_width;

  int_fast16_t sum = 0;
  for (int_fast8_t i = 0; i < width; ++i) {
    sum += ref_top[i + 1 + multi_ref_idx];
    sum += ref_left[i + 1 + multi_ref_idx];
  }

  const uvg_pixel dc_val = (sum + width) >> (log2_width + 1);

  // Filter top-left with ([1 2 1] / 4)
  out_block[0] = (ref_left[1] + 2 * dc_val + ref_top[1] + 2) / 4;

  // Filter rest of the boundary with ([1 3] / 4)
  for (int_fast8_t x = 1; x < width; ++x) {
    out_block[x] = (ref_top[x + 1] + 3 * dc_val + 2) / 4;
  }
  for (int_fast8_t y = 1; y < width; ++y) {
    out_block[y * width] = (ref_left[y + 1] + 3 * dc_val + 2) / 4;
    for (int_fast8_t x = 1; x < width; ++x) {
      out_block[y * width + x] = dc_val;
    }
  }
}

// TODO: update all ranges (in comments, etc.) from HEVC to VVC

/**
* \brief Position Dependent Prediction Combination for Planar and DC modes.
* \param cu_loc        CU location and size data.
* \param used_ref      Pointer used reference pixel struct.
* \param dst           Buffer of size width*width.
*/
static void uvg_pdpc_planar_dc_generic(
  const int mode,
  const cu_loc_t* const cu_loc,
  const color_t color,
  const uvg_intra_ref *const used_ref,
  uvg_pixel *const dst)
{
  assert(mode == 0 || mode == 1);  // planar or DC
  const int width =  color == COLOR_Y ? cu_loc->width : cu_loc->chroma_width;
  const int height = color == COLOR_Y ? cu_loc->height : cu_loc->chroma_height;
  const int log2_width  = uvg_g_convert_to_log2[width];
  const int log2_height = uvg_g_convert_to_log2[height];

  const int scale = (log2_width + log2_height - 2) >> 2;

  for (int y = 0; y < height; y++) {
    int wT = 32 >> MIN(31, ((y << 1) >> scale));
    for (int x = 0; x < width; x++) {
      int wL = 32 >> MIN(31, ((x << 1) >> scale));
      dst[x + y * width] = dst[x + y * width] + ((wL * (used_ref->left[y + 1] - dst[x + y * width])
        + wT * (used_ref->top[x + 1] - dst[x + y * width]) + 32) >> 6);
    }
  }
}


int uvg_strategy_register_intra_generic(void* opaque, uint8_t bitdepth)
{
  bool success = true;

  success &= uvg_strategyselector_register(opaque, "angular_pred", "generic", 0, &uvg_angular_pred_generic);
  success &= uvg_strategyselector_register(opaque, "intra_pred_planar", "generic", 0, &uvg_intra_pred_planar_generic);
  success &= uvg_strategyselector_register(opaque, "intra_pred_filtered_dc", "generic", 0, &uvg_intra_pred_filtered_dc_generic);
  success &= uvg_strategyselector_register(opaque, "pdpc_planar_dc", "generic", 0, &uvg_pdpc_planar_dc_generic);

  return success;
}
