TDSTMPSensor

This project reuses the HydroDozerPump infrastructure:
- PlatformIO / ESP8266 / LittleFS layout
- WiFiManager onboarding
- ElegantOTA web updates
- PubSubClient reconnect/publish strategy
- split Web UI and MQTT sections
- JSON config persisted in LittleFS

Device-specific behavior is now focused on:
- analog TDS sampling on A0
- temperature compensation with a DS18B20 probe on D4
- retained MQTT sensor/system state
- Home Assistant discovery for readings and calibration controls
