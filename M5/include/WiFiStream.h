// Copyright 2026 Chengdu Changshu Robot Co., Ltd.
// Licensed under the Apache License, Version 2.0.

#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "Common.h"
#include "Meta.h"
#include "StreamTypes.h"

class WiFiStream {
public:
    static const size_t MAX_FIELDS = 32;
    static const size_t MAX_BUF = 512;
    // Application-level UDP lease.  Heartbeats are intentionally tolerant of
    // weak Wi-Fi; the PC side sends one heartbeat per second.
    static constexpr uint32_t HEARTBEAT_TIMEOUT_MS = 30000;

    WiFiStream();

    size_t getBinTotal() const { return _bin_total; }
    bool add(const char* key, Type type, size_t count = 1);
    void begin(const String& ssid, const String& password, bool enabled = true);
    void setEnabled(bool enabled);
    bool enabled() const { return _enabled; }
    void setCredentials(const String& ssid, const String& password);
    void clearCredentials();
    bool mounted();
    bool hasError() const { return _error; }

    void set(const char* key, uint32_t value);
    void set(const char* key, uint16_t value);
    void set(const char* key, uint8_t value);
    void set(const char* key, int32_t value);
    void set(const char* key, int16_t value);
    void set(const char* key, float value);
    void set(const char* key, bool value);
    void set(const char* key, const uint32_t* array, size_t len);
    void set(const char* key, const uint16_t* array, size_t len);
    void set(const char* key, const int32_t* array, size_t len);
    void set(const char* key, const int16_t* array, size_t len);
    void set(const char* key, const float* array, size_t len);
    void set(const char* key, const bool* array, size_t len);

    bool send();
    size_t recv(uint8_t* buf, size_t len);
    void onCommand(CommandCallback cb);
    void sendPingResponse(const SensorSnapshot& snapshot,
                          const char* fw, const char* hw,
                          const char* updated);

    String ipAddress();
    const char* hostname() const { return KER_WIFI_HOSTNAME; }
    uint16_t port() const { return KER_WIFI_PORT; }
    int32_t rssi();
    bool wifiConnected();
    bool clientConnected();
    bool hasCredentials() const { return !_ssid.isEmpty(); }
    const String& ssid() const { return _ssid; }

private:
    static const size_t COMMAND_FRAME_SIZE = 6;
    static const size_t COMMAND_BUFFER_SIZE = 128;

    WiFiUDP _udp;
    bool _server_started;
    bool _mdns_started;
    bool _enabled;
    bool _error;
    uint32_t _last_wifi_attempt_ms;
    uint32_t _last_heartbeat_ms;
    IPAddress _remote_ip;
    uint16_t _remote_port;
    IPAddress _command_ip;
    uint16_t _command_port;
    bool _session_active;
    CommandCallback _on_command;

    FieldDef _fields[MAX_FIELDS];
    size_t _field_count;
    size_t _bin_total;
    uint8_t _bin_buf[MAX_BUF];
    uint8_t _command_buf[COMMAND_BUFFER_SIZE];
    size_t _command_size;
    String _ssid;
    String _password;

    void maintainNetwork();
    void resetNetworkServices();
    void resetSession();
    void parseCommands();
    bool sendDatagram(const uint8_t* data, size_t len);
    FieldDef* _find(const char* key);
    uint8_t _checksum() const;
    static size_t _typeSize(Type type);
};
