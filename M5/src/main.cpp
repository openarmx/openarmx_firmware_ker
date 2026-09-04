// Copyright 2026 Enactic, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <M5Unified.h>
#include <Preferences.h>
#if defined(USE_RUNTIME) || defined(USE_WIFI)
#include <ArduinoJson.h>
#include <WiFi.h>
#endif
#include "Common.h"
#include "AngleProcessor.h"
#include "RSNexus.h"
#include "GUIHandler.h"
#include "M5UnitScroll.h"
#include "Meta.h"

#if ((defined(USE_RUNTIME) ? 1 : 0) + (defined(USE_USB) ? 1 : 0) + \
     (defined(USE_WIFI) ? 1 : 0) + (defined(USE_SERIAL) ? 1 : 0)) != 1
#error "Define exactly one transport build environment"
#endif

#if defined(USE_RUNTIME)
#include "RuntimeStream.h"
RuntimeStream stream;
USBCDC config_serial;
#define KER_CONFIG_SERIAL config_serial
#elif defined(USE_USB)
#include "USBStream.h"
USBStream stream;
#elif defined(USE_WIFI)
#include "WiFiStream.h"
WiFiStream stream;
#define KER_CONFIG_SERIAL Serial
#else
#include "SerialStream.h"
SerialStream stream(Serial);
#endif

#if defined(USE_RUNTIME)
static constexpr const char* ACTIVE_FW_VERSION = FW_VERSION_RUNTIME;
#elif defined(USE_USB)
static constexpr const char* ACTIVE_FW_VERSION = FW_VERSION_USB;
#elif defined(USE_WIFI)
static constexpr const char* ACTIVE_FW_VERSION = FW_VERSION_WIFI;
#else
static constexpr const char* ACTIVE_FW_VERSION = FW_VERSION_SERIAL;
#endif

// =====================================================
// Global objects
// =====================================================
RSNexus      rs485nexus(Serial2, RS485_EN_PIN);
GUIHandler   gui;
Preferences  preferences;
#if defined(USE_RUNTIME) || defined(USE_WIFI)
Preferences  network_preferences;
#endif
M5UnitScroll scroll;

// =====================================================
// Angle processors
// =====================================================
AngleProcessor angle_processors[NUM_SENSORS];

// =====================================================
// System state
// =====================================================
SystemState g_state;

// =====================================================
// Queues
// =====================================================
QueueHandle_t dataQueue;
QueueHandle_t guiQueue;

// =====================================================
// Shared encoder state
// =====================================================
bool encoder_found = false;
// volatile int16_t shared_encoder_value  = 0;  // unused
// volatile uint8_t shared_encoder_button = 0;  // unused

#if defined(USE_RUNTIME) || defined(USE_WIFI)
static String config_line;

static void sendConfigResponse(JsonDocument& response) {
    serializeJson(response, KER_CONFIG_SERIAL);
    KER_CONFIG_SERIAL.println();
}

static const char* wifiStatusName() {
#if defined(USE_RUNTIME)
    if (stream.mode() != TransportMode::WIFI) return "inactive";
#endif
    switch (WiFi.status()) {
        case WL_CONNECTED: return "connected";
        case WL_NO_SSID_AVAIL: return "ssid_not_found";
        case WL_CONNECT_FAILED: return "connect_failed";
        case WL_CONNECTION_LOST: return "connection_lost";
        case WL_DISCONNECTED: return "disconnected";
        default: return "connecting";
    }
}

static void handleConfigCommand(const String& line) {
    DynamicJsonDocument request(1024);
    DeserializationError error = deserializeJson(request, line);
    DynamicJsonDocument response(4096);
    response["type"] = "response";

    if (error) {
        response["ok"] = false;
        response["error"] = "invalid_json";
        sendConfigResponse(response);
        return;
    }

    const char* command = request["cmd"] | "";
    response["cmd"] = command;

    if (strcmp(command, "get_status") == 0) {
        response["ok"] = true;
        response["fw"] = ACTIVE_FW_VERSION;
        response["hw"] = HW_VERSION;
#if defined(USE_RUNTIME)
        response["transport"] = stream.modeName();
        response["transport_switching"] = stream.transportSwitching();
        JsonArray transports = response.createNestedArray("available_transports");
        transports.add("usb");
        transports.add("wifi");
#else
        response["transport"] = "wifi";
        response["transport_switching"] = false;
#endif
        response["ssid"] = stream.ssid();
        response["wifi_configured"] = stream.hasCredentials();
        response["wifi"] = wifiStatusName();
        response["ip"] = stream.ipAddress();
        response["rssi"] = stream.rssi();
        response["hostname"] = stream.hostname();
        response["port"] = stream.port();
        response["tcp_client"] = stream.clientConnected();
        response["streaming"] = g_state.mode == AppMode::STREAM;
    } else if (strcmp(command, "scan_wifi") == 0) {
        int count = WiFi.scanNetworks();
        JsonArray networks = response.createNestedArray("networks");
        for (int i = 0; i < count; ++i) {
            JsonObject network = networks.createNestedObject();
            network["ssid"] = WiFi.SSID(i);
            network["rssi"] = WiFi.RSSI(i);
            network["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
        }
        WiFi.scanDelete();
        response["ok"] = count >= 0;
        if (count < 0) response["error"] = "scan_failed";
    } else if (strcmp(command, "set_wifi") == 0) {
        String ssid = request["ssid"] | "";
        String password = request["password"] | "";
        if (ssid.isEmpty() || ssid.length() > 32 || password.length() > 63) {
            response["ok"] = false;
            response["error"] = "invalid_wifi_credentials";
        } else {
            network_preferences.begin("ker-net", false);
            network_preferences.putString("ssid", ssid);
            network_preferences.putString("password", password);
#if defined(USE_RUNTIME)
            String requested_transport = network_preferences.getString("transport", "usb");
#endif
            network_preferences.end();
            stream.setCredentials(ssid, password);
#if defined(USE_RUNTIME)
            if (requested_transport == "wifi") {
                g_state.mode = AppMode::STANDBY;
                if (dataQueue != nullptr) xQueueReset(dataQueue);
                stream.switchTo(TransportMode::WIFI);
            }
#endif
            response["ok"] = true;
            response["ssid"] = ssid;
#if defined(USE_RUNTIME)
            response["transport"] = stream.modeName();
            response["switching"] = stream.transportSwitching();
            response["state"] = requested_transport == "wifi"
                ? "connecting" : "configured";
#else
            response["state"] = "connecting";
#endif
        }
    } else if (strcmp(command, "clear_wifi") == 0) {
        network_preferences.begin("ker-net", false);
        network_preferences.remove("ssid");
        network_preferences.remove("password");
#if defined(USE_RUNTIME)
        network_preferences.putString("transport", "usb");
#endif
        network_preferences.end();
        stream.clearCredentials();
        response["ok"] = true;
#if defined(USE_RUNTIME)
        response["transport"] = stream.modeName();
#endif
    } else if (strcmp(command, "set_transport") == 0) {
#if defined(USE_RUNTIME)
        String requested_transport = request["transport"] | "";
        TransportMode requested_mode;
        if (requested_transport == "usb") {
            requested_mode = TransportMode::USB;
        } else if (requested_transport == "wifi") {
            requested_mode = TransportMode::WIFI;
        } else {
            response["ok"] = false;
            response["error"] = "invalid_transport";
            sendConfigResponse(response);
            return;
        }

        g_state.mode = AppMode::STANDBY;
        if (dataQueue != nullptr) xQueueReset(dataQueue);
        if (!stream.switchTo(requested_mode)) {
            // Remember the user's requested mode. Saving credentials through
            // CDC will complete the switch without requiring a second click.
            network_preferences.begin("ker-net", false);
            network_preferences.putString("transport", requested_transport);
            network_preferences.end();
            response["ok"] = true;
            response["transport"] = stream.modeName();
            response["requested_transport"] = requested_transport;
            response["switching"] = false;
            response["state"] = "awaiting_credentials";
        } else {
            network_preferences.begin("ker-net", false);
            network_preferences.putString("transport", stream.modeName());
            network_preferences.end();
            response["ok"] = true;
            response["transport"] = stream.modeName();
            response["switching"] = stream.transportSwitching();
        }
#else
        response["ok"] = false;
        response["error"] = "runtime_transport_unavailable";
#endif
    } else if (strcmp(command, "get_sensors") == 0) {
        SensorSnapshot snapshot = {};
        bool available = xQueuePeek(guiQueue, &snapshot, 0) == pdTRUE;
        response["ok"] = available;
        if (!available) {
            response["error"] = "sensor_data_unavailable";
        } else {
            JsonArray angles = response.createNestedArray("angles");
            JsonArray raw_angles = response.createNestedArray("raw_angles");
            JsonArray errors = response.createNestedArray("errors");
            for (int i = 0; i < NUM_SENSORS; ++i) {
                angles.add(snapshot.sensors[i].angle);
                raw_angles.add(snapshot.sensors[i].raw_deg);
                errors.add(snapshot.sensors[i].error);
            }
        }
    } else if (strcmp(command, "calibrate") == 0) {
        uint16_t mask = request["mask"] | static_cast<uint16_t>(0xFFFF);
        SensorSnapshot snapshot = {};
        uint16_t invalid_mask = 0;
        bool available = xQueuePeek(guiQueue, &snapshot, 0) == pdTRUE;
        if (available) {
            for (int i = 0; i < NUM_SENSORS; ++i) {
                if (((mask >> i) & 1u) && snapshot.sensors[i].error) {
                    invalid_mask |= static_cast<uint16_t>(1u << i);
                }
            }
        }
        if (!available || mask == 0 || invalid_mask != 0) {
            response["ok"] = false;
            response["error"] = !available ? "sensor_data_unavailable" :
                                (mask == 0 ? "empty_calibration_mask" : "sensor_error");
            response["invalid_mask"] = invalid_mask;
        } else {
            g_state.zero_mask = mask;
            response["ok"] = true;
            response["mask"] = mask;
        }
    } else if (strcmp(command, "reboot") == 0) {
        response["ok"] = true;
        sendConfigResponse(response);
        KER_CONFIG_SERIAL.flush();
        delay(100);
        ESP.restart();
        return;
    } else {
        response["ok"] = false;
        response["error"] = "unknown_command";
    }

    sendConfigResponse(response);
}

static void serviceConfigConsole() {
    while (KER_CONFIG_SERIAL.available() > 0) {
        char value = static_cast<char>(KER_CONFIG_SERIAL.read());
        if (value == '\n') {
            config_line.trim();
            if (!config_line.isEmpty()) handleConfigCommand(config_line);
            config_line = "";
        } else if (value != '\r') {
            if (config_line.length() < 1024) config_line += value;
            else config_line = "";
        }
    }
}
#endif

// =====================================================
// Core 0: Sensor task
// I2C peripheral polling task.
// To add a new sensor (e.g. lifter, IMU):
//   1. Initialize the device in this task before the loop
//   2. Read the device in the loop and store to shared variables
//   3. Add the shared variable to SensorSnapshot in Common.h
//   4. Set the value in acquisitionTask() snapshot
//   5. Register the field with stream.add() in setup()
// =====================================================
void sensorTask(void* pvParameters) {
    const uint32_t COLOR_IDLE  = 0x001900;
    const uint32_t COLOR_PRESS = 0x000019;

    if (scroll.begin(&Wire, SCROLL_ADDR, I2C_SDA_PIN, I2C_SCL_PIN, 100000U)) {
        encoder_found = true;
        scroll.resetEncoder();
        scroll.setLEDColor(COLOR_IDLE);
    }

    for (;;) {
        if (encoder_found) {
            // shared_encoder_value = scroll.getEncoderValue();  // unused

            bool btn = scroll.getButtonStatus();
            static bool prev_btn = false;
            if (btn != prev_btn) {
                prev_btn = btn;
                scroll.setLEDColor(btn ? COLOR_PRESS : COLOR_IDLE);
            }
            // shared_encoder_button = btn;  // unused
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// =====================================================
// Core 0: Acquisition task
// =====================================================
void acquisitionTask(void* pvParameters) {
    rs485nexus.begin(RS485_BAUD, RS485_RX, RS485_TX);

    SensorSnapshot snapshot;
    uint32_t last_request_time = millis();
    uint8_t next_sensor_id = 1;

    for (;;) {
        snapshot.timestamp = micros();

        rs485nexus.readPacket();
        uint32_t now = millis();

        if (now - last_request_time >= 1) {
            last_request_time += 1;
            rs485nexus.requestPacket(next_sensor_id, 1);
            next_sensor_id = next_sensor_id >= NUM_SENSORS ? 1 : next_sensor_id + 1;
        }

        for (int i = 0; i < NUM_SENSORS; i++) {
            float   raw_deg = rs485nexus.getAngleDeg(i + 1);
            bool    valid   = rs485nexus.isValid(i + 1);


            if (valid) {
                snapshot.sensors[i].raw_deg = raw_deg;  // Store raw degree for debugging/GUI
                float angle = angle_processors[i].process(raw_deg, g_state.jig_angle_offsets[i], true);

                // Apply joint-specific mechanical offset
                angle += ENCODER_CONFIG[i].mech_joint_offset;

                // Avoid +/-360 jump at initialization when sensor and expected position are on opposite sides of 0/360 boundary
                if(!ENCODER_CONFIG[i].skip_jump_detect && angle > ENCODER_CONFIG[i].mech_max + MARGIN_DEG){
                    angle -= 360.0f;
                }
                else if (!ENCODER_CONFIG[i].skip_jump_detect && angle < ENCODER_CONFIG[i].mech_min - MARGIN_DEG)
                {
                    angle += 360.0f;
                }

                snapshot.sensors[i].angle = angle;
                snapshot.sensors[i].error = false;

            } else {
                snapshot.sensors[i].angle = angle_processors[i].getCumulative();
                snapshot.sensors[i].error = true;
            }
        }

        // snapshot.encoder_value  = shared_encoder_value;   // unused
        // snapshot.encoder_button = shared_encoder_button;  // unused

        // save jig zero offsets if requested from GUI
        if (g_state.zero_all) {
            g_state.zero_mask = (1u << NUM_SENSORS) - 1;
            g_state.zero_all  = false;
        }
        if (g_state.zero_mask > 0) {
            for (int i = 0; i < NUM_SENSORS; i++) {
                if ((g_state.zero_mask >> i) & 1) {
                    float save_val = snapshot.sensors[i].raw_deg;
                    if (ENCODER_CONFIG[i].invert)
                        save_val = 360.0f - fmod(save_val, 360.0f);
                    g_state.jig_angle_offsets[i] = save_val;
                    angle_processors[i].reset();
                }
            }
            preferences.begin("ker-cal", false);
            preferences.putBytes("offsets", g_state.jig_angle_offsets, sizeof(g_state.jig_angle_offsets));
            preferences.end();
            g_state.zero_mask = 0;
        }

        if (g_state.mode == AppMode::STREAM) {
            xQueueOverwrite(dataQueue, &snapshot);
        }
        xQueueOverwrite(guiQueue, &snapshot);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// =====================================================
// Core 1: GUI task
// The live sensor GUI remains visible in both STANDBY and STREAM modes.
// During STREAM, only the bottom-right STOP button accepts input.
// =====================================================
void guiTask(void* pvParameters) {
    static SensorSnapshot snapshot;

    for (;;) {
        M5.update();

        AppMode current_mode = g_state.mode;

        xQueuePeek(guiQueue, &snapshot, 0);
        #if defined(USE_RUNTIME)
            if (stream.mode() == TransportMode::WIFI) {
                String transport_status = String("WIFI ") + stream.ipAddress();
                transport_status += stream.clientConnected() ? " C" : " --";
                gui.setTransportStatus(transport_status);
            } else {
                gui.setTransportStatus(stream.mounted() ? "USB CONNECTED" : "USB WAIT");
            }
        #elif defined(USE_WIFI)
            String transport_status = String("WIFI ") + stream.ipAddress();
            transport_status += stream.clientConnected() ? " C" : " --";
            gui.setTransportStatus(transport_status);
        #elif defined(USE_USB)
            gui.setTransportStatus(stream.mounted() ? "USB CONNECTED" : "USB WAIT");
        #else
            gui.setTransportStatus("SERIAL");
        #endif
        GUICommand cmd = gui.tick(snapshot, current_mode);

        switch (cmd.type) {
            case GUICommand::Type::START:
                g_state.mode = AppMode::STREAM;
                break;
            case GUICommand::Type::STOP:
                g_state.mode = AppMode::STANDBY;
                break;
            case GUICommand::Type::ZERO_ALL:
                g_state.zero_all = true;
                break;
            case GUICommand::Type::ZERO_MASK:
                g_state.zero_mask = cmd.mask;
                break;
            default:
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(33));  // 30fps
    }
}

// =====================================================
// Setup
// =====================================================
void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Speaker.end();

    // Load offsets
    preferences.begin("ker-cal", true);
    preferences.getBytes("offsets", g_state.jig_angle_offsets, sizeof(g_state.jig_angle_offsets));
    preferences.end();

    // GUI init
    gui.init();

    for (int i = 0; i < NUM_SENSORS; i++) {
        angle_processors[i] = AngleProcessor(
            ENCODER_CONFIG[i].invert,
            ENCODER_CONFIG[i].mech_min,
            ENCODER_CONFIG[i].mech_max
        );
    }

    // Transport-independent KER schema
    stream.add("timestamp",      Type::UINT32);
    stream.add("angles",         Type::FLOAT, NUM_SENSORS);
    stream.add("errors",         Type::BOOL,  NUM_SENSORS);
    // stream.add("encoder_value",  Type::INT16);  // unused
    // stream.add("encoder_button", Type::UINT8);  // unused

    stream.onCommand([](const uint8_t* buf, size_t len) -> bool {
        switch (buf[0]) {
            case 0x00:  // PING
                g_state.mode = AppMode::STANDBY;
                g_state.ping_requested = true;
                return true;
            case 0x01: g_state.mode     = AppMode::STANDBY; return true;
            case 0x02: g_state.mode     = AppMode::STREAM;  return true;
            case 0x03: g_state.zero_all = true;             return true;
            case 0x04:
                if (len >= 3)
                    g_state.zero_mask = (buf[1] << 8) | buf[2];
                return true;
            default: return false;
        }
    });

    #if defined(USE_RUNTIME)
        config_serial.begin(115200);
        network_preferences.begin("ker-net", true);
        String wifi_ssid = network_preferences.getString("ssid", "");
        String wifi_password = network_preferences.getString("password", "");
        String saved_transport = network_preferences.getString("transport", "usb");
        network_preferences.end();
        TransportMode initial_transport =
            saved_transport == "wifi" ? TransportMode::WIFI : TransportMode::USB;
        stream.begin(wifi_ssid, wifi_password, initial_transport);
    #elif defined(USE_USB)
        stream.begin();
    #elif defined(USE_WIFI)
        Serial.begin(115200);
        network_preferences.begin("ker-net", true);
        String wifi_ssid = network_preferences.getString("ssid", "");
        String wifi_password = network_preferences.getString("password", "");
        network_preferences.end();
        stream.begin(wifi_ssid, wifi_password);
    #else
        Serial.begin(2000000);
        stream.begin(2000000);
    #endif

    // Start in STANDBY mode
    g_state.mode = AppMode::STANDBY;

    // Queues
    dataQueue = xQueueCreate(1, sizeof(SensorSnapshot));
    guiQueue  = xQueueCreate(1, sizeof(SensorSnapshot));

    delay(500);

    // Tasks
    xTaskCreatePinnedToCore(acquisitionTask, "Acquisition", 8192, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(sensorTask,      "Sensor",      4096, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(guiTask,         "GUI",         4096, NULL, 1, NULL, 1);
}

// =====================================================
// Loop (Core 1) - transport command handling & stream.send()
// =====================================================
void loop() {
    static SensorSnapshot snapshot;
    uint8_t               cmd_buf[64];

    stream.recv(cmd_buf, sizeof(cmd_buf));
    #if defined(USE_RUNTIME) || defined(USE_WIFI)
        serviceConfigConsole();
    #endif

    #if defined(USE_RUNTIME)
        if (stream.consumeFallback()) {
            g_state.mode = AppMode::STANDBY;
            if (dataQueue != nullptr) xQueueReset(dataQueue);
            network_preferences.begin("ker-net", false);
            network_preferences.putString("transport", "usb");
            network_preferences.end();
        }
    #endif

    if (g_state.ping_requested) {
        g_state.ping_requested = false;
        xQueuePeek(dataQueue, &snapshot, 0);
        stream.sendPingResponse(snapshot, ACTIVE_FW_VERSION, HW_VERSION, LAST_UPDATED);
    }

    if (g_state.mode == AppMode::STANDBY) {
        vTaskDelay(pdMS_TO_TICKS(10));
    } else {
        #if defined(USE_RUNTIME)
            if (stream.mode() == TransportMode::WIFI && !stream.clientConnected()) {
                vTaskDelay(pdMS_TO_TICKS(10));
                return;
            }
        #elif defined(USE_WIFI)
            // STREAM is a user-selected mode. Keep it active while waiting for
            // the first client or for a disconnected client to reconnect.
            if (!stream.clientConnected()) {
                vTaskDelay(pdMS_TO_TICKS(10));
                return;
            }
        #endif

        if (xQueueReceive(dataQueue, &snapshot, 0) == pdTRUE) {
            float angles[NUM_SENSORS];
            bool  errors[NUM_SENSORS];
            for (int i = 0; i < NUM_SENSORS; i++) {
                angles[i] = snapshot.sensors[i].angle;
                errors[i] = snapshot.sensors[i].error;
            }

            stream.set("timestamp",      snapshot.timestamp);
            stream.set("angles",         angles,             NUM_SENSORS);
            stream.set("errors",         errors,             NUM_SENSORS);
            // stream.set("encoder_value",  snapshot.encoder_value);   // unused
            // stream.set("encoder_button", snapshot.encoder_button);  // unused
            stream.send();
        } else {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
}
