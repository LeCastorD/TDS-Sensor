# TDSTMPSensor Hardware

Board target: Wemos D1 Mini (ESP8266)
TDS Sensor : DFRobot SEN0244
Temperature Sensor : DFRobot DS18B20

## Pin assignments
- `A0`: analog TDS sensor input
- `D2` (`GPIO4`): `DS18B20` one-wire temperature probe
- `D5` (`GPIO14`): emergency factory reset input (jumper to GND at boot)
- `5V`: buck-converter output to the Wemos power input only
- `3V3`: Wemos 3.3 V output to the SEN0244 and DS18B20 power inputs
- `GND`: common ground for the buck converter, Wemos, SEN0244, and DS18B20

## Notes
- The current firmware assumes a Gravity-style analog TDS front-end on `A0`.
- The temperature path is isolated so it can be swapped later if you move away from `DS18B20`.
- On most ESP8266 dev boards, `A0` must stay within the board's supported analog input range.
- The buck converter supplies only the Wemos `5V` input. Do not connect its 5 V output directly to either sensor.
- The SEN0244 and DS18B20 are powered from the Wemos `3V3` and `GND` pins.
