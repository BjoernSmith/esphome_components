import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_CHANNEL

from .. import WavinAHC9000

CONF_PARENT_ID = "wavinahc9000v3_id"

CONFIG_SCHEMA = binary_sensor.binary_sensor_schema().extend(
    {
        cv.GenerateID(CONF_PARENT_ID): cv.use_id(WavinAHC9000),
        cv.Required(CONF_CHANNEL): cv.int_range(min=1, max=16),
    }
)


async def to_code(config):
    hub = await cg.get_variable(config[CONF_PARENT_ID])
    sens = await binary_sensor.new_binary_sensor(config)
    channel = config[CONF_CHANNEL]
    cg.add(hub.add_channel_output_binary_sensor(channel, sens))
    cg.add(hub.add_active_channel(channel))