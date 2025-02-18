// Copyright 2020 Google LLC
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <xnnpack.h>
#include <xnnpack/log.h>
#include <xnnpack/params.h>
#include <xnnpack/subgraph.h>


static enum xnn_status create_subtract_operator(
  const struct xnn_node* node,
  const struct xnn_value* values,
  size_t num_values,
  struct xnn_operator_data* opdata,
  struct xnn_code_cache* code_cache)
{
  assert(node->num_inputs == 2);
  const uint32_t input1_id = node->inputs[0];
  assert(input1_id != XNN_INVALID_VALUE_ID);
  assert(input1_id < num_values);
  const uint32_t input2_id = node->inputs[1];
  assert(input2_id != XNN_INVALID_VALUE_ID);
  assert(input2_id < num_values);

  assert(node->num_outputs == 1);
  const uint32_t output_id = node->outputs[0];
  assert(output_id != XNN_INVALID_VALUE_ID);
  assert(output_id < num_values);

  enum xnn_status status;
  switch (node->compute_type) {
    case xnn_compute_type_fp32:
      status = xnn_create_subtract_nd_f32(
        node->activation.output_min,
        node->activation.output_max,
        node->flags,
        &opdata->operator_object);
      break;
#ifndef XNN_NO_QS8_OPERATORS
    case xnn_compute_type_qs8:
    {
      const float output_scale = values[output_id].quantization.scale;
      const int32_t output_zero_point = values[output_id].quantization.zero_point;
      const int8_t output_min =
        (int8_t) lrintf(fminf(fmaxf(node->activation.output_min / output_scale + (float) output_zero_point, -128.0f), 127.0f));
      const int8_t output_max =
        (int8_t) lrintf(fminf(fmaxf(node->activation.output_max / output_scale + (float) output_zero_point, -128.0f), 127.0f));
      status = xnn_create_subtract_nd_qs8(
        (int8_t) values[input1_id].quantization.zero_point,
        values[input1_id].quantization.scale,
        (int8_t) values[input2_id].quantization.zero_point,
        values[input2_id].quantization.scale,
        (int8_t) output_zero_point,
        output_scale, output_min, output_max, node->flags,
        &opdata->operator_object);
      break;
    }
#endif  // !defined(XNN_NO_QS8_OPERATORS)
#ifndef XNN_NO_QU8_OPERATORS
    case xnn_compute_type_qu8:
    {
      const float output_scale = values[output_id].quantization.scale;
      const int32_t output_zero_point = values[output_id].quantization.zero_point;
      const uint8_t output_min =
        (uint8_t) lrintf(fminf(fmaxf(node->activation.output_min / output_scale + (float) output_zero_point, 0.0f), 255.0f));
      const uint8_t output_max =
        (uint8_t) lrintf(fminf(fmaxf(node->activation.output_max / output_scale + (float) output_zero_point, 0.0f), 255.0f));
      status = xnn_create_subtract_nd_qu8(
        (uint8_t) values[input1_id].quantization.zero_point,
        values[input1_id].quantization.scale,
        (uint8_t) values[input2_id].quantization.zero_point,
        values[input2_id].quantization.scale,
        (uint8_t) output_zero_point,
        output_scale, output_min, output_max, node->flags,
        &opdata->operator_object);
      break;
    }
#endif  // !defined(XNN_NO_QU8_OPERATORS)
    default:
      XNN_UNREACHABLE;
  }
  if (status == xnn_status_success) {
    opdata->shape1.num_dims = values[input1_id].shape.num_dims;
    opdata->shape2.num_dims = values[input2_id].shape.num_dims;
    if (values[output_id].layout == xnn_layout_type_nchw) {
      assert(values[input1_id].layout == xnn_layout_type_nchw);
      assert(values[input2_id].layout == xnn_layout_type_nchw);
      opdata->shape1.dim[0] = values[input1_id].shape.dim[0];
      opdata->shape1.dim[1] = values[input1_id].shape.dim[values[input1_id].shape.num_dims - 1];
      if (values[input1_id].shape.num_dims > 2) {
        memcpy(&opdata->shape1.dim[2], &values[input1_id].shape.dim[1], (values[input1_id].shape.num_dims - 2) * sizeof(size_t));
      }
      opdata->shape2.dim[0] = values[input2_id].shape.dim[0];
      opdata->shape2.dim[1] = values[input2_id].shape.dim[values[input2_id].shape.num_dims - 1];
      if (values[input1_id].shape.num_dims > 2) {
        memcpy(&opdata->shape2.dim[2], &values[input2_id].shape.dim[1], (values[input2_id].shape.num_dims - 2) * sizeof(size_t));
      }
    } else {
      assert(values[output_id].layout == xnn_layout_type_nhwc);
      assert(values[input1_id].layout == xnn_layout_type_nhwc);
      assert(values[input2_id].layout == xnn_layout_type_nhwc);
      memcpy(opdata->shape1.dim, values[input1_id].shape.dim, values[input1_id].shape.num_dims * sizeof(size_t));
      memcpy(opdata->shape2.dim, values[input2_id].shape.dim, values[input2_id].shape.num_dims * sizeof(size_t));
    }
    opdata->inputs[0] = input1_id;
    opdata->inputs[1] = input2_id;
    opdata->outputs[0] = output_id;
  }
  return status;
}

static enum xnn_status setup_subtract_operator(
  const struct xnn_operator_data* opdata,
  const struct xnn_blob* blobs,
  size_t num_blobs,
  pthreadpool_t threadpool)
{
  const uint32_t input1_id = opdata->inputs[0];
  assert(input1_id != XNN_INVALID_VALUE_ID);
  assert(input1_id < num_blobs);

  const uint32_t input2_id = opdata->inputs[1];
  assert(input2_id != XNN_INVALID_VALUE_ID);
  assert(input2_id < num_blobs);

  const uint32_t output_id = opdata->outputs[0];
  assert(output_id != XNN_INVALID_VALUE_ID);
  assert(output_id < num_blobs);

  const struct xnn_blob* input1_blob = blobs + input1_id;
  const void* input1_data = input1_blob->data;
  assert(input1_data != NULL);

  const struct xnn_blob* input2_blob = blobs + input2_id;
  const void* input2_data = input2_blob->data;
  assert(input2_data != NULL);

  const struct xnn_blob* output_blob = blobs + output_id;
  void* output_data = output_blob->data;
  assert(output_data != NULL);

  switch (opdata->operator_object->type) {
    case xnn_operator_type_subtract_nd_f32:
      return xnn_setup_subtract_nd_f32(
        opdata->operator_object,
        opdata->shape1.num_dims,
        opdata->shape1.dim,
        opdata->shape2.num_dims,
        opdata->shape2.dim,
        input1_data, input2_data, output_data,
        threadpool);
      break;
#ifndef XNN_NO_QS8_OPERATORS
    case xnn_operator_type_subtract_nd_qs8:
      return xnn_setup_subtract_nd_qs8(
        opdata->operator_object,
        opdata->shape1.num_dims,
        opdata->shape1.dim,
        opdata->shape2.num_dims,
        opdata->shape2.dim,
        input1_data, input2_data, output_data,
        threadpool);
      break;
#endif  // !defined(XNN_NO_QS8_OPERATORS)
#ifndef XNN_NO_QU8_OPERATORS
    case xnn_operator_type_subtract_nd_qu8:
      return xnn_setup_subtract_nd_qu8(
        opdata->operator_object,
        opdata->shape1.num_dims,
        opdata->shape1.dim,
        opdata->shape2.num_dims,
        opdata->shape2.dim,
        input1_data, input2_data, output_data,
        threadpool);
      break;
#endif  // !defined(XNN_NO_QU8_OPERATORS)
    default:
      XNN_UNREACHABLE;
  }
}

enum xnn_status xnn_define_subtract(
  xnn_subgraph_t subgraph,
  float output_min,
  float output_max,
  uint32_t input1_id,
  uint32_t input2_id,
  uint32_t output_id,
  uint32_t flags)
{
  if ((xnn_params.init_flags & XNN_INIT_FLAG_XNNPACK) == 0) {
    xnn_log_error("failed to define %s operator: XNNPACK is not initialized",
      xnn_node_type_to_string(xnn_node_type_subtract));
    return xnn_status_uninitialized;
  }

  if (isnan(output_min)) {
    xnn_log_error(
      "failed to define %s operator with NaN output lower bound: lower bound must be non-NaN",
      xnn_node_type_to_string(xnn_node_type_subtract));
    return xnn_status_invalid_parameter;
  }

  if (isnan(output_max)) {
    xnn_log_error(
      "failed to define %s operator with NaN output upper bound: upper bound must be non-NaN",
      xnn_node_type_to_string(xnn_node_type_subtract));
    return xnn_status_invalid_parameter;
  }

  if (output_min >= output_max) {
    xnn_log_error(
      "failed to define %s operator with [%.7g, %.7g] output range: lower bound must be below upper bound",
      xnn_node_type_to_string(xnn_node_type_subtract), output_min, output_max);
    return xnn_status_invalid_parameter;
  }

  if (input1_id >= subgraph->num_values) {
    xnn_log_error(
      "failed to define %s operator with the first input ID #%" PRIu32 ": invalid Value ID",
      xnn_node_type_to_string(xnn_node_type_subtract), input1_id);
    return xnn_status_invalid_parameter;
  }

  const struct xnn_value* input1_value = &subgraph->values[input1_id];
  if (input1_value->type != xnn_value_type_dense_tensor) {
    xnn_log_error(
      "failed to define %s operator with the first input ID #%" PRIu32 ": unsupported Value type %d (expected dense tensor)",
      xnn_node_type_to_string(xnn_node_type_subtract), input1_id, input1_value->type);
    return xnn_status_invalid_parameter;
  }

  switch (input1_value->datatype) {
    case xnn_datatype_fp32:
#ifndef XNN_NO_QS8_OPERATORS
    case xnn_datatype_qint8:
#endif  // !defined(XNN_NO_QS8_OPERATORS)
#ifndef XNN_NO_QU8_OPERATORS
    case xnn_datatype_quint8:
#endif  // !defined(XNN_NO_QU8_OPERATORS)
      break;
    default:
      xnn_log_error(
        "failed to define %s operator with the first input ID #%" PRIu32 ": unsupported Value datatype %s (%d)",
        xnn_node_type_to_string(xnn_node_type_subtract), input1_id,
        xnn_datatype_to_string(input1_value->datatype), input1_value->datatype);
      return xnn_status_invalid_parameter;
  }

  if (input2_id >= subgraph->num_values) {
    xnn_log_error(
      "failed to define %s operator with the second input ID #%" PRIu32 ": invalid Value ID",
      xnn_node_type_to_string(xnn_node_type_subtract), input2_id);
    return xnn_status_invalid_parameter;
  }

  const struct xnn_value* input2_value = &subgraph->values[input2_id];
  if (input2_value->type != xnn_value_type_dense_tensor) {
    xnn_log_error(
      "failed to define %s operator with the second input ID #%" PRIu32 ": unsupported Value type %d (expected dense tensor)",
      xnn_node_type_to_string(xnn_node_type_subtract), input2_id, input2_value->type);
    return xnn_status_invalid_parameter;
  }

  switch (input2_value->datatype) {
    case xnn_datatype_fp32:
#ifndef XNN_NO_QS8_OPERATORS
    case xnn_datatype_qint8:
#endif  // !defined(XNN_NO_QS8_OPERATORS)
#ifndef XNN_NO_QU8_OPERATORS
    case xnn_datatype_quint8:
#endif  // !defined(XNN_NO_QU8_OPERATORS)
      break;
    default:
      xnn_log_error(
        "failed to define %s operator with the second input ID #%" PRIu32 ": unsupported Value datatype %s (%d)",
        xnn_node_type_to_string(xnn_node_type_subtract), input2_id,
        xnn_datatype_to_string(input2_value->datatype), input2_value->datatype);
      return xnn_status_invalid_parameter;
  }

  if (output_id >= subgraph->num_values) {
    xnn_log_error(
      "failed to define %s operator with output ID #%" PRIu32 ": invalid Value ID",
      xnn_node_type_to_string(xnn_node_type_subtract), output_id);
    return xnn_status_invalid_parameter;
  }

  const struct xnn_value* output_value = &subgraph->values[output_id];
  if (output_value->type != xnn_value_type_dense_tensor) {
    xnn_log_error(
      "failed to define %s operator with output ID #%" PRIu32 ": unsupported Value type %d (expected dense tensor)",
      xnn_node_type_to_string(xnn_node_type_subtract), output_id, output_value->type);
    return xnn_status_invalid_parameter;
  }

  enum xnn_compute_type compute_type = xnn_compute_type_invalid;
  switch (output_value->datatype) {
    case xnn_datatype_fp32:
      compute_type = xnn_compute_type_fp32;
      break;
#ifndef XNN_NO_QS8_OPERATORS
    case xnn_datatype_qint8:
      compute_type = xnn_compute_type_qs8;
      break;
#endif  // !defined(XNN_NO_QS8_OPERATORS)
#ifndef XNN_NO_QU8_OPERATORS
    case xnn_datatype_quint8:
      compute_type = xnn_compute_type_qu8;
      break;
#endif  // !defined(XNN_NO_QU8_OPERATORS)
    default:
      xnn_log_error(
        "failed to define %s operator with output ID #%" PRIu32 ": unsupported Value datatype %s (%d)",
        xnn_node_type_to_string(xnn_node_type_subtract), output_id,
        xnn_datatype_to_string(output_value->datatype), output_value->datatype);
      return xnn_status_invalid_parameter;
  }

  if (input1_value->datatype != input2_value->datatype ||
      input1_value->datatype != output_value->datatype)
  {
    xnn_log_error(
      "failed to define %s operator with input IDs #%" PRIu32 " and #%" PRIu32 " and output ID #%" PRIu32
      ": mismatching datatypes across the first input (%s), the second input (%s), and output (%s)",
      xnn_node_type_to_string(xnn_node_type_subtract), input1_id, input2_id, output_id,
      xnn_datatype_to_string(input1_value->datatype),
      xnn_datatype_to_string(input2_value->datatype),
      xnn_datatype_to_string(output_value->datatype));
    return xnn_status_invalid_parameter;
  }

  struct xnn_node* node = xnn_subgraph_new_node(subgraph);
  if (node == NULL) {
    return xnn_status_out_of_memory;
  }

  node->type = xnn_node_type_subtract;
  node->compute_type = compute_type;
  node->activation.output_min = output_min;
  node->activation.output_max = output_max;
  node->num_inputs = 2;
  node->inputs[0] = input1_id;
  node->inputs[1] = input2_id;
  node->num_outputs = 1;
  node->outputs[0] = output_id;
  node->flags = flags;

  node->create = create_subtract_operator;
  node->setup = setup_subtract_operator;

  return xnn_status_success;
}
