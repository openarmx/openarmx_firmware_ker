// Copyright 2026 Chengdu Changshu Robot Co., Ltd.
// Licensed under the Apache License, Version 2.0.

#pragma once

#include <Arduino.h>
#include <atomic>

#include "Common.h"
#include "StreamTypes.h"
#include "USBStream.h"
#include "WiFiStream.h"

enum class TransportMode : uint8_t {
    USB,
    WIFI,
};

class RuntimeStream {
public:
    static constexpr uint32_t WIFI_ACTIVATION_TIMEOUT_MS = 15000;

    RuntimeStream();

    bool add(const char* key, Type type, size_t count = 1);
    void begin(const String& ssid, const String& password, TransportMode initial_mode);
    void onCommand(CommandCallback callback);

    bool switchTo(TransportMode mode);
    TransportMode mode() const { return _mode.load(); }
    const char* modeName() const;
    bool transportSwitching() const { return _wifi_activation_pending; }
    bool consumeFallback();

    void setCredentials(const String& ssid, const String& password);
    void clearCredentials();
    bool hasCredentials() const { return _wifi.hasCredentials(); }
    const String& ssid() const { return _wifi.ssid(); }
    String ipAddress() { return _wifi.ipAddress(); }
    const char* hostname() const { return _wifi.hostname(); }
    uint16_t port() const { return _wifi.port(); }
    int32_t rssi() { return _wifi.rssi(); }
    bool wifiConnected() { return _wifi.wifiConnected(); }
    bool clientConnected() { return _wifi.clientConnected(); }
    bool dataClientConnected();

    bool mounted();
    bool hasError() const;
    size_t recv(uint8_t* buffer, size_t length);
    bool send();
    void sendPingResponse(const SensorSnapshot& snapshot,
                          const char* firmware,
                          const char* hardware,
                          const char* updated);

    void set(const char* key, uint32_t value);
    void set(const char* key, uint16_t value);
    void set(const char* key, uint8_t value);
    void set(const char* key, int32_t value);
    void set(const char* key, int16_t value);
    void set(const char* key, float value);
    void set(const char* key, bool value);
    void set(const char* key, const uint32_t* values, size_t length);
    void set(const char* key, const uint16_t* values, size_t length);
    void set(const char* key, const int32_t* values, size_t length);
    void set(const char* key, const int16_t* values, size_t length);
    void set(const char* key, const float* values, size_t length);
    void set(const char* key, const bool* values, size_t length);

private:
    USBStream _usb;
    WiFiStream _wifi;
    std::atomic<TransportMode> _mode;
    bool _wifi_activation_pending;
    bool _fallback_pending;
    uint32_t _wifi_activation_started_ms;

    void serviceTransport();
};
