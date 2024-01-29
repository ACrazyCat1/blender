/* SPDX-FileCopyrightText: 2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "BKE_scene.h"

#include "DEG_depsgraph_query.hh"

#include "DNA_camera_types.h"

#include "BKE_camera.h"

#include "node_geometry_util.hh"

namespace blender::nodes::node_geo_input_camera_info_cc {

static void node_declare(NodeDeclarationBuilder &b)
{
  b.add_input<decl::Object>("Camera").hide_label();
  b.add_output<decl::Float>("Focal Length");
  b.add_output<decl::Float>("Sensor Width");
  b.add_output<decl::Int>("Res X");
  b.add_output<decl::Int>("Res Y");
  b.add_output<decl::Float>("Shift X");
  b.add_output<decl::Float>("Shift Y");
  b.add_output<decl::Float>("Clip Start");
  b.add_output<decl::Float>("Clip End");
  b.add_output<decl::Float>("Focus Distance");
  b.add_output<decl::Float>("Fstop");
  b.add_output<decl::Int>("Blades");
  b.add_output<decl::Float>("Rotation");
  b.add_output<decl::Float>("Ratio");


}

static void node_exec(GeoNodeExecParams params)
{
  // get camera pointer
  const Scene *scene = DEG_get_evaluated_scene(params.depsgraph());
  //Object *camera = DEG_get_evaluated_object(params.depsgraph(), scene->camera);
  Object *camera = params.get_input<Object *>("Camera");
  // need to add check whether selected object is a camera

  // get renderdata settings
  RenderData renderData = scene->r;
  bool use_crop = false;
  int res_x, res_y;
  BKE_render_resolution(&renderData, use_crop, &res_x, &res_y);

  //set render outputs
  params.set_output("Res X", res_x);
  params.set_output("Res Y", res_y);
  // check if camera does not exist
  if (camera == nullptr) {
    params.set_default_remaining_outputs();
    return;
  }
  // get camera parameters
  CameraParams cameraParams;
  BKE_camera_params_from_object(&cameraParams, camera);

  // get dof settings
  Camera *cameraData = (Camera *)camera->data;
  CameraDOFSettings dof = cameraData->dof;
 

  // set camera dependent settings
  params.set_output("Focal Length", cameraParams.lens);
  params.set_output("Sensor Width", cameraParams.sensor_x);
  params.set_output("Shift X", cameraParams.shiftx);
  params.set_output("Shift Y", cameraParams.shifty);
  params.set_output("Clip Start", cameraParams.clip_start);
  params.set_output("Clip End", cameraParams.clip_end);
  params.set_output("Focus Distance", dof.focus_distance);
  params.set_output("Fstop", dof.aperture_fstop);
  params.set_output("Blades", dof.aperture_blades);
  params.set_output("Rotation", dof.aperture_rotation);
  params.set_output("Ratio", dof.aperture_ratio);

}

static void node_register() {
  static bNodeType ntype;
  geo_node_type_base(&ntype, GEO_NODE_INPUT_CAMERA_INFO, "Camera Info", NODE_CLASS_INPUT);
  ntype.geometry_node_execute = node_exec;
  ntype.declare = node_declare;
  nodeRegisterType(&ntype);
}
NOD_REGISTER_NODE(node_register)
} // namespace blender::nodes::node_geo_input_camera_info_cc

