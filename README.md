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
  <li><a href="#component-reference">Component reference</a></li>
  <li><a href="#wiring">Wiring</a></li>
  <li><a href="#bill-of-materials">Bill of materials</a></li>
  <li><a href="#enclosure-and-assembly">Enclosure and assembly</a></li>
  <li><a href="#build-with-platformio">Build with PlatformIO</a></li>
  <li><a href="#upload-firmware">Upload firmware</a></li>
  <li><a href="#first-setup">First setup</a></li>
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

<h2 id="component-reference">Component reference</h2>

<p>These photos show the assembled components and their arrangement. Individual product photos are not included yet.</p>

<table>
  <tr>
    <td align="center"><img src="docs/Mounting%20plate.jpeg" alt="Controller, TDS meter, terminal, and cable holders on the mounting plate" width="100%"><br><em>Controller and TDS electronics on the mounting plate</em></td>
    <td align="center"><img src="docs/Plate_in_box.jpeg" alt="Wired components installed on the mounting plate" width="100%"><br><em>Wired components installed in the enclosure</em></td>
  </tr>
</table>

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

<p>No license has been selected for this project yet. Add a <code>LICENSE</code> file before accepting or requesting contributions if you want to define reuse terms.</p>
