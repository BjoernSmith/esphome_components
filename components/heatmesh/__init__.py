"""Local HeatMesh zones. BLE decoding remains in bthome_mithermometer."""
import math

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, climate, sensor, switch
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_DURATION,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_PERCENT,
    UNIT_SECOND,
)

CODEOWNERS = ["@BjoernSmith"]
AUTO_LOAD = ["binary_sensor", "climate", "sensor"]
DEPENDENCIES = ["switch"]

heatmesh_ns = cg.esphome_ns.namespace("heatmesh")
HeatmeshComponent = heatmesh_ns.class_("HeatmeshComponent", cg.Component)
ZoneController = heatmesh_ns.class_("ZoneController")
ZoneClimate = heatmesh_ns.class_("ZoneClimate", climate.Climate)

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
CONF_BOOST_TIME = "boost_time"
CONF_CLIMATE = "climate"
CONF_HEAT_DEMAND = "heat_demand"
CONF_SENSOR_AGE = "sensor_age"
CONF_VALVE_SWITCH_COUNT = "valve_switch_count"
CONF_SENSOR_VALID = "sensor_valid"
CONF_VALVE_STATE = "valve_state"

# Optional entity key -> ZoneController setter.
_SENSORS = {
    CONF_HEAT_DEMAND: "set_heat_demand_sensor",
    CONF_SENSOR_AGE: "set_sensor_age_sensor",
    CONF_VALVE_SWITCH_COUNT: "set_valve_switch_count_sensor",
}
_BINARY_SENSORS = {
    CONF_SENSOR_VALID: "set_sensor_valid_binary_sensor",
    CONF_VALVE_STATE: "set_valve_state_binary_sensor",
}


# Rejects non-finite numeric values during ESPHome configuration validation.
def _finite(value):
    if not math.isfinite(value):
        raise cv.Invalid("Value must be finite")
    return value


# Validates a positive sensor timeout representable by the controller's millisecond counter.
def _sensor_timeout(value):
    value = cv.positive_time_period_milliseconds(value)
    if not 0 < value.total_milliseconds <= 0x7FFFFFFF:
        raise cv.Invalid("sensor_timeout must be > 0 and <= 2147483647ms")
    return value


# Ensures zones have unique IDs and valves, and compatible timing parameters.
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
        boost_ms = zone[CONF_BOOST_TIME].total_milliseconds
        if 0 < boost_ms < zone[CONF_MIN_ON_TIME].total_milliseconds:
            raise cv.Invalid(f"boost_time must be 0s or at least min_on_time in zone {name}")
    return zones


ZONE_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(ZoneController),
        cv.Required(CONF_ZONE_ID): cv.string,
        cv.Required(CONF_TEMPERATURE_SENSOR): cv.use_id(sensor.Sensor),
        cv.Required(CONF_VALVE): cv.use_id(switch.Switch),
        # Used until the climate entity has stored a setpoint in flash.
        cv.Optional(CONF_SETPOINT, default=21.0): cv.All(cv.float_range(min=5.0, max=30.0), _finite),
        cv.Optional(CONF_SENSOR_TIMEOUT, default="15min"): _sensor_timeout,
        # Gains retain the original units: percentage points / K, and / (K*s).
        # Default kp=100: 1 K below setpoint = 100 % demand. ki=0.028: Ti = kp/ki ~ 60 min.
        cv.Optional(CONF_KP, default=100.0): cv.All(cv.float_range(min=0.1, max=1000.0), _finite),
        cv.Optional(CONF_KI, default=0.028): cv.All(cv.float_range(min=0.0, max=1.0), _finite),
        cv.Optional(CONF_PERIOD_MS, default=30000): cv.int_range(min=1000, max=120000),
        cv.Optional(CONF_PWM_PERIOD_MIN, default=15): cv.int_range(min=5, max=60),
        # Pulses shorter than this are carried into later PWM cycles.
        cv.Optional(CONF_MIN_ON_TIME, default="5min"): cv.positive_time_period_milliseconds,
        # Forced ON after raising the setpoint above the room temperature (or OFF -> HEAT
        # below setpoint), before PI takes over. 0s disables.
        cv.Optional(CONF_BOOST_TIME, default="30min"): cv.All(
            cv.positive_time_period_milliseconds,
            cv.Range(max=cv.TimePeriod(hours=24)),
        ),
        cv.Optional(CONF_CLIMATE): climate.climate_schema(ZoneClimate),
        cv.Optional(CONF_HEAT_DEMAND): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            icon="mdi:radiator",
            accuracy_decimals=1,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_SENSOR_AGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_SECOND,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_DURATION,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_VALVE_SWITCH_COUNT): sensor.sensor_schema(
            icon="mdi:counter",
            accuracy_decimals=0,
            state_class=STATE_CLASS_TOTAL_INCREASING,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_SENSOR_VALID): binary_sensor.binary_sensor_schema(
            icon="mdi:thermometer-check",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        # Electrical output command, not measured valve position.
        cv.Optional(CONF_VALVE_STATE): binary_sensor.binary_sensor_schema(
            icon="mdi:valve",
        ),
    }
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(HeatmeshComponent),
        cv.Required(CONF_ZONES): cv.All(cv.ensure_list(ZONE_SCHEMA), _validate_zones),
    }
).extend(cv.COMPONENT_SCHEMA)


# Generates the component and zone objects, then connects their optional entities.
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    for zone_conf in config[CONF_ZONES]:
        temp_sensor = await cg.get_variable(zone_conf[CONF_TEMPERATURE_SENSOR])
        valve = await cg.get_variable(zone_conf[CONF_VALVE])
        zone = cg.new_Pvariable(
            zone_conf[CONF_ID],
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
            zone_conf[CONF_BOOST_TIME].total_milliseconds,
        )
        cg.add(var.add_zone(zone))

        if CONF_CLIMATE in zone_conf:
            clim = await climate.new_climate(zone_conf[CONF_CLIMATE], zone)
            cg.add(zone.set_climate(clim))
        for key, setter in _SENSORS.items():
            if key in zone_conf:
                sens = await sensor.new_sensor(zone_conf[key])
                cg.add(getattr(zone, setter)(sens))
        for key, setter in _BINARY_SENSORS.items():
            if key in zone_conf:
                bin_sens = await binary_sensor.new_binary_sensor(zone_conf[key])
                cg.add(getattr(zone, setter)(bin_sens))
