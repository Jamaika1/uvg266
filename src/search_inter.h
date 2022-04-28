#ifndef SEARCH_INTER_H_
#define SEARCH_INTER_H_
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
 * \ingroup Compression
 * \file
 * Inter prediction parameter search.
 */

#include "cu.h"
#include "encoderstate.h"
#include "global.h" // IWYU pragma: keep
#include "inter.h"
#include "uvg266.h"

#define UVG_LUMA_FILTER_TAPS 8
#define UVG_LUMA_FILTER_OFFSET 3
#define UVG_CHROMA_FILTER_TAPS 4
#define UVG_CHROMA_FILTER_OFFSET 1

 // Maximum extra width a block needs to filter 
 // a fractional pixel with positive fractional mv.x and mv.y
#define UVG_EXT_PADDING_LUMA (UVG_LUMA_FILTER_TAPS - 1)
#define UVG_EXT_PADDING_CHROMA (UVG_CHROMA_FILTER_TAPS - 1)

 // Maximum block width for extended block
#define UVG_EXT_BLOCK_W_LUMA (LCU_WIDTH + UVG_EXT_PADDING_LUMA)
#define UVG_EXT_BLOCK_W_CHROMA (LCU_WIDTH_C + UVG_EXT_PADDING_CHROMA)

enum hpel_position {
  HPEL_POS_HOR = 0,
  HPEL_POS_VER = 1,
  HPEL_POS_DIA = 2
};

typedef uint32_t uvg_mvd_cost_func(const encoder_state_t *state,
                                  int x, int y,
                                  int mv_shift,
                                  mv_t mv_cand[2][2],
                                  inter_merge_cand_t merge_cand[MRG_MAX_NUM_CANDS],
                                  int16_t num_cand,
                                  int32_t ref_idx,
                                  uint32_t *bitcost);

void uvg_search_cu_inter(encoder_state_t * const state,
                         int x, int y, int depth,
                         lcu_t *lcu,
                         double *inter_cost,
                         uint32_t *inter_bitcost);

void uvg_search_cu_smp(encoder_state_t * const state,
                       int x, int y,
                       int depth,
                       part_mode_t part_mode,
                       lcu_t *lcu,
                       double *inter_cost,
                       uint32_t *inter_bitcost);


unsigned uvg_inter_satd_cost(const encoder_state_t* state,
                             const lcu_t *lcu,
                             int x,
                             int y);

#endif // SEARCH_INTER_H_
