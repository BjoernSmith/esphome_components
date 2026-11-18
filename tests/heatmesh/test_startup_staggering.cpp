#include "../../components/heatmesh/heatmesh.h"
#include "../../components/heatmesh/zone_climate.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

static uint32_t now_ms = 0;
static constexpr uint32_t MINUTE = 60000;

namespace esphome {
uint32_t millis() { return now_ms; }
namespace heatmesh {
void ZoneClimate::restore() {}
void ZoneClimate::sync_from_zone() {}
}  // namespace heatmesh
}  // namespace esphome

using esphome::heatmesh::HeatmeshComponent;
using esphome::heatmesh::ZoneController;
using esphome::sensor::Sensor;
using esphome::switch_::Switch;

struct TestGroup {
  Sensor sensor0;
  Sensor sensor1;
  Sensor sensor2;
  Switch valve0;
  Switch valve1;
  Switch valve2;
  ZoneController zone0;
  ZoneController zone1;
  ZoneController zone2;
  HeatmeshComponent controller;

  explicit TestGroup(uint32_t period_min = 180, uint32_t min_on = 30 * MINUTE,
                     uint32_t boost = 30 * MINUTE, uint32_t timeout = 24 * 60 * MINUTE)
      : zone0("first", &sensor0, &valve0, 21.0f, timeout, 100.0f, 0.0f, 30000,
              period_min, min_on, 0, boost),
        zone1("second", &sensor1, &valve1, 21.0f, timeout, 100.0f, 0.0f, 30000,
              period_min, min_on, 0, boost),
        zone2("third", &sensor2, &valve2, 21.0f, timeout, 100.0f, 0.0f, 30000,
              period_min, min_on, 0, boost) {
    controller.add_zone(&zone0);
    controller.add_zone(&zone1);
    controller.add_zone(&zone2);
    controller.setup();
  }

  void publish(float first, float second, float third) {
    sensor0.publish_state(first);
    sensor1.publish_state(second);
    sensor2.publish_state(third);
    controller.loop();
  }

  void tick(uint32_t time) {
    now_ms = time;
    controller.loop();
  }
};

static void test_first_starts_and_pwm_phases() {
  now_ms = 0;
  TestGroup test;
  test.publish(20.5f, 20.5f, 20.5f);
  assert(test.valve0.state && !test.valve1.state && !test.valve2.state);
  test.valve1.turn_on();
  test.tick(1);
  assert(!test.valve1.state && test.zone1.get_valve_switch_count() == 0);
  test.tick(5 * MINUTE - 1);
  assert(!test.valve1.state && !test.valve2.state);
  test.tick(5 * MINUTE);
  assert(test.valve1.state && !test.valve2.state);
  test.tick(10 * MINUTE - 1);
  assert(!test.valve2.state);
  test.tick(10 * MINUTE);
  assert(test.valve2.state);

  test.tick(40 * MINUTE);
  assert(!test.valve2.state);
  test.tick(65 * MINUTE);
  assert(!test.valve1.state);
  test.tick(70 * MINUTE);
  assert(test.valve2.state);
  test.tick(90 * MINUTE);
  assert(!test.valve0.state);
  test.tick(125 * MINUTE);
  assert(test.valve1.state);
  test.tick(160 * MINUTE);
  assert(!test.valve2.state);
  test.tick(180 * MINUTE);
  assert(test.valve0.state);
}

static void test_short_first_period_carries_heat() {
  now_ms = 0;
  TestGroup test;
  test.publish(21.0f, 21.0f, 20.75f);
  assert(!test.valve2.state);
  test.tick(60 * MINUTE - 1);
  assert(!test.valve2.state);
  test.tick(60 * MINUTE);
  assert(test.valve2.state);
  test.tick(120 * MINUTE - 1);
  assert(test.valve2.state);
  test.tick(120 * MINUTE);
  assert(!test.valve2.state);
}

static void test_first_period_cannot_be_shorter_than_min_on() {
  now_ms = 0;
  TestGroup test(30, 20 * MINUTE, 0);
  test.publish(21.0f, 21.0f, 20.25f);
  test.tick(10 * MINUTE);
  assert(!test.valve2.state);
  test.tick(20 * MINUTE);
  assert(test.valve2.state);
  test.tick(40 * MINUTE - 1);
  assert(test.valve2.state);
}

static void test_waiting_cancellation_and_failsafe() {
  now_ms = 0;
  TestGroup test;
  test.publish(20.5f, 20.5f, 20.5f);
  now_ms = MINUTE;
  test.sensor1.publish_state(NAN);
  test.zone2.set_enabled(false);
  test.tick(MINUTE);
  test.tick(5 * MINUTE);
  assert(!test.valve1.state && !test.valve2.state);

  test.zone2.set_enabled(true);
  test.tick(5 * MINUTE);
  assert(test.valve2.state);
  now_ms = 6 * MINUTE;
  test.sensor1.publish_state(20.5f);
  test.tick(6 * MINUTE);
  assert(!test.valve1.state);
  test.tick(10 * MINUTE - 1);
  assert(!test.valve1.state);
  test.tick(10 * MINUTE);
  assert(test.valve1.state);
  test.sensor1.publish_state(NAN);
  test.zone2.set_enabled(false);
  assert(!test.valve1.state && !test.valve2.state);
}

static void test_sensor_timeout_cancels_wait() {
  now_ms = 0;
  TestGroup test(180, 30 * MINUTE, 30 * MINUTE, 4 * MINUTE);
  test.publish(20.5f, 20.5f, 20.5f);
  test.tick(4 * MINUTE);
  assert(!test.zone1.get_sensor_valid() && !test.valve0.state && !test.valve1.state && !test.valve2.state);
  test.tick(5 * MINUTE);
  assert(!test.valve1.state);
  test.sensor1.publish_state(20.5f);
  test.tick(5 * MINUTE);
  assert(test.zone1.get_sensor_valid() && test.valve1.state);
}

static void test_boost_waits_until_energized() {
  now_ms = 0;
  TestGroup test;
  test.publish(20.5f, 20.5f, 21.0f);
  now_ms = MINUTE;
  test.zone1.set_target_temperature(22.0f);
  test.valve1.turn_on();
  test.tick(MINUTE);
  assert(test.zone1.is_boost_active() && !test.valve1.state);
  test.tick(5 * MINUTE);
  assert(test.zone1.is_boost_active() && test.valve1.state);
  now_ms = 6 * MINUTE;
  test.sensor1.publish_state(22.0f);
  test.tick(6 * MINUTE);
  assert(test.zone1.is_boost_active() && test.valve1.state);
  test.tick(35 * MINUTE - 1);
  assert(test.zone1.is_boost_active() && test.valve1.state);
  test.tick(35 * MINUTE);
  assert(!test.zone1.is_boost_active() && !test.valve1.state);
}

static void test_boost_waits_longer_than_its_duration() {
  now_ms = 0;
  Sensor sensors[8];
  Switch valves[8];
  std::vector<std::unique_ptr<ZoneController>> zones;
  HeatmeshComponent controller;
  for (size_t i = 0; i < 8; ++i) {
    zones.emplace_back(std::make_unique<ZoneController>(std::to_string(i), &sensors[i], &valves[i], 21.0f,
                                                        24 * 60 * MINUTE, 100.0f, 0.0f, 30000, 180,
                                                        30 * MINUTE, 0, 30 * MINUTE));
    controller.add_zone(zones.back().get());
  }
  controller.setup();
  for (auto &sensor : sensors)
    sensor.publish_state(19.5f);
  controller.loop();
  zones.back()->set_target_temperature(22.0f);
  for (uint32_t minute = 5; minute <= 30; minute += 5) {
    now_ms = minute * MINUTE;
    controller.loop();
  }
  assert(zones.back()->is_boost_active() && !valves[7].state);
  now_ms = 35 * MINUTE;
  controller.loop();
  assert(zones.back()->is_boost_active() && valves[7].state);
  now_ms = 65 * MINUTE - 1;
  controller.loop();
  assert(zones.back()->is_boost_active());
  now_ms = 65 * MINUTE;
  controller.loop();
  assert(!zones.back()->is_boost_active());
}

static void test_boost_cancelled_while_waiting() {
  now_ms = 0;
  TestGroup test;
  test.publish(20.5f, 20.5f, 21.0f);
  now_ms = MINUTE;
  test.zone1.set_target_temperature(22.0f);
  test.tick(MINUTE);
  now_ms = 2 * MINUTE;
  test.sensor1.publish_state(22.0f);
  test.tick(2 * MINUTE);
  assert(!test.zone1.is_boost_active() && !test.valve1.state);
  now_ms = 5 * MINUTE;
  test.sensor2.publish_state(20.5f);
  test.tick(5 * MINUTE);
  assert(!test.valve1.state && test.valve2.state);
}

static void test_later_starts_are_not_staggered() {
  now_ms = 0;
  TestGroup test;
  test.publish(20.5f, 20.5f, 20.5f);
  test.tick(5 * MINUTE);
  test.tick(10 * MINUTE);
  now_ms = 10 * MINUTE;
  test.sensor0.publish_state(22.0f);
  test.sensor1.publish_state(22.0f);
  test.sensor2.publish_state(22.0f);
  test.tick(10 * MINUTE + 30000);
  assert(!test.valve0.state && !test.valve1.state && !test.valve2.state);
  test.sensor0.publish_state(20.5f);
  test.sensor1.publish_state(20.5f);
  test.sensor2.publish_state(20.5f);
  test.tick(11 * MINUTE);
  assert(test.valve0.state && test.valve1.state && test.valve2.state);
  test.tick(101 * MINUTE - 1);
  assert(test.valve0.state && test.valve1.state && test.valve2.state);
  test.tick(101 * MINUTE);
  assert(!test.valve0.state && !test.valve1.state && !test.valve2.state);
  test.tick(191 * MINUTE);
  assert(test.valve0.state && test.valve1.state && test.valve2.state);
}

static void test_clock_rollover() {
  const uint32_t start = UINT32_MAX - 2 * MINUTE;
  now_ms = start;
  TestGroup test;
  test.publish(20.5f, 20.5f, 20.5f);
  test.tick(start + 5 * MINUTE - 1);
  assert(!test.valve1.state && !test.valve2.state);
  test.tick(start + 5 * MINUTE);
  assert(test.valve1.state && !test.valve2.state);
  test.tick(start + 10 * MINUTE);
  assert(test.valve2.state);
}

static void test_long_idle_does_not_restore_spacing() {
  now_ms = 0;
  TestGroup test;
  test.publish(20.5f, 21.0f, 21.0f);
  test.tick(5 * MINUTE);
  test.tick(UINT32_MAX - 1000);
  now_ms = 2000;
  test.sensor1.publish_state(20.5f);
  test.tick(2000);
  assert(test.valve1.state);
}

static void test_single_zone_keeps_its_period() {
  now_ms = 0;
  Sensor sensor;
  Switch valve;
  ZoneController zone("single", &sensor, &valve, 21.0f, 24 * 60 * MINUTE, 100.0f, 0.0f, 30000,
                      15, 5 * MINUTE, 0, 0);
  HeatmeshComponent controller;
  controller.add_zone(&zone);
  controller.setup();
  sensor.publish_state(20.5f);
  controller.loop();
  assert(valve.state);
  now_ms = 450000;
  controller.loop();
  assert(!valve.state);
  now_ms = 900000;
  controller.loop();
  assert(valve.state);
}

int main() {
  test_first_starts_and_pwm_phases();
  test_short_first_period_carries_heat();
  test_first_period_cannot_be_shorter_than_min_on();
  test_waiting_cancellation_and_failsafe();
  test_sensor_timeout_cancels_wait();
  test_boost_waits_until_energized();
  test_boost_waits_longer_than_its_duration();
  test_boost_cancelled_while_waiting();
  test_later_starts_are_not_staggered();
  test_clock_rollover();
  test_long_idle_does_not_restore_spacing();
  test_single_zone_keeps_its_period();
  std::puts("startup staggering tests passed");
  return 0;
}
