/* SPDX-FileCopyrightText: 2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "BLI_math_vector_types.hh"
#include "BLI_utildefines.h"

#include "GPU_shader.hh"

#include "COM_context.hh"
#include "COM_conversion_operation.hh"
#include "COM_input_descriptor.hh"
#include "COM_result.hh"
#include "COM_utilities.hh"
#include "COM_utilities_type_conversion.hh"

namespace blender::compositor {

ConversionOperation::ConversionOperation(Context &context,
                                         const ResultType input_type,
                                         const ResultType expected_type)
    : SimpleOperation(context)
{
  this->declare_input_descriptor(InputDescriptor{input_type});
  this->populate_result(context.create_result(expected_type));
}

void ConversionOperation::execute()
{
  Result &result = this->get_result();
  const Result &input = this->get_input();

  if (input.is_single_value()) {
    result.allocate_single_value();
    this->execute_single(input, result);
    return;
  }

  result.allocate_texture(input.domain());
  if (this->context().use_gpu()) {
    GPUShader *shader = this->context().get_shader(this->get_conversion_shader_name());
    GPU_shader_bind(shader);

    if (this->get_input().type() == ResultType::Color &&
        ELEM(this->get_result().type(), ResultType::Float, ResultType::Int))
    {
      float luminance_coefficients[3];
      IMB_colormanagement_get_luminance_coefficients(luminance_coefficients);
      GPU_shader_uniform_3fv(shader, "luminance_coefficients_u", luminance_coefficients);
    }

    input.bind_as_texture(shader, "input_tx");
    result.bind_as_image(shader, "output_img");

    compute_dispatch_threads_at_least(shader, input.domain().size);

    input.unbind_as_texture();
    result.unbind_as_image();
    GPU_shader_unbind();
  }
  else {
    this->execute_cpu(input, result);
  }
}

SimpleOperation *ConversionOperation::construct_if_needed(Context &context,
                                                          const Result &input_result,
                                                          const InputDescriptor &input_descriptor)
{
  if (input_descriptor.skip_type_conversion) {
    return nullptr;
  }

  const ResultType result_type = input_result.type();
  const ResultType expected_type = input_descriptor.type;
  if (result_type != expected_type) {
    return new ConversionOperation(context, result_type, expected_type);
  }
  return nullptr;
}

const char *ConversionOperation::get_conversion_shader_name()
{
  switch (this->get_input().type()) {
    case ResultType::Float:
      switch (this->get_result().type()) {
        case ResultType::Int:
          return "compositor_convert_float_to_int";
        case ResultType::Float3:
          return "compositor_convert_float_to_float3";
        case ResultType::Color:
          return "compositor_convert_float_to_color";
        case ResultType::Float4:
          return "compositor_convert_float_to_float4";
        case ResultType::Float:
          /* Same type, no conversion needed. */
          break;
        case ResultType::Float2:
        case ResultType::Int2:
          /* Types are not user facing, so we needn't implement them. */
          break;
      }
      break;
    case ResultType::Int:
      switch (this->get_result().type()) {
        case ResultType::Float:
          return "compositor_convert_int_to_float";
        case ResultType::Float3:
          return "compositor_convert_int_to_float3";
        case ResultType::Color:
          return "compositor_convert_int_to_color";
        case ResultType::Float4:
          return "compositor_convert_int_to_float4";
        case ResultType::Int:
          /* Same type, no conversion needed. */
          break;
        case ResultType::Float2:
        case ResultType::Int2:
          /* Types are not user facing, so we needn't implement them. */
          break;
      }
      break;
    case ResultType::Float3:
      switch (this->get_result().type()) {
        case ResultType::Float:
          return "compositor_convert_float3_to_float";
        case ResultType::Int:
          return "compositor_convert_float3_to_int";
        case ResultType::Color:
          return "compositor_convert_float3_to_color";
        case ResultType::Float4:
          return "compositor_convert_float3_to_float4";
        case ResultType::Float3:
          /* Same type, no conversion needed. */
          break;
        case ResultType::Float2:
        case ResultType::Int2:
          /* Types are not user facing, so we needn't implement them. */
          break;
      }
      break;
    case ResultType::Color:
      switch (this->get_result().type()) {
        case ResultType::Float:
          return "compositor_convert_color_to_float";
        case ResultType::Int:
          return "compositor_convert_color_to_int";
        case ResultType::Float3:
          return "compositor_convert_color_to_float3";
        case ResultType::Float4:
          return "compositor_convert_color_to_float4";
        case ResultType::Color:
          /* Same type, no conversion needed. */
          break;
        case ResultType::Float2:
        case ResultType::Int2:
          /* Types are not user facing, so we needn't implement them. */
          break;
      }
      break;
    case ResultType::Float4:
      switch (this->get_result().type()) {
        case ResultType::Float:
          return "compositor_convert_float4_to_float";
        case ResultType::Int:
          return "compositor_convert_float4_to_int";
        case ResultType::Float3:
          return "compositor_convert_float4_to_float3";
        case ResultType::Color:
          return "compositor_convert_float4_to_color";
        case ResultType::Float4:
          /* Same type, no conversion needed. */
          break;
        case ResultType::Float2:
        case ResultType::Int2:
          /* Types are not user facing, so we needn't implement them. */
          break;
      }
      break;
    case ResultType::Float2:
    case ResultType::Int2:
      /* Types are not user facing, so we needn't implement them. */
      break;
  }

  BLI_assert_unreachable();
  return nullptr;
}

void ConversionOperation::execute_single(const Result &input, Result &output)
{
  switch (this->get_input().type()) {
    case ResultType::Float:
      switch (this->get_result().type()) {
        case ResultType::Int:
          output.set_single_value(float_to_int(input.get_single_value<float>()));
          return;
        case ResultType::Float3:
          output.set_single_value(float_to_float3(input.get_single_value<float>()));
          return;
        case ResultType::Color:
          output.set_single_value(float_to_color(input.get_single_value<float>()));
          return;
        case ResultType::Float4:
          output.set_single_value(float_to_float4(input.get_single_value<float>()));
          return;
        case ResultType::Float:
          /* Same type, no conversion needed. */
          break;
        case ResultType::Float2:
        case ResultType::Int2:
          /* Types are not user facing, so we needn't implement them. */
          break;
      }
      break;
    case ResultType::Int:
      switch (this->get_result().type()) {
        case ResultType::Float:
          output.set_single_value(int_to_float(input.get_single_value<int32_t>()));
          return;
        case ResultType::Float3:
          output.set_single_value(int_to_float3(input.get_single_value<int32_t>()));
          return;
        case ResultType::Color:
          output.set_single_value(int_to_color(input.get_single_value<int32_t>()));
          return;
        case ResultType::Float4:
          output.set_single_value(int_to_float4(input.get_single_value<int32_t>()));
          return;
        case ResultType::Int:
          /* Same type, no conversion needed. */
          break;
        case ResultType::Float2:
        case ResultType::Int2:
          /* Types are not user facing, so we needn't implement them. */
          break;
      }
      break;
    case ResultType::Float3:
      switch (this->get_result().type()) {
        case ResultType::Float:
          output.set_single_value(float3_to_float(input.get_single_value<float3>()));
          return;
        case ResultType::Int:
          output.set_single_value(float3_to_int(input.get_single_value<float3>()));
          return;
        case ResultType::Color:
          output.set_single_value(float3_to_color(input.get_single_value<float3>()));
          return;
        case ResultType::Float4:
          output.set_single_value(float3_to_float4(input.get_single_value<float3>()));
          return;
        case ResultType::Float3:
          /* Same type, no conversion needed. */
          break;
        case ResultType::Float2:
        case ResultType::Int2:
          /* Types are not user facing, so we needn't implement them. */
          break;
      }
      break;
    case ResultType::Color:
      switch (this->get_result().type()) {
        case ResultType::Float:
          output.set_single_value(color_to_float(input.get_single_value<float4>()));
          return;
        case ResultType::Int:
          output.set_single_value(color_to_int(input.get_single_value<float4>()));
          return;
        case ResultType::Float3:
          output.set_single_value(color_to_float3(input.get_single_value<float4>()));
          return;
        case ResultType::Float4:
          output.set_single_value(color_to_float4(input.get_single_value<float4>()));
          return;
        case ResultType::Color:
          /* Same type, no conversion needed. */
          break;
        case ResultType::Float2:
        case ResultType::Int2:
          /* Types are not user facing, so we needn't implement them. */
          break;
      }
      break;
    case ResultType::Float4:
      switch (this->get_result().type()) {
        case ResultType::Float:
          output.set_single_value(float4_to_float(input.get_single_value<float4>()));
          return;
        case ResultType::Int:
          output.set_single_value(float4_to_int(input.get_single_value<float4>()));
          return;
        case ResultType::Float3:
          output.set_single_value(float4_to_float3(input.get_single_value<float4>()));
          return;
        case ResultType::Color:
          output.set_single_value(float4_to_color(input.get_single_value<float4>()));
          return;
        case ResultType::Float4:
          /* Same type, no conversion needed. */
          break;
        case ResultType::Float2:
        case ResultType::Int2:
          /* Types are not user facing, so we needn't implement them. */
          break;
      }
      break;
    case ResultType::Float2:
    case ResultType::Int2:
      /* Types are not user facing, so we needn't implement them. */
      break;
  }

  BLI_assert_unreachable();
}

void ConversionOperation::execute_cpu(const Result &input, Result &output)
{
  switch (this->get_input().type()) {
    case ResultType::Float:
      switch (this->get_result().type()) {
        case ResultType::Int:
          parallel_for(input.domain().size, [&](const int2 texel) {
            output.store_pixel(texel, float_to_int(input.load_pixel<float>(texel)));
          });
          return;
        case ResultType::Float3:
          parallel_for(input.domain().size, [&](const int2 texel) {
            output.store_pixel(texel, float_to_float3(input.load_pixel<float>(texel)));
          });
          return;
        case ResultType::Color:
          parallel_for(input.domain().size, [&](const int2 texel) {
            output.store_pixel(texel, float_to_color(input.load_pixel<float>(texel)));
          });
          return;
        case ResultType::Float4:
          parallel_for(input.domain().size, [&](const int2 texel) {
            output.store_pixel(texel, float_to_float4(input.load_pixel<float>(texel)));
          });
          return;
        case ResultType::Float:
          /* Same type, no conversion needed. */
          break;
        case ResultType::Float2:
        case ResultType::Int2:
          /* Types are not user facing, so we needn't implement them. */
          break;
      }
      break;
    case ResultType::Int:
      switch (this->get_result().type()) {
        case ResultType::Float:
          parallel_for(input.domain().size, [&](const int2 texel) {
            output.store_pixel(texel, int_to_float(input.load_pixel<int32_t>(texel)));
          });
          return;
        case ResultType::Float3:
          parallel_for(input.domain().size, [&](const int2 texel) {
            output.store_pixel(texel, int_to_float3(input.load_pixel<int32_t>(texel)));
          });
          return;
        case ResultType::Color:
          parallel_for(input.domain().size, [&](const int2 texel) {
            output.store_pixel(texel, int_to_color(input.load_pixel<int32_t>(texel)));
          });
          return;
        case ResultType::Float4:
          parallel_for(input.domain().size, [&](const int2 texel) {
            output.store_pixel(texel, int_to_float4(input.load_pixel<int32_t>(texel)));
          });
          return;
        case ResultType::Int:
          /* Same type, no conversion needed. */
          break;
        case ResultType::Float2:
        case ResultType::Int2:
          /* Types are not user facing, so we needn't implement them. */
          break;
      }
      break;
    case ResultType::Float3:
      switch (this->get_result().type()) {
        case ResultType::Float:
          parallel_for(input.domain().size, [&](const int2 texel) {
            output.store_pixel(texel, float3_to_float(input.load_pixel<float3>(texel)));
          });
          return;
        case ResultType::Int:
          parallel_for(input.domain().size, [&](const int2 texel) {
            output.store_pixel(texel, float3_to_int(input.load_pixel<float3>(texel)));
          });
          return;
        case ResultType::Color:
          parallel_for(input.domain().size, [&](const int2 texel) {
            output.store_pixel(texel, float3_to_color(input.load_pixel<float3>(texel)));
          });
          return;
        case ResultType::Float4:
          parallel_for(input.domain().size, [&](const int2 texel) {
            output.store_pixel(texel, float3_to_float4(input.load_pixel<float3>(texel)));
          });
          return;
        case ResultType::Float3:
          /* Same type, no conversion needed. */
          break;
        case ResultType::Float2:
        case ResultType::Int2:
          /* Types are not user facing, so we needn't implement them. */
          break;
      }
      break;
    case ResultType::Color:
      switch (this->get_result().type()) {
        case ResultType::Float:
          parallel_for(input.domain().size, [&](const int2 texel) {
            output.store_pixel(texel, color_to_float(input.load_pixel<float4>(texel)));
          });
          return;
        case ResultType::Int:
          parallel_for(input.domain().size, [&](const int2 texel) {
            output.store_pixel(texel, color_to_int(input.load_pixel<float4>(texel)));
          });
          return;
        case ResultType::Float3:
          parallel_for(input.domain().size, [&](const int2 texel) {
            output.store_pixel(texel, color_to_float3(input.load_pixel<float4>(texel)));
          });
          return;
        case ResultType::Float4:
          parallel_for(input.domain().size, [&](const int2 texel) {
            output.store_pixel(texel, color_to_float4(input.load_pixel<float4>(texel)));
          });
          return;
        case ResultType::Color:
          /* Same type, no conversion needed. */
          break;
        case ResultType::Float2:
        case ResultType::Int2:
          /* Types are not user facing, so we needn't implement them. */
          break;
      }
      break;
    case ResultType::Float4:
      switch (this->get_result().type()) {
        case ResultType::Float:
          parallel_for(input.domain().size, [&](const int2 texel) {
            output.store_pixel(texel, float4_to_float(input.load_pixel<float4>(texel)));
          });
          return;
        case ResultType::Int:
          parallel_for(input.domain().size, [&](const int2 texel) {
            output.store_pixel(texel, float4_to_int(input.load_pixel<float4>(texel)));
          });
          return;
        case ResultType::Float3:
          parallel_for(input.domain().size, [&](const int2 texel) {
            output.store_pixel(texel, float4_to_float3(input.load_pixel<float4>(texel)));
          });
          return;
        case ResultType::Color:
          parallel_for(input.domain().size, [&](const int2 texel) {
            output.store_pixel(texel, float4_to_color(input.load_pixel<float4>(texel)));
          });
          return;
        case ResultType::Float4:
          /* Same type, no conversion needed. */
          break;
        case ResultType::Float2:
        case ResultType::Int2:
          /* Types are not user facing, so we needn't implement them. */
          break;
      }
      break;
    case ResultType::Float2:
    case ResultType::Int2:
      /* Types are not user facing, so we needn't implement them. */
      break;
  }

  BLI_assert_unreachable();
}

}  // namespace blender::compositor
