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

#include "transform.h"

#include "encode_coding_tree.h"
#include "image.h"
#include "uvg266.h"
#include "lfnst_tables.h"
#include "rdo.h"
#include "strategies/strategies-dct.h"
#include "strategies/strategies-quant.h"
#include "strategies/strategies-picture.h"
#include "tables.h"
#include "reshape.h"
#include "search.h"

/**
 * \brief RDPCM direction.
 */
typedef enum rdpcm_dir {
  RDPCM_VER = 0, // vertical
  RDPCM_HOR = 1, // horizontal
} rdpcm_dir;

//////////////////////////////////////////////////////////////////////////
// INITIALIZATIONS
//


const uint8_t uvg_g_chroma_scale[58]=
{
   0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,
  17,18,19,20,21,22,23,24,25,26,27,28,29,29,30,31,32,
  33,33,34,34,35,35,36,36,37,37,38,39,40,41,42,43,44,
  45,46,47,48,49,50,51
};

//////////////////////////////////////////////////////////////////////////
// FUNCTIONS
//

/**
 * \brief Bypass transform and quantization.
 *
 * Copies the reference pixels directly to reconstruction and the residual
 * directly to coefficients. Used when cu_transquant_bypass_flag is set.
 * Parameters pred_in and rec_out may be aliased.
 *
 * \param width       Transform width.
 * \param height      Transform height.
 * \param in_stride   Stride for ref_in and pred_in
 * \param out_stride  Stride for rec_out.
 * \param ref_in      Reference pixels.
 * \param pred_in     Predicted pixels.
 * \param rec_out     Returns the reconstructed pixels.
 * \param coeff_out   Returns the coefficients used for reconstruction of rec_out.
 *
 * \returns  Whether coeff_out contains any non-zero coefficients.
 */
static bool bypass_transquant(const int width,
                              const int height,
                              const int in_stride,
                              const int out_stride,
                              const uvg_pixel *const ref_in,
                              const uvg_pixel *const pred_in,
                              uvg_pixel *rec_out,
                              coeff_t *coeff_out)
{
  bool nonzero_coeffs = false;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int32_t in_idx    = x + y * in_stride;
      int32_t out_idx   = x + y * out_stride;
      int32_t coeff_idx = x + y * width;

      // The residual must be computed before writing to rec_out because
      // pred_in and rec_out may point to the same array.
      coeff_t coeff        = (coeff_t)(ref_in[in_idx] - pred_in[in_idx]);
      coeff_out[coeff_idx] = coeff;
      rec_out[out_idx]     = ref_in[in_idx];

      nonzero_coeffs |= (coeff != 0);
    }
  }

  return nonzero_coeffs;
}

/**
 * Apply DPCM to residual.
 *
 * \param width   width of the block
 * \param dir     RDPCM direction
 * \param coeff   coefficients (residual) to filter
 */
static void rdpcm(const int width,
                  const int height,
                  const rdpcm_dir dir,
                  coeff_t *coeff)
{
  const int offset = (dir == RDPCM_HOR) ? 1 : width;
  const int min_x  = (dir == RDPCM_HOR) ? 1 : 0;
  const int min_y  = (dir == RDPCM_HOR) ? 0 : 1;

  for (int y = height - 1; y >= min_y; y--) {
    for (int x = width - 1; x >= min_x; x--) {
      const int index = x + y * width;
      coeff[index] -= coeff[index - offset];
    }
  }
}

/**
 * \brief Get scaled QP used in quantization
 *
 */
int32_t uvg_get_scaled_qp(color_t color, int8_t qp, int8_t qp_offset, int8_t const * const chroma_scale)
{
  int32_t qp_scaled = 0;
  if(color == 0) {
    qp_scaled = qp + qp_offset;
  } else {
    qp_scaled = CLIP(-qp_offset, 57, qp);
    if (chroma_scale) {
      qp_scaled = chroma_scale[qp] + qp_offset;
    }
    else {
      qp_scaled = qp_scaled + qp_offset;
    } 
  }
  return qp_scaled;
}



/**
* \brief Derives lfnst constraints.
*
* \param pred_cu  Current prediction coding unit.
* \param lcu      Current lcu.
* \param depth    Current transform depth.
* \param lcu_px   Position of the top left pixel of current CU within current LCU.
*/
void uvg_derive_lfnst_constraints(
  cu_info_t* const pred_cu,
  bool* constraints,
  const coeff_t* coeff,
  const int width,
  const int height,
  const vector2d_t * const lcu_px,
  color_t color)
{
  coeff_scan_order_t scan_idx = SCAN_DIAG;
  // ToDo: large block support in VVC?

  const uint32_t log2_block_size = uvg_g_convert_to_log2[width];
  const uint32_t* scan = uvg_g_sig_last_scan[scan_idx][log2_block_size - 1];

  signed scan_pos_last = -1;
  coeff_t temp[TR_MAX_WIDTH * TR_MAX_WIDTH];
  if(lcu_px != NULL) {
    uvg_get_sub_coeff(temp, coeff, lcu_px->x, lcu_px->y, width, height, color == COLOR_Y? LCU_WIDTH : LCU_WIDTH_C);
    coeff = temp;
  }

  for (int i = 0; i < width * height; i++) {
    if (coeff[scan[i]]) {
      scan_pos_last = i;
    }
  }

  if (scan_pos_last < 0) return;

  if (pred_cu != NULL && pred_cu->tr_idx != MTS_SKIP && height >= 4 && width >= 4) {
    const int max_lfnst_pos = ((height == 4 && width == 4) || (height == 8 && width == 8)) ? 7 : 15;
    constraints[0] |= scan_pos_last > max_lfnst_pos;
    constraints[1] |= scan_pos_last >= 1;
  }
}


/**
 * \brief NxN inverse transform (2D)
 * \param coeff   input data (transform coefficients)
 * \param block   output data (residual)
 * \param width   transform width
 * \param height  transform height
 */
void uvg_transformskip(const encoder_control_t * const encoder, int16_t *block,int16_t *coeff, int8_t width, int8_t height)
{
  int32_t j, k;
  for (j = 0; j < height; j++) {
    for(k = 0; k < width; k ++) {
      // Casting back and forth to make UBSan not trigger due to left-shifting negatives
      coeff[j * width + k] = (int16_t)((uint16_t)(block[j * width + k]));
    }
  }
}

/**
 * \brief inverse transform skip
 * \param coeff input data (transform coefficients)
 * \param block output data (residual)
 * \param block_size width of transform
 */
void uvg_itransformskip(const encoder_control_t * const encoder, int16_t *block,int16_t *coeff, int8_t block_width, int8_t block_height)
{
  int32_t  j,k;
  for ( j = 0; j < block_height; j++ ) {
    for(k = 0; k < block_width; k ++) {
      block[j * block_width + k] =  coeff[j * block_width + k];
    }
  }
}

/**
 * \brief forward transform (2D)
 * \param block input residual
 * \param coeff transform coefficients
 * \param block_size width of transform
 */
void uvg_transform2d(const encoder_control_t * const encoder,
                     int16_t *block,
                     int16_t *coeff,
                     int8_t block_width,
                     int8_t block_height,
                     color_t color,
                     const cu_info_t *tu)
{
  if (encoder->cfg.mts || tu->lfnst_idx || tu->cr_lfnst_idx || block_width != block_height)
  {
    uvg_mts_dct(encoder->bitdepth, color, tu, block_width, block_height, block, coeff, encoder->cfg.mts);
  }
  else
  {
    dct_func *dct_func = uvg_get_dct_func(block_width, block_height, color, tu->type);
    dct_func(encoder->bitdepth, block, coeff);
  }
}

void uvg_itransform2d(const encoder_control_t * const encoder,
                      int16_t *block,
                      int16_t *coeff,
                      int8_t block_width,
                      int8_t block_height,
                      color_t color,
                      const cu_info_t *tu)
{
  if (encoder->cfg.mts || block_width != block_height)
  {
    uvg_mts_idct(encoder->bitdepth, color, tu, block_width, block_height, coeff, block, encoder->cfg.mts);
  }
  else
  {
    dct_func *idct_func = uvg_get_idct_func(block_width, block_height, color, tu->type);
    idct_func(encoder->bitdepth, coeff, block);
  }
}

static INLINE int64_t square(int x) {
  return x * (int64_t)x;
}

static void generate_jccr_transforms(
  encoder_state_t* const state,
  const cu_info_t* const pred_cu,
  int8_t width,
  int8_t height,
  int16_t u_resi[1024],
  int16_t v_resi[1024],
  coeff_t u_coeff[5120],
  enum uvg_chroma_transforms transforms[5],
  const int trans_offset,
  int* num_transforms)
{
  ALIGNED(64) int16_t temp_resi[LCU_WIDTH_C * LCU_WIDTH_C * 3];
  int64_t costs[4];
  costs[0] = INT64_MAX;
  for (int jccr = pred_cu->type == CU_INTRA ? 0 : 3; jccr < 4; jccr++) {
    int64_t d1 = 0;
    int64_t d2 = 0;
    const int cbf_mask = jccr * (state->frame->jccr_sign ? -1 : 1);
    int16_t* current_resi = &temp_resi[MAX((jccr - 1) , 0) * trans_offset];
    for (int y = 0; y < height; y++)
    {
      for (int x = 0; x < width; x++)
      {
        const int16_t cbx = u_resi[x + y * width], crx = v_resi[x + y * width];
        if (cbf_mask == 2)
        {
          const int16_t resi = ((4 * cbx + 2 * crx) / 5);
          current_resi[x + y * width] = resi;
          d1 += square(cbx - resi) + square(crx - (resi >> 1));
        }
        else if (cbf_mask == -2)
        {
          const int16_t resi = ((4 * cbx - 2 * crx) / 5);
          current_resi[x + y * width] = resi;
          d1 += square(cbx - resi) + square(crx - (-resi >> 1));
        }
        else if (cbf_mask == 3)
        {
          const int16_t resi = ((cbx + crx) / 2);
          current_resi[x + y * width] = resi;
          d1 += square(cbx - resi) + square(crx - resi);
        }
        else if (cbf_mask == -3)
        {
          const int16_t resi = ((cbx - crx) / 2);
          current_resi[x + y * width] = resi;
          d1 += square(cbx - resi) + square(crx + resi);
        }
        else if (cbf_mask == 1)
        {
          const int16_t resi = ((4 * crx + 2 * cbx) / 5);
          current_resi[x + y * width] = resi;
          d1 += square(cbx - (resi >> 1)) + square(crx - resi);
        }
        else if (cbf_mask == -1)
        {
          const int16_t resi = ((4 * crx - 2 * cbx) / 5);
          current_resi[x + y * width] = resi;
          d1 += square(cbx - (-resi >> 1)) + square(crx - resi);
        }
        else
        {
          d1 += square(cbx);
          d2 += square(crx);
        }
      }
    }
    costs[jccr] = d2 != 0 ? MIN(d1, d2) : d1;
  }
  int64_t min_dist1 = costs[0];
  int64_t min_dist2 = INT64_MAX;
  int     cbf_mask1 = 0;
  int     cbf_mask2 = 0;
  for (int cbfMask = 1; cbfMask < 4; cbfMask++)
  {
    if (costs[cbfMask] < min_dist1)
    {
      cbf_mask2 = cbf_mask1; min_dist2 = min_dist1;
      cbf_mask1 = cbfMask;  min_dist1 = costs[cbf_mask1];
    }
    else if (costs[cbfMask] < min_dist2)
    {
      cbf_mask2 = cbfMask;  min_dist2 = costs[cbf_mask2];
    }
  }
  if (cbf_mask1)
  {
    uvg_transform2d(
      state->encoder_control,
      &temp_resi[(cbf_mask1 - 1) * trans_offset],
      &u_coeff[*num_transforms * trans_offset],
      width,
      height,
      COLOR_U,
      pred_cu
    );
    transforms[(*num_transforms)] = cbf_mask1;
    (*num_transforms)++;
  }
  if (cbf_mask2 && ((min_dist2 < (9 * min_dist1) / 8) || (!cbf_mask1 && min_dist2 < (3 * min_dist1) / 2)))
  {
    uvg_transform2d(
      state->encoder_control,
      &temp_resi[(cbf_mask2 - 1) * trans_offset],
      &u_coeff[*num_transforms * trans_offset],
      width,
      height,
      COLOR_U,
      pred_cu
    );
    transforms[(*num_transforms)] = cbf_mask2;
    (*num_transforms)++;
  }
}



#define IS_JCCR_MODE(t) ((t) != DCT7_CHROMA && (t) != CHROMA_TS)


static void quantize_chroma(
  encoder_state_t* const state,
  int8_t width,
  int8_t height,
  coeff_t u_coeff[5120],
  coeff_t v_coeff[2048],
  enum uvg_chroma_transforms transform,
  coeff_t u_quant_coeff[1024],
  coeff_t v_quant_coeff[1024],
  const coeff_scan_order_t scan_order,
  bool* u_has_coeffs,
  bool* v_has_coeffs,
  uint8_t lfnst_idx)
{
  if (state->encoder_control->cfg.rdoq_enable &&
    (transform != CHROMA_TS || !state->encoder_control->cfg.rdoq_skip))
  {
    uvg_rdoq(state, u_coeff, u_quant_coeff, width, height, transform != JCCR_1 ? COLOR_U : COLOR_V,
             scan_order, CU_INTRA, 0, lfnst_idx);

    int j;
    for (j = 0; j < width * height; ++j) {
      if (u_quant_coeff[j]) {
        *u_has_coeffs = 1;
        break;
      }
    }

    if (transform == DCT7_CHROMA) {
      uint16_t temp_cbf = 0;
      if (*u_has_coeffs)cbf_set(&temp_cbf, COLOR_U);
      uvg_rdoq(state, v_coeff, v_quant_coeff, width, height, COLOR_V,
               scan_order, CU_INTRA, temp_cbf, lfnst_idx);

    }
  }
  else if (state->encoder_control->cfg.rdoq_enable && transform == CHROMA_TS) {
    uvg_ts_rdoq(state, u_coeff, u_quant_coeff, width, height, COLOR_U, scan_order);
    uvg_ts_rdoq(state, v_coeff, v_quant_coeff, width, height, COLOR_V, scan_order);
  }
  else {
    uvg_quant(state, u_coeff, u_quant_coeff, width, height, transform != JCCR_1 ? COLOR_U : COLOR_V,
      scan_order, CU_INTRA, transform == CHROMA_TS, lfnst_idx);

    if (!IS_JCCR_MODE(transform)) {
      uvg_quant(state, v_coeff, v_quant_coeff, width, height, COLOR_V,
        scan_order, CU_INTRA, transform == CHROMA_TS, lfnst_idx);
    }
  }

  for (int j = 0; j < width * height; ++j) {
    if (u_quant_coeff[j]) {
      *u_has_coeffs = 1;
      break;
    }
  }
  if (!IS_JCCR_MODE(transform)) {
    for (int j = 0; j < width * height; ++j) {
      if (v_quant_coeff[j]) {
        *v_has_coeffs = 1;
        break;
      }
    }
  }
}

void uvg_chroma_transform_search(
  encoder_state_t* const state,
  lcu_t* const lcu,
  cabac_data_t* temp_cabac,
  const cu_loc_t* const cu_loc,
  const int offset,
  cu_info_t* pred_cu,
  uvg_pixel u_pred[1024],
  uvg_pixel v_pred[1024],
  int16_t u_resi[1024],
  int16_t v_resi[1024],
  uvg_chorma_ts_out_t* chorma_ts_out,
  enum uvg_tree_type tree_type)
{
  ALIGNED(64) coeff_t u_coeff[LCU_WIDTH_C * LCU_WIDTH_C * 5];
  ALIGNED(64) uint8_t u_recon[LCU_WIDTH_C * LCU_WIDTH_C * 5];
  ALIGNED(64) coeff_t v_coeff[LCU_WIDTH_C * LCU_WIDTH_C * 2]; // In case of JCCR the v channel does not have coefficients
  ALIGNED(64) uint8_t v_recon[LCU_WIDTH_C * LCU_WIDTH_C * 5];
  const int width  = cu_loc->chroma_width;
  const int height = cu_loc->chroma_height;

  const int depth = 6 - uvg_g_convert_to_log2[cu_loc->width];

  uvg_transform2d(
    state->encoder_control, u_resi, u_coeff, width, height, COLOR_U, pred_cu
  );
  uvg_transform2d(
    state->encoder_control, v_resi, v_coeff, width, height, COLOR_V, pred_cu
  );
  enum uvg_chroma_transforms transforms[5];
  transforms[0] = DCT7_CHROMA;
  const int trans_offset = width * height;
  int num_transforms = 1;

  const int can_use_tr_skip = state->encoder_control->cfg.trskip_enable &&
    (1 << state->encoder_control->cfg.trskip_max_size) >= width &&
    state->encoder_control->cfg.chroma_trskip_enable && 
    pred_cu->cr_lfnst_idx == 0 ;

  if (can_use_tr_skip) {
    uvg_transformskip(state->encoder_control, u_resi, u_coeff + num_transforms * trans_offset, width, height);
    uvg_transformskip(state->encoder_control, v_resi, v_coeff + num_transforms * trans_offset, width, height);
    transforms[num_transforms] = CHROMA_TS;
    num_transforms++;
  }
  if (state->encoder_control->cfg.jccr) {
    generate_jccr_transforms(
      state,
      pred_cu,
      width,
      height,
      u_resi,
      v_resi,
      u_coeff,
      transforms,
      trans_offset,
      &num_transforms);
  }
  chorma_ts_out->best_u_cost = MAX_DOUBLE;
  chorma_ts_out->best_v_cost = MAX_DOUBLE;
  chorma_ts_out->best_combined_cost = MAX_DOUBLE;
  chorma_ts_out->best_u_index = -1;
  chorma_ts_out->best_v_index = -1;
  chorma_ts_out->best_combined_index = -1;
  for (int i = 0; i < num_transforms; i++) {
    coeff_t u_quant_coeff[LCU_WIDTH_C * LCU_WIDTH_C];
    coeff_t v_quant_coeff[LCU_WIDTH_C * LCU_WIDTH_C];
    int16_t u_recon_resi[LCU_WIDTH_C * LCU_WIDTH_C];
    int16_t v_recon_resi[LCU_WIDTH_C * LCU_WIDTH_C];
    bool u_has_coeffs = false;
    bool v_has_coeffs = false;
    bool is_jccr = IS_JCCR_MODE(transforms[i]);
    if(pred_cu->cr_lfnst_idx) {
      uvg_fwd_lfnst(pred_cu, width, height, COLOR_U, pred_cu->cr_lfnst_idx, &u_coeff[i * trans_offset], tree_type);
      if (!is_jccr) {
        uvg_fwd_lfnst(pred_cu, width, height, COLOR_V, pred_cu->cr_lfnst_idx, &v_coeff[i * trans_offset], tree_type);
      }
    }
    quantize_chroma(
      state,
      width,
      height,
      &u_coeff[i * trans_offset],
      &v_coeff[i * trans_offset],
      transforms[i],
      u_quant_coeff,
      v_quant_coeff,
      SCAN_DIAG,
      &u_has_coeffs,
      &v_has_coeffs,
      pred_cu->cr_lfnst_idx);
      if(pred_cu->cr_lfnst_idx !=0 && !u_has_coeffs && !v_has_coeffs) continue;
    
    if(pred_cu->type == CU_INTRA && transforms[i] != CHROMA_TS && (cu_loc->width == 4 || tree_type == UVG_CHROMA_T)) {
      bool constraints[2] = { false, false };
      uvg_derive_lfnst_constraints(pred_cu, constraints, u_quant_coeff, width, height, NULL, COLOR_U);
      if(!is_jccr) {
        uvg_derive_lfnst_constraints(pred_cu, constraints, v_quant_coeff, width, height, NULL, COLOR_V);
      }
      if (!constraints[1] && (u_has_coeffs || v_has_coeffs) && pred_cu->cr_lfnst_idx != 0) continue;
    }

    if (is_jccr && !u_has_coeffs) continue;

    if (u_has_coeffs) {
      uvg_dequant(state, u_quant_coeff, &u_coeff[i * trans_offset], width, width, transforms[i] != JCCR_1 ? COLOR_U : COLOR_V,
        pred_cu->type, transforms[i] == CHROMA_TS);

      if (transforms[i] != CHROMA_TS) {
        if (pred_cu->cr_lfnst_idx) {
          uvg_inv_lfnst(pred_cu, width, height, COLOR_U, pred_cu->cr_lfnst_idx, &u_coeff[i * trans_offset], tree_type);
        }
        uvg_itransform2d(state->encoder_control, u_recon_resi, &u_coeff[i * trans_offset], width, height,
          transforms[i] != JCCR_1 ? COLOR_U : COLOR_V, pred_cu);
      }
      else {
        uvg_itransformskip(state->encoder_control, u_recon_resi, &u_coeff[i * trans_offset], width, height);
      }

      if (transforms[i] != JCCR_1) {
        for (int j = 0; j < width * height; j++) {
          u_recon[trans_offset * i + j] = CLIP_TO_PIXEL((uvg_pixel)(u_pred[j] + u_recon_resi[j]));
        }
      }
      else {
        for (int j = 0; j < width * height; j++) {
          u_recon[trans_offset * i + j] = CLIP_TO_PIXEL(u_pred[j] + ((state->frame->jccr_sign ? -u_recon_resi[j] : u_recon_resi[j]) >> 1));
        }
      }
    }
    else {
      uvg_pixels_blit(u_pred, &u_recon[trans_offset * i], width, height, width, width);
    }


    if (v_has_coeffs && !is_jccr) {
      uvg_dequant(state, v_quant_coeff, &v_coeff[i * trans_offset], width, width, COLOR_V,
        pred_cu->type, transforms[i] == CHROMA_TS);

      if (transforms[i] != CHROMA_TS) {
        if (pred_cu->cr_lfnst_idx) {
          uvg_inv_lfnst(pred_cu, width, height, COLOR_V, pred_cu->cr_lfnst_idx, &v_coeff[i * trans_offset], tree_type);
        }
        uvg_itransform2d(state->encoder_control, v_recon_resi, &v_coeff[i * trans_offset], width, height,
          transforms[i] != JCCR_1 ? COLOR_U : COLOR_V, pred_cu);
      }
      else {
        uvg_itransformskip(state->encoder_control, v_recon_resi, &v_coeff[i * trans_offset], width, height);
      }

      for (int j = 0; j < width * height; j++) {
        v_recon[trans_offset * i + j] = CLIP_TO_PIXEL(v_pred[j] + v_recon_resi[j]);
      }
    }
    else if (u_has_coeffs && is_jccr) {
      if (transforms[i] == JCCR_1) {
        for (int j = 0; j < width * height; j++) {
          v_recon[trans_offset * i + j] = CLIP_TO_PIXEL(v_pred[j] + u_recon_resi[j]);
        }
      }
      else if (transforms[i] == JCCR_3) {
        for (int j = 0; j < width * height; j++) {
          v_recon[trans_offset * i + j] = CLIP_TO_PIXEL(v_pred[j] + (state->frame->jccr_sign ? -u_recon_resi[j] : u_recon_resi[j]));
        }
      }
      else {
        for (int j = 0; j < width * height; j++) {
          v_recon[trans_offset * i + j] = CLIP_TO_PIXEL(v_pred[j] + ((state->frame->jccr_sign ? -u_recon_resi[j] : u_recon_resi[j]) >> 1));
        }
      }
    }
    else {
      uvg_pixels_blit(v_pred, &v_recon[trans_offset * i], width, height, width, width);
    }

    unsigned ssd_u = 0;
    unsigned ssd_v = 0;
    if (!state->encoder_control->cfg.lossless) {
      ssd_u = uvg_pixels_calc_ssd(&lcu->ref.u[offset], &u_recon[trans_offset * i],
        LCU_WIDTH_C, width,
        width);
      ssd_v = uvg_pixels_calc_ssd(&lcu->ref.v[offset], &v_recon[trans_offset * i],
        LCU_WIDTH_C, width,
        width);
    }

    double u_bits = 0;
    double v_bits = 0;
    state->search_cabac.update = 1;

    int cbf_u = transforms[i] & 2 || (u_has_coeffs && !(transforms[i] & 1));
    CABAC_FBITS_UPDATE(&state->search_cabac, &state->search_cabac.ctx.qt_cbf_model_cb[0],
      cbf_u, u_bits, "cbf_u"
    );
    int cbf_v = transforms[i] & 1 || (v_has_coeffs && !(transforms[i] & 2));
    CABAC_FBITS_UPDATE(&state->search_cabac, &state->search_cabac.ctx.qt_cbf_model_cr[cbf_u],
      cbf_v, v_bits, "cbf_v"
    );

    if (state->encoder_control->cfg.jccr && (cbf_u || cbf_v)) {
      CABAC_FBITS_UPDATE(&state->search_cabac, &state->search_cabac.ctx.joint_cb_cr[cbf_u * 2 + cbf_v - 1],
        transforms[i] != DCT7_CHROMA && transforms[i] != CHROMA_TS, v_bits, "jccr_flag"
      );
    }

    if (cbf_u || (transforms[i] == JCCR_1 && u_has_coeffs)) {
      if (can_use_tr_skip) {
        CABAC_FBITS_UPDATE(&state->search_cabac, &state->search_cabac.ctx.transform_skip_model_chroma,
          transforms[i] == CHROMA_TS, u_bits, "tr_skip_u"
        );
      }
      double coeff_cost = uvg_get_coeff_cost(
        state,
        u_quant_coeff,
        pred_cu,
        cu_loc,
        COLOR_U,
        SCAN_DIAG,
        transforms[i] == CHROMA_TS,
        COEFF_ORDER_LINEAR);
      u_bits += coeff_cost;
    }
    if (cbf_v && !is_jccr) {
      if (can_use_tr_skip) {
        CABAC_FBITS_UPDATE(&state->search_cabac, &state->search_cabac.ctx.transform_skip_model_chroma,
          transforms[i] == CHROMA_TS, v_bits, "tr_skip_v"
        );
      }
      v_bits += uvg_get_coeff_cost(
        state,
        v_quant_coeff,
        pred_cu,
        cu_loc,
        COLOR_V,
        SCAN_DIAG,
        transforms[i] == CHROMA_TS,
        COEFF_ORDER_LINEAR);
    }
    if((depth == 4 || tree_type == UVG_CHROMA_T) && state->encoder_control->cfg.lfnst && 0) {
      if(uvg_is_lfnst_allowed(state, pred_cu, UVG_CHROMA_T, COLOR_UV, cu_loc)) {
        const int lfnst_idx = pred_cu->cr_lfnst_idx;
        CABAC_FBITS_UPDATE(
          &state->search_cabac,
          &state->search_cabac.ctx.lfnst_idx_model[1],
          lfnst_idx != 0,
          v_bits,
          "lfnst_idx");
        if (lfnst_idx > 0) {
          CABAC_FBITS_UPDATE(
            &state->search_cabac,
            &state->search_cabac.ctx.lfnst_idx_model[2],
            lfnst_idx == 2,
            v_bits,
            "lfnst_idx");
        }
      }
      pred_cu->lfnst_last_scan_pos = false;
      pred_cu->violates_lfnst_constrained_chroma = false;
    }
    if (!is_jccr) {
      double u_cost = UVG_CHROMA_MULT * ssd_u + u_bits * state->frame->lambda;
      double v_cost = UVG_CHROMA_MULT * ssd_v + v_bits * state->frame->lambda;
      if (u_cost < chorma_ts_out->best_u_cost) {
        chorma_ts_out->best_u_cost = u_cost;
        chorma_ts_out->best_u_index = u_has_coeffs ? transforms[i] : NO_RESIDUAL;
      }
      if (v_cost < chorma_ts_out->best_v_cost) {
        chorma_ts_out->best_v_cost = v_cost;
        chorma_ts_out->best_v_index = v_has_coeffs ? transforms[i] : NO_RESIDUAL;
      }
    }
    else {
      double cost = UVG_CHROMA_MULT * (ssd_u + ssd_v) + (u_bits + v_bits) * state->frame->lambda;
      if (cost < chorma_ts_out->best_combined_cost) {
        chorma_ts_out->best_combined_cost = cost;
        chorma_ts_out->best_combined_index = transforms[i];
      }
    }
    memcpy(&state->search_cabac, temp_cabac, sizeof(cabac_data_t));
  }
}


void uvg_fwd_lfnst_NxN(coeff_t *src, coeff_t *dst, const int8_t mode, const int8_t index, const int8_t size, int zero_out_size)
{
  const int8_t *tr_mat = (size > 4) ? uvg_lfnst_8x8[mode][index][0] : uvg_lfnst_4x4[mode][index][0];
  const int     tr_size = (size > 4) ? 48 : 16;
  int coef;
  coeff_t *out = dst;
  assert(index < 3 && "LFNST index must be in [0, 2]");

  for (int j = 0; j < zero_out_size; j++)
  {
    coeff_t *src_ptr = src;
    const int8_t* tr_mat_tmp = tr_mat;
    coef = 0;
    for (int i = 0; i < tr_size; i++)
    {
      coef += *src_ptr++ * *tr_mat_tmp++;
    }
    *out++ = (coeff_t)((coef + 64) >> 7);
    tr_mat += tr_size;
  }

  // Possible tr_size values 16, 48. Possible zero_out_size values 8, 16
  switch (tr_size - zero_out_size) {
    case 0:
      break;
    case 8:
      FILL_ARRAY(out, 0, 8);
      break;
    case 32:
      FILL_ARRAY(out, 0, 32);
      break;
    case 40:
      FILL_ARRAY(out, 0, 40);
      break;
    default:
      assert(false && "LFNST: This should never trip.");
  }
}

static inline bool get_transpose_flag(const int8_t intra_mode)
{
  return ((intra_mode >= NUM_LUMA_MODE) && (intra_mode >= (NUM_LUMA_MODE + (NUM_EXT_LUMA_MODE >> 1)))) ||
         ((intra_mode < NUM_LUMA_MODE) && (intra_mode > DIA_IDX));
}


static inline bool block_is_mip(const cu_info_t * const cur_cu, const color_t color, const bool is_sep_tree)
{
  if (cur_cu->type == CU_INTRA) {
    if (color == COLOR_Y) {
      return cur_cu->intra.mip_flag;
    }
    else {
      // MIP_TODO: currently, only chroma 420 is supported. Therefore this will always return false

      //bool derived_mode = cur_cu->intra.mode_chroma == (!cur_cu->intra.mip_flag ? cur_cu->intra.mode : 0);
      //bool is_chroma_mip = !is_sep_tree /*&& chroma_format == CHROMA_444*/ && cur_cu->intra.mip_flag;
      //return is_chroma_mip && derived_mode;

      return false;
    }
  }
  return false;
}

void uvg_fwd_lfnst(
  const cu_info_t* const cur_cu,
  const int width,
  const int height,
  const color_t color,
  const uint16_t lfnst_idx,
  coeff_t *coeffs,
  enum uvg_tree_type tree_type)
{
  const uint16_t lfnst_index = lfnst_idx;
  int8_t intra_mode = (color == COLOR_Y) ? cur_cu->intra.mode : cur_cu->intra.mode_chroma;
  bool mts_skip = cur_cu->tr_idx == MTS_SKIP;
  bool is_separate_tree = cur_cu->log2_height + cur_cu->log2_width < 6 || tree_type != UVG_BOTH_T;
  bool is_cclm_mode = (intra_mode >= 81 && intra_mode <= 83); // CCLM modes are in [81, 83]

  bool is_mip = block_is_mip(cur_cu, color, is_separate_tree);
  bool is_wide_angle = false; // TODO: get wide angle mode when implemented
  
  const int scan_order = SCAN_DIAG;

  if (lfnst_index && !mts_skip && (is_separate_tree || color == COLOR_Y))
  {
    const uint32_t log2_block_size = uvg_g_convert_to_log2[width];
    assert(log2_block_size != -1 && "LFNST: invalid block width.");
    const bool whge3 = width >= 8 && height >= 8;
    const uint32_t* scan = whge3 ? uvg_coef_top_left_diag_scan_8x8[log2_block_size] : uvg_g_sig_last_scan[scan_order][log2_block_size - 1];

    if (is_cclm_mode) {
      intra_mode = cur_cu->intra.mode;
    }
    if (is_mip) {
      intra_mode = 0; // Set to planar mode
    }
    assert(intra_mode < NUM_INTRA_MODE && "LFNST: Invalid intra mode.");
    assert(lfnst_index < 3 && "LFNST: Invalid LFNST index. Must be in [0, 2]");

    if (is_wide_angle) {
      // Transform wide angle mode to intra mode
      intra_mode = intra_mode; // TODO: wide angle modes not implemented yet. Do nothing.
    }

    bool transpose = get_transpose_flag(intra_mode);
    const int sb_size = whge3 ? 8 : 4;
    bool tu_4x4 = (width == 4 && height == 4);
    bool tu_8x8 = (width == 8 && height == 8);
 
    coeff_t tmp_in_matrix[48];
    coeff_t tmp_out_matrix[48];
    coeff_t *lfnst_tmp = tmp_in_matrix; // forward low frequency non-separable transform
      
    coeff_t *coeff_tmp = coeffs;

    int y;
    if (transpose) {
      if (sb_size == 4) {
        for (y = 0; y < 4; y++) {
          lfnst_tmp[0] = coeff_tmp[0];
          lfnst_tmp[4] = coeff_tmp[1];
          lfnst_tmp[8] = coeff_tmp[2];
          lfnst_tmp[12] = coeff_tmp[3];
          lfnst_tmp++;
          coeff_tmp += width;
        }
      }
      else { // ( sb_size == 8 )
        for (y = 0; y < 8; y++) {
          lfnst_tmp[0] = coeff_tmp[0];
          lfnst_tmp[8] = coeff_tmp[1];
          lfnst_tmp[16] = coeff_tmp[2];
          lfnst_tmp[24] = coeff_tmp[3];
          if (y < 4) {
            lfnst_tmp[32] = coeff_tmp[4];
            lfnst_tmp[36] = coeff_tmp[5];
            lfnst_tmp[40] = coeff_tmp[6];
            lfnst_tmp[44] = coeff_tmp[7];
          }
          lfnst_tmp++;
          coeff_tmp += width;
        }
      }
    }
    else {
      for (y = 0; y < sb_size; y++) {
        uint32_t stride = (y < 4) ? sb_size : 4;
        memcpy(lfnst_tmp, coeff_tmp, stride * sizeof(coeff_t));
        lfnst_tmp += stride;
        coeff_tmp += width;
      }
    }

    uvg_fwd_lfnst_NxN(tmp_in_matrix, tmp_out_matrix, uvg_lfnst_lut[intra_mode], lfnst_index - 1, sb_size,
      (tu_4x4 || tu_8x8) ? 8 : 16);

    lfnst_tmp = tmp_out_matrix;   // forward spectral rearrangement
    coeff_tmp = coeffs;
    int lfnst_coeff_num = (sb_size == 4) ? sb_size * sb_size : 48;

    const uint32_t *scan_ptr = scan;

    for (y = 0; y < lfnst_coeff_num; y++) {
      coeff_tmp[*scan_ptr] = *lfnst_tmp++;
      scan_ptr++;
    }
  }
}

void uvg_inv_lfnst_NxN(coeff_t *src, coeff_t *dst, const uint32_t mode, const uint32_t index, const uint32_t size, int zero_out_size, const int max_log2_tr_dyn_range)
{
  const coeff_t output_min = -(1 << max_log2_tr_dyn_range);
  const coeff_t output_max = (1 << max_log2_tr_dyn_range) - 1;
  const int8_t *tr_mat = (size > 4) ? uvg_lfnst_8x8[mode][index][0] : uvg_lfnst_4x4[mode][index][0];
  const int tr_size = (size > 4) ? 48 : 16;
  int resi;
  coeff_t *out = dst;
  assert(index < 3);

  for (int j = 0; j < tr_size; j++)
  {
    resi = 0;
    const int8_t* tr_mat_tmp = tr_mat;
    coeff_t *src_ptr = src;
    for (int i = 0; i < zero_out_size; i++)
    {
      resi += *src_ptr++ * *tr_mat_tmp;
      tr_mat_tmp += tr_size;
    }
    *out++ = CLIP(output_min, output_max, (coeff_t)((resi + 64) >> 7));
    tr_mat++;
  }
}

void uvg_inv_lfnst(
  const cu_info_t *cur_cu,
  const int width,
  const int height,
  const color_t color,
  const uint16_t lfnst_idx,
  coeff_t *coeffs,
  enum uvg_tree_type tree_type)
{
  // In VTM, max log2 dynamic range is something in range [15, 20] depending on whether extended precision processing is enabled
  // Such is not yet present in uvg266 so use 15 for now
  const int max_log2_dyn_range = 15;
  const uint32_t  lfnst_index = lfnst_idx;
  int8_t intra_mode = (color == COLOR_Y) ? cur_cu->intra.mode : cur_cu->intra.mode_chroma;
  bool mts_skip = cur_cu->tr_idx == MTS_SKIP;
  bool is_separate_tree = cur_cu->log2_height + cur_cu->log2_width < 6 || tree_type != UVG_BOTH_T;
  bool is_cclm_mode = (intra_mode >= 81 && intra_mode <= 83); // CCLM modes are in [81, 83]

  bool is_mip = block_is_mip(cur_cu, color, is_separate_tree);
  bool is_wide_angle = false; // TODO: get wide angle mode when implemented
  
  const int scan_order = SCAN_DIAG;
  
  if (lfnst_index && !mts_skip && (is_separate_tree || color == COLOR_Y)) {
    const uint32_t log2_block_size = uvg_g_convert_to_log2[width];
    const bool whge3 = width >= 8 && height >= 8;
    const uint32_t* scan = whge3 ? uvg_coef_top_left_diag_scan_8x8[log2_block_size] : uvg_g_sig_last_scan[scan_order][log2_block_size - 1];
    
    if (is_cclm_mode) {
      intra_mode = cur_cu->intra.mip_flag ? 0 : cur_cu->intra.mode;
    }
    if (is_mip) {
      intra_mode = 0; // Set to planar mode
    }
    assert(intra_mode < NUM_INTRA_MODE && "LFNST: Invalid intra mode.");
    assert(lfnst_index < 3 && "LFNST: Invalid LFNST index. Must be in [0, 2]");

    if (is_wide_angle) {
      // Transform wide angle mode to intra mode
      intra_mode = intra_mode; // TODO: wide angle modes not implemented yet. Do nothing.
    }

    bool          transpose_flag = get_transpose_flag(intra_mode);
    const int     sb_size = whge3 ? 8 : 4;
    bool          tu_4x4_flag = (width == 4 && height == 4);
    bool          tu_8x8_flag = (width == 8 && height == 8);
    coeff_t tmp_in_matrix[48];
    coeff_t tmp_out_matrix[48];
    coeff_t *lfnst_tmp;
    coeff_t *coeff_tmp;
    int           y;
    lfnst_tmp = tmp_in_matrix;   // inverse spectral rearrangement
    coeff_tmp = coeffs;
    coeff_t *dst = lfnst_tmp;

    const uint32_t *scan_ptr = scan;
    for (y = 0; y < 16; y++) {
      *dst++ = coeff_tmp[*scan_ptr];
      scan_ptr++;
    }

    uvg_inv_lfnst_NxN(tmp_in_matrix, tmp_out_matrix, uvg_lfnst_lut[intra_mode], lfnst_index - 1, sb_size,
      (tu_4x4_flag || tu_8x8_flag) ? 8 : 16, max_log2_dyn_range);
    lfnst_tmp = tmp_out_matrix;   // inverse low frequency non-separale transform

    if (transpose_flag) {
      if (sb_size == 4) {
        for (y = 0; y < 4; y++) {
          coeff_tmp[0] = lfnst_tmp[0];
          coeff_tmp[1] = lfnst_tmp[4];
          coeff_tmp[2] = lfnst_tmp[8];
          coeff_tmp[3] = lfnst_tmp[12];
          lfnst_tmp++;
          coeff_tmp += width;
        }
      }
      else { // ( sb_size == 8 )
        for (y = 0; y < 8; y++) {
          coeff_tmp[0] = lfnst_tmp[0];
          coeff_tmp[1] = lfnst_tmp[8];
          coeff_tmp[2] = lfnst_tmp[16];
          coeff_tmp[3] = lfnst_tmp[24];
          if (y < 4) {
            coeff_tmp[4] = lfnst_tmp[32];
            coeff_tmp[5] = lfnst_tmp[36];
            coeff_tmp[6] = lfnst_tmp[40];
            coeff_tmp[7] = lfnst_tmp[44];
          }
          lfnst_tmp++;
          coeff_tmp += width;
        }
      }
    }
    else {
      for (y = 0; y < sb_size; y++) {
        uint32_t uiStride = (y < 4) ? sb_size : 4;
        memcpy(coeff_tmp, lfnst_tmp, uiStride * sizeof(coeff_t));
        lfnst_tmp += uiStride;
        coeff_tmp += width;
      }
    }
  }
}

/**
 * \brief Like uvg_quantize_residual except that this uses trskip if that is better.
 *
 * Using this function saves one step of quantization and inverse quantization
 * compared to doing the decision separately from the actual operation.
 *
 * \param width  Transform width.
 * \param color  Color.
 * \param scan_order  Coefficient scan order.
 * \param trskip_out  Whether transform skip is used.
 * \param stride  Stride for ref_in, pred_in and rec_out.
 * \param ref_in  Reference pixels.
 * \param pred_in  Predicted pixels.
 * \param rec_out  Reconstructed pixels.
 * \param coeff_out  Coefficients used for reconstruction of rec_out.
 *
 * \returns  Whether coeff_out contains any non-zero coefficients.
 */
int uvg_quantize_residual_trskip(
    encoder_state_t *const state,
    const cu_info_t *const cur_cu, const int width, const int height, const color_t color,
    const coeff_scan_order_t scan_order, int8_t *trskip_out, 
    const int in_stride, const int out_stride,
    const uvg_pixel *const ref_in, const uvg_pixel *const pred_in, 
    uvg_pixel *rec_out, coeff_t *coeff_out, int lmcs_chroma_adj)
{
  struct {
    uvg_pixel rec[LCU_WIDTH * LCU_WIDTH];
    coeff_t coeff[LCU_WIDTH * LCU_WIDTH];
    double cost;
    int has_coeffs;
  } skip, *best;
  
  //noskip.has_coeffs = uvg_quantize_residual(
  //    state, cur_cu, width, color, scan_order,
  //    0, in_stride, 4,
  //    ref_in, pred_in, noskip.rec, noskip.coeff, false);
  //noskip.cost = uvg_pixels_calc_ssd(ref_in, noskip.rec, in_stride, 4, 4);
  //noskip.cost += uvg_get_coeff_cost(state, noskip.coeff, 4, 0, scan_order) * bit_cost;

  skip.has_coeffs = uvg_quantize_residual(
    state, cur_cu, width, height, color, scan_order,
    1, in_stride, width,
    ref_in, pred_in, skip.rec, skip.coeff, false, lmcs_chroma_adj, 
    UVG_BOTH_T /* tree type doesn't matter for transformskip*/);

/*  if (noskip.cost <= skip.cost) {
    *trskip_out = 0;
    best = &noskip;
  } else */{
    *trskip_out = 1;
    best = &skip;
  }

  if (best->has_coeffs || rec_out != pred_in) {
    // If there is no residual and reconstruction is already in rec_out, 
    // we can skip this.
    uvg_pixels_blit(best->rec, rec_out, width, height, width, out_stride);
  }
  // TODO: copying coeffs here is very suspect
  copy_coeffs(best->coeff, coeff_out, width, height, width);

  return best->has_coeffs;
}


/**
 * Calculate the residual coefficients for a single TU.
 *
 * \param early_skip if this is used for early skip, bypass IT and IQ
 */
static void quantize_tr_residual(
  encoder_state_t * const state,
  const color_t color,
  const cu_loc_t *cu_loc,
  cu_info_t *cur_pu,
  lcu_t* lcu,
  bool early_skip,
  enum uvg_tree_type tree_type)
{
  const int x = cu_loc->x;
  const int y = cu_loc->y;

  const uvg_config *cfg    = &state->encoder_control->cfg;
  const int32_t shift      = color == COLOR_Y ? 0 : 1;
  const vector2d_t lcu_px  = { SUB_SCU(x) >> shift, SUB_SCU(y) >> shift};

  // If luma is 4x4, do chroma for the 8x8 luma area when handling the top
  // left PU because the coordinates are correct.
  bool handled_elsewhere = color != COLOR_Y &&
                           cur_pu->log2_width + cur_pu-> log2_height < 6&&
                           (x % 4 != 0 || y % 4 != 0);
  if (handled_elsewhere) {
    return;
  }

  // Clear coded block flag structures for depths lower than current depth.
  // This should ensure that the CBF data doesn't get corrupted if this function
  // is called more than once.

  const int32_t tr_width  = color == COLOR_Y ? cu_loc->width  : cu_loc->chroma_width;
  const int32_t tr_height = color == COLOR_Y ? cu_loc->height : cu_loc->chroma_height;
  
  const int32_t lcu_width = LCU_WIDTH >> shift;
  const int8_t mode =
    (color == COLOR_Y) ? cur_pu->intra.mode : cur_pu->intra.mode_chroma;
  
  const coeff_scan_order_t scan_idx = SCAN_DIAG; 
  const int offset = lcu_px.x + lcu_px.y * lcu_width;
  //const int z_index = xy_to_zorder(lcu_width, lcu_px.x, lcu_px.y);

  // Pointers to current location in arrays with prediction. The
  // reconstruction will be written to this array.
  uvg_pixel *pred = NULL;
  // Pointers to current location in arrays with reference.
  const uvg_pixel *ref = NULL;
  // Temp coeff array
  coeff_t coeff[TR_MAX_WIDTH * TR_MAX_WIDTH];
  coeff_t *dst_coeff = NULL;

  switch (color) {
    case COLOR_Y:
      pred      = &lcu->rec.y[offset];
      ref       = &lcu->ref.y[offset];
      dst_coeff = &lcu->coeff.y[lcu_px.x + lcu_px.y * lcu_width];
      break;
    case COLOR_U:
      pred      = &lcu->rec.u[offset];
      ref       = &lcu->ref.u[offset];
      dst_coeff = &lcu->coeff.u[lcu_px.x + lcu_px.y * lcu_width];
      break;
    case COLOR_V:
      pred      = &lcu->rec.v[offset];
      ref       = &lcu->ref.v[offset];
      dst_coeff = &lcu->coeff.v[lcu_px.x + lcu_px.y * lcu_width];
      break;
    case COLOR_UV:
      dst_coeff = &lcu->coeff.joint_uv[lcu_px.x + lcu_px.y * lcu_width];
      break;
    default:
      break;
  }

  const bool can_use_trskip = tr_width <= (1 << state->encoder_control->cfg.trskip_max_size) &&
                              cfg->trskip_enable && 
                              cur_pu->tr_skip & (1 << color);

  uint8_t has_coeffs;


  int lmcs_chroma_adj = 0;
  if (state->tile->frame->lmcs_aps->m_sliceReshapeInfo.enableChromaAdj && color != COLOR_Y) {
    lmcs_chroma_adj = uvg_calculate_lmcs_chroma_adj_vpdu_nei(state, state->tile->frame->lmcs_aps, x, y);
  }

  if (cfg->lossless) {
    has_coeffs = bypass_transquant(tr_width,
                                   tr_height,
                                   lcu_width, // in stride
                                   lcu_width, // out stride
                                   ref,
                                   pred,
                                   pred,
                                   coeff);
    if (cfg->implicit_rdpcm && cur_pu->type == CU_INTRA) {
      // implicit rdpcm for horizontal and vertical intra modes
      if (mode == 18) {
        rdpcm(tr_width, tr_height, RDPCM_HOR, coeff);
      } else if (mode == 50) {
        rdpcm(tr_width, tr_height, RDPCM_VER, coeff);
      }
    }

  } else if (can_use_trskip) {
    int8_t tr_skip = 0;

    // Try quantization with trskip and use it if it's better.
    has_coeffs = uvg_quantize_residual_trskip(state,
                                              cur_pu,
                                              tr_width,
                                              tr_height,
                                              color,
                                              scan_idx,
                                              &tr_skip,
                                              lcu_width,
                                              lcu_width,
                                              ref,
                                              pred,
                                              pred,
                                              coeff,
                                              lmcs_chroma_adj);
  } else {
    if(color == COLOR_UV) {
      has_coeffs = uvg_quant_cbcr_residual(
        state,
        cur_pu,
        tr_width,
        tr_height,
        scan_idx,
        lcu_width,
        lcu_width,
        &lcu->ref.u[offset], &lcu->ref.v[offset],
        &lcu->rec.u[offset], &lcu->rec.v[offset],
        &lcu->rec.u[offset], &lcu->rec.v[offset],
        coeff,
        early_skip,
        lmcs_chroma_adj,
        tree_type
      );
      cur_pu->joint_cb_cr = has_coeffs;
      if (has_coeffs) {
        for (int j = 0; j < tr_height; ++j) {
          memcpy(&dst_coeff[j * lcu_width], &coeff[j * tr_width], tr_width * sizeof(coeff_t));
        }
        cbf_set(&cur_pu->cbf, color);
      }
      else {
        for (int j = 0; j < tr_height; ++j) {
          memset(&dst_coeff[j * lcu_width], 0, (sizeof(coeff_t) * tr_width));
        }
      }
      return;
    }

    has_coeffs = uvg_quantize_residual(state,
                                       cur_pu,
                                       tr_width,
                                       tr_height,
                                       color,
                                       scan_idx,
                                       false, // tr skip
                                       lcu_width,
                                       lcu_width,
                                       ref,
                                       pred,
                                       pred,
                                       coeff,
                                       early_skip,
                                       lmcs_chroma_adj,
                                       tree_type);
    
  }

  cbf_clear(&cur_pu->cbf, color);
  if (has_coeffs) {
    for (int j = 0; j < tr_height; ++j) {
      memcpy(&dst_coeff[j * lcu_width], &coeff[j * tr_width], tr_width * sizeof(coeff_t));
    }
    cbf_set(&cur_pu->cbf, color);
  }
  else {
    for (int j = 0; j < tr_height; ++j) {
      memset(&dst_coeff[j * lcu_width], 0, (sizeof(coeff_t) * tr_width));
    }
  }
}

/**
 * This function calculates the residual coefficients for a region of the LCU
 * (defined by x, y and depth) and updates the reconstruction with the
 * kvantized residual. Processes the TU tree recursively.
 *
 * Inputs are:
 * - lcu->rec   pixels after prediction for the area
 * - lcu->ref   reference pixels for the area
 * - lcu->cu    for the area
 * - early_skip if this is used for early skip, bypass IT and IQ
 *
 * Outputs are:
 * - lcu->rec               reconstruction after quantized residual
 * - lcu->coeff             quantized coefficients for the area
 * - lcu->cbf               coded block flags for the area
 * - lcu->cu.intra.tr_skip  tr skip flags for the area (in case of luma)
 */
void uvg_quantize_lcu_residual(
  encoder_state_t * const state,
  const bool luma,
  const bool chroma,
  const bool jccr,
  const cu_loc_t * cu_loc,
  cu_info_t *cur_pu,
  lcu_t* lcu,
  bool early_skip,
  enum uvg_tree_type tree_type)
{
  const int x = cu_loc->x;
  const int y = cu_loc->y;
  const int width = cu_loc->width;
  const int height = cu_loc->height;
  
  const vector2d_t lcu_px  = { SUB_SCU(x), SUB_SCU(y) };

  if (cur_pu == NULL) {
    cur_pu = LCU_GET_CU_AT_PX(lcu, lcu_px.x, lcu_px.y);
  }

  // Tell clang-analyzer what is up. For some reason it can't figure out from
  // asserting just depth.
  // Width 2 is possible with ISP blocks // ISP_TODO: no, they actually are not
  assert(width ==  2 ||
         width ==  4 ||
         width ==  8 ||
         width == 16 ||
         width == 32 ||
         width == 64);

  // Reset CBFs because CBFs might have been set
  // for depth earlier
  // ISP_TODO: does this cur_cu point to the correct place when ISP is used for small blocks?
  if (luma) {
    cbf_clear(&cur_pu->cbf, COLOR_Y);
  }
  if (chroma || jccr) {
    cbf_clear(&cur_pu->cbf, COLOR_U);
    cbf_clear(&cur_pu->cbf, COLOR_V);
  }

  if (cu_loc->width > TR_MAX_WIDTH || cu_loc->height > TR_MAX_WIDTH) {

    // Split transform and increase depth
    const int offset = width / 2;
    for (int j = 0; j < 2; ++j) {
      for (int i = 0; i < 2; ++i) {
        cu_loc_t loc;
        uvg_cu_loc_ctor(&loc, (x + i * offset), (y + j * offset), width >> 1, height >> 1);
        // jccr is currently not supported if transform is split
        uvg_quantize_lcu_residual(state, luma, chroma, 0, &loc, NULL, lcu, early_skip, tree_type);
      }
    }
    

    // Propagate coded block flags from child CUs to parent CU.
    uint16_t child_cbfs[3] = {
      LCU_GET_CU_AT_PX(lcu, lcu_px.x + offset, lcu_px.y         )->cbf,
      LCU_GET_CU_AT_PX(lcu, lcu_px.x,          lcu_px.y + offset)->cbf,
      LCU_GET_CU_AT_PX(lcu, lcu_px.x + offset, lcu_px.y + offset)->cbf,
    };
    
    cur_pu->root_cbf = cbf_is_set_any(cur_pu->cbf)
      || cbf_is_set_any(child_cbfs[0])
      || cbf_is_set_any(child_cbfs[1])
      || cbf_is_set_any(child_cbfs[2]);
    

  } else {
    // Process a leaf TU.
    cu_loc_t loc;
    uvg_cu_loc_ctor(&loc, x, y, width, height);

    if (luma) {
      quantize_tr_residual(state, COLOR_Y, &loc, cur_pu, lcu, early_skip, tree_type);
    }
    if (chroma) {
      quantize_tr_residual(state, COLOR_U, &loc, cur_pu, lcu, early_skip, tree_type);
      quantize_tr_residual(state, COLOR_V, &loc, cur_pu, lcu, early_skip, tree_type);   
    }
    if (jccr && PU_IS_TU(cur_pu)) {
      quantize_tr_residual(state, COLOR_UV, &loc, cur_pu, lcu, early_skip, tree_type);
    }
    if(chroma && jccr && PU_IS_TU(cur_pu)) {
      assert( 0 && "Trying to quantize both jccr and regular at the same time.\n");
    }
  }
}
