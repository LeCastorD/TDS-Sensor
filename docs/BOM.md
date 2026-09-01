# TDSTMPSensor Hardware BOM

<p>
  This bill of materials lists the electronics, connectors, assembly hardware, and printable parts used by the TDSTMPSensor project. Quantities are for one sensor assembly unless noted otherwise.
</p>

<h2 id="component-grid">Components</h2>

<table>
  <tr>
    <td valign="top" width="33%">
      <h3>Wemos D1 Mini</h3>
      <p><strong>Quantity:</strong> 1</p>
      <p>ESP8266 controller running the sensor firmware, Web UI, MQTT, and Home Assistant discovery.</p>
      <p align="center"><img src="wemos-d1-mini.jpeg" alt="Wemos D1 Mini ESP8266 controller" width="180"></p>
    </td>
    <td valign="top" width="33%">
      <h3>DFRobot SEN0244</h3>
      <p><strong>Quantity:</strong> 1</p>
      <p>Analog TDS sensor. Specified measurement range: 0-1000 ppm.</p>
      <p align="center"><img src="tds-sen0244.jpg" alt="DFRobot SEN0244 analog TDS sensor" width="180"></p>
    </td>
    <td valign="top" width="33%">
      <h3>DFRobot DS18B20</h3>
      <p><strong>Quantity:</strong> 1</p>
      <p>Waterproof temperature probe used for temperature compensation.</p>
      <p align="center"><img src="ds18b20.jpg" alt="DFRobot DS18B20 temperature probe" width="180"></p>
    </td>
  </tr>
  <tr>
    <td valign="top">
      <h3>Buck converter</h3>
      <p><strong>Quantity:</strong> 1</p>
      <p>12-24 V input to regulated 5 V output, 3 A module. The 5 V output feeds the Wemos only; the sensors use the Wemos 3.3 V rail.</p>
      <p align="center"><img src="buck-converter.jpeg" alt="12-24 V to 5 V buck converter" width="180"></p>
    </td>
    <td valign="top">
      <h3>Lever wire connectors</h3>
      <p><strong>Quantity:</strong> 1 set</p>
      <p>Two three-position electrical lever connectors for serviceable power wiring.</p>
      <p><em>Reference image not available.</em></p>
    </td>
    <td valign="top">
      <h3>JST-XH connector kit</h3>
      <p><strong>Quantity:</strong> 1 kit</p>
      <p>XH2.54 housings and contacts for the sensor and controller harness.</p>
      <p align="center"><img src="connectors.jpeg" alt="JST-XH connector kit" width="180"></p>
    </td>
  </tr>
  <tr>
    <td valign="top">
      <h3>Prewired XH2.54 cables</h3>
      <p><strong>Quantity:</strong> As required</p>
      <p>Prewired and crimped connector cables used to reduce assembly time.</p>
      <p align="center"><img src="connectors.jpeg" alt="Prewired JST-XH connector cables" width="180"></p>
    </td>
    <td valign="top">
      <h3>XH2.54 female contacts</h3>
      <p><strong>Quantity:</strong> As required</p>
      <p>Female pin contacts for custom wires and replacement harnesses.</p>
      <p align="center"><img src="connectors.jpeg" alt="XH2.54 female connector contacts" width="180"></p>
    </td>
    <td valign="top">
      <h3>JST crimping tool</h3>
      <p><strong>Quantity:</strong> 1</p>
      <p>Small connector crimping tool for JST-XH terminals.</p>
      <p align="center"><img src="Crimper.jpeg" alt="JST connector crimping tool" width="180"></p>
    </td>
  </tr>
  <tr>
    <td valign="top">
      <h3>12 V power supply</h3>
      <p><strong>Quantity:</strong> 1</p>
      <p>DC supply connected to the buck converter input. Do not connect it directly to the Wemos or sensors.</p>
      <p><em>Reference image not available.</em></p>
    </td>
    <td valign="top">
      <h3>Cable ties</h3>
      <p><strong>Quantity:</strong> As required</p>
      <p>Harness management and strain relief inside the enclosure.</p>
      <p><em>Reference image not available.</em></p>
    </td>
    <td valign="top">
      <h3>Printable enclosure parts</h3>
      <p><strong>Quantity:</strong> 1 set</p>
      <p>Print the enclosure box and the PCB/sensor support plate.</p>
      <p><a href="../Enclosure%20and%20mount/Box.3mf">Box.3mf</a><br><a href="../Enclosure%20and%20mount/Sensor%20Plate.3mf">Sensor Plate.3mf</a></p>
    </td>
  </tr>
</table>

<h2 id="assembly-hardware">Assembly hardware</h2>

<table>
  <thead>
    <tr>
      <th>Hardware</th>
      <th>Quantity</th>
      <th>Use</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>M3 x 16 screws</td>
      <td>4</td>
      <td>General enclosure or component assembly</td>
    </tr>
    <tr>
      <td>M3 x 6 screws</td>
      <td>6</td>
      <td>PCB mounting</td>
    </tr>
    <tr>
      <td>M3 x 10 screws</td>
      <td>4</td>
      <td>Support plate mounting</td>
    </tr>
  </tbody>
</table>

<h2 id="component-images">Component images</h2>

<p>
  The component cards above contain the available reference images. The images are also kept as individual files in this folder so they can be reused in other documentation pages.
</p>

<h2 id="printable-parts-and-credits">Printable parts and credits</h2>

<table>
  <thead>
    <tr>
      <th>Part or holder</th>
      <th>Source</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>Wemos D1 Mini board holder</td>
      <td><a href="https://www.printables.com/model/48425-wemos-d1-mini-frame-module-for-enclosures-openscad">Printables model 48425</a></td>
    </tr>
    <tr>
      <td>Capacitor holder</td>
      <td><a href="https://www.thingiverse.com/thing:4089178">Thingiverse model 4089178</a></td>
    </tr>
    <tr>
      <td>Generic lever wire connector holder</td>
      <td><a href="https://www.printables.com/model/170622-generic-lever-wire-connector-holder">Printables model 170622</a></td>
    </tr>
    <tr>
      <td>Cable tie holder</td>
      <td><a href="https://github.com/pfliegster/cable-tie-holders">pfliegster/cable-tie-holders</a></td>
    </tr>
    <tr>
      <td>Standoff</td>
      <td><a href="https://www.thingiverse.com/thing:351092">Thingiverse model 351092</a></td>
    </tr>
    <tr>
      <td>Enclosure box</td>
      <td><a href="https://www.printables.com/model/72839-customizable-parametric-stable-and-waterproof-elec">Printables enclosure model 72839</a></td>
    </tr>
  </tbody>
</table>

<h2 id="power-and-wiring-note">Power and wiring note</h2>

<p>
  The power path is <strong>12 V supply &rarr; buck converter &rarr; Wemos 5V</strong>. The SEN0244 and DS18B20 receive power from the Wemos <code>3V3</code> and <code>GND</code> pins. See the <a href="hardware-pinout.md">hardware pinout</a> and the <a href="../README.md#wiring">Wiring</a> section for the complete connection details.
</p>
