#ifndef INTRA_AVX2_TABLES_H
#define INTRA_AVX2_TABLES_H

#include "global.h"

// Test tables
ALIGNED(32) const int8_t  intra_chroma_linear_interpolation_w4_m40[] = {
  16,  16,  16,  16,  16,  16,  16,  16,  32,   0,  32,   0,  32,   0,  32,   0,
};

ALIGNED(32) const int8_t intra_chroma_linear_interpolation_shuffle_w4_m30[] = {
  0x02, 0x03, 0x01, 0x02, 0x01, 0x02, 0x00, 0x01,
  0x03, 0x04, 0x02, 0x03, 0x02, 0x03, 0x01, 0x02,
  0x0a, 0x0b, 0x09, 0x0a, 0x09, 0x0a, 0x08, 0x09,
  0x0b, 0x0c, 0x0a, 0x0b, 0x0a, 0x0b, 0x09, 0x0a
};

ALIGNED(32) const int8_t intra_chroma_linear_interpolation_w4_m30_coeff[] = {
  20, 12,  8, 24, 28, 04, 16, 16, 20, 12,  8, 24, 28, 04, 16, 16,
};

// The number of unique 128-bit coefficient vectors for a given prediction mode. Applicable for width 4 chroma linear interpolation.
const int8_t coeff_vector128_num_by_mode[33] = {
  1, 16,  8, 16, 4,  8, 1,  8, 4,  8, 2,  8, 4, 16,  8, 16,
  1, 16,  8, 16, 4,  8, 2,  8, 4,  8, 1,  8, 4, 16,  8, 16, 1
};


const int16_t coeff_table_mode_offsets[33] = {
  0, 16, 272, 400, 656, 720, 848, 864, 992, 1056, 1184, 1216, 1344, 1408, 1664, 1792, 
  2048, 2064, 2320, 2448, 2704, 2768, 2896, 2928, 3056, 3120, 3248, 3264, 3392, 3456, 3712, 3840, 4096
};

ALIGNED(32) const int16_t mode_to_weight_table_offset_w4_hor[35] = {
  0, 0, 0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 256, 272, 288, 304, 320, 336, 352, 368, 384, 400, 416, 432, 448, 464, 480, 496, 512
};

ALIGNED(32) const int16_t mode_to_shuffle_vector_table_offset_w4_hor[35] = {
  0, 0, 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672, 704, 736, 768, 800, 832, 864, 896, 928, 960, 992, 1024
};


ALIGNED(32) const int8_t intra_chroma_linear_interpolation_shuffle_vectors_w4_hor[] = {
  0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04,  // Mode 2
  0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05,
  0x08, 0x09, 0x09, 0x0a, 0x0a, 0x0b, 0x0b, 0x0c,
  0x09, 0x0a, 0x0a, 0x0b, 0x0b, 0x0c, 0x0c, 0x0d,
  0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04,  // Mode 3
  0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05,
  0x08, 0x09, 0x09, 0x0a, 0x0a, 0x0b, 0x0b, 0x0c,
  0x09, 0x0a, 0x0a, 0x0b, 0x0b, 0x0c, 0x0c, 0x0d,
  0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04,  // Mode 4
  0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05,
  0x08, 0x09, 0x09, 0x0a, 0x0a, 0x0b, 0x0b, 0x0c,
  0x09, 0x0a, 0x0a, 0x0b, 0x0b, 0x0c, 0x0c, 0x0d,
  0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x02, 0x03,  // Mode 5
  0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x03, 0x04,
  0x08, 0x09, 0x09, 0x0a, 0x0a, 0x0b, 0x0a, 0x0b,
  0x09, 0x0a, 0x0a, 0x0b, 0x0b, 0x0c, 0x0b, 0x0c,
  0x00, 0x01, 0x01, 0x02, 0x01, 0x02, 0x02, 0x03,  // Mode 6
  0x01, 0x02, 0x02, 0x03, 0x02, 0x03, 0x03, 0x04,
  0x08, 0x09, 0x09, 0x0a, 0x09, 0x0a, 0x0a, 0x0b,
  0x09, 0x0a, 0x0a, 0x0b, 0x0a, 0x0b, 0x0b, 0x0c,
  0x00, 0x01, 0x01, 0x02, 0x01, 0x02, 0x02, 0x03,  // Mode 7
  0x01, 0x02, 0x02, 0x03, 0x02, 0x03, 0x03, 0x04,
  0x08, 0x09, 0x09, 0x0a, 0x09, 0x0a, 0x0a, 0x0b,
  0x09, 0x0a, 0x0a, 0x0b, 0x0a, 0x0b, 0x0b, 0x0c,
  0x00, 0x01, 0x01, 0x02, 0x01, 0x02, 0x02, 0x03,  // Mode 8
  0x01, 0x02, 0x02, 0x03, 0x02, 0x03, 0x03, 0x04,
  0x08, 0x09, 0x09, 0x0a, 0x09, 0x0a, 0x0a, 0x0b,
  0x09, 0x0a, 0x0a, 0x0b, 0x0a, 0x0b, 0x0b, 0x0c,
  0x00, 0x01, 0x00, 0x01, 0x01, 0x02, 0x01, 0x02,  // Mode 9
  0x01, 0x02, 0x01, 0x02, 0x02, 0x03, 0x02, 0x03,
  0x08, 0x09, 0x08, 0x09, 0x09, 0x0a, 0x09, 0x0a,
  0x09, 0x0a, 0x09, 0x0a, 0x0a, 0x0b, 0x0a, 0x0b,
  0x00, 0x01, 0x00, 0x01, 0x01, 0x02, 0x01, 0x02,  // Mode 10
  0x01, 0x02, 0x01, 0x02, 0x02, 0x03, 0x02, 0x03,
  0x08, 0x09, 0x08, 0x09, 0x09, 0x0a, 0x09, 0x0a,
  0x09, 0x0a, 0x09, 0x0a, 0x0a, 0x0b, 0x0a, 0x0b,
  0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x01, 0x02,  // Mode 11
  0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x02, 0x03,
  0x08, 0x09, 0x08, 0x09, 0x08, 0x09, 0x09, 0x0a,
  0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a, 0x0a, 0x0b,
  0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x01, 0x02,  // Mode 12
  0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x02, 0x03,
  0x08, 0x09, 0x08, 0x09, 0x08, 0x09, 0x09, 0x0a,
  0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a, 0x0a, 0x0b,
  0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,  // Mode 13
  0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02,
  0x08, 0x09, 0x08, 0x09, 0x08, 0x09, 0x08, 0x09,
  0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a,
  0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,  // Mode 14
  0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02,
  0x08, 0x09, 0x08, 0x09, 0x08, 0x09, 0x08, 0x09,
  0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a,
  0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,  // Mode 15
  0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02,
  0x08, 0x09, 0x08, 0x09, 0x08, 0x09, 0x08, 0x09,
  0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a,
  0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,  // Mode 16
  0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02,
  0x08, 0x09, 0x08, 0x09, 0x08, 0x09, 0x08, 0x09,
  0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a,
  0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,  // Mode 17
  0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02,
  0x08, 0x09, 0x08, 0x09, 0x08, 0x09, 0x08, 0x09,
  0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a,
  0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,  // Mode 18
  0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02,
  0x08, 0x09, 0x08, 0x09, 0x08, 0x09, 0x08, 0x09,
  0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a,
  0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,  // Mode 19
  0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02,
  0x08, 0x09, 0x08, 0x09, 0x08, 0x09, 0x08, 0x09,
  0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a,
  0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,  // Mode 20
  0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02,
  0x08, 0x09, 0x08, 0x09, 0x08, 0x09, 0x08, 0x09,
  0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a,
  0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,  // Mode 21
  0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02,
  0x08, 0x09, 0x08, 0x09, 0x08, 0x09, 0x08, 0x09,
  0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a,
  0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,  // Mode 22
  0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02,
  0x08, 0x09, 0x08, 0x09, 0x08, 0x09, 0x08, 0x09,
  0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a,
  0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,  // Mode 23
  0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02,
  0x08, 0x09, 0x08, 0x09, 0x08, 0x09, 0x08, 0x09,
  0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a,
  0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01,  // Mode 24
  0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x01, 0x02,
  0x08, 0x09, 0x08, 0x09, 0x08, 0x09, 0x08, 0x09,
  0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a,
  0x01, 0x02, 0x01, 0x02, 0x01, 0x02, 0x00, 0x01,  // Mode 25
  0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x01, 0x02,
  0x09, 0x0a, 0x09, 0x0a, 0x09, 0x0a, 0x08, 0x09,
  0x0a, 0x0b, 0x0a, 0x0b, 0x0a, 0x0b, 0x09, 0x0a,
  0x01, 0x02, 0x01, 0x02, 0x00, 0x01, 0x00, 0x01,  // Mode 26
  0x02, 0x03, 0x02, 0x03, 0x01, 0x02, 0x01, 0x02,
  0x09, 0x0a, 0x09, 0x0a, 0x08, 0x09, 0x08, 0x09,
  0x0a, 0x0b, 0x0a, 0x0b, 0x09, 0x0a, 0x09, 0x0a,
  0x01, 0x02, 0x01, 0x02, 0x00, 0x01, 0x00, 0x01,  // Mode 27
  0x02, 0x03, 0x02, 0x03, 0x01, 0x02, 0x01, 0x02,
  0x09, 0x0a, 0x09, 0x0a, 0x08, 0x09, 0x08, 0x09,
  0x0a, 0x0b, 0x0a, 0x0b, 0x09, 0x0a, 0x09, 0x0a,
  0x01, 0x02, 0x01, 0x02, 0x00, 0x01, 0x00, 0x01,  // Mode 28
  0x02, 0x03, 0x02, 0x03, 0x01, 0x02, 0x01, 0x02,
  0x09, 0x0a, 0x09, 0x0a, 0x08, 0x09, 0x08, 0x09,
  0x0a, 0x0b, 0x0a, 0x0b, 0x09, 0x0a, 0x09, 0x0a,
  0x02, 0x03, 0x01, 0x02, 0x01, 0x02, 0x00, 0x01,  // Mode 29
  0x03, 0x04, 0x02, 0x03, 0x02, 0x03, 0x01, 0x02,
  0x0a, 0x0b, 0x09, 0x0a, 0x09, 0x0a, 0x08, 0x09,
  0x0b, 0x0c, 0x0a, 0x0b, 0x0a, 0x0b, 0x09, 0x0a,
  0x02, 0x03, 0x01, 0x02, 0x01, 0x02, 0x00, 0x01,  // Mode 30
  0x03, 0x04, 0x02, 0x03, 0x02, 0x03, 0x01, 0x02,
  0x0a, 0x0b, 0x09, 0x0a, 0x09, 0x0a, 0x08, 0x09,
  0x0b, 0x0c, 0x0a, 0x0b, 0x0a, 0x0b, 0x09, 0x0a,
  0x02, 0x03, 0x01, 0x02, 0x00, 0x01, 0x00, 0x01,  // Mode 31
  0x03, 0x04, 0x02, 0x03, 0x01, 0x02, 0x01, 0x02,
  0x0a, 0x0b, 0x09, 0x0a, 0x08, 0x09, 0x08, 0x09,
  0x0b, 0x0c, 0x0a, 0x0b, 0x09, 0x0a, 0x09, 0x0a,
  0x03, 0x04, 0x02, 0x03, 0x01, 0x02, 0x00, 0x01,  // Mode 32
  0x04, 0x05, 0x03, 0x04, 0x02, 0x03, 0x01, 0x02,
  0x0b, 0x0c, 0x0a, 0x0b, 0x09, 0x0a, 0x08, 0x09,
  0x0c, 0x0d, 0x0b, 0x0c, 0x0a, 0x0b, 0x09, 0x0a,
  0x03, 0x04, 0x02, 0x03, 0x01, 0x02, 0x00, 0x01,  // Mode 33
  0x04, 0x05, 0x03, 0x04, 0x02, 0x03, 0x01, 0x02,
  0x0b, 0x0c, 0x0a, 0x0b, 0x09, 0x0a, 0x08, 0x09,
  0x0c, 0x0d, 0x0b, 0x0c, 0x0a, 0x0b, 0x09, 0x0a,
  0x03, 0x04, 0x02, 0x03, 0x01, 0x02, 0x00, 0x01,  // Mode 34
  0x04, 0x05, 0x03, 0x04, 0x02, 0x03, 0x01, 0x02,
  0x0b, 0x0c, 0x0a, 0x0b, 0x09, 0x0a, 0x08, 0x09,
  0x0c, 0x0d, 0x0b, 0x0c, 0x0a, 0x0b, 0x09, 0x0a
};


// Chroma linear interpolation filter weights for width 4, horizontal modes
ALIGNED(32) const int8_t intra_chroma_linear_interpolation_weights_w4_hor[] = {
32,  0, 32,  0, 32,  0, 32,  0, 32,  0, 32,  0, 32,  0, 32,  0,   // Mode 2
 3, 29,  6, 26,  9, 23, 12, 20,  3, 29,  6, 26,  9, 23, 12, 20,   // Mode 3
 6, 26, 12, 20, 18, 14, 24,  8,  6, 26, 12, 20, 18, 14, 24,  8,   // Mode 4
 9, 23, 18, 14, 27,  5,  4, 28,  9, 23, 18, 14, 27,  5,  4, 28,   // Mode 5
12, 20, 24,  8,  4, 28, 16, 16, 12, 20, 24,  8,  4, 28, 16, 16,   // Mode 6
14, 18, 28,  4, 10, 22, 24,  8, 14, 18, 28,  4, 10, 22, 24,  8,   // Mode 7
16, 16, 32,  0, 16, 16, 32,  0, 16, 16, 32,  0, 16, 16, 32,  0,   // Mode 8
18, 14,  4, 28, 22, 10,  8, 24, 18, 14,  4, 28, 22, 10,  8, 24,   // Mode 9
20, 12,  8, 24, 28,  4, 16, 16, 20, 12,  8, 24, 28,  4, 16, 16,   // Mode 10
22, 10, 12, 20,  2, 30, 24,  8, 22, 10, 12, 20,  2, 30, 24,  8,   // Mode 11
24,  8, 16, 16,  8, 24, 32,  0, 24,  8, 16, 16,  8, 24, 32,  0,   // Mode 12
26,  6, 20, 12, 14, 18,  8, 24, 26,  6, 20, 12, 14, 18,  8, 24,   // Mode 13
28,  4, 24,  8, 20, 12, 16, 16, 28,  4, 24,  8, 20, 12, 16, 16,   // Mode 14
29,  3, 26,  6, 23,  9, 20, 12, 29,  3, 26,  6, 23,  9, 20, 12,   // Mode 15
30,  2, 28,  4, 26,  6, 24,  8, 30,  2, 28,  4, 26,  6, 24,  8,   // Mode 16
31,  1, 30,  2, 29,  3, 28,  4, 31,  1, 30,  2, 29,  3, 28,  4,   // Mode 17
32,  0, 32,  0, 32,  0, 32,  0, 32,  0, 32,  0, 32,  0, 32,  0,   // Mode 18
 1, 31,  2, 30,  3, 29,  4, 28,  1, 31,  2, 30,  3, 29,  4, 28,   // Mode 19
 2, 30,  4, 28,  6, 26,  8, 24,  2, 30,  4, 28,  6, 26,  8, 24,   // Mode 20
 3, 29,  6, 26,  9, 23, 12, 20,  3, 29,  6, 26,  9, 23, 12, 20,   // Mode 21
 4, 28,  8, 24, 12, 20, 16, 16,  4, 28,  8, 24, 12, 20, 16, 16,   // Mode 22
 6, 26, 12, 20, 18, 14, 24,  8,  6, 26, 12, 20, 18, 14, 24,  8,   // Mode 23
 8, 24, 16, 16, 24,  8, 32,  0,  8, 24, 16, 16, 24,  8, 32,  0,   // Mode 24
10, 22, 20, 12, 30,  2,  8, 24, 10, 22, 20, 12, 30,  2,  8, 24,   // Mode 25
12, 20, 24,  8,  4, 28, 16, 16, 12, 20, 24,  8,  4, 28, 16, 16,   // Mode 26
14, 18, 28,  4, 10, 22, 24,  8, 14, 18, 28,  4, 10, 22, 24,  8,   // Mode 27
16, 16, 32,  0, 16, 16, 32,  0, 16, 16, 32,  0, 16, 16, 32,  0,   // Mode 28
18, 14,  4, 28, 22, 10,  8, 24, 18, 14,  4, 28, 22, 10,  8, 24,   // Mode 29
20, 12,  8, 24, 28,  4, 16, 16, 20, 12,  8, 24, 28,  4, 16, 16,   // Mode 30
23,  9, 14, 18,  5, 27, 28,  4, 23,  9, 14, 18,  5, 27, 28,  4,   // Mode 31
26,  6, 20, 12, 14, 18,  8, 24, 26,  6, 20, 12, 14, 18,  8, 24,   // Mode 32
29,  3, 26,  6, 23,  9, 20, 12, 29,  3, 26,  6, 23,  9, 20, 12,   // Mode 33
32,  0, 32,  0, 32,  0, 32,  0, 32,  0, 32,  0, 32,  0, 32,  0,   // Mode 34
};


// Chroma linear interpolation filter weights for width 4, vertical modes.
ALIGNED(32) const int8_t intra_chroma_linear_interpolation_weights_w4_ver[4112] = {
32,  0, 32,  0, 32,  0, 32,  0, 32,  0, 32,  0, 32,  0, 32,  0,  // Mode 2 Offset 0
 3, 29,  3, 29,  3, 29,  3, 29,  6, 26,  6, 26,  6, 26,  6, 26,  // Mode 3 Offset 16
 9, 23,  9, 23,  9, 23,  9, 23, 12, 20, 12, 20, 12, 20, 12, 20,
15, 17, 15, 17, 15, 17, 15, 17, 18, 14, 18, 14, 18, 14, 18, 14,
21, 11, 21, 11, 21, 11, 21, 11, 24,  8, 24,  8, 24,  8, 24,  8,
27,  5, 27,  5, 27,  5, 27,  5, 30,  2, 30,  2, 30,  2, 30,  2,
 1, 31,  1, 31,  1, 31,  1, 31,  4, 28,  4, 28,  4, 28,  4, 28,
 7, 25,  7, 25,  7, 25,  7, 25, 10, 22, 10, 22, 10, 22, 10, 22,
13, 19, 13, 19, 13, 19, 13, 19, 16, 16, 16, 16, 16, 16, 16, 16,
19, 13, 19, 13, 19, 13, 19, 13, 22, 10, 22, 10, 22, 10, 22, 10,
25,  7, 25,  7, 25,  7, 25,  7, 28,  4, 28,  4, 28,  4, 28,  4,
31,  1, 31,  1, 31,  1, 31,  1,  2, 30,  2, 30,  2, 30,  2, 30,
 5, 27,  5, 27,  5, 27,  5, 27,  8, 24,  8, 24,  8, 24,  8, 24,
11, 21, 11, 21, 11, 21, 11, 21, 14, 18, 14, 18, 14, 18, 14, 18,
17, 15, 17, 15, 17, 15, 17, 15, 20, 12, 20, 12, 20, 12, 20, 12,
23,  9, 23,  9, 23,  9, 23,  9, 26,  6, 26,  6, 26,  6, 26,  6,
29,  3, 29,  3, 29,  3, 29,  3, 32,  0, 32,  0, 32,  0, 32,  0,
 6, 26,  6, 26,  6, 26,  6, 26, 12, 20, 12, 20, 12, 20, 12, 20,  // Mode 4 Offset 272
18, 14, 18, 14, 18, 14, 18, 14, 24,  8, 24,  8, 24,  8, 24,  8,
30,  2, 30,  2, 30,  2, 30,  2,  4, 28,  4, 28,  4, 28,  4, 28,
10, 22, 10, 22, 10, 22, 10, 22, 16, 16, 16, 16, 16, 16, 16, 16,
22, 10, 22, 10, 22, 10, 22, 10, 28,  4, 28,  4, 28,  4, 28,  4,
 2, 30,  2, 30,  2, 30,  2, 30,  8, 24,  8, 24,  8, 24,  8, 24,
14, 18, 14, 18, 14, 18, 14, 18, 20, 12, 20, 12, 20, 12, 20, 12,
26,  6, 26,  6, 26,  6, 26,  6, 32,  0, 32,  0, 32,  0, 32,  0,
 9, 23,  9, 23,  9, 23,  9, 23, 18, 14, 18, 14, 18, 14, 18, 14,  // Mode 5 Offset 400
27,  5, 27,  5, 27,  5, 27,  5,  4, 28,  4, 28,  4, 28,  4, 28,
13, 19, 13, 19, 13, 19, 13, 19, 22, 10, 22, 10, 22, 10, 22, 10,
31,  1, 31,  1, 31,  1, 31,  1,  8, 24,  8, 24,  8, 24,  8, 24,
17, 15, 17, 15, 17, 15, 17, 15, 26,  6, 26,  6, 26,  6, 26,  6,
 3, 29,  3, 29,  3, 29,  3, 29, 12, 20, 12, 20, 12, 20, 12, 20,
21, 11, 21, 11, 21, 11, 21, 11, 30,  2, 30,  2, 30,  2, 30,  2,
 7, 25,  7, 25,  7, 25,  7, 25, 16, 16, 16, 16, 16, 16, 16, 16,
25,  7, 25,  7, 25,  7, 25,  7,  2, 30,  2, 30,  2, 30,  2, 30,
11, 21, 11, 21, 11, 21, 11, 21, 20, 12, 20, 12, 20, 12, 20, 12,
29,  3, 29,  3, 29,  3, 29,  3,  6, 26,  6, 26,  6, 26,  6, 26,
15, 17, 15, 17, 15, 17, 15, 17, 24,  8, 24,  8, 24,  8, 24,  8,
 1, 31,  1, 31,  1, 31,  1, 31, 10, 22, 10, 22, 10, 22, 10, 22,
19, 13, 19, 13, 19, 13, 19, 13, 28,  4, 28,  4, 28,  4, 28,  4,
 5, 27,  5, 27,  5, 27,  5, 27, 14, 18, 14, 18, 14, 18, 14, 18,
23,  9, 23,  9, 23,  9, 23,  9, 32,  0, 32,  0, 32,  0, 32,  0,
12, 20, 12, 20, 12, 20, 12, 20, 24,  8, 24,  8, 24,  8, 24,  8,  // Mode 6 Offset 656
 4, 28,  4, 28,  4, 28,  4, 28, 16, 16, 16, 16, 16, 16, 16, 16,
28,  4, 28,  4, 28,  4, 28,  4,  8, 24,  8, 24,  8, 24,  8, 24,
20, 12, 20, 12, 20, 12, 20, 12, 32,  0, 32,  0, 32,  0, 32,  0,
14, 18, 14, 18, 14, 18, 14, 18, 28,  4, 28,  4, 28,  4, 28,  4,  // Mode 7 Offset 720
10, 22, 10, 22, 10, 22, 10, 22, 24,  8, 24,  8, 24,  8, 24,  8,
 6, 26,  6, 26,  6, 26,  6, 26, 20, 12, 20, 12, 20, 12, 20, 12,
 2, 30,  2, 30,  2, 30,  2, 30, 16, 16, 16, 16, 16, 16, 16, 16,
30,  2, 30,  2, 30,  2, 30,  2, 12, 20, 12, 20, 12, 20, 12, 20,
26,  6, 26,  6, 26,  6, 26,  6,  8, 24,  8, 24,  8, 24,  8, 24,
22, 10, 22, 10, 22, 10, 22, 10,  4, 28,  4, 28,  4, 28,  4, 28,
18, 14, 18, 14, 18, 14, 18, 14, 32,  0, 32,  0, 32,  0, 32,  0,
16, 16, 16, 16, 16, 16, 16, 16, 32,  0, 32,  0, 32,  0, 32,  0,  // Mode 8 Offset 848
18, 14, 18, 14, 18, 14, 18, 14,  4, 28,  4, 28,  4, 28,  4, 28,  // Mode 9 Offset 864
22, 10, 22, 10, 22, 10, 22, 10,  8, 24,  8, 24,  8, 24,  8, 24,
26,  6, 26,  6, 26,  6, 26,  6, 12, 20, 12, 20, 12, 20, 12, 20,
30,  2, 30,  2, 30,  2, 30,  2, 16, 16, 16, 16, 16, 16, 16, 16,
 2, 30,  2, 30,  2, 30,  2, 30, 20, 12, 20, 12, 20, 12, 20, 12,
 6, 26,  6, 26,  6, 26,  6, 26, 24,  8, 24,  8, 24,  8, 24,  8,
10, 22, 10, 22, 10, 22, 10, 22, 28,  4, 28,  4, 28,  4, 28,  4,
14, 18, 14, 18, 14, 18, 14, 18, 32,  0, 32,  0, 32,  0, 32,  0,
20, 12, 20, 12, 20, 12, 20, 12,  8, 24,  8, 24,  8, 24,  8, 24,  // Mode 10 Offset 992
28,  4, 28,  4, 28,  4, 28,  4, 16, 16, 16, 16, 16, 16, 16, 16,
 4, 28,  4, 28,  4, 28,  4, 28, 24,  8, 24,  8, 24,  8, 24,  8,
12, 20, 12, 20, 12, 20, 12, 20, 32,  0, 32,  0, 32,  0, 32,  0,
22, 10, 22, 10, 22, 10, 22, 10, 12, 20, 12, 20, 12, 20, 12, 20,  // Mode 11 Offset 1056
 2, 30,  2, 30,  2, 30,  2, 30, 24,  8, 24,  8, 24,  8, 24,  8,
14, 18, 14, 18, 14, 18, 14, 18,  4, 28,  4, 28,  4, 28,  4, 28,
26,  6, 26,  6, 26,  6, 26,  6, 16, 16, 16, 16, 16, 16, 16, 16,
 6, 26,  6, 26,  6, 26,  6, 26, 28,  4, 28,  4, 28,  4, 28,  4,
18, 14, 18, 14, 18, 14, 18, 14,  8, 24,  8, 24,  8, 24,  8, 24,
30,  2, 30,  2, 30,  2, 30,  2, 20, 12, 20, 12, 20, 12, 20, 12,
10, 22, 10, 22, 10, 22, 10, 22, 32,  0, 32,  0, 32,  0, 32,  0,
24,  8, 24,  8, 24,  8, 24,  8, 16, 16, 16, 16, 16, 16, 16, 16,  // Mode 12 Offset 1184
 8, 24,  8, 24,  8, 24,  8, 24, 32,  0, 32,  0, 32,  0, 32,  0,
26,  6, 26,  6, 26,  6, 26,  6, 20, 12, 20, 12, 20, 12, 20, 12,  // Mode 13 Offset 1216
14, 18, 14, 18, 14, 18, 14, 18,  8, 24,  8, 24,  8, 24,  8, 24,
 2, 30,  2, 30,  2, 30,  2, 30, 28,  4, 28,  4, 28,  4, 28,  4,
22, 10, 22, 10, 22, 10, 22, 10, 16, 16, 16, 16, 16, 16, 16, 16,
10, 22, 10, 22, 10, 22, 10, 22,  4, 28,  4, 28,  4, 28,  4, 28,
30,  2, 30,  2, 30,  2, 30,  2, 24,  8, 24,  8, 24,  8, 24,  8,
18, 14, 18, 14, 18, 14, 18, 14, 12, 20, 12, 20, 12, 20, 12, 20,
 6, 26,  6, 26,  6, 26,  6, 26, 32,  0, 32,  0, 32,  0, 32,  0,
28,  4, 28,  4, 28,  4, 28,  4, 24,  8, 24,  8, 24,  8, 24,  8,  // Mode 14 Offset 1344
20, 12, 20, 12, 20, 12, 20, 12, 16, 16, 16, 16, 16, 16, 16, 16,
12, 20, 12, 20, 12, 20, 12, 20,  8, 24,  8, 24,  8, 24,  8, 24,
 4, 28,  4, 28,  4, 28,  4, 28, 32,  0, 32,  0, 32,  0, 32,  0,
29,  3, 29,  3, 29,  3, 29,  3, 26,  6, 26,  6, 26,  6, 26,  6,  // Mode 15 Offset 1408
23,  9, 23,  9, 23,  9, 23,  9, 20, 12, 20, 12, 20, 12, 20, 12,
17, 15, 17, 15, 17, 15, 17, 15, 14, 18, 14, 18, 14, 18, 14, 18,
11, 21, 11, 21, 11, 21, 11, 21,  8, 24,  8, 24,  8, 24,  8, 24,
 5, 27,  5, 27,  5, 27,  5, 27,  2, 30,  2, 30,  2, 30,  2, 30,
31,  1, 31,  1, 31,  1, 31,  1, 28,  4, 28,  4, 28,  4, 28,  4,
25,  7, 25,  7, 25,  7, 25,  7, 22, 10, 22, 10, 22, 10, 22, 10,
19, 13, 19, 13, 19, 13, 19, 13, 16, 16, 16, 16, 16, 16, 16, 16,
13, 19, 13, 19, 13, 19, 13, 19, 10, 22, 10, 22, 10, 22, 10, 22,
 7, 25,  7, 25,  7, 25,  7, 25,  4, 28,  4, 28,  4, 28,  4, 28,
 1, 31,  1, 31,  1, 31,  1, 31, 30,  2, 30,  2, 30,  2, 30,  2,
27,  5, 27,  5, 27,  5, 27,  5, 24,  8, 24,  8, 24,  8, 24,  8,
21, 11, 21, 11, 21, 11, 21, 11, 18, 14, 18, 14, 18, 14, 18, 14,
15, 17, 15, 17, 15, 17, 15, 17, 12, 20, 12, 20, 12, 20, 12, 20,
 9, 23,  9, 23,  9, 23,  9, 23,  6, 26,  6, 26,  6, 26,  6, 26,
 3, 29,  3, 29,  3, 29,  3, 29, 32,  0, 32,  0, 32,  0, 32,  0,
30,  2, 30,  2, 30,  2, 30,  2, 28,  4, 28,  4, 28,  4, 28,  4,  // Mode 16 Offset 1664
26,  6, 26,  6, 26,  6, 26,  6, 24,  8, 24,  8, 24,  8, 24,  8,
22, 10, 22, 10, 22, 10, 22, 10, 20, 12, 20, 12, 20, 12, 20, 12,
18, 14, 18, 14, 18, 14, 18, 14, 16, 16, 16, 16, 16, 16, 16, 16,
14, 18, 14, 18, 14, 18, 14, 18, 12, 20, 12, 20, 12, 20, 12, 20,
10, 22, 10, 22, 10, 22, 10, 22,  8, 24,  8, 24,  8, 24,  8, 24,
 6, 26,  6, 26,  6, 26,  6, 26,  4, 28,  4, 28,  4, 28,  4, 28,
 2, 30,  2, 30,  2, 30,  2, 30, 32,  0, 32,  0, 32,  0, 32,  0,
31,  1, 31,  1, 31,  1, 31,  1, 30,  2, 30,  2, 30,  2, 30,  2,  // Mode 17 Offset 1792
29,  3, 29,  3, 29,  3, 29,  3, 28,  4, 28,  4, 28,  4, 28,  4,
27,  5, 27,  5, 27,  5, 27,  5, 26,  6, 26,  6, 26,  6, 26,  6,
25,  7, 25,  7, 25,  7, 25,  7, 24,  8, 24,  8, 24,  8, 24,  8,
23,  9, 23,  9, 23,  9, 23,  9, 22, 10, 22, 10, 22, 10, 22, 10,
21, 11, 21, 11, 21, 11, 21, 11, 20, 12, 20, 12, 20, 12, 20, 12,
19, 13, 19, 13, 19, 13, 19, 13, 18, 14, 18, 14, 18, 14, 18, 14,
17, 15, 17, 15, 17, 15, 17, 15, 16, 16, 16, 16, 16, 16, 16, 16,
15, 17, 15, 17, 15, 17, 15, 17, 14, 18, 14, 18, 14, 18, 14, 18,
13, 19, 13, 19, 13, 19, 13, 19, 12, 20, 12, 20, 12, 20, 12, 20,
11, 21, 11, 21, 11, 21, 11, 21, 10, 22, 10, 22, 10, 22, 10, 22,
 9, 23,  9, 23,  9, 23,  9, 23,  8, 24,  8, 24,  8, 24,  8, 24,
 7, 25,  7, 25,  7, 25,  7, 25,  6, 26,  6, 26,  6, 26,  6, 26,
 5, 27,  5, 27,  5, 27,  5, 27,  4, 28,  4, 28,  4, 28,  4, 28,
 3, 29,  3, 29,  3, 29,  3, 29,  2, 30,  2, 30,  2, 30,  2, 30,
 1, 31,  1, 31,  1, 31,  1, 31, 32,  0, 32,  0, 32,  0, 32,  0,
32,  0, 32,  0, 32,  0, 32,  0, 32,  0, 32,  0, 32,  0, 32,  0,  // Mode 18 Offset 2048
 1, 31,  1, 31,  1, 31,  1, 31,  2, 30,  2, 30,  2, 30,  2, 30,  // Mode 19 Offset 2064
 3, 29,  3, 29,  3, 29,  3, 29,  4, 28,  4, 28,  4, 28,  4, 28,
 5, 27,  5, 27,  5, 27,  5, 27,  6, 26,  6, 26,  6, 26,  6, 26,
 7, 25,  7, 25,  7, 25,  7, 25,  8, 24,  8, 24,  8, 24,  8, 24,
 9, 23,  9, 23,  9, 23,  9, 23, 10, 22, 10, 22, 10, 22, 10, 22,
11, 21, 11, 21, 11, 21, 11, 21, 12, 20, 12, 20, 12, 20, 12, 20,
13, 19, 13, 19, 13, 19, 13, 19, 14, 18, 14, 18, 14, 18, 14, 18,
15, 17, 15, 17, 15, 17, 15, 17, 16, 16, 16, 16, 16, 16, 16, 16,
17, 15, 17, 15, 17, 15, 17, 15, 18, 14, 18, 14, 18, 14, 18, 14,
19, 13, 19, 13, 19, 13, 19, 13, 20, 12, 20, 12, 20, 12, 20, 12,
21, 11, 21, 11, 21, 11, 21, 11, 22, 10, 22, 10, 22, 10, 22, 10,
23,  9, 23,  9, 23,  9, 23,  9, 24,  8, 24,  8, 24,  8, 24,  8,
25,  7, 25,  7, 25,  7, 25,  7, 26,  6, 26,  6, 26,  6, 26,  6,
27,  5, 27,  5, 27,  5, 27,  5, 28,  4, 28,  4, 28,  4, 28,  4,
29,  3, 29,  3, 29,  3, 29,  3, 30,  2, 30,  2, 30,  2, 30,  2,
31,  1, 31,  1, 31,  1, 31,  1, 32,  0, 32,  0, 32,  0, 32,  0,
 2, 30,  2, 30,  2, 30,  2, 30,  4, 28,  4, 28,  4, 28,  4, 28,  // Mode 20 Offset 2320
 6, 26,  6, 26,  6, 26,  6, 26,  8, 24,  8, 24,  8, 24,  8, 24,
10, 22, 10, 22, 10, 22, 10, 22, 12, 20, 12, 20, 12, 20, 12, 20,
14, 18, 14, 18, 14, 18, 14, 18, 16, 16, 16, 16, 16, 16, 16, 16,
18, 14, 18, 14, 18, 14, 18, 14, 20, 12, 20, 12, 20, 12, 20, 12,
22, 10, 22, 10, 22, 10, 22, 10, 24,  8, 24,  8, 24,  8, 24,  8,
26,  6, 26,  6, 26,  6, 26,  6, 28,  4, 28,  4, 28,  4, 28,  4,
30,  2, 30,  2, 30,  2, 30,  2, 32,  0, 32,  0, 32,  0, 32,  0,
 3, 29,  3, 29,  3, 29,  3, 29,  6, 26,  6, 26,  6, 26,  6, 26,  // Mode 21 Offset 2448
 9, 23,  9, 23,  9, 23,  9, 23, 12, 20, 12, 20, 12, 20, 12, 20,
15, 17, 15, 17, 15, 17, 15, 17, 18, 14, 18, 14, 18, 14, 18, 14,
21, 11, 21, 11, 21, 11, 21, 11, 24,  8, 24,  8, 24,  8, 24,  8,
27,  5, 27,  5, 27,  5, 27,  5, 30,  2, 30,  2, 30,  2, 30,  2,
 1, 31,  1, 31,  1, 31,  1, 31,  4, 28,  4, 28,  4, 28,  4, 28,
 7, 25,  7, 25,  7, 25,  7, 25, 10, 22, 10, 22, 10, 22, 10, 22,
13, 19, 13, 19, 13, 19, 13, 19, 16, 16, 16, 16, 16, 16, 16, 16,
19, 13, 19, 13, 19, 13, 19, 13, 22, 10, 22, 10, 22, 10, 22, 10,
25,  7, 25,  7, 25,  7, 25,  7, 28,  4, 28,  4, 28,  4, 28,  4,
31,  1, 31,  1, 31,  1, 31,  1,  2, 30,  2, 30,  2, 30,  2, 30,
 5, 27,  5, 27,  5, 27,  5, 27,  8, 24,  8, 24,  8, 24,  8, 24,
11, 21, 11, 21, 11, 21, 11, 21, 14, 18, 14, 18, 14, 18, 14, 18,
17, 15, 17, 15, 17, 15, 17, 15, 20, 12, 20, 12, 20, 12, 20, 12,
23,  9, 23,  9, 23,  9, 23,  9, 26,  6, 26,  6, 26,  6, 26,  6,
29,  3, 29,  3, 29,  3, 29,  3, 32,  0, 32,  0, 32,  0, 32,  0,
 4, 28,  4, 28,  4, 28,  4, 28,  8, 24,  8, 24,  8, 24,  8, 24,  // Mode 22 Offset 2704
12, 20, 12, 20, 12, 20, 12, 20, 16, 16, 16, 16, 16, 16, 16, 16,
20, 12, 20, 12, 20, 12, 20, 12, 24,  8, 24,  8, 24,  8, 24,  8,
28,  4, 28,  4, 28,  4, 28,  4, 32,  0, 32,  0, 32,  0, 32,  0,
 6, 26,  6, 26,  6, 26,  6, 26, 12, 20, 12, 20, 12, 20, 12, 20,  // Mode 23 Offset 2768
18, 14, 18, 14, 18, 14, 18, 14, 24,  8, 24,  8, 24,  8, 24,  8,
30,  2, 30,  2, 30,  2, 30,  2,  4, 28,  4, 28,  4, 28,  4, 28,
10, 22, 10, 22, 10, 22, 10, 22, 16, 16, 16, 16, 16, 16, 16, 16,
22, 10, 22, 10, 22, 10, 22, 10, 28,  4, 28,  4, 28,  4, 28,  4,
 2, 30,  2, 30,  2, 30,  2, 30,  8, 24,  8, 24,  8, 24,  8, 24,
14, 18, 14, 18, 14, 18, 14, 18, 20, 12, 20, 12, 20, 12, 20, 12,
26,  6, 26,  6, 26,  6, 26,  6, 32,  0, 32,  0, 32,  0, 32,  0,
 8, 24,  8, 24,  8, 24,  8, 24, 16, 16, 16, 16, 16, 16, 16, 16,  // Mode 24 Offset 2896
24,  8, 24,  8, 24,  8, 24,  8, 32,  0, 32,  0, 32,  0, 32,  0,
10, 22, 10, 22, 10, 22, 10, 22, 20, 12, 20, 12, 20, 12, 20, 12,  // Mode 25 Offset 2928
30,  2, 30,  2, 30,  2, 30,  2,  8, 24,  8, 24,  8, 24,  8, 24,
18, 14, 18, 14, 18, 14, 18, 14, 28,  4, 28,  4, 28,  4, 28,  4,
 6, 26,  6, 26,  6, 26,  6, 26, 16, 16, 16, 16, 16, 16, 16, 16,
26,  6, 26,  6, 26,  6, 26,  6,  4, 28,  4, 28,  4, 28,  4, 28,
14, 18, 14, 18, 14, 18, 14, 18, 24,  8, 24,  8, 24,  8, 24,  8,
 2, 30,  2, 30,  2, 30,  2, 30, 12, 20, 12, 20, 12, 20, 12, 20,
22, 10, 22, 10, 22, 10, 22, 10, 32,  0, 32,  0, 32,  0, 32,  0,
12, 20, 12, 20, 12, 20, 12, 20, 24,  8, 24,  8, 24,  8, 24,  8,  // Mode 26 Offset 3056
 4, 28,  4, 28,  4, 28,  4, 28, 16, 16, 16, 16, 16, 16, 16, 16,
28,  4, 28,  4, 28,  4, 28,  4,  8, 24,  8, 24,  8, 24,  8, 24,
20, 12, 20, 12, 20, 12, 20, 12, 32,  0, 32,  0, 32,  0, 32,  0,
14, 18, 14, 18, 14, 18, 14, 18, 28,  4, 28,  4, 28,  4, 28,  4,  // Mode 27 Offset 3120
10, 22, 10, 22, 10, 22, 10, 22, 24,  8, 24,  8, 24,  8, 24,  8,
 6, 26,  6, 26,  6, 26,  6, 26, 20, 12, 20, 12, 20, 12, 20, 12,
 2, 30,  2, 30,  2, 30,  2, 30, 16, 16, 16, 16, 16, 16, 16, 16,
30,  2, 30,  2, 30,  2, 30,  2, 12, 20, 12, 20, 12, 20, 12, 20,
26,  6, 26,  6, 26,  6, 26,  6,  8, 24,  8, 24,  8, 24,  8, 24,
22, 10, 22, 10, 22, 10, 22, 10,  4, 28,  4, 28,  4, 28,  4, 28,
18, 14, 18, 14, 18, 14, 18, 14, 32,  0, 32,  0, 32,  0, 32,  0,
16, 16, 16, 16, 16, 16, 16, 16, 32,  0, 32,  0, 32,  0, 32,  0,  // Mode 28 Offset 3248
18, 14, 18, 14, 18, 14, 18, 14,  4, 28,  4, 28,  4, 28,  4, 28,  // Mode 29 Offset 3264
22, 10, 22, 10, 22, 10, 22, 10,  8, 24,  8, 24,  8, 24,  8, 24,
26,  6, 26,  6, 26,  6, 26,  6, 12, 20, 12, 20, 12, 20, 12, 20,
30,  2, 30,  2, 30,  2, 30,  2, 16, 16, 16, 16, 16, 16, 16, 16,
 2, 30,  2, 30,  2, 30,  2, 30, 20, 12, 20, 12, 20, 12, 20, 12,
 6, 26,  6, 26,  6, 26,  6, 26, 24,  8, 24,  8, 24,  8, 24,  8,
10, 22, 10, 22, 10, 22, 10, 22, 28,  4, 28,  4, 28,  4, 28,  4,
14, 18, 14, 18, 14, 18, 14, 18, 32,  0, 32,  0, 32,  0, 32,  0,
20, 12, 20, 12, 20, 12, 20, 12,  8, 24,  8, 24,  8, 24,  8, 24,  // Mode 30 Offset 3392
28,  4, 28,  4, 28,  4, 28,  4, 16, 16, 16, 16, 16, 16, 16, 16,
 4, 28,  4, 28,  4, 28,  4, 28, 24,  8, 24,  8, 24,  8, 24,  8,
12, 20, 12, 20, 12, 20, 12, 20, 32,  0, 32,  0, 32,  0, 32,  0,
23,  9, 23,  9, 23,  9, 23,  9, 14, 18, 14, 18, 14, 18, 14, 18,  // Mode 31 Offset 3456
 5, 27,  5, 27,  5, 27,  5, 27, 28,  4, 28,  4, 28,  4, 28,  4,
19, 13, 19, 13, 19, 13, 19, 13, 10, 22, 10, 22, 10, 22, 10, 22,
 1, 31,  1, 31,  1, 31,  1, 31, 24,  8, 24,  8, 24,  8, 24,  8,
15, 17, 15, 17, 15, 17, 15, 17,  6, 26,  6, 26,  6, 26,  6, 26,
29,  3, 29,  3, 29,  3, 29,  3, 20, 12, 20, 12, 20, 12, 20, 12,
11, 21, 11, 21, 11, 21, 11, 21,  2, 30,  2, 30,  2, 30,  2, 30,
25,  7, 25,  7, 25,  7, 25,  7, 16, 16, 16, 16, 16, 16, 16, 16,
 7, 25,  7, 25,  7, 25,  7, 25, 30,  2, 30,  2, 30,  2, 30,  2,
21, 11, 21, 11, 21, 11, 21, 11, 12, 20, 12, 20, 12, 20, 12, 20,
 3, 29,  3, 29,  3, 29,  3, 29, 26,  6, 26,  6, 26,  6, 26,  6,
17, 15, 17, 15, 17, 15, 17, 15,  8, 24,  8, 24,  8, 24,  8, 24,
31,  1, 31,  1, 31,  1, 31,  1, 22, 10, 22, 10, 22, 10, 22, 10,
13, 19, 13, 19, 13, 19, 13, 19,  4, 28,  4, 28,  4, 28,  4, 28,
27,  5, 27,  5, 27,  5, 27,  5, 18, 14, 18, 14, 18, 14, 18, 14,
 9, 23,  9, 23,  9, 23,  9, 23, 32,  0, 32,  0, 32,  0, 32,  0,
26,  6, 26,  6, 26,  6, 26,  6, 20, 12, 20, 12, 20, 12, 20, 12,  // Mode 32 Offset 3712
14, 18, 14, 18, 14, 18, 14, 18,  8, 24,  8, 24,  8, 24,  8, 24,
 2, 30,  2, 30,  2, 30,  2, 30, 28,  4, 28,  4, 28,  4, 28,  4,
22, 10, 22, 10, 22, 10, 22, 10, 16, 16, 16, 16, 16, 16, 16, 16,
10, 22, 10, 22, 10, 22, 10, 22,  4, 28,  4, 28,  4, 28,  4, 28,
30,  2, 30,  2, 30,  2, 30,  2, 24,  8, 24,  8, 24,  8, 24,  8,
18, 14, 18, 14, 18, 14, 18, 14, 12, 20, 12, 20, 12, 20, 12, 20,
 6, 26,  6, 26,  6, 26,  6, 26, 32,  0, 32,  0, 32,  0, 32,  0,
29,  3, 29,  3, 29,  3, 29,  3, 26,  6, 26,  6, 26,  6, 26,  6,  // Mode 33 Offset 3840
23,  9, 23,  9, 23,  9, 23,  9, 20, 12, 20, 12, 20, 12, 20, 12,
17, 15, 17, 15, 17, 15, 17, 15, 14, 18, 14, 18, 14, 18, 14, 18,
11, 21, 11, 21, 11, 21, 11, 21,  8, 24,  8, 24,  8, 24,  8, 24,
 5, 27,  5, 27,  5, 27,  5, 27,  2, 30,  2, 30,  2, 30,  2, 30,
31,  1, 31,  1, 31,  1, 31,  1, 28,  4, 28,  4, 28,  4, 28,  4,
25,  7, 25,  7, 25,  7, 25,  7, 22, 10, 22, 10, 22, 10, 22, 10,
19, 13, 19, 13, 19, 13, 19, 13, 16, 16, 16, 16, 16, 16, 16, 16,
13, 19, 13, 19, 13, 19, 13, 19, 10, 22, 10, 22, 10, 22, 10, 22,
 7, 25,  7, 25,  7, 25,  7, 25,  4, 28,  4, 28,  4, 28,  4, 28,
 1, 31,  1, 31,  1, 31,  1, 31, 30,  2, 30,  2, 30,  2, 30,  2,
27,  5, 27,  5, 27,  5, 27,  5, 24,  8, 24,  8, 24,  8, 24,  8,
21, 11, 21, 11, 21, 11, 21, 11, 18, 14, 18, 14, 18, 14, 18, 14,
15, 17, 15, 17, 15, 17, 15, 17, 12, 20, 12, 20, 12, 20, 12, 20,
 9, 23,  9, 23,  9, 23,  9, 23,  6, 26,  6, 26,  6, 26,  6, 26,
 3, 29,  3, 29,  3, 29,  3, 29, 32,  0, 32,  0, 32,  0, 32,  0,
32,  0, 32,  0, 32,  0, 32,  0, 32,  0, 32,  0, 32,  0, 32,  0,  // Mode 34 Offset 4096
};

#endif INTRA_AVX2_TABLES_H
