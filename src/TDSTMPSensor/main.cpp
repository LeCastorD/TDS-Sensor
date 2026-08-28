#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ElegantOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <time.h>
#include <stdarg.h>
#include <math.h>
#include "fw_version_auto.h"

#ifndef FW_VERSION
#define FW_VERSION "0.0.0-dev"
#endif

#ifndef FW_BUILD_STAMP
#define FW_BUILD_STAMP "unknown"
#endif

#define DEVICE_NAME_LEN 32
#define MQTT_HOST_LEN 40
#define MQTT_CLIENT_ID_LEN 32
#define MQTT_USER_LEN 24
#define MQTT_PASS_LEN 32
#define OTA_USER_LEN 24
#define OTA_PASS_LEN 24
#define WEB_UI_SESSION_TOKEN_LEN 33
#define IPV4_STR_LEN 16
#define TIMEZONE_LEN 64
#define MQTT_TOPIC_SEGMENT_LEN 32
#define MQTT_TOPIC_BASE_LEN 96
#define MQTT_TOPIC_LEN 128

const char* DEFAULT_DEVICE_NAME_BASE = "tds-tmp-sensor";
const char* DEFAULT_MQTT_CLIENT_ID_BASE = "TDSTMPSensor";
const char* PREFERRED_TEMP_SENSOR_MODEL = "DS18B20";
const char* CONFIG_PATH = "/config.json";
const uint8_t TEMP_SENSOR_PIN = D2;
const uint8_t FACTORY_RESET_PIN = D5;
const unsigned long FACTORY_RESET_HOLD_MS = 3000UL;
const unsigned long MQTT_DANGEROUS_CMD_GUARD_MS = 60000UL;
const unsigned long mqttInterval = 5000UL;
const uint16_t MQTT_KEEPALIVE_SEC = 180;
const unsigned long TIME_SYNC_INTERVAL_MS = 12UL * 60UL * 60UL * 1000UL;
const unsigned long TIME_SYNC_RETRY_MS = 60000UL;
const time_t TIME_VALID_AFTER_EPOCH = 1704067200;
const char* NTP_SERVER_PRIMARY = "pool.ntp.org";
const char* NTP_SERVER_SECONDARY = "time.nist.gov";
const uint8_t TDS_SAMPLE_COUNT = 30;
const unsigned long TDS_ANALOG_SAMPLE_INTERVAL_MS = 40UL;
const float TDS_FALLBACK_TEMP_C = 25.0f;
const char* SENSOR_LOG_CSV_PATH = "/sensor_readings.csv";
const size_t SENSOR_LOG_MAX_BYTES = 102400;
const uint8_t SENSOR_LOG_RECENT_MAX_ENTRIES = 10;
const size_t SENSOR_LOG_LINE_BUF_LEN = 128;

char deviceName[DEVICE_NAME_LEN] = "tds-tmp-sensor";
char mqttBroker[MQTT_HOST_LEN] = "127.0.0.1";
uint16_t mqttPort = 1883;
char mqttClientId[MQTT_CLIENT_ID_LEN] = "TDSTMPSensor";
char mqttUser[MQTT_USER_LEN] = "homeassistant";
char mqttPass[MQTT_PASS_LEN] = "homeassistantpass";
char otaUser[OTA_USER_LEN] = "admin";
char otaPass[OTA_PASS_LEN] = "adminpass";
bool webUiLoginRequired = true;
char webUiSessionToken[WEB_UI_SESSION_TOKEN_LEN] = "";
bool networkUseDhcp = true;
char networkIp[IPV4_STR_LEN] = "";
char networkGateway[IPV4_STR_LEN] = "";
char networkNetmask[IPV4_STR_LEN] = "";
char networkDns[IPV4_STR_LEN] = "";
char timeZoneSpec[TIMEZONE_LEN] = "EST5EDT,M3.2.0/2,M11.1.0/2";
char mqttTopicNode[MQTT_TOPIC_SEGMENT_LEN] = "tds_tmp_sensor";
char MQTT_TOPIC_BASE[MQTT_TOPIC_BASE_LEN] = "tds_tmp_sensor/tds_tmp_sensor";
char MQTT_TOPIC_AVAIL[MQTT_TOPIC_LEN] = "tds_tmp_sensor/tds_tmp_sensor/status";
char MQTT_TOPIC_HEALTH[MQTT_TOPIC_LEN] = "tds_tmp_sensor/tds_tmp_sensor/health";
char MQTT_TOPIC_SYSTEM_STATE[MQTT_TOPIC_LEN] = "tds_tmp_sensor/tds_tmp_sensor/system/state";
char MQTT_TOPIC_SENSOR_STATE[MQTT_TOPIC_LEN] = "tds_tmp_sensor/tds_tmp_sensor/sensors/state";
char MQTT_TOPIC_CONTROL_STATE[MQTT_TOPIC_LEN] = "tds_tmp_sensor/tds_tmp_sensor/controls/state";
char MQTT_TOPIC_CMD[MQTT_TOPIC_LEN] = "tds_tmp_sensor/tds_tmp_sensor/cmd";
char MQTT_TOPIC_HA_CMD[MQTT_TOPIC_LEN] = "tds_tmp_sensor/tds_tmp_sensor/ha/cmd";
char MQTT_TOPIC_CMD_ACK[MQTT_TOPIC_LEN] = "tds_tmp_sensor/tds_tmp_sensor/cmd/ack";
const char* MQTT_TOPIC_HA_STATUS = "homeassistant/status";

struct SensorRuntime
{
    float temperatureC;
    float tdsPpm;
    float voltage;
    float temperatureRawC;
    uint16_t rawAdc;
    bool temperatureValid;
    bool tempBusParasitePower;
    uint8_t tempDeviceCount;
    bool tempAddressValid;
    char tempAddressHex[24];
    unsigned long sampledAtMs;
    time_t sampledAtEpoch;
    uint32_t sampleCount;
};

SensorRuntime sensorState = {0.0f, 0.0f, 0.0f, DEVICE_DISCONNECTED_C, 0, false, false, 0, false, "", 0UL, 0, 0};

float tdsCalibrationFactor = 1.0f;
float tdsOffsetPpm = 0.0f;
float tempOffsetC = 0.0f;
float tdsVref = 3.30f;
float tdsTempCompCutoffV = 2.3396f;
uint16_t sampleIntervalSec = 10;
uint32_t publishIntervalSec = 60;
uint32_t logSampleIntervalMin = 15;
unsigned long lastSampleMs = 0;
unsigned long lastPublishMs = 0;
unsigned long lastLogSampleMs = 0;
unsigned long lastMQTTAttempt = 0;
unsigned long lastTimeSyncMs = 0;
unsigned long bootMs = 0;
uint32_t mqttReconnectCount = 0;
uint32_t minFreeHeap = UINT32_MAX;
bool timeSyncHadValidTime = false;
bool sensorStatePublishPending = false;
bool haDiscoveryPublishPending = false;
unsigned long sensorStatePublishAfterMs = 0;
unsigned long haDiscoveryPublishAfterMs = 0;
int tdsAnalogBuffer[TDS_SAMPLE_COUNT] = {0};
int tdsAnalogBufferTemp[TDS_SAMPLE_COUNT] = {0};
uint8_t tdsAnalogBufferIndex = 0;
bool tdsAnalogBufferPrimed = false;
unsigned long lastTdsAnalogSampleMs = 0;

WiFiManager wm;
ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient mqttClient(espClient);
OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature tempSensors(&oneWire);

void printBootInfo();
void printHealth();
void setupWiFi();
void setupWebServer();
void setupOTA();
void setupMQTT();
void ensureMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void setupTimeSync();
void processTimeSync();
void requestTimeSync(const char* reason);
bool hasValidSystemTime();
void applyTimeZone();
void applyDefaultIdentityFromMac();
void rebuildMqttTopics();
bool isValidTimeZoneSpec(const String& tzSpec);
void sampleSensors();
void publishSensorState();
void publishSystemState();
void publishControlState();
bool publishHADiscovery();
void processDeferredPublishes();
void requestSensorStatePublish(unsigned long delayMs);
void requestHADiscoveryPublish(unsigned long delayMs);
void publishCmdAck(bool ok, const char* type, const char* detail);
void performFactoryReset(bool wipeFileSystem);
void checkHardwareFactoryReset();
void saveConfig();
void loadConfig();
void appendDebugLog(const char* fmt, ...);
void clearDebugLog();
String normalizeWebUiPath(const String& rawPath);
String buildWebUiSessionCookie();
String generateWebUiSessionToken();
String buildExpectedWebUiSessionToken();
void clearWebUiSession();
bool hasValidWebUiSession();
bool ensureWebUiAuth();
bool sendLittleFSFile(const String& path, bool noCache);
String formatUptimeCompact();
bool canExecuteDangerousMqttCommand();
bool isValidIpv4String(const String& value);
bool parseIpString(const char* text, IPAddress& out);
float parseLocalizedFloatArg(const String& rawValue, float fallbackValue);
long parseLocalizedLongArg(const String& rawValue, long fallbackValue);
void appendWebUiPageStart(String& html, const char* title, const char* heading);
void appendWebUiPageEnd(String& html);
void processTdsAnalogSampling();
int getMedianNum(const int* source, size_t len);
void appendSensorLogCsv();
void processSensorLogging();
uint8_t loadRecentSensorLogLines(char lines[][SENSOR_LOG_LINE_BUF_LEN], uint8_t maxEntries);
void appendHtmlEscaped(String& out, const char* text);

#include "mqtt_section.h"
#include "web_ui_section.h"

static bool isBlankOrDefault(const char* value, const char* def)
{
    if (!value || value[0] == '\0')
        return true;
    return strcmp(value, def) == 0;
}

void applyDefaultIdentityFromMac()
{
    uint32_t chipId = ESP.getChipId();
    char suffix[9];
    snprintf(suffix, sizeof(suffix), "%06X", (unsigned int)(chipId & 0xFFFFFF));

    if (isBlankOrDefault(deviceName, DEFAULT_DEVICE_NAME_BASE))
        snprintf(deviceName, sizeof(deviceName), "%s-%s", DEFAULT_DEVICE_NAME_BASE, suffix);
    if (isBlankOrDefault(mqttClientId, DEFAULT_MQTT_CLIENT_ID_BASE))
        snprintf(mqttClientId, sizeof(mqttClientId), "%s-%s", DEFAULT_MQTT_CLIENT_ID_BASE, suffix);
    if (strcmp(mqttTopicNode, "tds_tmp_sensor") == 0 || mqttTopicNode[0] == '\0')
        snprintf(mqttTopicNode, sizeof(mqttTopicNode), "tds_tmp_%s", suffix);
}

void rebuildMqttTopics()
{
    snprintf(MQTT_TOPIC_BASE, sizeof(MQTT_TOPIC_BASE), "tds_tmp_sensor/%s", mqttTopicNode);
    snprintf(MQTT_TOPIC_AVAIL, sizeof(MQTT_TOPIC_AVAIL), "%s/status", MQTT_TOPIC_BASE);
    snprintf(MQTT_TOPIC_HEALTH, sizeof(MQTT_TOPIC_HEALTH), "%s/health", MQTT_TOPIC_BASE);
    snprintf(MQTT_TOPIC_SYSTEM_STATE, sizeof(MQTT_TOPIC_SYSTEM_STATE), "%s/system/state", MQTT_TOPIC_BASE);
    snprintf(MQTT_TOPIC_SENSOR_STATE, sizeof(MQTT_TOPIC_SENSOR_STATE), "%s/sensors/state", MQTT_TOPIC_BASE);
    snprintf(MQTT_TOPIC_CONTROL_STATE, sizeof(MQTT_TOPIC_CONTROL_STATE), "%s/controls/state", MQTT_TOPIC_BASE);
    snprintf(MQTT_TOPIC_CMD, sizeof(MQTT_TOPIC_CMD), "%s/cmd", MQTT_TOPIC_BASE);
    snprintf(MQTT_TOPIC_HA_CMD, sizeof(MQTT_TOPIC_HA_CMD), "%s/ha/cmd", MQTT_TOPIC_BASE);
    snprintf(MQTT_TOPIC_CMD_ACK, sizeof(MQTT_TOPIC_CMD_ACK), "%s/cmd/ack", MQTT_TOPIC_BASE);
}

void printBootInfo()
{
    Serial.println();
    Serial.println("========================================");
    Serial.println("TDSTMPSensor boot");
    Serial.print("FW version: ");
    Serial.println(FW_VERSION);
    Serial.print("Build stamp: ");
    Serial.println(FW_BUILD_STAMP);
    Serial.print("Device name: ");
    Serial.println(deviceName);
    Serial.print("MQTT base: ");
    Serial.println(MQTT_TOPIC_BASE);
    Serial.println("========================================");
}

void appendDebugLog(const char* fmt, ...)
{
    char buffer[192];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    Serial.println(buffer);
}

void clearDebugLog()
{
}

uint8_t configSecretKeyByte(size_t index)
{
    static const uint8_t k[] = {0x41, 0x13, 0x5A, 0xD2, 0x27, 0x90, 0x6C, 0x3E};
    return k[index % sizeof(k)];
}

int hexNibbleFromChar(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

String obfuscateConfigSecret(const char* plain)
{
    if (!plain)
        return "";
    String out;
    const char* hex = "0123456789ABCDEF";
    for (size_t i = 0; plain[i] != '\0'; ++i)
    {
        uint8_t value = ((uint8_t)plain[i]) ^ configSecretKeyByte(i);
        out += hex[(value >> 4) & 0x0F];
        out += hex[value & 0x0F];
    }
    return out;
}

bool deobfuscateConfigSecret(const char* stored, char* out, size_t outLen)
{
    if (!out || outLen == 0)
        return false;
    out[0] = '\0';
    if (!stored || stored[0] == '\0')
        return true;

    size_t len = strlen(stored);
    if ((len % 2) != 0)
        return false;

    size_t plainLen = len / 2;
    if (plainLen >= outLen)
        plainLen = outLen - 1;

    for (size_t i = 0; i < plainLen; ++i)
    {
        int hi = hexNibbleFromChar(stored[i * 2]);
        int lo = hexNibbleFromChar(stored[i * 2 + 1]);
        if (hi < 0 || lo < 0)
            return false;
        out[i] = (char)(((hi << 4) | lo) ^ configSecretKeyByte(i));
    }
    out[plainLen] = '\0';
    return true;
}

void saveConfig()
{
    DynamicJsonDocument doc(3072);
    doc["device_name"] = deviceName;
    doc["mqtt_broker"] = mqttBroker;
    doc["mqtt_port"] = mqttPort;
    doc["mqtt_client_id"] = mqttClientId;
    doc["mqtt_user"] = mqttUser;
    doc["mqtt_pass_obf"] = obfuscateConfigSecret(mqttPass);
    doc["ota_user"] = otaUser;
    doc["ota_pass_obf"] = obfuscateConfigSecret(otaPass);
    doc["web_ui_login_required"] = webUiLoginRequired;
    doc["network_use_dhcp"] = networkUseDhcp;
    doc["network_ip"] = networkIp;
    doc["network_gateway"] = networkGateway;
    doc["network_netmask"] = networkNetmask;
    doc["network_dns"] = networkDns;
    doc["time_zone"] = timeZoneSpec;
    doc["mqtt_topic_node"] = mqttTopicNode;
    doc["tds_calibration_factor"] = tdsCalibrationFactor;
    doc["tds_offset_ppm"] = tdsOffsetPpm;
    doc["temp_offset_c"] = tempOffsetC;
    doc["tds_vref"] = tdsVref;
    doc["tds_temp_comp_cutoff_v"] = tdsTempCompCutoffV;
    doc["sample_interval_sec"] = sampleIntervalSec;
    doc["publish_interval_sec"] = publishIntervalSec;
    doc["log_sample_interval_min"] = logSampleIntervalMin;

    File f = LittleFS.open(CONFIG_PATH, "w");
    if (!f)
    {
        Serial.println("[CFG] Failed to open config for write");
        return;
    }
    serializeJsonPretty(doc, f);
    f.close();
}

void loadConfig()
{
    if (!LittleFS.exists(CONFIG_PATH))
        return;

    File f = LittleFS.open(CONFIG_PATH, "r");
    if (!f)
        return;

    DynamicJsonDocument doc(3072);
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err || !doc.is<JsonObject>())
    {
        Serial.println("[CFG] Invalid config.json");
        return;
    }

    strlcpy(deviceName, doc["device_name"] | deviceName, sizeof(deviceName));
    strlcpy(mqttBroker, doc["mqtt_broker"] | mqttBroker, sizeof(mqttBroker));
    mqttPort = (uint16_t)(doc["mqtt_port"] | mqttPort);
    strlcpy(mqttClientId, doc["mqtt_client_id"] | mqttClientId, sizeof(mqttClientId));
    strlcpy(mqttUser, doc["mqtt_user"] | mqttUser, sizeof(mqttUser));
    strlcpy(otaUser, doc["ota_user"] | otaUser, sizeof(otaUser));
    networkUseDhcp = doc["network_use_dhcp"] | networkUseDhcp;
    strlcpy(networkIp, doc["network_ip"] | networkIp, sizeof(networkIp));
    strlcpy(networkGateway, doc["network_gateway"] | networkGateway, sizeof(networkGateway));
    strlcpy(networkNetmask, doc["network_netmask"] | networkNetmask, sizeof(networkNetmask));
    strlcpy(networkDns, doc["network_dns"] | networkDns, sizeof(networkDns));
    strlcpy(timeZoneSpec, doc["time_zone"] | timeZoneSpec, sizeof(timeZoneSpec));
    strlcpy(mqttTopicNode, doc["mqtt_topic_node"] | mqttTopicNode, sizeof(mqttTopicNode));
    webUiLoginRequired = doc["web_ui_login_required"] | webUiLoginRequired;
    tdsCalibrationFactor = doc["tds_calibration_factor"] | tdsCalibrationFactor;
    tdsOffsetPpm = doc["tds_offset_ppm"] | tdsOffsetPpm;
    tempOffsetC = doc["temp_offset_c"] | tempOffsetC;
    tdsVref = doc["tds_vref"] | tdsVref;
    tdsTempCompCutoffV = doc["tds_temp_comp_cutoff_v"] | tdsTempCompCutoffV;
    sampleIntervalSec = doc["sample_interval_sec"] | sampleIntervalSec;
    publishIntervalSec = doc["publish_interval_sec"] | publishIntervalSec;
    logSampleIntervalMin = doc["log_sample_interval_min"] | logSampleIntervalMin;

    char plain[MQTT_PASS_LEN];
    if (deobfuscateConfigSecret(doc["mqtt_pass_obf"] | "", plain, sizeof(plain)))
        strlcpy(mqttPass, plain, sizeof(mqttPass));
    if (deobfuscateConfigSecret(doc["ota_pass_obf"] | "", plain, sizeof(plain)))
        strlcpy(otaPass, plain, sizeof(otaPass));

    if (sampleIntervalSec < 1) sampleIntervalSec = 1;
    if (sampleIntervalSec > 3600) sampleIntervalSec = 3600;
    if (publishIntervalSec < 5) publishIntervalSec = 5;
    if (publishIntervalSec > 86400) publishIntervalSec = 86400;
    if (logSampleIntervalMin > 10080) logSampleIntervalMin = 10080;
    if (tdsVref < 1.0f) tdsVref = 1.0f;
    if (tdsVref > 5.0f) tdsVref = 5.0f;
    if (tdsTempCompCutoffV < 0.0f) tdsTempCompCutoffV = 0.0f;
    if (tdsTempCompCutoffV > tdsVref) tdsTempCompCutoffV = tdsVref;
}

bool parseIpString(const char* text, IPAddress& out)
{
    if (!text || text[0] == '\0')
        return false;
    return out.fromString(text);
}

bool isValidIpv4String(const String& value)
{
    IPAddress ip;
    return value.length() > 0 && ip.fromString(value);
}

void sampleSensors()
{
    if (!tdsAnalogBufferPrimed)
    {
        for (uint8_t i = 0; i < TDS_SAMPLE_COUNT; ++i)
        {
            tdsAnalogBuffer[i] = analogRead(A0);
            delay(5);
        }
        tdsAnalogBufferPrimed = true;
    }

    sensorState.rawAdc = (uint16_t)getMedianNum(tdsAnalogBuffer, TDS_SAMPLE_COUNT);
    sensorState.voltage = ((float)sensorState.rawAdc * tdsVref) / 1024.0f;

    sensorState.tempDeviceCount = tempSensors.getDeviceCount();
    sensorState.tempBusParasitePower = tempSensors.isParasitePowerMode();
    DeviceAddress addr;
    sensorState.tempAddressValid = (sensorState.tempDeviceCount > 0) && tempSensors.getAddress(addr, 0);
    if (sensorState.tempAddressValid)
    {
        snprintf(
            sensorState.tempAddressHex,
            sizeof(sensorState.tempAddressHex),
            "%02X%02X%02X%02X%02X%02X%02X%02X",
            addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
    }
    else
    {
        sensorState.tempAddressHex[0] = '\0';
    }

    tempSensors.requestTemperatures();
    float rawTemp = tempSensors.getTempCByIndex(0);
    sensorState.temperatureRawC = rawTemp;
    sensorState.temperatureValid = (rawTemp > -100.0f && rawTemp < 125.0f);
    float compensationTemperatureC = TDS_FALLBACK_TEMP_C;
    if (sensorState.temperatureValid)
    {
        sensorState.temperatureC = rawTemp + tempOffsetC;
        compensationTemperatureC = sensorState.temperatureC;
    }
    else
    {
        sensorState.temperatureC = TDS_FALLBACK_TEMP_C + tempOffsetC;
    }

    float compensationCoefficient = 1.0f + 0.02f * (compensationTemperatureC - 25.0f);
    if (fabsf(compensationCoefficient) < 0.01f)
        compensationCoefficient = 1.0f;
    float compensatedVoltage = sensorState.voltage;
    if (sensorState.voltage < tdsTempCompCutoffV)
        compensatedVoltage = sensorState.voltage / compensationCoefficient;
    float tds = (133.42f * compensatedVoltage * compensatedVoltage * compensatedVoltage
        - 255.86f * compensatedVoltage * compensatedVoltage
        + 857.39f * compensatedVoltage) * 0.5f;
    tds = (tds * tdsCalibrationFactor) + tdsOffsetPpm;
    if (tds < 0.0f)
        tds = 0.0f;

    sensorState.tdsPpm = tds;
    sensorState.sampledAtMs = millis();
    sensorState.sampledAtEpoch = time(nullptr);
    sensorState.sampleCount++;

    requestSensorStatePublish(0);
}

void processTdsAnalogSampling()
{
    unsigned long now = millis();
    if (now - lastTdsAnalogSampleMs < TDS_ANALOG_SAMPLE_INTERVAL_MS)
        return;

    lastTdsAnalogSampleMs = now;
    tdsAnalogBuffer[tdsAnalogBufferIndex] = analogRead(A0);
    tdsAnalogBufferIndex++;
    if (tdsAnalogBufferIndex >= TDS_SAMPLE_COUNT)
    {
        tdsAnalogBufferIndex = 0;
        tdsAnalogBufferPrimed = true;
    }
}

int getMedianNum(const int* source, size_t len)
{
    if (!source || len == 0)
        return 0;

    if (len > TDS_SAMPLE_COUNT)
        len = TDS_SAMPLE_COUNT;

    for (size_t i = 0; i < len; ++i)
        tdsAnalogBufferTemp[i] = source[i];

    for (size_t j = 0; j < len; ++j)
    {
        for (size_t i = 0; i + 1 < len - j; ++i)
        {
            if (tdsAnalogBufferTemp[i] > tdsAnalogBufferTemp[i + 1])
            {
                int tmp = tdsAnalogBufferTemp[i];
                tdsAnalogBufferTemp[i] = tdsAnalogBufferTemp[i + 1];
                tdsAnalogBufferTemp[i + 1] = tmp;
            }
        }
    }

    if ((len & 1U) != 0U)
        return tdsAnalogBufferTemp[(len - 1U) / 2U];

    return (tdsAnalogBufferTemp[len / 2U] + tdsAnalogBufferTemp[(len / 2U) - 1U]) / 2;
}

void appendSensorLogCsv()
{
    if (sensorState.sampleCount == 0)
        return;

    bool shouldWriteHeader = false;
    if (LittleFS.exists(SENSOR_LOG_CSV_PATH))
    {
        File fCheck = LittleFS.open(SENSOR_LOG_CSV_PATH, "r");
        if (fCheck)
        {
            size_t currentSize = fCheck.size();
            fCheck.close();
            if (currentSize >= SENSOR_LOG_MAX_BYTES)
                LittleFS.remove(SENSOR_LOG_CSV_PATH);
            else if (currentSize == 0)
                shouldWriteHeader = true;
        }
    }
    else
    {
        shouldWriteHeader = true;
    }

    if (!LittleFS.exists(SENSOR_LOG_CSV_PATH))
        shouldWriteHeader = true;

    File f = LittleFS.open(SENSOR_LOG_CSV_PATH, "a");
    if (!f)
        return;

    if (shouldWriteHeader)
    {
        static const char* kSensorLogHeader = "Date, Temperature C, TDS PPM, Raw ADC, Voltage V\n";
        f.write((const uint8_t*)kSensorLogHeader, strlen(kSensorLogHeader));
    }

    char timestamp[24];
    if (hasValidSystemTime())
    {
        time_t nowTs = time(nullptr);
        struct tm localTm;
        localtime_r(&nowTs, &localTm);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &localTm);
    }
    else
    {
        unsigned long totalSec = millis() / 1000UL;
        unsigned long hh = (totalSec / 3600UL) % 24UL;
        unsigned long mm = (totalSec / 60UL) % 60UL;
        unsigned long ss = totalSec % 60UL;
        snprintf(timestamp, sizeof(timestamp), "0000-00-00 %02lu:%02lu:%02lu", hh, mm, ss);
    }

    char line[160];
    int n = snprintf(
        line,
        sizeof(line),
        "%s,%.2f,%.2f,%u,%.4f\n",
        timestamp,
        sensorState.temperatureValid ? sensorState.temperatureC : sensorState.temperatureRawC,
        sensorState.tdsPpm,
        (unsigned int)sensorState.rawAdc,
        sensorState.voltage);
    if (n > 0)
        f.write((const uint8_t*)line, (size_t)n);
    f.close();
}

void processSensorLogging()
{
    if (logSampleIntervalMin == 0)
        return;

    unsigned long intervalMs = logSampleIntervalMin * 60000UL;
    if (intervalMs == 0)
        return;

    unsigned long now = millis();
    if (lastLogSampleMs == 0)
    {
        lastLogSampleMs = now;
        return;
    }

    if (now - lastLogSampleMs < intervalMs)
        return;

    lastLogSampleMs = now;
    sampleSensors();
    appendSensorLogCsv();
}

uint8_t loadRecentSensorLogLines(char lines[][SENSOR_LOG_LINE_BUF_LEN], uint8_t maxEntries)
{
    if (!LittleFS.exists(SENSOR_LOG_CSV_PATH))
        return 0;

    File f = LittleFS.open(SENSOR_LOG_CSV_PATH, "r");
    if (!f)
        return 0;

    if (maxEntries > SENSOR_LOG_RECENT_MAX_ENTRIES)
        maxEntries = SENSOR_LOG_RECENT_MAX_ENTRIES;
    uint8_t lineCount = 0;
    static char revLine[SENSOR_LOG_LINE_BUF_LEN];
    size_t revLen = 0;

    auto pushReversedLine = [&](size_t len)
    {
        if (len == 0 || lineCount >= maxEntries)
            return;

        char line[SENSOR_LOG_LINE_BUF_LEN];
        for (size_t i = 0; i < len; i++)
            line[i] = revLine[len - 1 - i];
        line[len] = '\0';

        if (strncmp(line, "Date,", 5) == 0)
            return;

        strncpy(lines[lineCount], line, SENSOR_LOG_LINE_BUF_LEN - 1);
        lines[lineCount][SENSOR_LOG_LINE_BUF_LEN - 1] = '\0';
        lineCount++;
    };

    size_t fileSize = f.size();
    if (fileSize > 0)
    {
        size_t pos = fileSize;
        while (pos > 0 && lineCount < maxEntries)
        {
            pos--;
            if (!f.seek((uint32_t)pos, SeekSet))
                break;

            int ch = f.read();
            if (ch < 0)
                break;

            if (ch == '\n' || ch == '\r')
            {
                if (revLen > 0)
                {
                    pushReversedLine(revLen);
                    revLen = 0;
                }
                continue;
            }

            if (revLen < (SENSOR_LOG_LINE_BUF_LEN - 1))
                revLine[revLen++] = (char)ch;
        }

        if (lineCount < maxEntries && revLen > 0)
            pushReversedLine(revLen);
    }
    f.close();

    return lineCount;
}

void appendHtmlEscaped(String& out, const char* text)
{
    if (!text)
        return;

    while (*text)
    {
        char c = *text++;
        switch (c)
        {
            case '&':
                out += "&amp;";
                break;
            case '<':
                out += "&lt;";
                break;
            case '>':
                out += "&gt;";
                break;
            case '"':
                out += "&quot;";
                break;
            case '\'':
                out += "&#39;";
                break;
            case '\r':
                break;
            default:
                if ((uint8_t)c < 0x20)
                    out += ' ';
                else
                    out += c;
                break;
        }
    }
}

bool isValidTimeZoneSpec(const String& tzSpec)
{
    if (tzSpec.length() < 3 || tzSpec.length() >= TIMEZONE_LEN)
        return false;
    for (size_t i = 0; i < tzSpec.length(); ++i)
    {
        char c = tzSpec[i];
        bool ok = isalnum((unsigned char)c) || c == ',' || c == '.' || c == '/' || c == '+' || c == '-' || c == ':';
        if (!ok)
            return false;
    }
    return true;
}

void applyTimeZone()
{
    setenv("TZ", timeZoneSpec, 1);
    tzset();
}

bool hasValidSystemTime()
{
    return time(nullptr) >= TIME_VALID_AFTER_EPOCH;
}

void requestTimeSync(const char* reason)
{
    appendDebugLog("[TIME] Sync requested: %s", reason ? reason : "manual");
    configTime(timeZoneSpec, NTP_SERVER_PRIMARY, NTP_SERVER_SECONDARY);
    lastTimeSyncMs = millis();
}

void setupTimeSync()
{
    applyTimeZone();
    requestTimeSync("boot");
}

void processTimeSync()
{
    unsigned long now = millis();
    bool valid = hasValidSystemTime();
    if (valid)
    {
        if (!timeSyncHadValidTime)
            appendDebugLog("[TIME] Valid system time acquired");
        timeSyncHadValidTime = true;
        if (now - lastTimeSyncMs >= TIME_SYNC_INTERVAL_MS)
            requestTimeSync("periodic");
    }
    else if (now - lastTimeSyncMs >= TIME_SYNC_RETRY_MS)
    {
        requestTimeSync("retry");
    }
}

void setupWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.hostname(deviceName);

    if (!networkUseDhcp)
    {
        IPAddress ip;
        IPAddress gw;
        IPAddress mask;
        IPAddress dns;
        if (parseIpString(networkIp, ip) &&
            parseIpString(networkGateway, gw) &&
            parseIpString(networkNetmask, mask))
        {
            bool dnsOk = (networkDns[0] == '\0') || parseIpString(networkDns, dns);
            if (dnsOk)
            {
                if (networkDns[0] == '\0')
                    WiFi.config(ip, gw, mask);
                else
                    WiFi.config(ip, gw, mask, dns);
            }
        }
    }

    wm.setConfigPortalBlocking(true);
    wm.setConfigPortalTimeout(180);
    bool connected = wm.autoConnect(deviceName);
    if (!connected)
    {
        appendDebugLog("[WIFI] AutoConnect failed, rebooting");
        delay(1500);
        ESP.restart();
    }

    if (MDNS.begin(deviceName))
        appendDebugLog("[MDNS] Ready at http://%s.local", deviceName);
}

bool isIntervalElapsed(unsigned long now, unsigned long* lastTs, unsigned long intervalMs)
{
    if (now - *lastTs < intervalMs)
        return false;
    *lastTs = now;
    return true;
}

void printHealth()
{
    uint32_t heap = ESP.getFreeHeap();
    if (heap < minFreeHeap)
        minFreeHeap = heap;

    if (!mqttClient.connected())
        return;

    StaticJsonDocument<320> doc;
    char ipBuf[16];
    formatIpAddress(WiFi.localIP(), ipBuf, sizeof(ipBuf));
    doc["uptime_s"] = millis() / 1000UL;
    doc["wifi"] = WiFi.isConnected();
    doc["ip"] = ipBuf;
    doc["rssi"] = WiFi.RSSI();
    doc["heap"] = heap;
    doc["min_heap"] = minFreeHeap;
    doc["mqtt_reconnects"] = mqttReconnectCount;

    char payload[320];
    serializeJson(doc, payload, sizeof(payload));
    mqttClient.publish(MQTT_TOPIC_HEALTH, payload, true);
}

void performFactoryReset(bool wipeFileSystem)
{
    appendDebugLog("[RESET] Factory reset requested, wipeFileSystem=%d", wipeFileSystem ? 1 : 0);
    wm.resetSettings();
    if (wipeFileSystem)
    {
        LittleFS.remove(CONFIG_PATH);
    }
    delay(250);
    ESP.restart();
}

void checkHardwareFactoryReset()
{
    pinMode(FACTORY_RESET_PIN, INPUT_PULLUP);
    if (digitalRead(FACTORY_RESET_PIN) != LOW)
        return;

    unsigned long started = millis();
    while (digitalRead(FACTORY_RESET_PIN) == LOW)
    {
        if (millis() - started >= FACTORY_RESET_HOLD_MS)
        {
            performFactoryReset(true);
            return;
        }
        delay(10);
    }
}

String formatUptimeCompact()
{
    unsigned long total = millis() / 1000UL;
    unsigned long days = total / 86400UL;
    unsigned long hours = (total % 86400UL) / 3600UL;
    unsigned long mins = (total % 3600UL) / 60UL;
    unsigned long secs = total % 60UL;

    char out[48];
    snprintf(out, sizeof(out), "%lud %02luh %02lum %02lus", days, hours, mins, secs);
    return String(out);
}

void appendWebUiPageStart(String& html, const char* title, const char* heading)
{
    html.reserve(1200);
    html += "<!doctype html><html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'><title>";
    html += title;
    html += "</title><link rel='stylesheet' href='/ui/app.css'></head><body><div class='main'><h3>&#127811; ";
    html += heading;
    html += " &#127811;</h3>";
}

void appendWebUiPageEnd(String& html)
{
    html += "</div></body></html>";
}

String normalizeWebUiPath(const String& rawPath)
{
    if (!rawPath.startsWith("/"))
        return "/";
    if (rawPath.startsWith("//"))
        return "/";
    return rawPath;
}

float parseLocalizedFloatArg(const String& rawValue, float fallbackValue)
{
    String value = rawValue;
    value.trim();
    if (value.length() == 0)
        return fallbackValue;
    value.replace(',', '.');
    return value.toFloat();
}

long parseLocalizedLongArg(const String& rawValue, long fallbackValue)
{
    String value = rawValue;
    value.trim();
    if (value.length() == 0)
        return fallbackValue;
    value.replace(',', '.');
    int dotPos = value.indexOf('.');
    if (dotPos >= 0)
        value.remove(dotPos);
    return value.toInt();
}

String buildWebUiSessionCookie()
{
    String cookie = "TDSSSESSID=";
    cookie += buildExpectedWebUiSessionToken();
    cookie += "; Path=/; HttpOnly; SameSite=Lax";
    return cookie;
}

String buildExpectedWebUiSessionToken()
{
    uint32_t hash = 2166136261UL;

    auto mixChar = [&](char c)
    {
        hash ^= (uint8_t)c;
        hash *= 16777619UL;
    };

    auto mixString = [&](const char* s)
    {
        if (!s)
            return;
        while (*s)
        {
            mixChar(*s);
            s++;
        }
    };

    mixString(deviceName);
    mixString(otaUser);
    mixString(otaPass);
    mixString("TDSSSESSID");
    hash ^= ESP.getChipId();
    hash *= 16777619UL;

    char token[WEB_UI_SESSION_TOKEN_LEN];
    snprintf(
        token,
        sizeof(token),
        "%08lx%08lx",
        (unsigned long)ESP.getChipId(),
        (unsigned long)hash);
    return String(token);
}

String generateWebUiSessionToken()
{
    return buildExpectedWebUiSessionToken();
}

void clearWebUiSession()
{
    webUiSessionToken[0] = '\0';
}

bool hasValidWebUiSession()
{
    if (!webUiLoginRequired)
        return true;
    String cookie = server.header("Cookie");
    String expected = String("TDSSSESSID=") + buildExpectedWebUiSessionToken();
    return cookie.indexOf(expected) >= 0;
}

bool ensureWebUiAuth()
{
    if (hasValidWebUiSession())
        return true;
    server.sendHeader("Location", "/login?next=" + normalizeWebUiPath(server.uri()), true);
    server.send(303, "text/plain", "");
    return false;
}

bool sendLittleFSFile(const String& path, bool noCache)
{
    if (!LittleFS.exists(path))
        return false;
    File f = LittleFS.open(path, "r");
    if (!f)
        return false;
    if (noCache)
    {
        server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
        server.sendHeader("Pragma", "no-cache");
        server.sendHeader("Expires", "0");
    }
    String contentType = "text/plain";
    if (path.endsWith(".html")) contentType = "text/html";
    else if (path.endsWith(".css")) contentType = "text/css";
    else if (path.endsWith(".js")) contentType = "application/javascript";
    server.streamFile(f, contentType);
    f.close();
    return true;
}

bool canExecuteDangerousMqttCommand()
{
    return millis() >= MQTT_DANGEROUS_CMD_GUARD_MS;
}

void setupOTA()
{
    ElegantOTA.begin(&server, otaUser, otaPass);
}

void setup()
{
    Serial.begin(115200);
    delay(100);
    randomSeed(ESP.getCycleCount());
    bootMs = millis();

    if (!LittleFS.begin())
        Serial.println("[FS] LittleFS mount failed");

    loadConfig();
    applyDefaultIdentityFromMac();
    rebuildMqttTopics();
    checkHardwareFactoryReset();
    printBootInfo();

    tempSensors.begin();
    tempSensors.setResolution(12);
    setupWiFi();
    setupTimeSync();
    setupMQTT();
    setupWebServer();
    setupOTA();
    sampleSensors();
    appendSensorLogCsv();
    requestHADiscoveryPublish(500);
}

void loop()
{
    server.handleClient();
    if (WiFi.status() == WL_CONNECTED)
        MDNS.update();

    processTdsAnalogSampling();
    ensureMQTT();
    mqttClient.loop();
    processTimeSync();
    processDeferredPublishes();
    processSensorLogging();

    unsigned long now = millis();
    if (isIntervalElapsed(now, &lastSampleMs, (unsigned long)sampleIntervalSec * 1000UL))
        sampleSensors();
    if (isIntervalElapsed(now, &lastPublishMs, (unsigned long)publishIntervalSec * 1000UL))
    {
        requestSensorStatePublish(0);
        printHealth();
    }
}
