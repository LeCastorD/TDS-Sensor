static File backupRestoreUploadFile;
static bool backupRestoreUploadFailed = false;
static char backupRestoreUploadTmpPath[32] = {0};

static void backupRestoreResetUploadState()
{
    if (backupRestoreUploadFile)
        backupRestoreUploadFile.close();
    backupRestoreUploadFailed = false;
    backupRestoreUploadTmpPath[0] = '\0';
}

static void backupRestoreStartUpload(const char* tmpPath)
{
    backupRestoreResetUploadState();
    strlcpy(backupRestoreUploadTmpPath, tmpPath, sizeof(backupRestoreUploadTmpPath));
    if (LittleFS.exists(backupRestoreUploadTmpPath))
        LittleFS.remove(backupRestoreUploadTmpPath);
    backupRestoreUploadFile = LittleFS.open(backupRestoreUploadTmpPath, "w");
    if (!backupRestoreUploadFile)
        backupRestoreUploadFailed = true;
}

static void backupRestoreWriteUploadChunk(const uint8_t* data, size_t len)
{
    if (backupRestoreUploadFailed || !backupRestoreUploadFile)
        return;
    if (backupRestoreUploadFile.write(data, len) != len)
        backupRestoreUploadFailed = true;
}

static bool backupRestoreFinishUpload(size_t totalSize)
{
    if (backupRestoreUploadFile)
        backupRestoreUploadFile.close();
    if (backupRestoreUploadFailed || totalSize == 0)
        return false;
    return LittleFS.exists(backupRestoreUploadTmpPath);
}

static bool backupRestorePromoteUploadedFile(const char* tmpPath, const char* finalPath)
{
    if (!LittleFS.exists(tmpPath))
        return false;
    if (LittleFS.exists(finalPath))
        LittleFS.remove(finalPath);
    return LittleFS.rename(tmpPath, finalPath);
}

static bool backupRestoreValidateConfigJson(const char* path)
{
    if (!LittleFS.exists(path))
        return false;
    File f = LittleFS.open(path, "r");
    if (!f)
        return false;
    DynamicJsonDocument doc(3072);
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    return !err && doc.is<JsonObject>();
}

void setupWebServer()
{
    const char* headerKeys[] = {"Cookie"};
    server.collectHeaders(headerKeys, sizeof(headerKeys) / sizeof(headerKeys[0]));

#define REQUIRE_WEB_UI_AUTH() do { if (!ensureWebUiAuth()) return; } while (0)

    server.on("/login", HTTP_GET, []()
    {
        if (!webUiLoginRequired)
        {
            server.sendHeader("Location", "/", true);
            server.send(303, "text/plain", "");
            return;
        }

        if (hasValidWebUiSession())
        {
            String next = normalizeWebUiPath(server.arg("next"));
            server.sendHeader("Location", next, true);
            server.send(303, "text/plain", "");
            return;
        }

        String next = normalizeWebUiPath(server.arg("next"));
        bool invalidCreds = (server.hasArg("err") && server.arg("err") == "1");

        String html;
        html.reserve(2800);
        appendWebUiPageStart(html, "TDSTMPSensor - Login", "Sign in");
        html += "<p>Use your OTA credentials to access the Web UI.</p>";
        if (invalidCreds)
            html += "<p><b>Invalid username or password.</b></p>";
        html += "<fieldset class='r'><form method='POST' action='/login'>";
        html += "<input type='hidden' name='next' value='";
        html += next;
        html += "'>";
        html += "Username:<br><input id='username' name='username' type='text' autocomplete='username' autocapitalize='none' autocorrect='off' spellcheck='false'><br><br>";
        html += "Password:<br><input id='password' name='password' type='password' autocomplete='current-password'><br><br>";
        html += "<button type='submit'>Sign in</button></form></fieldset>";
        appendWebUiPageEnd(html);
        server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
        server.sendHeader("Pragma", "no-cache");
        server.sendHeader("Expires", "0");
        server.send(200, "text/html", html);
    });

    server.on("/login", HTTP_POST, []()
    {
        String next = normalizeWebUiPath(server.arg("next"));
        if (server.arg("username") == String(otaUser) && server.arg("password") == String(otaPass))
        {
            String token = generateWebUiSessionToken();
            strlcpy(webUiSessionToken, token.c_str(), sizeof(webUiSessionToken));
            server.sendHeader("Set-Cookie", buildWebUiSessionCookie());
            server.sendHeader("Location", next, true);
            server.send(303, "text/plain", "");
            return;
        }

        clearWebUiSession();
        server.sendHeader("Set-Cookie", "TDSSSESSID=; Path=/; Max-Age=0; HttpOnly; SameSite=Lax");
        server.sendHeader("Location", "/login?err=1&next=" + next, true);
        server.send(303, "text/plain", "");
    });

    server.on("/logout", HTTP_GET, []()
    {
        clearWebUiSession();
        server.sendHeader("Set-Cookie", "TDSSSESSID=; Path=/; Max-Age=0; HttpOnly; SameSite=Lax");
        server.sendHeader("Location", "/login", true);
        server.send(303, "text/plain", "");
    });

    server.on("/ui/app.css", HTTP_GET, []()
    {
        if (!sendLittleFSFile("/web/app.css", true))
            server.send(404, "text/plain", "Missing app.css");
    });

    server.on("/", HTTP_GET, []()
    {
        REQUIRE_WEB_UI_AUTH();
        if (!sendLittleFSFile("/web/index.html", true))
            server.send(404, "text/plain", "Missing index.html");
    });

    server.on("/measurements", HTTP_GET, []()
    {
        if (!sendLittleFSFile("/web/measurements.html", true))
            server.send(404, "text/plain", "Missing measurements.html");
    });

    server.on("/mqtt_send", HTTP_GET, []()
    {
        REQUIRE_WEB_UI_AUTH();
        if (!sendLittleFSFile("/web/mqtt-send.html", true))
            server.send(404, "text/plain", "Missing mqtt-send.html");
    });

    server.on("/mqtt_send", HTTP_POST, []()
    {
        REQUIRE_WEB_UI_AUTH();
        String topic = server.arg("topic");
        String payload = server.arg("payload");
        if (topic.length() == 0 || payload.length() == 0 || !mqttClient.connected())
        {
            server.sendHeader("Location", "/mqtt_send?sent=0", true);
            server.send(303, "text/plain", "");
            return;
        }
        bool ok = mqttClient.publish(topic.c_str(), payload.c_str(), false);
        server.sendHeader("Location", ok ? "/mqtt_send?sent=1" : "/mqtt_send?sent=0", true);
        server.send(303, "text/plain", "");
    });

    server.on("/settings", HTTP_GET, []()
    {
        REQUIRE_WEB_UI_AUTH();
        if (!sendLittleFSFile("/web/settings.html", true))
            server.send(404, "text/plain", "Missing settings.html");
    });

    server.on("/settings/network", HTTP_GET, []()
    {
        REQUIRE_WEB_UI_AUTH();
        sendLittleFSFile("/web/settings-network.html", true);
    });

    server.on("/settings/mqtt", HTTP_GET, []()
    {
        REQUIRE_WEB_UI_AUTH();
        sendLittleFSFile("/web/settings-mqtt.html", true);
    });

    server.on("/settings/time", HTTP_GET, []()
    {
        REQUIRE_WEB_UI_AUTH();
        sendLittleFSFile("/web/settings-time.html", true);
    });

    server.on("/settings/ota", HTTP_GET, []()
    {
        REQUIRE_WEB_UI_AUTH();
        sendLittleFSFile("/web/settings-ota.html", true);
    });

    server.on("/settings/sensors", HTTP_GET, []()
    {
        REQUIRE_WEB_UI_AUTH();
        sendLittleFSFile("/web/settings-sensors.html", true);
    });

    server.on("/settings/backup_restore", HTTP_GET, []()
    {
        REQUIRE_WEB_UI_AUTH();
        sendLittleFSFile("/web/settings-backup.html", true);
    });

    server.on("/reboot", HTTP_GET, []()
    {
        REQUIRE_WEB_UI_AUTH();
        sendLittleFSFile("/web/reboot.html", true);
    });

    server.on("/reboot_do", HTTP_GET, []()
    {
        REQUIRE_WEB_UI_AUTH();
        String html;
        appendWebUiPageStart(html, "TDSTMPSensor - Rebooting", "Rebooting");
        html += "<p>The device is rebooting now.</p>";
        appendWebUiPageEnd(html);
        server.send(200, "text/html", html);
        delay(250);
        ESP.restart();
    });

    server.on("/factory", HTTP_GET, []()
    {
        REQUIRE_WEB_UI_AUTH();
        sendLittleFSFile("/web/factory.html", true);
    });

    server.on("/factory_confirm", HTTP_GET, []()
    {
        REQUIRE_WEB_UI_AUTH();
        String html;
        appendWebUiPageStart(html, "TDSTMPSensor - Factory Reset", "Factory Reset");
        html += "<p>Factory reset accepted. The device will reboot.</p>";
        appendWebUiPageEnd(html);
        server.send(200, "text/html", html);
        delay(250);
        performFactoryReset(true);
    });

    server.on("/wifi_manager", HTTP_GET, []()
    {
        REQUIRE_WEB_UI_AUTH();
        wm.resetSettings();
        String html;
        appendWebUiPageStart(html, "TDSTMPSensor - WiFiManager", "WiFiManager");
        html += "<p>WiFi credentials cleared. The device is rebooting into setup mode.</p>";
        appendWebUiPageEnd(html);
        server.send(200, "text/html", html);
        delay(250);
        ESP.restart();
    });

    server.on("/ha_discovery_reset", HTTP_GET, []()
    {
        REQUIRE_WEB_UI_AUTH();
        requestHADiscoveryPublish(100);
        server.sendHeader("Location", "/settings?ha_reset=1", true);
        server.send(303, "text/plain", "");
    });

    server.on("/sample_now", HTTP_GET, []()
    {
        REQUIRE_WEB_UI_AUTH();
        sampleSensors();
        server.sendHeader("Location", "/measurements?sampled=1", true);
        server.send(303, "text/plain", "");
    });

    server.on("/api/ui/home", HTTP_GET, []()
    {
        REQUIRE_WEB_UI_AUTH();
        StaticJsonDocument<512> doc;
        doc["fw_version"] = FW_VERSION;
        doc["fw_build_stamp"] = FW_BUILD_STAMP;
        doc["min_free_heap"] = (minFreeHeap == UINT32_MAX) ? ESP.getFreeHeap() : minFreeHeap;
        doc["rssi"] = WiFi.RSSI();
        doc["uptime"] = formatUptimeCompact();
        doc["web_login_required"] = webUiLoginRequired;
        doc["device_name"] = deviceName;
        doc["mqtt_base"] = MQTT_TOPIC_BASE;
        if (sensorState.temperatureValid)
            doc["temperature_c"] = sensorState.temperatureC;
        else
            doc["temperature_c"] = nullptr;
        doc["tds_ppm"] = sensorState.tdsPpm;
        doc["log_sample_interval_min"] = logSampleIntervalMin;
        char payload[512];
        serializeJson(doc, payload, sizeof(payload));
        server.send(200, "application/json", payload);
    });

    server.on("/api/ui/log", HTTP_GET, []()
    {
        static char recentLines[SENSOR_LOG_RECENT_MAX_ENTRIES][SENSOR_LOG_LINE_BUF_LEN];
        uint8_t lineCount = loadRecentSensorLogLines(recentLines, SENSOR_LOG_RECENT_MAX_ENTRIES);

        String payload;
        payload.reserve(1600);
        if (lineCount == 0)
        {
            payload += "<p>No logged readings yet.</p>";
        }
        else
        {
            payload += "<pre style='white-space:pre-wrap;border:1px solid #444;padding:10px;background:#1f1f1f;color:#65c115;'>";
            for (uint8_t i = 0; i < lineCount; i++)
            {
                appendHtmlEscaped(payload, recentLines[i]);
                payload += "\n";
            }
            payload += "</pre>";
        }
        server.send(200, "text/html", payload);
    });

    server.on("/api/ui/measurements", HTTP_GET, []()
    {
        StaticJsonDocument<512> doc;
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
        doc["log_sample_interval_min"] = logSampleIntervalMin;
        doc["tds_calibration_factor"] = tdsCalibrationFactor;
        doc["tds_offset_ppm"] = tdsOffsetPpm;
        doc["temp_offset_c"] = tempOffsetC;
        doc["mqtt_connected"] = mqttClient.connected();
        doc["mqtt_reconnects"] = mqttReconnectCount;
        doc["uptime"] = formatUptimeCompact();
        char payload[512];
        serializeJson(doc, payload, sizeof(payload));
        server.send(200, "application/json", payload);
    });

    server.on("/api/ui/settings/network", HTTP_GET, []()
    {
        REQUIRE_WEB_UI_AUTH();
        StaticJsonDocument<192> doc;
        doc["network_mode"] = networkUseDhcp ? "dhcp" : "manual";
        doc["device_name"] = deviceName;
        doc["ip"] = networkIp;
        doc["gateway"] = networkGateway;
        doc["netmask"] = networkNetmask;
        doc["dns"] = networkDns;
        char payload[192];
        serializeJson(doc, payload, sizeof(payload));
        server.send(200, "application/json", payload);
    });

    server.on("/api/ui/settings/mqtt", HTTP_GET, []()
    {
        REQUIRE_WEB_UI_AUTH();
        StaticJsonDocument<256> doc;
        doc["broker"] = mqttBroker;
        doc["port"] = mqttPort;
        doc["client_id"] = mqttClientId;
        doc["user"] = mqttUser;
        doc["pass"] = mqttPass;
        doc["connected"] = mqttClient.connected();
        doc["reconnects"] = mqttReconnectCount;
        char payload[256];
        serializeJson(doc, payload, sizeof(payload));
        server.send(200, "application/json", payload);
    });

    server.on("/api/ui/settings/time", HTTP_GET, []()
    {
        REQUIRE_WEB_UI_AUTH();
        StaticJsonDocument<192> doc;
        doc["time_zone"] = timeZoneSpec;
        if (hasValidSystemTime())
        {
            char timeBuf[48];
            time_t now = time(nullptr);
            struct tm tmNow;
            localtime_r(&now, &tmNow);
            strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S %Z", &tmNow);
            doc["current_time"] = timeBuf;
        }
        else
        {
            doc["current_time"] = "Time not synchronized yet";
        }
        char payload[192];
        serializeJson(doc, payload, sizeof(payload));
        server.send(200, "application/json", payload);
    });

    server.on("/api/ui/settings/ota", HTTP_GET, []()
    {
        REQUIRE_WEB_UI_AUTH();
        StaticJsonDocument<160> doc;
        doc["user"] = otaUser;
        doc["pass"] = otaPass;
        doc["web_login_required"] = webUiLoginRequired;
        char payload[160];
        serializeJson(doc, payload, sizeof(payload));
        server.send(200, "application/json", payload);
    });

    server.on("/api/ui/settings/sensors", HTTP_GET, []()
    {
        REQUIRE_WEB_UI_AUTH();
        StaticJsonDocument<256> doc;
        doc["tds_calibration_factor"] = tdsCalibrationFactor;
        doc["tds_offset_ppm"] = tdsOffsetPpm;
        doc["temp_offset_c"] = tempOffsetC;
        doc["tds_vref"] = tdsVref;
        doc["tds_temp_comp_cutoff_v"] = tdsTempCompCutoffV;
        doc["sample_interval_sec"] = sampleIntervalSec;
        doc["publish_interval_sec"] = publishIntervalSec;
        doc["log_sample_interval_min"] = logSampleIntervalMin;
        doc["temp_sensor_model"] = PREFERRED_TEMP_SENSOR_MODEL;
        char payload[256];
        serializeJson(doc, payload, sizeof(payload));
        server.send(200, "application/json", payload);
    });

    server.on("/api/ui/settings/backup_restore", HTTP_GET, []()
    {
        REQUIRE_WEB_UI_AUTH();
        StaticJsonDocument<160> doc;
        bool configExists = LittleFS.exists(CONFIG_PATH);
        size_t configSize = 0;
        size_t logSize = 0;
        if (configExists)
        {
            File f = LittleFS.open(CONFIG_PATH, "r");
            if (f)
            {
                configSize = f.size();
                f.close();
            }
        }
        if (LittleFS.exists(SENSOR_LOG_CSV_PATH))
        {
            File f = LittleFS.open(SENSOR_LOG_CSV_PATH, "r");
            if (f)
            {
                logSize = f.size();
                f.close();
            }
        }
        doc["config_exists"] = configExists;
        doc["config_size"] = (unsigned long)configSize;
        doc["sensor_log_exists"] = LittleFS.exists(SENSOR_LOG_CSV_PATH);
        doc["sensor_log_size"] = (unsigned long)logSize;
        doc["sensor_log_max_bytes"] = (unsigned long)SENSOR_LOG_MAX_BYTES;
        char payload[160];
        serializeJson(doc, payload, sizeof(payload));
        server.send(200, "application/json", payload);
    });

    server.on("/settings/network_save", HTTP_POST, []()
    {
        REQUIRE_WEB_UI_AUTH();
        bool warning = false;
        bool networkChanged = false;
        String mode = server.arg("network_mode");
        bool newDhcp = (mode != "manual");
        if (newDhcp != networkUseDhcp)
        {
            networkUseDhcp = newDhcp;
            networkChanged = true;
        }

        String newDeviceName = server.arg("device_name");
        newDeviceName.trim();
        if (newDeviceName.length() > 0 && newDeviceName.length() < DEVICE_NAME_LEN)
        {
            if (newDeviceName != String(deviceName))
            {
                strlcpy(deviceName, newDeviceName.c_str(), sizeof(deviceName));
                networkChanged = true;
            }
        }

        if (!networkUseDhcp)
        {
            String ip = server.arg("network_ip");
            String gw = server.arg("network_gateway");
            String mask = server.arg("network_netmask");
            String dns = server.arg("network_dns");
            if (!isValidIpv4String(ip) || !isValidIpv4String(gw) || !isValidIpv4String(mask) || (dns.length() > 0 && !isValidIpv4String(dns)))
                warning = true;
            else
            {
                strlcpy(networkIp, ip.c_str(), sizeof(networkIp));
                strlcpy(networkGateway, gw.c_str(), sizeof(networkGateway));
                strlcpy(networkNetmask, mask.c_str(), sizeof(networkNetmask));
                strlcpy(networkDns, dns.c_str(), sizeof(networkDns));
                networkChanged = true;
            }
        }

        saveConfig();
        rebuildMqttTopics();
        server.sendHeader("Location", String("/settings/network?saved=1") + (warning ? "&warning=1" : "") + (networkChanged ? "&network_changed=1" : ""), true);
        server.send(303, "text/plain", "");
    });

    server.on("/settings/mqtt_save", HTTP_POST, []()
    {
        REQUIRE_WEB_UI_AUTH();
        bool warning = false;
        strlcpy(mqttBroker, server.arg("mqtt_broker").c_str(), sizeof(mqttBroker));
        uint32_t port = (uint32_t)server.arg("mqtt_port").toInt();
        if (port < 1 || port > 65535)
            warning = true;
        else
            mqttPort = (uint16_t)port;
        strlcpy(mqttClientId, server.arg("mqtt_client_id").c_str(), sizeof(mqttClientId));
        strlcpy(mqttUser, server.arg("mqtt_user").c_str(), sizeof(mqttUser));
        if (server.arg("mqtt_pass").length() > 0)
            strlcpy(mqttPass, server.arg("mqtt_pass").c_str(), sizeof(mqttPass));
        saveConfig();
        setupMQTT();
        if (mqttClient.connected())
            mqttClient.disconnect();
        server.sendHeader("Location", String("/settings/mqtt?saved=1") + (warning ? "&warning=1" : ""), true);
        server.send(303, "text/plain", "");
    });

    server.on("/settings/time_save", HTTP_POST, []()
    {
        REQUIRE_WEB_UI_AUTH();
        String tz = server.arg("time_tz");
        bool warning = !isValidTimeZoneSpec(tz);
        if (!warning)
        {
            strlcpy(timeZoneSpec, tz.c_str(), sizeof(timeZoneSpec));
            applyTimeZone();
            requestTimeSync("web_ui");
            saveConfig();
        }
        server.sendHeader("Location", String("/settings/time?saved=") + (warning ? "0&warning=1" : "1&time_changed=1"), true);
        server.send(303, "text/plain", "");
    });

    server.on("/settings/ota_save", HTTP_POST, []()
    {
        REQUIRE_WEB_UI_AUTH();
        if (server.arg("ota_user").length() > 0)
            strlcpy(otaUser, server.arg("ota_user").c_str(), sizeof(otaUser));
        if (server.arg("ota_pass").length() > 0)
            strlcpy(otaPass, server.arg("ota_pass").c_str(), sizeof(otaPass));
        webUiLoginRequired = server.hasArg("web_login_required");
        saveConfig();
        clearWebUiSession();
        server.sendHeader("Location", "/settings/ota?saved=1", true);
        server.send(303, "text/plain", "");
    });

    server.on("/settings/sensors_save", HTTP_POST, []()
    {
        REQUIRE_WEB_UI_AUTH();
        tdsCalibrationFactor = parseLocalizedFloatArg(server.arg("tds_calibration_factor"), tdsCalibrationFactor);
        tdsOffsetPpm = parseLocalizedFloatArg(server.arg("tds_offset_ppm"), tdsOffsetPpm);
        tempOffsetC = parseLocalizedFloatArg(server.arg("temp_offset_c"), tempOffsetC);
        tdsVref = parseLocalizedFloatArg(server.arg("tds_vref"), tdsVref);
        tdsTempCompCutoffV = parseLocalizedFloatArg(server.arg("tds_temp_comp_cutoff_v"), tdsTempCompCutoffV);
        long requestedSampleInterval = parseLocalizedLongArg(server.arg("sample_interval_sec"), sampleIntervalSec);
        long requestedPublishInterval = parseLocalizedLongArg(server.arg("publish_interval_sec"), publishIntervalSec);
        long requestedLogInterval = parseLocalizedLongArg(server.arg("log_sample_interval_min"), logSampleIntervalMin);
        if (requestedSampleInterval < 1) requestedSampleInterval = 1;
        if (requestedPublishInterval < 5) requestedPublishInterval = 5;
        if (requestedLogInterval < 0) requestedLogInterval = 0;
        if (requestedLogInterval > 10080) requestedLogInterval = 10080;
        sampleIntervalSec = (uint16_t)requestedSampleInterval;
        publishIntervalSec = (uint32_t)requestedPublishInterval;
        logSampleIntervalMin = (uint32_t)requestedLogInterval;
        if (tdsCalibrationFactor < 0.1f) tdsCalibrationFactor = 0.1f;
        if (tdsCalibrationFactor > 5.0f) tdsCalibrationFactor = 5.0f;
        if (tdsVref < 1.0f) tdsVref = 1.0f;
        if (tdsVref > 5.0f) tdsVref = 5.0f;
        if (tdsTempCompCutoffV < 0.0f) tdsTempCompCutoffV = 0.0f;
        if (tdsTempCompCutoffV > tdsVref) tdsTempCompCutoffV = tdsVref;
        saveConfig();
        sampleSensors();
        server.sendHeader("Location", "/settings/sensors?saved=1", true);
        server.send(303, "text/plain", "");
    });

    server.on("/settings/backup_restore/download_sensor_log", HTTP_GET, []()
    {
        REQUIRE_WEB_UI_AUTH();
        if (!LittleFS.exists(SENSOR_LOG_CSV_PATH))
        {
            server.send(404, "text/plain", "sensor_readings.csv not found");
            return;
        }
        File f = LittleFS.open(SENSOR_LOG_CSV_PATH, "r");
        if (!f)
        {
            server.send(500, "text/plain", "Unable to open sensor log");
            return;
        }
        server.sendHeader("Content-Disposition", "attachment; filename=\"sensor_readings.csv\"");
        server.streamFile(f, "text/csv");
        f.close();
    });

    server.on("/settings/backup_restore/download_config", HTTP_GET, []()
    {
        REQUIRE_WEB_UI_AUTH();
        if (!LittleFS.exists(CONFIG_PATH))
        {
            server.send(404, "text/plain", "config.json not found");
            return;
        }
        File f = LittleFS.open(CONFIG_PATH, "r");
        if (!f)
        {
            server.send(500, "text/plain", "Unable to open config.json");
            return;
        }
        server.sendHeader("Content-Disposition", "attachment; filename=\"config.json\"");
        server.streamFile(f, "application/json");
        f.close();
    });

    server.on("/settings/backup_restore/upload_config", HTTP_POST, []()
    {
        REQUIRE_WEB_UI_AUTH();
        bool ok = backupRestoreFinishUpload(server.upload().totalSize) &&
            backupRestoreValidateConfigJson("/upload_config.tmp") &&
            backupRestorePromoteUploadedFile("/upload_config.tmp", CONFIG_PATH);
        if (LittleFS.exists("/upload_config.tmp"))
            LittleFS.remove("/upload_config.tmp");
        if (ok)
            loadConfig();
        server.sendHeader("Location", ok ? "/settings/backup_restore?config_restored=1" : "/settings/backup_restore?config_restored=0&error=1", true);
        server.send(303, "text/plain", "");
    }, []()
    {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START)
            backupRestoreStartUpload("/upload_config.tmp");
        else if (upload.status == UPLOAD_FILE_WRITE)
            backupRestoreWriteUploadChunk(upload.buf, upload.currentSize);
        else if (upload.status == UPLOAD_FILE_END || upload.status == UPLOAD_FILE_ABORTED)
        {
            if (backupRestoreUploadFile)
                backupRestoreUploadFile.close();
        }
    });

    server.onNotFound([]()
    {
        REQUIRE_WEB_UI_AUTH();
        server.send(404, "text/plain", "Not found");
    });

    server.begin();
}
