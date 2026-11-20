# bthome_mithermometer override

Copy of ESPHome 2026.10.2 `bthome_mithermometer` (GPLv3, [esphome/esphome](https://github.com/esphome/esphome/tree/2026.10.2/esphome/components/bthome_mithermometer)) with one change to duplicate detection.

pvvx firmware alternates two BTHome packets with the same packet ID for each measurement: battery/temperature/humidity, then battery voltage. Upstream remembers only the last packet ID, so the temperature packet is dropped if the voltage packet is received first. The temperature sensor then stops publishing until a later measurement happens to arrive in the right order.

This override ignores only exact repeats of one of the last four payloads that contain a packet ID. Each packet type is processed once per measurement, and retransmissions remain ignored. Configuration is unchanged:

```yaml
external_components:
  - source: github://BjoernSmith/esphome_components@main
    components: [bthome_mithermometer]
```

Retest or remove the override when ESPHome is upgraded.

## Host test

From the repository root:

```powershell
python -m pip install ziglang
python -m ziglang c++ -std=c++20 -Itests/bthome_mithermometer/stubs tests/bthome_mithermometer/test_packet_dedup.cpp components/bthome_mithermometer/bthome_ble.cpp -o test_packet_dedup.exe
.\test_packet_dedup.exe
```
