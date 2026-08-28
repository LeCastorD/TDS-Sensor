# TDSTMPSensor MQTT Contract v1

Topic namespace base: `tds_tmp_sensor/<node>/`

## Command topics
- `.../cmd`
- `.../ha/cmd`

Examples:

```json
{"cmd":"sample_now"}
{"ha":"sample_now"}
{"ha":"set_tds_calibration","value":1.02}
{"ha":"set_tds_offset","ppm":-12}
{"ha":"set_temp_offset","c":0.4}
{"ha":"set_sample_interval","seconds":10}
{"ha":"set_publish_interval","seconds":60}
{"action":"reboot"}
{"action":"factory_reset"}
```

## State topics
- `.../status`
- `.../health`
- `.../system/state`
- `.../sensors/state`

## Home Assistant discovery
The firmware republishes discovery on reconnect and when Home Assistant publishes `online` to `homeassistant/status`.
