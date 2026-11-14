import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.core import coroutine_with_priority

CODEOWNERS = ["@BjoernSmith"]
DEPENDENCIES = []

heatmesh_ns = cg.esphome_ns.namespace("heatmesh")
HeatmeshComponent = heatmesh_ns.class_("HeatmeshComponent", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Optional("heatmesh"): cv.Schema(
            {
                cv.GenerateID(): cv.declare_id(HeatmeshComponent),
            }
        ).extend(cv.COMPONENT_SCHEMA),
    },
    extra=cv.ALLOW_EXTRA
)


@coroutine_with_priority(50.0)
async def to_code(config):
    if "heatmesh" not in config:
        return
    
    hm_config = config["heatmesh"]
    var = cg.new_Pvariable(hm_config[CONF_ID])
    await cg.register_component(var, hm_config)


