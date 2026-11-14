import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

CODEOWNERS = ["@BjoernSmith"]

heatmesh_ns = cg.esphome_ns.namespace("heatmesh")
HeatmeshComponent = heatmesh_ns.class_("HeatmeshComponent", cg.Component)

CONFIG_SCHEMA = cv.COMPONENT_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(HeatmeshComponent),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

