/* Copyright (c) 2016 PaddlePaddle Authors. All Rights Reserve.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#pragma once
#include "paddle/fluid/operators/elementwise_op_function.h"

namespace paddle {
namespace operators {

template <typename T>
struct SubFunctor {
  inline HOSTDEVICE T operator()(T a, T b) const { return a - b; }
};

template <typename DeviceContext, typename T>
class ElementwiseSubKernel : public framework::OpKernel<T> {
 public:
  void Compute(const framework::ExecutionContext& ctx) const override {
    using Tensor = framework::Tensor;

    auto* x = ctx.Input<Tensor>("X");
    auto* y = ctx.Input<Tensor>("Y");
    auto* z = ctx.Output<Tensor>("Out");
    z->mutable_data<T>(ctx.GetPlace());
    int axis = ctx.Attr<int>("axis");
    ElementwiseComputeEx<SubFunctor<T>, DeviceContext, T>(ctx, x, y, axis,
                                                          SubFunctor<T>(), z);
  }
};

template <typename T>
struct ElementwiseSubGradFunctor {
  template <typename Device, typename X, typename Y, typename Z, typename dX,
            typename dY, typename dZ>
  void operator()(Device d, X x, Y y, Z z, dX dx, dY dy, dZ dz) {
    auto dz_e = framework::EigenVector<T>::Flatten(*dz);
    if (dx) {
      auto dx_e = framework::EigenVector<T>::Flatten(*dx);
      dx_e.device(d) = dz_e;
    }
    if (dy) {
      auto dy_e = framework::EigenVector<T>::Flatten(*dy);
      dy_e.device(d) = (-1.0) * dz_e;
    }
  }
};

template <typename T>
struct ElementwiseSubBroadCastGradFunctor {
  template <typename Device, typename X, typename Y, typename Z, typename dX,
            typename dY, typename dZ, typename Pre, typename N>
  void operator()(Device d, X x, Y y, Z z, dX dx, dY dy, dZ dz, Pre pre, N n) {
    auto dz_e = framework::EigenVector<T>::Flatten(*dz);
    if (dx) {
      auto dx_e = framework::EigenVector<T>::Flatten(*dx);
      dx_e.device(d) = dz_e;
    }

    if (dy) {
      auto dy_e = framework::EigenVector<T>::Flatten(*dy);
      dy_e.device(d) = (-1.0) *
                       dz_e.reshape(Eigen::DSizes<int, 2>(pre, n))
                           .sum(Eigen::array<int, 1>{{0}});
    }
  }
};

template <typename T>
struct ElementwiseSubBroadCast2GradFunctor {
  template <typename Device, typename X, typename Y, typename Z, typename dX,
            typename dY, typename dZ, typename Pre, typename N, typename Post>
  void operator()(Device d, X x, Y y, Z z, dX dx, dY dy, dZ dz, Pre pre, N n,
                  Post post) {
    auto dz_e = framework::EigenVector<T>::Flatten(*dz);
    if (dx) {
      auto dx_e = framework::EigenVector<T>::Flatten(*dx);
      dx_e.device(d) = dz_e;
    }

    if (dy) {
      auto dy_e = framework::EigenVector<T>::Flatten(*dy);
      dy_e.device(d) = (-1.0) *
                       dz_e.reshape(Eigen::DSizes<int, 3>(pre, n, post))
                           .sum(Eigen::array<int, 2>{{0, 2}});
    }
  }
};

template <typename DeviceContext, typename T>
class ElementwiseSubGradKernel : public framework::OpKernel<T> {
 public:
  void Compute(const framework::ExecutionContext& ctx) const override {
    using Tensor = framework::Tensor;

    auto* x = ctx.Input<Tensor>("X");
    auto* y = ctx.Input<Tensor>("Y");
    auto* out = ctx.Input<Tensor>("Out");
    auto* dout = ctx.Input<Tensor>(framework::GradVarName("Out"));
    auto* dx = ctx.Output<Tensor>(framework::GradVarName("X"));
    auto* dy = ctx.Output<Tensor>(framework::GradVarName("Y"));
    int axis = ctx.Attr<int>("axis");
    ElementwiseGradCompute<DeviceContext, T, ElementwiseSubGradFunctor<T>,
                           ElementwiseSubBroadCastGradFunctor<T>,
                           ElementwiseSubBroadCast2GradFunctor<T>>(
        ctx, x, y, out, dout, axis, dx, dy);
  }
};

}  // namespace operators
}  // namespace paddle
