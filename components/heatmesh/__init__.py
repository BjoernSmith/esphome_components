"""Local HeatMesh zones. BLE decoding remains in bthome_mithermometer."""
import math

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, switch
from esphome.const import CONF_ID

CODEOWNERS = ["@BjoernSmith"]
DEPENDENCIES = ["sensor", "switch"]

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
CONF_MIN_ON_TIME = "min_on_time"


def _finite(value):
    if not math.isfinite(value):
        raise cv.Invalid("Value must be finite")
    return value


def _sensor_timeout(value):
    value = cv.positive_time_period_milliseconds(value)
    if not 0 < value.total_milliseconds <= 0x7FFFFFFF:
        raise cv.Invalid("sensor_timeout must be > 0 and <= 2147483647ms")
    return value


def _validate_zones(zones):
    if not zones:
        raise cv.Invalid("Configure at least one zone")
    names = set()
    valves = set()
    for zone in zones:
        name = zone[CONF_ZONE_ID]
        if not name.strip():
            raise cv.Invalid("zone_id must not be empty")
        if name in names:
            raise cv.Invalid(f"Duplicate zone_id: {name}")
        valve_id = str(zone[CONF_VALVE])
        if valve_id in valves:
            raise cv.Invalid(f"Valve {valve_id} is assigned to more than one zone")
        names.add(name)
        valves.add(valve_id)
        if zone[CONF_MIN_ON_TIME].total_milliseconds >= zone[CONF_PWM_PERIOD_MIN] * 60000:
            raise cv.Invalid(f"min_on_time must be shorter than pwm_period_min in zone {name}")
    return zones


ZONE_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ZONE_ID): cv.string,
        cv.Required(CONF_TEMPERATURE_SENSOR): cv.use_id(sensor.Sensor),
        cv.Required(CONF_VALVE): cv.use_id(switch.Switch),
        cv.Optional(CONF_SETPOINT, default=21.0): cv.All(cv.float_range(min=5.0, max=30.0), _finite),
        cv.Optional(CONF_SENSOR_TIMEOUT, default="15min"): _sensor_timeout,
        # Gains retain the original units: percentage points / K, and / (K*s).
        cv.Optional(CONF_KP, default=1.0): cv.All(cv.float_range(min=0.1, max=10.0), _finite),
        cv.Optional(CONF_KI, default=0.001): cv.All(cv.float_range(min=0.0, max=1.0), _finite),
        cv.Optional(CONF_PERIOD_MS, default=30000): cv.int_range(min=1000, max=120000),
        cv.Optional(CONF_PWM_PERIOD_MIN, default=15): cv.int_range(min=5, max=60),
        # Pulses shorter than this are carried into later PWM cycles.
        cv.Optional(CONF_MIN_ON_TIME, default="5min"): cv.positive_time_period_milliseconds,
    }
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(HeatmeshComponent),
        cv.Required(CONF_ZONES): cv.All(cv.ensure_list(ZONE_SCHEMA), _validate_zones),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    for zone_conf in config[CONF_ZONES]:
        temp_sensor = await cg.get_variable(zone_conf[CONF_TEMPERATURE_SENSOR])
        valve = await cg.get_variable(zone_conf[CONF_VALVE])
        cg.add(
            var.add_zone(
                # Pass a Python string. Codegen adds/escapes the C++ quotes itself.
                zone_conf[CONF_ZONE_ID],
                temp_sensor,
                valve,
                zone_conf[CONF_SETPOINT],
                # ESPHome TimePeriod properties are not datetime.timedelta methods.
                zone_conf[CONF_SENSOR_TIMEOUT].total_milliseconds,
                zone_conf[CONF_KP],
                zone_conf[CONF_KI],
                zone_conf[CONF_PERIOD_MS],
                zone_conf[CONF_PWM_PERIOD_MIN],
                zone_conf[CONF_MIN_ON_TIME].total_milliseconds,
            )
        )
