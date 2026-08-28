# TDSTMPSensor Hardware

Board target: Wemos D1 Mini (ESP8266)
TDS Sensor : DFRobot SEN0244
Temperature Sensor : DFRobot DS18B20

## Pin assignments
- `A0`: analog TDS sensor input
- `D2` (`GPIO4`): `DS18B20` one-wire temperature probe
- `D5` (`GPIO14`): emergency factory reset input (jumper to GND at boot)

## Notes
- The current firmware assumes a Gravity-style analog TDS front-end on `A0`.
- The temperature path is isolated so it can be swapped later if you move away from `DS18B20`.
- On most ESP8266 dev boards, `A0` must stay within the board's supported analog input range.
