import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, switch
from esphome.const import CONF_ID

CODEOWNERS = ["@BjoernSmith"]

heatmesh_ns = cg.esphome_ns.namespace("heatmesh")
HeatmeshComponent = heatmesh_ns.class_("HeatmeshComponent", cg.Component)

CONF_ZONES = "zones"
CONF_ZONE_ID = "zone_id"
CONF_TEMPERATURE_SENSOR = "temperature_sensor"
CONF_VALVE = "valve"
CONF_SENSOR_TIMEOUT = "sensor_timeout"
CONF_SETPOINT = "target_temperature"
CONF_KP = "kp"
CONF_KI = "ki"
CONF_PERIOD_MS = "period_ms"
CONF_PWM_PERIOD_MIN = "pwm_period_min"

ZONE_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ZONE_ID): cv.string,
        cv.Required(CONF_TEMPERATURE_SENSOR): cv.use_id(sensor.Sensor),
        cv.Required(CONF_VALVE): cv.use_id(switch.Switch),
        cv.Optional(CONF_SETPOINT, default=21.0): cv.float_range(min=5.0, max=30.0),
        cv.Optional(CONF_SENSOR_TIMEOUT, default="15min"): cv.time_period,
        cv.Optional(CONF_KP, default=1.0): cv.float_range(min=0.1, max=10.0),
        cv.Optional(CONF_KI, default=0.05): cv.float_range(min=0.0, max=1.0),
        cv.Optional(CONF_PERIOD_MS, default=30000): cv.int_range(min=1000, max=120000),
        cv.Optional(CONF_PWM_PERIOD_MIN, default=15): cv.int_range(min=5, max=60),
    }
)

CONFIG_SCHEMA = cv.COMPONENT_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(HeatmeshComponent),
        cv.Optional(CONF_ZONES, default=[]): cv.All(cv.ensure_list, [ZONE_SCHEMA]),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add_define("HEATMESH_COMPONENT")

    for zone_conf in config[CONF_ZONES]:
        zone_id = zone_conf[CONF_ZONE_ID]
        temp_sensor = await cg.get_variable(zone_conf[CONF_TEMPERATURE_SENSOR])
        valve = await cg.get_variable(zone_conf[CONF_VALVE])
        
        cg.add(var.add_zone(
            cg.safe_exp(f'"{zone_id}"'),
            temp_sensor,
            valve,
            zone_conf[CONF_SETPOINT],
            int(zone_conf[CONF_SENSOR_TIMEOUT].total_seconds() * 1000),
            zone_conf[CONF_KP],
            zone_conf[CONF_KI],
            zone_conf[CONF_PERIOD_MS],
            zone_conf[CONF_PWM_PERIOD_MIN],
        ))


