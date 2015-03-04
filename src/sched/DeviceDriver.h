
#include <functional>
#include <math.h>
#include <random>
#include "DeviceMemoryPointer.h"
#include "cblas.h"

#ifndef _DEVICE_DRIVER_H
#define _DEVICE_DRIVER_H

using std::max;
using std::min;
using std::random_device;
using std::mt19937;
using std::uniform_real_distribution;
using std::bernoulli_distribution;
using std::normal_distribution;

/**
 * A DeviceDriver is the only way
 * that CcT talks to a certain device
 * to invoke computation and data
 * movement *INSIDE* a device. 
 *
 * Given a DeviceDriver, all Bridges
 * should be purely *logical*. This is
 * the goal of introducing DeviceDriver.
 *
 * All cross device operation needs to
 * be done via derefercing a DeviceMemoryPointer,
 * it is not DeviceDriver's job to worry
 * about cross-device data movement.
 *
 * A DeviceDriver needs to provide certain
 * interface, e.g., how BLAS can be called,
 * or different helper functions, e.g.,
 * axpy.
 * 
 * One question is what function should we
 * put in DeviceDriver and what function should
 * we put in Util? The answer is that
 * Util contains all functions that are
 * device-independent, e.g., get_learning_rate,
 * and DeviceDriver contains all functions
 * that are device-dependent. 
 *
 * Error handelling. All functions return void, however
 * if error occurs, it assert(false). In short, we
 * assume it is the worker's repsonbility to deal
 * with error, instead of the caller.
 *
 * TODO:
 *  - Template this by double, float etc.
 **/
class DeviceDriver{
public:

  /**
   * Memory manipulation.
   **/
  virtual void memcpy(DeviceMemoryPointer dst, DeviceMemoryPointer src) = 0;
  virtual void memset(DeviceMemoryPointer dst, const char value) = 0;

  /**
   * Single-precision operations.
   **/
  virtual void smath_axpy(const float alpha, DeviceMemoryPointer X, DeviceMemoryPointer Y) = 0;
  virtual void sapply(DeviceMemoryPointer dst, size_t n_element, std::function<void(float&)> func) = 0;
  virtual void smath_axpby(const float alpha, DeviceMemoryPointer X, const float beta, DeviceMemoryPointer Y) = 0;
  virtual void set_num_threads(const int nThreads) = 0;

  /**
   * Single-precison random number generator.
   **/
  virtual std::function<void(float&)> srand_uni(float, float) = 0;
  virtual std::function<void(float&)> srand_bern(float) = 0;
  virtual std::function<void(float&)> srand_gaussian(float, float) = 0;

  /**
   * Logical functions that only depends on other virtual functions.
   **/
    void sinitialize_xavier(DeviceMemoryPointer arr, const size_t n_arr_elements, const size_t n_batch) {
      const size_t fan_in = n_arr_elements / n_batch;
      const float scale = sqrt(3.0 / fan_in);
      auto f_uni = this->srand_uni(-scale, scale);
      sapply(arr, n_arr_elements, f_uni);
    }

   void bernoulli_initialize(DeviceMemoryPointer arr, const size_t n_arr_elements, const float p) {
      auto f_bern = this->srand_bern(p);
      sapply(arr, n_arr_elements, f_bern);
    }

    void gaussian_initialize(DeviceMemoryPointer arr, const size_t n_arr_elements, const float mean, const float std_dev) {
      auto f_gaussian = this->srand_gaussian(mean, std_dev);
      sapply(arr, n_arr_elements, f_gaussian);
    }

    void constant_initialize(DeviceMemoryPointer arr, const float value, const size_t n_arr_elements) {
      auto f_set_to_const = [=](float & b) { b = value; };
      sapply(arr, n_arr_elements, f_set_to_const);
    }

    void smath_apply_grad(DeviceMemoryPointer X, DeviceMemoryPointer Y) {
      smath_axpy(-1.0, Y, X);
    }

};

#endif











