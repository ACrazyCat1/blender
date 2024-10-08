/* SPDX-FileCopyrightText: 2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup gpu
 *
 * Mimics old style OpenGL immediate mode drawing.
 */

#include "vk_immediate.hh"
#include "vk_backend.hh"
#include "vk_data_conversion.hh"
#include "vk_framebuffer.hh"
#include "vk_state_manager.hh"

namespace blender::gpu {

VKImmediate::VKImmediate() {}
VKImmediate::~VKImmediate() {}

uchar *VKImmediate::begin()
{
  VKContext &context = *VKContext::get();
  const VKWorkarounds &workarounds = VKBackend::get().device.workarounds_get();
  vertex_format_converter.init(&vertex_format, workarounds);
  const size_t bytes_needed = vertex_buffer_size(&vertex_format_converter.device_format_get(),
                                                 vertex_len);
  const bool new_buffer_needed = !has_active_resource() || buffer_bytes_free() < bytes_needed;

  std::unique_ptr<VKBuffer> &buffer = tracked_resource_for(context, new_buffer_needed);
  current_subbuffer_len_ = bytes_needed;

  uchar *data = static_cast<uchar *>(buffer->mapped_memory_get());
  return data + subbuffer_offset_get();
}

void VKImmediate::end()
{
  BLI_assert_msg(prim_type != GPU_PRIM_NONE, "Illegal state: not between an immBegin/End pair.");
  if (vertex_idx == 0) {
    return;
  }

  if (vertex_format_converter.needs_conversion()) {
    /* Determine the start of the subbuffer. The `vertex_data` attribute changes when new vertices
     * are loaded.
     */
    uchar *data = static_cast<uchar *>(active_resource()->mapped_memory_get()) +
                  subbuffer_offset_get();
    vertex_format_converter.convert(data, data, vertex_idx);
  }

  VKContext &context = *VKContext::get();
  BLI_assert(context.shader == unwrap(shader));
  render_graph::VKResourceAccessInfo &resource_access_info = context.update_and_get_access_info();
  VKStateManager &state_manager = context.state_manager_get();
  state_manager.apply_state();
  vertex_attributes_.update_bindings(*this);
  vertex_attributes_.ensure_vbos_uploaded();
  context.active_framebuffer_get()->rendering_ensure(context);

  render_graph::VKDrawNode::CreateInfo draw(resource_access_info);
  draw.node_data.vertex_count = vertex_idx;
  draw.node_data.instance_count = 1;
  draw.node_data.first_vertex = 0;
  draw.node_data.first_instance = 0;
  vertex_attributes_.bind(draw.node_data.vertex_buffers);
  context.update_pipeline_data(prim_type, vertex_attributes_, draw.node_data.pipeline_data);

  context.render_graph.add_node(draw);

  buffer_offset_ += current_subbuffer_len_;
  current_subbuffer_len_ = 0;
  vertex_format_converter.reset();
}

VkDeviceSize VKImmediate::subbuffer_offset_get()
{
  return buffer_offset_;
}

VkDeviceSize VKImmediate::buffer_bytes_free()
{
  return active_resource()->size_in_bytes() - subbuffer_offset_get();
}

static VkDeviceSize new_buffer_size(size_t sub_buffer_size)
{
  return max_ulul(sub_buffer_size, DEFAULT_INTERNAL_BUFFER_SIZE);
}

std::unique_ptr<VKBuffer> VKImmediate::create_resource(VKContext & /*context*/)
{
  const size_t bytes_needed = vertex_buffer_size(&vertex_format, vertex_len);
  std::unique_ptr<VKBuffer> result = std::make_unique<VKBuffer>();
  result->create(new_buffer_size(bytes_needed),
                 GPU_USAGE_DYNAMIC,
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  debug::object_label(result->vk_handle(), "Immediate");
  buffer_offset_ = 0;
  return result;
}

}  // namespace blender::gpu
