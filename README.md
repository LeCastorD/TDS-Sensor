<div align="center">
  <h1>TDS Sensor</h1>
  <p><strong>ESP8266 TDS and temperature sensor for Home Assistant</strong></p>
  <p>
    <a href="#build-with-platformio">Build</a> |
    <a href="#wiring">Wiring</a> |
    <a href="#enclosure-and-assembly">Enclosure</a> |
    <a href="#first-setup">First setup</a>
  </p>
</div>

<p align="center">
  <img src="docs/Box_installed.jpeg" alt="Installed TDS Sensor enclosure" width="32%">
  <img src="docs/Mounting%20plate.jpeg" alt="Populated sensor mounting plate" width="32%">
  <img src="docs/Plate_in_box.jpeg" alt="Mounting plate installed in the enclosure" width="32%">
</p>

<h2 id="table-of-contents">Table of Contents</h2>

<ul>
  <li><a href="#overview">Overview</a></li>
  <li><a href="#features">Features</a></li>
  <li><a href="#hardware">Hardware</a></li>
  <li><a href="#device-naming">Device naming</a></li>
  <li><a href="#component-reference">Component reference</a></li>
  <li><a href="#soldering">Soldering the power and controller connections</a></li>
  <li><a href="#buck-converter-5v-jumper">Buck converter 5 V jumper</a></li>
  <li><a href="#crimping-and-wire-preparation">Crimping connectors and preparing wires</a></li>
  <li><a href="#wiring">Wiring</a></li>
  <li><a href="#bill-of-materials">Bill of materials</a></li>
  <li><a href="#enclosure-and-assembly">Enclosure and assembly</a></li>
  <li><a href="#build-with-platformio">Build with PlatformIO</a></li>
  <li><a href="#upload-firmware">Upload firmware</a></li>
  <li><a href="#initial-wi-fi-configuration">Initial Wi-Fi configuration</a></li>
  <li><a href="#first-setup">First setup</a></li>
  <li><a href="#home-assistant-interface">Home Assistant interface</a></li>
  <li><a href="#security">Security</a></li>
  <li><a href="#repository-layout">Repository layout</a></li>
  <li><a href="#license">License</a></li>
</ul>

<h2 id="overview">Overview</h2>

<p>
  This project measures total dissolved solids (TDS) and water temperature with an ESP8266 controller. It reads a DFRobot SEN0244 analog TDS sensor, applies temperature compensation using a DS18B20 probe, and publishes readings through MQTT for Home Assistant.
</p>

<h2 id="features">Features</h2>

<ul>
  <li>TDS measurement in ppm through an analog input</li>
  <li>DS18B20 temperature measurement and temperature compensation</li>
  <li>WiFiManager onboarding</li>
  <li>MQTT state publishing and Home Assistant MQTT discovery</li>
  <li>Browser-based configuration and sensor calibration</li>
  <li>OTA firmware updates</li>
  <li>LittleFS configuration and sensor log storage</li>
  <li>Factory reset input on the controller board</li>
</ul>

<h2 id="hardware">Hardware</h2>

<ul>
  <li>Wemos D1 Mini (ESP8266)</li>
  <li>DFRobot SEN0244 analog TDS sensor</li>
  <li>DFRobot DS18B20 temperature probe</li>
  <li>5 V supply for the controller board</li>
  <li>Optional 12 V supply and 12-24 V to 5 V buck converter</li>
</ul>

<h2 id="device-naming">Device naming</h2>

<p>
  When no custom name is saved, the firmware generates the device name using this format:
  <code>tds-tmp-sensor-XXXXXX</code>
</p>

<ul>
  <li>Prefix: <code>tds-tmp-sensor-</code></li>
  <li>Suffix: six uppercase hexadecimal characters</li>
  <li>Identifier source: the ESP8266 value returned by <code>ESP.getChipId()</code>, masked to 24 bits</li>
</ul>

<p>Example: <code>tds-tmp-sensor-ABC123</code>.</p>

<p>
  The code does not take the last four characters of the Wi-Fi MAC address. The helper function is named <code>applyDefaultIdentityFromMac()</code>, but its current implementation uses <code>ESP.getChipId()</code> to create a six-character suffix.
</p>

<p>
  A custom name can be saved from the network settings page. When set, that name replaces the generated default and is used as the Wi-Fi hostname, the mDNS hostname (<code>http://&lt;device-name&gt;.local</code>), the WiFiManager portal name, and the Home Assistant MQTT discovery device identifier.
</p>

<h2 id="component-reference">Component reference</h2>

<p>The following reference photos identify the main electronic, wiring, power, and assembly tools used by this project.</p>

<table>
  <tr>
    <td align="center"><img src="docs/wemos-d1-mini.jpeg" alt="Wemos D1 Mini ESP8266 board" width="100%"><br><em>Wemos D1 Mini</em></td>
    <td align="center"><img src="docs/tds-sen0244.jpg" alt="DFRobot SEN0244 TDS sensor" width="100%"><br><em>DFRobot SEN0244 TDS sensor</em></td>
    <td align="center"><img src="docs/ds18b20.jpg" alt="DFRobot DS18B20 temperature sensor" width="100%"><br><em>DFRobot DS18B20</em></td>
  </tr>
  <tr>
    <td align="center"><img src="docs/buck-converter.jpeg" alt="12 to 5 volt buck converter" width="100%"><br><em>12-24 V to 5 V buck converter</em></td>
    <td align="center"><img src="docs/connectors.jpeg" alt="JST XH connector kit" width="100%"><br><em>JST-XH connector kit</em></td>
    <td align="center"><img src="docs/Crimper.jpeg" alt="JST connector crimping tool" width="100%"><br><em>JST connector crimping tool</em></td>
  </tr>
</table>

<h2 id="soldering">Soldering the power and controller connections</h2>

<p>
  Disconnect the 12 V supply and USB cable before soldering. Work from the labels printed on each board and verify every connection with a multimeter before applying power.
</p>

<table>
  <tr>
    <td align="center"><img src="docs/buck-converter.jpeg" alt="Buck converter for the controller power supply" width="100%"><br><em>Buck converter</em></td>
    <td align="center"><img src="docs/wemos-d1-mini.jpeg" alt="Wemos D1 Mini controller board" width="100%"><br><em>Wemos D1 Mini</em></td>
  </tr>
</table>

<ol>
  <li>Identify the buck-converter input and output pads. Connect the 12 V supply to <code>IN+</code> and <code>IN-</code>, observing polarity.</li>
  <li>Connect the regulated output to the controller power wiring: <code>OUT+</code> to the Wemos <code>5V</code> pin and <code>OUT-</code> to <code>GND</code>.</li>
  <li>Before connecting the Wemos, power the buck converter from the supply and adjust or verify its output at approximately 5.0 V DC.</li>
  <li>Solder short, clearly identified leads or connector wires to the Wemos pins required by the installation: <code>5V</code>, <code>GND</code>, <code>A0</code>, <code>D2</code> / <code>GPIO4</code>, and the optional <code>D5</code> / <code>GPIO14</code> factory-reset input.</li>
  <li>Use heat-shrink tubing on exposed solder joints and add strain relief so the connector cannot pull directly on a pad.</li>
  <li>Check continuity, polarity, and the absence of shorts between <code>5V</code> and <code>GND</code> before connecting the Wemos and sensors.</li>
</ol>

<p><strong>Important:</strong> never connect the 12 V supply directly to the Wemos <code>5V</code> pin. The Wemos and connected sensors must receive the regulated output from the buck converter.</p>

<h3 id="buck-converter-5v-jumper">Set the buck converter to 5 V</h3>

<p>
  This buck-converter board uses solder-selectable output pads on its back. The Wemos and the sensors in this project require a regulated 5 V output, so bridge only the two pads marked <code>5V</code> before connecting the controller.
</p>

<p align="center">
  <img src="docs/buck-converter-back.jpeg" alt="Back of the buck converter showing the solder-selectable voltage pads" width="70%">
</p>

<ol>
  <li>Disconnect the 12 V supply and USB cable from the project.</li>
  <li>Turn the converter over and locate the output-selection row and the <code>5V</code> marking shown in the reference image.</li>
  <li>Use a small amount of flux and solder to bridge the two pads for the <code>5V</code> selection.</li>
  <li>Check that the solder bridge does not touch the neighboring voltage-selection pads. Remove any unintended bridge before powering the board.</li>
  <li>Power the converter from the 12 V supply and measure between <code>VOUT+</code> and <code>GND</code>. Confirm approximately 5.0 V DC before connecting the Wemos.</li>
</ol>

<p><strong>Important:</strong> the jumper selects the converter output voltage; it does not replace the voltage measurement. Do not connect the Wemos until the measured output is correct.</p>

<h2 id="crimping-and-wire-preparation">Crimping connectors and preparing wires</h2>

<p>
  This section refers to the JST-XH/XH2.54 connectors listed in the BOM. Use the correct contact size and housing for the prewired connector kit; do not force a terminal into a mismatched housing.
</p>

<p align="center">
  <img src="docs/connectors.jpeg" alt="JST-XH connector kit and prewired wires" width="48%">
  <img src="docs/Crimper.jpeg" alt="JST connector crimping tool" width="48%">
</p>

<ol>
  <li>Install the buck converter, Wemos, terminal blocks, and sensor electronics in their final positions on the mounting plate before measuring wires.</li>
  <li>Route each wire along its final path and measure from connector to connector. Keep runs as short as practical, but leave enough slack to unplug a connector and remove the plate without stressing the wires.</li>
  <li>Cut and label one wire at a time. Keep power, ground, analog signal, and one-wire data conductors identifiable at both ends.</li>
  <li>Strip only the length required by the contact manufacturer, typically about 2-3 mm for small JST-XH contacts. Do not nick or remove conductor strands.</li>
  <li>Place the stripped conductor in the contact so the conductor wings crimp onto bare copper and the insulation wings support the cable jacket.</li>
  <li>Crimp with the correct die position, then inspect the joint. The conductor should be held firmly without cutting through the wire or leaving loose strands.</li>
  <li>Perform a gentle pull test on every crimp, then insert the contact into the housing until the locking tang clicks. Confirm the pin order and connector keying before mating it.</li>
  <li>Use a multimeter to verify continuity end-to-end and confirm that adjacent pins are not shorted.</li>
</ol>

<table>
  <thead>
    <tr>
      <th>Connection</th>
      <th>Routing guidance</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>Buck <code>OUT+</code> to Wemos <code>5V</code></td>
      <td>Use the shortest safe power run and verify 5 V before connection.</td>
    </tr>
    <tr>
      <td>Buck <code>OUT-</code> to Wemos <code>GND</code></td>
      <td>Use a short ground run and maintain a common ground for the sensors.</td>
    </tr>
    <tr>
      <td>Wemos <code>A0</code> to SEN0244 analog output</td>
      <td>Keep the analog signal wire short and separated from unnecessary power loops.</td>
    </tr>
    <tr>
      <td>Wemos <code>D2</code> / <code>GPIO4</code> to DS18B20 data</td>
      <td>Route directly to the temperature connector and avoid excess coiled wire.</td>
    </tr>
  </tbody>
</table>

<p>
  The goal is a compact harness with no loose loops, while retaining enough service slack for inspection, connector removal, and enclosure maintenance. See <a href="docs/BOM.md#component-images">docs/BOM.md</a> for the component images and parts list.
</p>

<h2 id="wiring">Wiring</h2>

<table>
  <thead>
    <tr>
      <th>Controller pin</th>
      <th>Connection</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><code>A0</code></td>
      <td>SEN0244 analog TDS output</td>
    </tr>
    <tr>
      <td><code>D2</code> / <code>GPIO4</code></td>
      <td>DS18B20 one-wire data</td>
    </tr>
    <tr>
      <td><code>D5</code> / <code>GPIO14</code></td>
      <td>Factory reset input, jumper to GND at boot</td>
    </tr>
    <tr>
      <td><code>5V</code> and <code>GND</code></td>
      <td>Sensor and controller power connections as appropriate</td>
    </tr>
  </tbody>
</table>

<p>See <a href="docs/hardware-pinout.md">docs/hardware-pinout.md</a> for board assumptions and electrical notes.</p>

<h2 id="bill-of-materials">Bill of materials</h2>

<p>See the complete parts list, assembly hardware, 3D-printing notes, and enclosure credits in <a href="docs/BOM.md">docs/BOM.md</a>.</p>

<h2 id="enclosure-and-assembly">Enclosure and assembly</h2>

<p>The repository includes the current 3MF files for the enclosure and sensor plate:</p>

<ul>
  <li><a href="Enclosure%20and%20mount/Box.3mf">Box.3mf</a></li>
  <li><a href="Enclosure%20and%20mount/Sensor%20Plate.3mf">Sensor Plate.3mf</a></li>
</ul>

<table>
  <tr>
    <td align="center"><img src="docs/Mounting%20plate.jpeg" alt="Populated mounting plate" width="100%"><br><em>Populated mounting plate</em></td>
    <td align="center"><img src="docs/Plate_in_box.jpeg" alt="Plate installed in the enclosure" width="100%"><br><em>Plate installed in the enclosure</em></td>
  </tr>
  <tr>
    <td align="center" colspan="2"><img src="docs/Box_installed.jpeg" alt="Installed enclosure" width="70%"><br><em>Installed enclosure</em></td>
  </tr>
</table>

<p>
  The parts list identifies the board holders, capacitor holder, connector holder, cable-tie holder, and standoffs used by the assembly. Confirm dimensions and clearances against your exact components before printing.
</p>

<h2 id="build-with-platformio">Build with PlatformIO</h2>

<h3>Requirements</h3>

<ul>
  <li>VS Code with the PlatformIO extension, or PlatformIO CLI</li>
  <li>A Wemos D1 Mini connected by USB</li>
  <li>The dependencies listed in <a href="platformio.ini">platformio.ini</a></li>
</ul>

<p>From the project directory:</p>

<pre><code>pio run</code></pre>

<p>
  The build target is <code>WEMOS_D1_Mini</code>. The pre-build script automatically increments the local build number and updates <code>include/fw_version_auto.h</code>.
</p>

<h2 id="upload-firmware">Upload firmware</h2>

<p>Replace <code>&lt;PORT&gt;</code> with the serial port assigned to your board:</p>

<pre><code>pio run -t upload --upload-port &lt;PORT&gt;
pio run -t uploadfs --upload-port &lt;PORT&gt;</code></pre>

<p>
  The first command uploads the firmware. The second uploads the LittleFS web interface and filesystem data. For the exported release images, see <a href="artifacts/reflash/FLASHING.md">artifacts/reflash/FLASHING.md</a>.
</p>

<h2 id="initial-wi-fi-configuration">Initial Wi-Fi configuration</h2>

<p>
  This project uses <a href="https://github.com/tzapu/WiFiManager">WiFiManager</a> to configure the wireless network without hard-coding an SSID or password in the firmware. The official WiFiManager project provides the library documentation, examples, and configuration details.
</p>

<ol>
  <li>Power the device with no saved Wi-Fi credentials, or clear its previous network configuration.</li>
  <li>Wait for the controller to start its WiFiManager configuration access point. The access-point name is the current device name, for example <code>tds-tmp-sensor-ABC123</code>.</li>
  <li>Connect a phone or computer to that temporary access point. A captive portal should open automatically; if it does not, open the configuration page shown by WiFiManager.</li>
  <li>Select the local Wi-Fi network, enter its password, and save the configuration.</li>
  <li>After the controller restarts or reconnects, return to the normal Wi-Fi network and open the device using its assigned IP address or mDNS name, such as <code>http://tds-tmp-sensor-ABC123.local</code>.</li>
</ol>

<p>
  The firmware calls <code>wm.autoConnect(deviceName)</code> during startup. Once credentials are saved, it attempts to reconnect automatically on later boots. For WiFiManager-specific behavior and troubleshooting, see the <a href="https://github.com/tzapu/WiFiManager">official WiFiManager documentation</a>.
</p>

<h2 id="first-setup">First setup</h2>

<ol>
  <li>Upload the firmware and LittleFS image.</li>
  <li>Power-cycle the controller.</li>
  <li>Join the WiFiManager access point if the device has not been configured yet.</li>
  <li>Configure Wi-Fi, MQTT, time zone, and OTA credentials in the web interface.</li>
  <li>Confirm that the device publishes MQTT state and appears through Home Assistant discovery.</li>
  <li>Calibrate the TDS reading using a known reference solution.</li>
</ol>

<p>The MQTT topic contract is documented in <a href="docs/mqtt-contract-v1.md">docs/mqtt-contract-v1.md</a>.</p>

<h2 id="home-assistant-interface">Home Assistant interface</h2>

<p>
  After MQTT discovery is enabled, the device appears in Home Assistant with its sensor readings and calibration controls. The device page exposes TDS, water temperature, raw ADC, voltage, calibration factor, offsets, sample interval, publish interval, and a manual sample control.
</p>

<table>
  <tr>
    <td align="center"><img src="docs/HA_MQTT.jpg" alt="Home Assistant MQTT device page for the TDS sensor" width="100%"><br><em>MQTT-discovered TDS sensor device page</em></td>
    <td align="center"><img src="docs/HA-DashBoard.jpg" alt="Home Assistant dashboard with TDS and water temperature history" width="100%"><br><em>Dashboard view with TDS and water temperature history</em></td>
  </tr>
</table>

<p>
  The dashboard screenshot is from a broader HydroDozer installation and includes unrelated pump controls and local network details. Those controls are not provided by this TDS Sensor firmware; use the image as an example of adding the TDS and temperature entities to a Home Assistant dashboard.
</p>

<h2 id="security">Security</h2>

<p>
  The firmware includes example default MQTT and OTA credentials for initial development. Change them before deploying the device on a network that is not fully trusted. Do not commit a device-specific configuration file, Wi-Fi password, MQTT password, or OTA password to this repository.
</p>

<h2 id="repository-layout">Repository layout</h2>

<pre><code>src/TDSTMPSensor/       Firmware source
data/web/               Web interface files stored in LittleFS
docs/                   Wiring, BOM, photos, and MQTT documentation
Enclosure and mount/    3D-printable 3MF files
artifacts/reflash/      Exported firmware and LittleFS images
platformio.ini          PlatformIO project configuration</code></pre>

<h2 id="license">License</h2>

<p>This project is licensed under the MIT License. See <a href="LICENSE">LICENSE</a> for the full text.</p>
