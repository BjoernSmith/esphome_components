esphome_component_register(
  COMPONENT_NAME heatmesh
  SRC heatmesh.cpp zone_controller.cpp sensor_manager.cpp
  INCLUDE_DIRS .
  REQUIRES sensor switch
)

