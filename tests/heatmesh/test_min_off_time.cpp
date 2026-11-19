// Host test: compile this file and the real zone_controller.cpp with -I stubs.
#include "../../components/heatmesh/zone_controller.h"
#include "../../components/heatmesh/zone_climate.h"
#include <cassert>
#include <cstdint>
#include <cstdio>

static uint32_t now_ms = 0;

namespace esphome {
// Supplies a controllable clock to the real zone controller.
uint32_t millis() { return now_ms; }
namespace heatmesh {
// Climate is not used by these output-timing tests.
void ZoneClimate::restore() {}
void ZoneClimate::sync_from_zone() {}
}  // namespace heatmesh
}  // namespace esphome

using esphome::heatmesh::ZoneController;
using esphome::sensor::Sensor;
using esphome::switch_::Switch;

struct TestZone {
  Sensor sensor;
  Switch valve;
  ZoneController zone;

  // Creates a 50% demand zone with a 15-minute PWM period.
  explicit TestZone(uint32_t off_time, uint32_t boost_time = 0)
      : zone("test", &sensor, &valve, 21.0f, 14400000, 100.0f, 0.0f, 30000, 15, 300000,
             off_time, boost_time) {
    zone.setup();
    sensor.publish_state(20.5f);
    zone.loop();
  }

  // Advances the controller to a specific simulated time.
  void tick(uint32_t time) {
    now_ms = time;
    zone.loop();
  }
};

// Confirms zero disables the lockout and preserves the existing PWM schedule.
static void test_zero_bypass() {
  now_ms = 0;
  TestZone test(0);
  assert(test.valve.state);
  test.tick(450000);
  assert(!test.valve.state);
  test.tick(900000);
  assert(test.valve.state);
}

// A delayed cycle starts at the end of the OFF time, without shortening its ON pulse.
static void test_deferred_cycle(uint32_t start) {
  now_ms = start;
  TestZone test(900000);
  assert(test.valve.state);
  test.tick(start + 450000);
  assert(!test.valve.state);
  test.tick(start + 900000);
  assert(!test.valve.state);
  test.tick(start + 1349999);
  assert(!test.valve.state);
  test.tick(start + 1350000);
  assert(test.valve.state);
  test.tick(start + 1799999);
  assert(test.valve.state);
  test.tick(start + 1800000);
  assert(!test.valve.state);
}

// Once demand disappears, expiration of the OFF time must not trigger heating.
static void test_demand_removed_while_waiting() {
  now_ms = 0;
  TestZone test(900000);
  test.tick(450000);
  test.sensor.publish_state(22.0f);
  test.tick(900000);
  test.tick(1350000);
  assert(!test.valve.state);
}

// Boost can start during the OFF interval, but invalid input still turns it off immediately.
static void test_boost_and_failsafe() {
  now_ms = 0;
  TestZone test(900000, 1800000);
  test.tick(450000);
  assert(!test.valve.state);
  test.zone.set_target_temperature(22.0f);
  test.zone.loop();
  assert(test.valve.state && test.zone.is_boost_active());
  test.sensor.publish_state(NAN);
  assert(!test.valve.state && !test.zone.is_boost_active());
  test.sensor.publish_state(20.5f);
  test.tick(450001);
  assert(!test.valve.state);
  test.tick(1350000);
  assert(test.valve.state);
  test.zone.set_enabled(false);
  assert(!test.valve.state);
  test.zone.set_enabled(true);
  test.zone.loop();
  assert(test.valve.state && test.zone.is_boost_active());
}

// A 100% PWM demand stays continuously on across cycle boundaries.
static void test_full_demand() {
  now_ms = 0;
  Sensor sensor;
  Switch valve;
  ZoneController zone("full", &sensor, &valve, 21.0f, 14400000, 100.0f, 0.0f, 30000, 15, 300000,
                      900000, 0);
  zone.setup();
  sensor.publish_state(19.0f);
  zone.loop();
  now_ms = 900000;
  zone.loop();
  now_ms = 1800000;
  zone.loop();
  assert(valve.state);
  assert(zone.get_valve_switch_count() == 1);
}

// Reboot discards the previous lockout and allows the first PWM cycle.
static void test_reboot_bypass() {
  now_ms = 0;
  TestZone first(900000);
  first.tick(450000);
  assert(!first.valve.state);
  TestZone rebooted(900000);
  assert(rebooted.valve.state);
}

// Reaching the setpoint ends boost only after min_on_time, but lowering the setpoint stops it at once.
static void test_boost_min_on_hold() {
  now_ms = 0;
  TestZone test(0, 1800000);
  test.tick(450000);
  assert(!test.valve.state);
  test.zone.set_target_temperature(22.0f);
  test.zone.loop();
  assert(test.valve.state && test.zone.is_boost_active());
  test.sensor.publish_state(22.0f);
  test.tick(460000);
  assert(test.valve.state && test.zone.is_boost_active());
  test.tick(749999);
  assert(test.valve.state && test.zone.is_boost_active());
  test.tick(750000);
  assert(!test.valve.state && !test.zone.is_boost_active());

  test.sensor.publish_state(20.5f);
  test.zone.set_target_temperature(23.0f);
  test.tick(800000);
  assert(test.valve.state && test.zone.is_boost_active());
  test.sensor.publish_state(23.0f);
  test.zone.set_target_temperature(22.0f);
  test.tick(810000);
  assert(!test.valve.state && !test.zone.is_boost_active());
}

// A boost that reaches the setpoint before the output is switched on produces no pulse.
static void test_boost_cancelled_before_on() {
  now_ms = 0;
  TestZone test(0, 1800000);
  test.tick(450000);
  test.zone.set_target_temperature(22.0f);
  test.sensor.publish_state(22.0f);
  test.tick(450001);
  assert(!test.valve.state && !test.zone.is_boost_active());
  assert(test.zone.get_valve_switch_count() == 2);
}

static void test_falling_demand_shortens_pulse() {
  now_ms = 0;
  TestZone test(0);
  test.sensor.publish_state(20.9f);
  test.tick(180000);
  assert(test.valve.state);
  test.tick(299999);
  assert(test.valve.state);
  test.tick(300000);
  assert(!test.valve.state);
}

static void test_rising_demand_does_not_extend_current_pulse() {
  now_ms = 0;
  TestZone test(0);
  test.sensor.publish_state(20.25f);
  test.tick(180000);
  test.tick(449999);
  assert(test.valve.state);
  test.tick(450000);
  assert(!test.valve.state);
}

static void test_failsafe_overrides_min_on() {
  now_ms = 0;
  TestZone test(0);
  test.tick(30000);
  test.sensor.publish_state(NAN);
  assert(!test.valve.state);
}

static void test_rising_demand_restarts_early(uint32_t off_time, uint32_t expected_start, uint32_t start = 0) {
  now_ms = start;
  TestZone test(off_time);
  test.tick(start + 450000);
  assert(!test.valve.state);
  test.sensor.publish_state(20.25f);
  test.tick(start + 480000);
  test.tick(start + expected_start - 1);
  assert(!test.valve.state);
  test.tick(start + expected_start);
  assert(test.valve.state);
}

static void test_reference_period_timing() {
  static constexpr uint32_t minute = 60000;
  now_ms = 0;
  Sensor sensor;
  Switch valve;
  ZoneController zone("reference", &sensor, &valve, 21.0f, 14400000, 100.0f, 0.0f, 30000,
                      180, 30 * minute, 0, 0);
  zone.setup();
  sensor.publish_state(20.5f);
  zone.loop();
  assert(valve.state);
  now_ms = 90 * minute;
  zone.loop();
  assert(!valve.state);
  sensor.publish_state(20.25f);
  now_ms = 95 * minute;
  zone.loop();
  now_ms = 135 * minute - 1;
  zone.loop();
  assert(!valve.state);
  now_ms = 135 * minute;
  zone.loop();
  assert(valve.state);
  sensor.publish_state(20.9f);
  now_ms = 140 * minute;
  zone.loop();
  now_ms = 165 * minute - 1;
  zone.loop();
  assert(valve.state);
  now_ms = 165 * minute;
  zone.loop();
  assert(!valve.state);
}

static void test_full_demand_at_pulse_end_avoids_off_blink() {
  now_ms = 0;
  TestZone test(0);
  test.sensor.publish_state(20.0f);
  test.tick(450000);
  assert(test.valve.state);
  assert(test.zone.get_valve_switch_count() == 1);
}

static void test_skipped_cycle_earns_only_elapsed_carry() {
  now_ms = 0;
  Sensor sensor;
  Switch valve;
  ZoneController zone("carry", &sensor, &valve, 21.0f, 14400000, 100.0f, 0.0f, 30000,
                      15, 300000, 0, 0);
  zone.setup();
  sensor.publish_state(20.9f);
  zone.loop();
  now_ms = 300000;
  zone.loop();
  assert(!valve.state);
  sensor.publish_state(20.625f);
  now_ms = 562499;
  zone.loop();
  assert(!valve.state);
  now_ms = 562500;
  zone.loop();
  assert(valve.state);
  now_ms = 940000;
  zone.loop();
  assert(valve.state);
  now_ms = 960000;
  zone.loop();
  assert(!valve.state);
}

static void test_constant_small_demand_waits_full_cycle() {
  now_ms = 0;
  Sensor sensor;
  Switch valve;
  ZoneController zone("low", &sensor, &valve, 21.0f, 14400000, 100.0f, 0.0f, 30000,
                      15, 300000, 0, 0);
  zone.setup();
  sensor.publish_state(20.8f);
  zone.loop();
  now_ms = 899999;
  zone.loop();
  assert(!valve.state);
  now_ms = 900000;
  zone.loop();
  assert(valve.state);
}

// Runs OFF-time tests, including rollover of the millisecond clock.
int main() {
  test_zero_bypass();
  test_deferred_cycle(0);
  test_deferred_cycle(UINT32_MAX - 1000000);
  test_demand_removed_while_waiting();
  test_boost_and_failsafe();
  test_boost_min_on_hold();
  test_boost_cancelled_before_on();
  test_full_demand();
  test_reboot_bypass();
  test_falling_demand_shortens_pulse();
  test_rising_demand_does_not_extend_current_pulse();
  test_failsafe_overrides_min_on();
  test_rising_demand_restarts_early(0, 675000);
  test_rising_demand_restarts_early(300000, 750000);
  test_rising_demand_restarts_early(0, 675000, UINT32_MAX - 500000);
  test_reference_period_timing();
  test_full_demand_at_pulse_end_avoids_off_blink();
  test_skipped_cycle_earns_only_elapsed_carry();
  test_constant_small_demand_waits_full_cycle();
  std::puts("min_off_time tests passed");
  return 0;
}
