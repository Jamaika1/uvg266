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

#include <string.h>
#include <stdlib.h>

#include "cu.h"
#include "threads.h"



/**
 * \brief Number of PUs in a CU.
 *
 * Indexed by part_mode_t values.
 */
const uint8_t uvg_part_mode_num_parts[] = {
  1, // 2Nx2N
  2, // 2NxN
  2, // Nx2N
  4, // NxN
  2, // 2NxnU
  2, // 2NxnD
  2, // nLx2N
  2, // nRx2N
};

/**
 * \brief PU offsets.
 *
 * Indexed by [part mode][PU number][axis].
 *
 * Units are 1/4 of the width of the CU.
 */
const uint8_t uvg_part_mode_offsets[][4][2] = {
  { {0, 0}                         }, // 2Nx2N
  { {0, 0}, {0, 2}                 }, // 2NxN
  { {0, 0}, {2, 0}                 }, // Nx2N
  { {0, 0}, {2, 0}, {0, 2}, {2, 2} }, // NxN
  { {0, 0}, {0, 1}                 }, // 2NxnU
  { {0, 0}, {0, 3}                 }, // 2NxnD
  { {0, 0}, {1, 0}                 }, // nLx2N
  { {0, 0}, {3, 0}                 }, // nRx2N
};

/**
 * \brief PU sizes.
 *
 * Indexed by [part mode][PU number][axis].
 *
 * Units are 1/4 of the width of the CU.
 */
const uint8_t uvg_part_mode_sizes[][4][2] = {
  { {4, 4}                         }, // 2Nx2N
  { {4, 2}, {4, 2}                 }, // 2NxN
  { {2, 4}, {2, 4}                 }, // Nx2N
  { {2, 2}, {2, 2}, {2, 2}, {2, 2} }, // NxN
  { {4, 1}, {4, 3}                 }, // 2NxnU
  { {4, 3}, {4, 1}                 }, // 2NxnD
  { {1, 4}, {3, 4}                 }, // nLx2N
  { {3, 4}, {1, 4}                 }, // nRx2N
};


cu_info_t* uvg_cu_array_at(cu_array_t *cua, unsigned x_px, unsigned y_px)
{
  return (cu_info_t*) uvg_cu_array_at_const(cua, x_px, y_px);
}


void uvg_get_isp_cu_arr_coords(int *x, int *y)
{
  // Do nothing if dimensions are divisible by 4
  if (*y % 4 == 0 && *x % 4 == 0) return;
  const int remainder_y = *y % 4;
  const int remainder_x = *x % 4;

  if (remainder_y != 0) {
    // Horizontal ISP split
    if (remainder_y % 2 == 0) {
      // 8x2 block
      *y -= 2;
      *x += 4;
    }
    else {
      // 16x1 block
      *y -= remainder_y;
      *x += remainder_y * 4;
    }
  }
  else {
    // Vertical ISP split
    if (*x % 2 == 0) {
      // 2x8 block
      *y += 4;
      *x -= 2;
    }
    else {
      // 1x16 block
      *y += remainder_x * 4;
      *x -= remainder_x;
    }
  }
}


const cu_info_t* uvg_cu_array_at_const(const cu_array_t *cua, unsigned x_px, unsigned y_px)
{
  assert(x_px < cua->width);
  assert(y_px < cua->height);
  return &(cua)->data[(x_px >> 2) + (y_px >> 2) * ((cua)->stride >> 2)];
}


/**
 * \brief Allocate a CU array.
 *
 * \param width   width of the array in luma pixels
 * \param height  height of the array in luma pixels
 */
cu_array_t * uvg_cu_array_alloc(const int width, const int height)
{
  cu_array_t *cua = MALLOC(cu_array_t, 1);
  if (cua == NULL) return NULL;

  // Round up to a multiple of LCU width and divide by cell width.
  const int width_scu  = CEILDIV(width,  LCU_WIDTH) * LCU_WIDTH / SCU_WIDTH;
  const int height_scu = CEILDIV(height, LCU_WIDTH) * LCU_WIDTH / SCU_WIDTH;
  const unsigned cu_array_size = width_scu * height_scu;

  cua->base     = NULL;
  cua->data     = calloc(cu_array_size, sizeof(cu_info_t));
  cua->width    = width_scu  * SCU_WIDTH;
  cua->height   = height_scu * SCU_WIDTH;
  cua->stride   = cua->width;
  cua->refcount = 1;

  return cua;
}
cu_array_t * uvg_cu_array_chroma_alloc(const int width, const int height, enum uvg_chroma_format chroma)
{
  cu_array_t *cua = MALLOC(cu_array_t, 1);
  if (cua == NULL) return NULL;

  // Round up to a multiple of LCU width and divide by cell width.
  const int chroma_height = chroma == UVG_CSP_444 ? LCU_WIDTH : LCU_WIDTH_C;
  const int width_scu  = CEILDIV(width,  LCU_WIDTH_C) * LCU_WIDTH_C / SCU_WIDTH;
  const int height_scu = CEILDIV(height, chroma_height) * chroma_height / SCU_WIDTH;
  const unsigned cu_array_size = width_scu * height_scu;

  cua->base     = NULL;
  cua->data     = calloc(cu_array_size, sizeof(cu_info_t));
  cua->width    = width_scu  * SCU_WIDTH;
  cua->height   = height_scu * SCU_WIDTH;
  cua->stride   = cua->width;
  cua->refcount = 1;

  return cua;
}


cu_array_t * uvg_cu_subarray(cu_array_t *base,
                             const unsigned x_offset,
                             const unsigned y_offset,
                             const unsigned width,
                             const unsigned height)
{
  assert(x_offset + width <= base->width);
  assert(y_offset + height <= base->height);

  if (x_offset == 0 &&
      y_offset == 0 &&
      width == base->width &&
      height == base->height)
  {
    return uvg_cu_array_copy_ref(base);
  }

  cu_array_t *cua = MALLOC(cu_array_t, 1);
  if (cua == NULL) return NULL;

  // Find the real base array.
  cu_array_t *real_base = base;
  while (real_base->base) {
    real_base = real_base->base;
  }
  cua->base     = uvg_cu_array_copy_ref(real_base);
  cua->data     = uvg_cu_array_at(base, x_offset, y_offset);
  cua->width    = width;
  cua->height   = height;
  cua->stride   = base->stride;
  cua->refcount = 1;

  return cua;
}

void uvg_cu_array_free(cu_array_t **cua_ptr)
{
  cu_array_t *cua = *cua_ptr;
  if (cua == NULL) return;
  *cua_ptr = NULL;

  int new_refcount = UVG_ATOMIC_DEC(&cua->refcount);
  if (new_refcount > 0) {
    // Still we have some references, do nothing.
    return;
  }

  assert(new_refcount == 0);

  if (!cua->base) {
    FREE_POINTER(cua->data);
  } else {
    uvg_cu_array_free(&cua->base);
    cua->data = NULL;
  }

  FREE_POINTER(cua);
}


/**
 * \brief Get a new pointer to a cu array.
 *
 * Increment reference count and return the cu array.
 */
cu_array_t * uvg_cu_array_copy_ref(cu_array_t* cua)
{
  int32_t new_refcount = UVG_ATOMIC_INC(&cua->refcount);
  // The caller should have had another reference and we added one
  // reference so refcount should be at least 2.
  assert(new_refcount >= 2);
  return cua;
}


/**
 * \brief Copy an lcu to a cu array.
 *
 * All values are in luma pixels.
 *
 * \param dst     destination array
 * \param dst_x   x-coordinate of the left edge of the copied area in dst
 * \param dst_y   y-coordinate of the top edge of the copied area in dst
 * \param src     source lcu
 */
void uvg_cu_array_copy_from_lcu(cu_array_t* dst, int dst_x, int dst_y, const lcu_t *src, enum uvg_tree_type tree_type)
{
  const int dst_stride = dst->stride >> 2;
  const int width = tree_type != UVG_CHROMA_T ? LCU_WIDTH : LCU_WIDTH_C;
  for (int y = 0; y < width; y += SCU_WIDTH) {
    for (int x = 0; x < width; x += SCU_WIDTH) {
      const cu_info_t *from_cu = LCU_GET_CU_AT_PX(src, x, y);
      const int x_scu = (dst_x + x) >> 2;
      const int y_scu = (dst_y + y) >> 2;
      cu_info_t *to_cu = &dst->data[x_scu + y_scu * dst_stride];
      memcpy(to_cu,                  from_cu, sizeof(*to_cu));
    }
  }
}

/*
 * \brief Constructs cu_loc_t based on given parameters. Calculates chroma dimensions automatically.
 *
 * \param loc     Destination cu_loc.
 * \param x       Block top left x coordinate.
 * \param y       Block top left y coordinate.
 * \param width   Block width.
 * \param height  Block height.
*/
void uvg_cu_loc_ctor(cu_loc_t* loc, int x, int y, int width, int height)
{
  assert(x >= 0 && y >= 0 && width >= 0 && height >= 0 && "Cannot give negative coordinates or block dimensions.");
  assert(!(width > LCU_WIDTH || height > LCU_WIDTH) && "Luma CU dimension exceeds maximum (dim > LCU_WIDTH).");
  // This check is no longer valid. With non-square blocks and ISP enabled, even 1x16 and 16x1 (ISP needs at least 16 samples) blocks are valid
  //assert(!(width < 4 || height < 4) && "Luma CU dimension smaller than 4.");
  
  loc->x = x;
  loc->y = y;
  loc->width = width;
  loc->height = height;
  // TODO: when MTT is implemented, chroma dimensions can be minimum 2.
  // Chroma width is half of luma width, when not at maximum depth.
  loc->chroma_width = MAX(width >> 1, 4);
  loc->chroma_height = MAX(height >> 1, 4);
}
