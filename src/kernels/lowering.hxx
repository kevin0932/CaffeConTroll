
#ifndef _KERNEL_LOWERING_HXX
#define _KERNEL_LOWERING_HXX

#include "lowering.h"

#ifdef _GPU_TARGET
__host__ __device__
#endif
inline float device_min(const float a, const float b) {
    return (a>b)?b:a;
}

#ifdef _GPU_TARGET
inline __host__ __device__ // Inline to prevent multiple definitions
#endif
size_t _f_src_to_dst_inverse_lower_cube(size_t src_pos, void * const _arg) {
  const _inverse_lower_cube_arg_helper * const arg = (_inverse_lower_cube_arg_helper *) _arg;
  return (src_pos / arg->kernel_size / arg->kernel_size / arg->data_output_width / \
      arg->data_output_height / arg->iB) * arg->iR * arg->iC;
}

#ifdef _GPU_TARGET
inline __host__ __device__ // Inline to prevent multiple definitions
#endif
void _f_inverse_lower_cube(void * input, void * output, void * const _arg, const size_t dst_index) {

  const _inverse_lower_cube_arg_helper * const arg = (_inverse_lower_cube_arg_helper *) _arg;
  const size_t data_output_width = arg->data_output_width;
  const size_t data_output_height = arg->data_output_height;
  const size_t kernel_size = arg->kernel_size;
  const size_t stride = arg->stride;
  const size_t padding = arg->padding;
  const int iR = arg->iR;
  const int iC = arg->iC;
  const int iD = arg->iD;
  const unsigned int iB = arg->iB; // unsigned because, otherwise, we get a warning on line 45

  float * const input_data = (float *) input;
  const float * const output_data = (float *) output;

  size_t out_index = 0;
  // First, we iterate over K * K, which is the number of rows in the output gradient
  // cube for a given depth D. (Remember: the output gradient cube has dimensions
  // K * K * iD x oR * oC * iB x 1 x 1, where oR and oC do NOT refer the variables above.)
  for (size_t kr = 0; kr < kernel_size; ++kr) {
    for (size_t kc = 0; kc < kernel_size; ++kc) {

      // Then, we iterate over oR * oC * iB, the number of columns in the output gradient
      // cr and cc represent the row index and column index of the convolutional "window"
      // in the input gradient cube, which means that they must be incremented by stride
      for (size_t ib = 0; ib < iB; ++ib) {
        const int batch_offset = ib*iR*iC*iD;

        // (cr + kr - padding, cc + kc - padding) represents the index into
        // the input gradient cube. If we aren't within [0, iR) and [0, iC)
        // then we shouldn't update, because we are in the padded area
        for (size_t cr = 0; cr < stride * data_output_width; cr += stride) {
          const int input_row_index = cr + kr - padding;

          if (input_row_index >= 0 && input_row_index < iR) {
            const int row_offset = input_row_index*iC;

            for (size_t cc = 0; cc < stride * data_output_height; cc += stride) {
              const int input_col_index = cc + kc - padding;

              if (input_col_index >= 0 && input_col_index < iC) {
                input_data[input_col_index + row_offset + batch_offset] += output_data[out_index];
              }
              // increment out_index regardless, a single cell from the output gradient cube
              // can only make a single contribution to the input gradient cube (Remember: this
              // is the *lowered* output gradient cube!)
              ++out_index;
            }
          } else {
            out_index += data_output_height; // if we skip this row, we need still need to update out_index
          }
        }
      }
    }
  }
}

#ifdef _GPU_TARGET
inline __host__ __device__
#endif
void _f_lower_cube(void * output, void * input, void * const _arg, const size_t dst_index, const size_t b_i,
    const size_t d_i) {

  // const _lower_cube_arg_helper * const arg = (_lower_cube_arg_helper *) _arg;
  // const size_t kernel_size  = arg->kernel_size;
  // const size_t stride       = arg->stride;
  // const size_t padding      = arg->padding;
  // const int height          = arg->iR;
  // const int width           = arg->iC;

  // // const int height = input_matrix->R;
  // // const int width = input_matrix->C;

  // // const T * const input_data = input_matrix->p_data;
  // const float * const input_data = (float *) input;

  // const size_t single_lowering_width  = (width + 2 * padding - kernel_size) / stride + 1;
  // const size_t single_lowering_height = (height + 2 * padding - kernel_size) / stride + 1;

  // const size_t dst_row_base = d_i * kernel_size * kernel_size; // K^2*D
  // const size_t dst_col_base = b_i * single_lowering_height * single_lowering_width; // M^2*B

  // const int num_height_windows  = (height + 2 * padding - kernel_size) / stride + 1; // number of convolution "windows" row-wise
  // const int num_width_windows   = (width + 2 * padding - kernel_size) / stride + 1;  // number of convolution "windows" column-wise

  // // i & j keep track of which "window" we're currently calculating. Incremented by 1 each time.
  // // src_row_base and src_col_base indicate the starting indices for the window. Incremented by stride each time.
  // // dst_col increases every time we calculate a new window. Incremented by 1 each time.
  // for (int src_row_base = -padding, i = 0, dst_col = dst_col_base; i < num_height_windows; src_row_base += stride, ++i) {
  //   for (int src_col_base = -padding, j = 0; j < num_width_windows; src_col_base += stride, ++dst_col, ++j) {
  //     // src_row and src_col start at src_row_base and src_col_base, respectively, and iterate a total of kernel_size times. Incremented by 1 each time.
  //     // dst_row_i starts at dst_row_base. Incremented by kernel_size each time.
  //     // dst_row starts at dst_row_i. Incremented by 1 each time.
  //     for (int src_row = src_row_base, dst_row_i = dst_row_base; src_row < kernel_size + src_row_base; ++src_row, dst_row_i += kernel_size) {
  //       for (int src_col = src_col_base, dst_row = dst_row_i; src_col < kernel_size + src_col_base; ++src_col, ++dst_row) {
  //         const size_t dst = dst_col + dst_row*cube.C;
  //         if (src_row < 0 || src_row >= width || src_col < 0 || src_col >= height) {
  //           cube.p_data[dst] = 0;
  //         } else {
  //           cube.p_data[dst] = input_data[src_row*width + src_col];
  //         }
  //       }
  //     }
  //   }
  // }
}

#ifdef _GPU_TARGET
__host__ __device__
#endif
inline void _fpmap_id(Block2D * const output_block, const Block2D * const input_block, const PMapHelper * const args) {
  output_block->r = 0;
  output_block->c = 0;
  output_block->d = 0;
  output_block->d = 0;
  output_block->dr = args->kR;
  output_block->dc = args->kC;
};

#ifdef _GPU_TARGET
__host__ __device__
#endif
// return next multiple of s greater than x, if x % s > 0. Otherwise, x remains the same.
inline int next_multiple(int x, const int s) {
  const int m = x % s;
  x += (x > 0 && m > 0) ? s - m : -m;
  return x;
}

#ifdef _GPU_TARGET
__host__ __device__
#endif
// return smallest j >= 0 such that x + j*stride >= p
inline int next_largest_multiple(const int x, const int p, const int stride) {
  if (x >= p) return x;
  const int q = (p - x)/stride;
  int y       = x + q*stride;
  if (y < p) { y += stride; }
  return y;
}


#ifdef _GPU_TARGET
__host__ __device__
#endif
inline void _fmap_lower(float * output, const Block2D * const output_block, const PointIn2DBlock * const input_point, const PMapHelper * const args) {

  // PROFILE_ONLY(Timer t; float seconds_elapsed = 0.;)
  const int ir = (int) input_point->r;
  const int ic = (int) input_point->c;
  const int ib = (int) input_point->block.b;
  const int id = (int) input_point->block.d;

  const int kR = (int) args->kR;
  const int kC = (int) args->kC;
  const int iR = (int) args->sR;
  const int iC = (int) args->sC;
  const int iB = (int) args->sB;

  const int padding = (int) args->padding;
  const int stride = (int) args->stride;
#ifdef _DO_ASSERT
  assert(stride > 0);
#endif

  const int output_R = (iR - kR + 2*padding) / stride + 1;
  const int output_C = (iC - kC + 2*padding) / stride + 1;

  const int o_base_col = ib * output_R * output_C;

  const int o_base_row = id * kR * kC;
  const int oC = iB * output_R * output_C;

  const float input = input_point->data;

  // First, calculate the bounds for the row iteration. r_begin and r_end are the
  // bounds for the for loop on line 173; they are calculated to be equivalent to the
  // following code:
  //
  // for (int r = next_multiple(ir - kR + 1, stride); r < ir + 1; r += stride) {
  //   if (r >= -padding && r < (iR - kR + 1) + padding) {
  //     .......
  //     .......
  //     .......
  //   }
  // }
  //
  int r_begin       = next_multiple(ir - kR + 1 + padding, stride) - padding;
  r_begin           = next_largest_multiple(r_begin, -padding, stride);
  const int r_end   = device_min(ir, iR - kR + padding) + 1; // std::min is inlined
  const int i_start = (r_begin + padding) / stride; // Invariant: index i will always equal r / stride

  // Do the same for the column iteration. Same bounds as above apply, but for
  // the column indices instead of the row indices
  int c_begin       = next_multiple(ic - kC + 1 + padding, stride) - padding;
  c_begin           = next_largest_multiple(c_begin, -padding, stride);
  const int c_end   = device_min(ic, iC - kC + padding) + 1;
  const int j_start = (c_begin + padding) / stride;

  // Note that  r + padding >= 0 since r >= r_begin >= -padding. (These also hold true for the index c.)
  // This is an important invariant for the blocking optimization below.
  // To see why, if r + padding were allowed to be negative, then set stride = 4, padding = 0, r = -2
  // r        = -2, 2, 6
  // i        =  0, 1, 2
  // r/stride =  0, 0, 1 <-- we would count incorrectly around 0, invariant above would not hold.
  for (int r = r_begin, i = i_start; r < r_end; r += stride, ++i) {
    const int dr = ir - r;
    const int drKc = dr*kC;
    const int ioC  = i*output_C;

    for (int c = c_begin, j = j_start; c < c_end; c += stride, j++) {
      const int dc = ic - c;
      const int o_col_offset = ioC + j;
      const int o_row_offset = drKc + dc;

      const int o_col = o_col_offset + o_base_col;
      const int o_row = o_row_offset + o_base_row;

      output[o_col + o_row*oC] = input;
    }
  }
  // PROFILE_ONLY(seconds_elapsed = t.elapsed(); std::cout << "LOWERING PROFILE _fmap_lower: " << seconds_elapsed << " seconds." << std::endl; t.restart();)
}

#ifdef _GPU_TARGET
__host__ __device__
#endif
inline void _fmap_remap(float * output, const Block2D * const output_block, const PointIn2DBlock * const input_point, const PMapHelper * const args) {
  const size_t ir = input_point->r;
  const size_t ic = input_point->c;
  const size_t ib = input_point->block.b;
  const size_t id = input_point->block.d;

  const size_t iR = args->sR;
  const size_t iC = args->sC;
  const size_t iB = args->sB;
  const size_t iD = args->sD;

  const size_t reald = (id + ib * iD) / (iB);
  const size_t realb = (id + ib * iD) % (iB);

  const float input = input_point->data;

  output[ic + ir*iC + reald*iR*iC + realb*iR*iC*iD] = input;
}

#endif
