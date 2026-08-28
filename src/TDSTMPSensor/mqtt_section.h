static void formatIpAddress(const IPAddress& ip, char* out, size_t outLen)
{
    if (outLen == 0)
        return;
    snprintf(out, outLen, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

void setupMQTT()
{
    mqttClient.setServer(mqttBroker, mqttPort);
    mqttClient.setKeepAlive(MQTT_KEEPALIVE_SEC);
    mqttClient.setCallback(mqttCallback);
}

void requestSensorStatePublish(unsigned long delayMs)
{
    sensorStatePublishPending = true;
    sensorStatePublishAfterMs = millis() + delayMs;
}

void requestHADiscoveryPublish(unsigned long delayMs)
{
    haDiscoveryPublishPending = true;
    haDiscoveryPublishAfterMs = millis() + delayMs;
}

void publishCmdAck(bool ok, const char* type, const char* detail)
{
    if (!mqttClient.connected())
        return;

    StaticJsonDocument<192> doc;
    doc["ok"] = ok;
    doc["type"] = type;
    doc["detail"] = detail;
    doc["uptime_s"] = millis() / 1000UL;

    char payload[192];
    serializeJson(doc, payload, sizeof(payload));
    mqttClient.publish(MQTT_TOPIC_CMD_ACK, payload, false);
}

void publishSensorState()
{
    if (!mqttClient.connected())
        return;

    StaticJsonDocument<384> doc;
    if (sensorState.temperatureValid)
        doc["temperature_c"] = sensorState.temperatureC;
    else
        doc["temperature_c"] = nullptr;
    doc["temperature_valid"] = sensorState.temperatureValid;
    doc["temperature_raw_c"] = sensorState.temperatureRawC;
    doc["temp_device_count"] = sensorState.tempDeviceCount;
    doc["temp_parasite_power"] = sensorState.tempBusParasitePower;
    doc["temp_address_valid"] = sensorState.tempAddressValid;
    doc["temp_address"] = sensorState.tempAddressHex;
    doc["tds_ppm"] = sensorState.tdsPpm;
    doc["raw_adc"] = sensorState.rawAdc;
    doc["voltage"] = sensorState.voltage;
    doc["sample_count"] = sensorState.sampleCount;
    doc["sample_interval_sec"] = sampleIntervalSec;
    doc["publish_interval_sec"] = publishIntervalSec;
    doc["sampled_at_epoch"] = sensorState.sampledAtEpoch;
    doc["temp_sensor_model"] = PREFERRED_TEMP_SENSOR_MODEL;

    char payload[384];
    serializeJson(doc, payload, sizeof(payload));
    mqttClient.publish(MQTT_TOPIC_SENSOR_STATE, payload, true);
}

void publishSystemState()
{
    if (!mqttClient.connected())
        return;

    StaticJsonDocument<512> doc;
    char ipBuf[16];
    formatIpAddress(WiFi.localIP(), ipBuf, sizeof(ipBuf));
    doc["device_name"] = deviceName;
    doc["fw_version"] = FW_VERSION;
    doc["fw_build_stamp"] = FW_BUILD_STAMP;
    doc["wifi"] = WiFi.isConnected();
    doc["ip"] = ipBuf;
    doc["rssi"] = WiFi.RSSI();
    doc["uptime_s"] = millis() / 1000UL;
    doc["mqtt_base"] = MQTT_TOPIC_BASE;

    JsonObject sensor = doc.createNestedObject("sensor");
    if (sensorState.temperatureValid)
        sensor["temperature_c"] = sensorState.temperatureC;
    else
        sensor["temperature_c"] = nullptr;
    sensor["temperature_valid"] = sensorState.temperatureValid;
    sensor["temperature_raw_c"] = sensorState.temperatureRawC;
    sensor["temp_device_count"] = sensorState.tempDeviceCount;
    sensor["temp_parasite_power"] = sensorState.tempBusParasitePower;
    sensor["temp_address_valid"] = sensorState.tempAddressValid;
    sensor["temp_address"] = sensorState.tempAddressHex;
    sensor["tds_ppm"] = sensorState.tdsPpm;
    sensor["raw_adc"] = sensorState.rawAdc;
    sensor["voltage"] = sensorState.voltage;

    JsonObject cfg = doc.createNestedObject("config");
    cfg["tds_calibration_factor"] = tdsCalibrationFactor;
    cfg["tds_offset_ppm"] = tdsOffsetPpm;
    cfg["temp_offset_c"] = tempOffsetC;
    cfg["sample_interval_sec"] = sampleIntervalSec;
    cfg["publish_interval_sec"] = publishIntervalSec;

    char payload[512];
    serializeJson(doc, payload, sizeof(payload));
    mqttClient.publish(MQTT_TOPIC_SYSTEM_STATE, payload, true);
}

void publishControlState()
{
    if (!mqttClient.connected())
        return;

    StaticJsonDocument<256> doc;
    doc["tds_calibration_factor"] = tdsCalibrationFactor;
    doc["tds_offset_ppm"] = tdsOffsetPpm;
    doc["temp_offset_c"] = tempOffsetC;
    doc["sample_interval_sec"] = sampleIntervalSec;
    doc["publish_interval_sec"] = publishIntervalSec;

    char payload[256];
    serializeJson(doc, payload, sizeof(payload));
    mqttClient.publish(MQTT_TOPIC_CONTROL_STATE, payload, true);
}

static bool publishHASensorDiscovery(
    const char* objectId,
    const char* name,
    const char* stateTopic,
    const char* valueTemplate,
    const char* unit,
    const char* deviceClass,
    const char* icon)
{
    char topic[160];
    snprintf(topic, sizeof(topic), "homeassistant/sensor/%s/%s/config", deviceName, objectId);

    StaticJsonDocument<512> doc;
    doc["name"] = name;
    doc["uniq_id"] = String(deviceName) + "_" + objectId;
    doc["stat_t"] = stateTopic;
    doc["val_tpl"] = valueTemplate;
    doc["avty_t"] = MQTT_TOPIC_AVAIL;
    doc["pl_avail"] = "online";
    doc["pl_not_avail"] = "offline";
    if (unit && unit[0] != '\0') doc["unit_of_meas"] = unit;
    if (deviceClass && deviceClass[0] != '\0') doc["dev_cla"] = deviceClass;
    if (icon && icon[0] != '\0') doc["icon"] = icon;

    JsonObject dev = doc.createNestedObject("dev");
    dev["ids"][0] = deviceName;
    dev["name"] = deviceName;
    dev["mdl"] = "TDSTMPSensor";
    dev["mf"] = "LeCastorD";
    dev["sw"] = FW_VERSION;

    char payload[512];
    serializeJson(doc, payload, sizeof(payload));
    return mqttClient.publish(topic, payload, true);
}

static bool publishHANumberDiscovery(
    const char* objectId,
    const char* name,
    const char* stateTopic,
    const char* valueTemplate,
    const char* commandTopic,
    const char* commandTemplate,
    float minValue,
    float maxValue,
    float stepValue,
    const char* unit,
    const char* icon)
{
    char topic[160];
    snprintf(topic, sizeof(topic), "homeassistant/number/%s/%s/config", deviceName, objectId);

    StaticJsonDocument<768> doc;
    doc["name"] = name;
    doc["uniq_id"] = String(deviceName) + "_" + objectId;
    doc["stat_t"] = stateTopic;
    doc["val_tpl"] = valueTemplate;
    doc["cmd_t"] = commandTopic;
    doc["cmd_tpl"] = commandTemplate;
    doc["avty_t"] = MQTT_TOPIC_AVAIL;
    doc["pl_avail"] = "online";
    doc["pl_not_avail"] = "offline";
    doc["min"] = minValue;
    doc["max"] = maxValue;
    doc["step"] = stepValue;
    if (unit && unit[0] != '\0') doc["unit_of_meas"] = unit;
    if (icon && icon[0] != '\0') doc["icon"] = icon;

    JsonObject dev = doc.createNestedObject("dev");
    dev["ids"][0] = deviceName;
    dev["name"] = deviceName;
    dev["mdl"] = "TDSTMPSensor";
    dev["mf"] = "LeCastorD";
    dev["sw"] = FW_VERSION;

    char payload[768];
    serializeJson(doc, payload, sizeof(payload));
    return mqttClient.publish(topic, payload, true);
}

static bool publishHAButtonDiscovery(
    const char* objectId,
    const char* name,
    const char* commandTopic,
    const char* payloadPress,
    const char* icon)
{
    char topic[160];
    snprintf(topic, sizeof(topic), "homeassistant/button/%s/%s/config", deviceName, objectId);

    StaticJsonDocument<512> doc;
    doc["name"] = name;
    doc["uniq_id"] = String(deviceName) + "_" + objectId;
    doc["cmd_t"] = commandTopic;
    doc["pl_prs"] = payloadPress;
    doc["avty_t"] = MQTT_TOPIC_AVAIL;
    doc["pl_avail"] = "online";
    doc["pl_not_avail"] = "offline";
    if (icon && icon[0] != '\0') doc["icon"] = icon;

    JsonObject dev = doc.createNestedObject("dev");
    dev["ids"][0] = deviceName;
    dev["name"] = deviceName;
    dev["mdl"] = "TDSTMPSensor";
    dev["mf"] = "LeCastorD";
    dev["sw"] = FW_VERSION;

    char payload[512];
    serializeJson(doc, payload, sizeof(payload));
    return mqttClient.publish(topic, payload, true);
}

bool publishHADiscovery()
{
    if (!mqttClient.connected())
        return false;

    bool ok = true;
    ok &= publishHASensorDiscovery(
        "temperature_c",
        "Water Temperature",
        MQTT_TOPIC_SENSOR_STATE,
        "{{ value_json.temperature_c | default(0, true) | float(0) | round(2) }}",
        "\xC2\xB0""C",
        "temperature",
        "mdi:thermometer");
    ok &= publishHASensorDiscovery(
        "tds_ppm",
        "TDS",
        MQTT_TOPIC_SENSOR_STATE,
        "{{ value_json.tds_ppm | default(0, true) | float(0) | round(0) }}",
        "ppm",
        "",
        "mdi:water");
    ok &= publishHASensorDiscovery(
        "tds_voltage",
        "TDS Voltage",
        MQTT_TOPIC_SENSOR_STATE,
        "{{ value_json.voltage | default(0, true) | float(0) | round(3) }}",
        "V",
        "voltage",
        "mdi:sine-wave");
    ok &= publishHASensorDiscovery(
        "tds_raw_adc",
        "TDS Raw ADC",
        MQTT_TOPIC_SENSOR_STATE,
        "{{ value_json.raw_adc | default(0, true) | int(0) }}",
        "",
        "",
        "mdi:chart-bell-curve");

    ok &= publishHANumberDiscovery(
        "tds_calibration_factor",
        "TDS Calibration Factor",
        MQTT_TOPIC_CONTROL_STATE,
        "{{ value_json.tds_calibration_factor | default(1, true) | float(1) }}",
        MQTT_TOPIC_HA_CMD,
        "{\"ha\":\"set_tds_calibration\",\"value\":{{ value }}}",
        0.10f, 5.00f, 0.01f, "", "mdi:tune-variant");
    ok &= publishHANumberDiscovery(
        "tds_offset_ppm",
        "TDS Offset",
        MQTT_TOPIC_CONTROL_STATE,
        "{{ value_json.tds_offset_ppm | default(0, true) | float(0) }}",
        MQTT_TOPIC_HA_CMD,
        "{\"ha\":\"set_tds_offset\",\"ppm\":{{ value }}}",
        -500.0f, 500.0f, 1.0f, "ppm", "mdi:plus-minus");
    ok &= publishHANumberDiscovery(
        "temp_offset_c",
        "Temperature Offset",
        MQTT_TOPIC_CONTROL_STATE,
        "{{ value_json.temp_offset_c | default(0, true) | float(0) }}",
        MQTT_TOPIC_HA_CMD,
        "{\"ha\":\"set_temp_offset\",\"c\":{{ value }}}",
        -10.0f, 10.0f, 0.1f, "C", "mdi:thermometer-lines");
    ok &= publishHANumberDiscovery(
        "sample_interval_sec",
        "Sample Interval",
        MQTT_TOPIC_CONTROL_STATE,
        "{{ value_json.sample_interval_sec | default(10, true) | int(10) }}",
        MQTT_TOPIC_HA_CMD,
        "{\"ha\":\"set_sample_interval\",\"seconds\":{{ value | int }}}",
        1.0f, 3600.0f, 1.0f, "s", "mdi:timer-cog-outline");
    ok &= publishHANumberDiscovery(
        "publish_interval_sec",
        "Publish Interval",
        MQTT_TOPIC_CONTROL_STATE,
        "{{ value_json.publish_interval_sec | default(60, true) | int(60) }}",
        MQTT_TOPIC_HA_CMD,
        "{\"ha\":\"set_publish_interval\",\"seconds\":{{ value | int }}}",
        5.0f, 86400.0f, 1.0f, "s", "mdi:timer-sync-outline");
    ok &= publishHAButtonDiscovery(
        "sample_now",
        "Sample Now",
        MQTT_TOPIC_HA_CMD,
        "{\"ha\":\"sample_now\"}",
        "mdi:flash");

    return ok;
}

void processDeferredPublishes()
{
    unsigned long now = millis();
    if (mqttClient.connected())
    {
        if (sensorStatePublishPending && (long)(now - sensorStatePublishAfterMs) >= 0)
        {
            sensorStatePublishPending = false;
            publishSensorState();
            publishSystemState();
            publishControlState();
        }
        if (haDiscoveryPublishPending && (long)(now - haDiscoveryPublishAfterMs) >= 0)
        {
            haDiscoveryPublishPending = !publishHADiscovery();
            if (haDiscoveryPublishPending)
                haDiscoveryPublishAfterMs = millis() + 5000UL;
        }
    }
}

void ensureMQTT()
{
    if (mqttClient.connected())
        return;

    unsigned long now = millis();
    if (now - lastMQTTAttempt < mqttInterval)
        return;

    lastMQTTAttempt = now;
    Serial.println("[MQTT] Attempting connection...");

    if (mqttClient.connect(mqttClientId, mqttUser, mqttPass, MQTT_TOPIC_AVAIL, 1, true, "offline"))
    {
        mqttReconnectCount++;
        mqttClient.publish(MQTT_TOPIC_AVAIL, "online", true);
        mqttClient.subscribe(MQTT_TOPIC_CMD);
        mqttClient.subscribe(MQTT_TOPIC_HA_CMD);
        mqttClient.subscribe(MQTT_TOPIC_HA_STATUS);
        publishSensorState();
        publishSystemState();
        publishControlState();
        printHealth();
        requestHADiscoveryPublish(250);
        Serial.println("[MQTT] Connected");
    }
    else
    {
        Serial.print("[MQTT] Failed rc=");
        Serial.println(mqttClient.state());
    }
}

static void applyRuntimeConfigChange()
{
    saveConfig();
    requestSensorStatePublish(0);
}

void mqttCallback(char* topic, byte* payload, unsigned int length)
{
    char msg[512];
    if (length >= sizeof(msg))
        length = sizeof(msg) - 1;
    memcpy(msg, payload, length);
    msg[length] = '\0';

    if (strcmp(topic, MQTT_TOPIC_HA_STATUS) == 0)
    {
        if (strcmp(msg, "online") == 0)
        {
            requestSensorStatePublish(0);
            requestHADiscoveryPublish(250);
        }
        return;
    }

    DynamicJsonDocument doc(768);
    DeserializationError err = deserializeJson(doc, msg);
    if (err)
    {
        publishCmdAck(false, "json", "invalid_json");
        return;
    }

    const char* haCmd = doc["ha"] | "";
    const char* cmd = doc["cmd"] | "";
    const char* action = doc["action"] | "";

    if (haCmd[0] != '\0')
    {
        if (strcmp(haCmd, "sample_now") == 0)
        {
            sampleSensors();
            requestSensorStatePublish(0);
            publishCmdAck(true, "ha_sample_now", "ok");
            return;
        }
        if (strcmp(haCmd, "set_tds_calibration") == 0 && (doc["value"].is<float>() || doc["value"].is<int>()))
        {
            float value = doc["value"].as<float>();
            if (value < 0.1f || value > 5.0f)
            {
                publishCmdAck(false, "ha_set_tds_calibration", "invalid_value");
                return;
            }
            tdsCalibrationFactor = value;
            applyRuntimeConfigChange();
            publishCmdAck(true, "ha_set_tds_calibration", "updated");
            return;
        }
        if (strcmp(haCmd, "set_tds_offset") == 0 && (doc["ppm"].is<float>() || doc["ppm"].is<int>()))
        {
            tdsOffsetPpm = doc["ppm"].as<float>();
            applyRuntimeConfigChange();
            publishCmdAck(true, "ha_set_tds_offset", "updated");
            return;
        }
        if (strcmp(haCmd, "set_temp_offset") == 0 && (doc["c"].is<float>() || doc["c"].is<int>()))
        {
            tempOffsetC = doc["c"].as<float>();
            applyRuntimeConfigChange();
            publishCmdAck(true, "ha_set_temp_offset", "updated");
            return;
        }
        if (strcmp(haCmd, "set_sample_interval") == 0 && doc["seconds"].is<int>())
        {
            int value = doc["seconds"].as<int>();
            if (value < 1 || value > 3600)
            {
                publishCmdAck(false, "ha_set_sample_interval", "invalid_value");
                return;
            }
            sampleIntervalSec = (uint16_t)value;
            applyRuntimeConfigChange();
            publishCmdAck(true, "ha_set_sample_interval", "updated");
            return;
        }
        if (strcmp(haCmd, "set_publish_interval") == 0 && doc["seconds"].is<int>())
        {
            int value = doc["seconds"].as<int>();
            if (value < 5 || value > 86400)
            {
                publishCmdAck(false, "ha_set_publish_interval", "invalid_value");
                return;
            }
            publishIntervalSec = (uint16_t)value;
            applyRuntimeConfigChange();
            publishCmdAck(true, "ha_set_publish_interval", "updated");
            return;
        }

        publishCmdAck(false, "ha", "unknown_cmd");
        return;
    }

    if (cmd[0] != '\0')
    {
        if (strcmp(cmd, "sample_now") == 0)
        {
            sampleSensors();
            requestSensorStatePublish(0);
            publishCmdAck(true, "sample_now", "ok");
            return;
        }
        if (strcmp(cmd, "set_tds_calibration") == 0 && (doc["value"].is<float>() || doc["value"].is<int>()))
        {
            tdsCalibrationFactor = doc["value"].as<float>();
            applyRuntimeConfigChange();
            publishCmdAck(true, "set_tds_calibration", "updated");
            return;
        }
        publishCmdAck(false, "cmd", "unknown_cmd");
        return;
    }

    if (action[0] != '\0')
    {
        if (!canExecuteDangerousMqttCommand())
        {
            publishCmdAck(false, "action", "boot_guard");
            return;
        }
        if (strcmp(action, "reboot") == 0)
        {
            publishCmdAck(true, "action_reboot", "accepted");
            delay(150);
            ESP.restart();
            return;
        }
        if (strcmp(action, "factory_reset") == 0)
        {
            publishCmdAck(true, "action_factory_reset", "accepted");
            delay(150);
            performFactoryReset(true);
            return;
        }
        publishCmdAck(false, "action", "unknown_action");
        return;
    }

    publishCmdAck(false, "json", "unknown_payload");
}
