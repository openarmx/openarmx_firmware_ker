// Copyright 2026 Chengdu Changshu Robot Co., Ltd.
// Licensed under the Apache License, Version 2.0.

#include "RuntimeStream.h"

RuntimeStream::RuntimeStream()
    : _mode(TransportMode::USB)
    , _wifi_activation_pending(false)
    , _fallback_pending(false)
    , _wifi_activation_started_ms(0)
{
}

bool RuntimeStream::add(const char* key, Type type, size_t count) {
    return _usb.add(key, type, count) && _wifi.add(key, type, count);
}

void RuntimeStream::begin(const String& ssid,
                          const String& password,
                          TransportMode initial_mode) {
    _usb.begin();
    _wifi.begin(ssid, password, false);
    if (initial_mode == TransportMode::WIFI && !_wifi.hasCredentials()) {
        initial_mode = TransportMode::USB;
    }
    switchTo(initial_mode);
}

void RuntimeStream::onCommand(CommandCallback callback) {
    _usb.onCommand(callback);
    _wifi.onCommand(callback);
}

bool RuntimeStream::switchTo(TransportMode mode) {
    if (mode == TransportMode::WIFI && !_wifi.hasCredentials()) {
        return false;
    }

    _fallback_pending = false;
    if (mode == TransportMode::USB) {
        _wifi.setEnabled(false);
        _usb.clearInput();
        _mode = TransportMode::USB;
        _wifi_activation_pending = false;
        return true;
    }

    _usb.clearInput();
    _wifi.setEnabled(true);
    _mode = TransportMode::WIFI;
    _wifi_activation_pending = !_wifi.wifiConnected();
    _wifi_activation_started_ms = millis();
    return true;
}

const char* RuntimeStream::modeName() const {
    return mode() == TransportMode::WIFI ? "wifi" : "usb";
}

bool RuntimeStream::consumeFallback() {
    bool value = _fallback_pending;
    _fallback_pending = false;
    return value;
}

void RuntimeStream::setCredentials(const String& ssid, const String& password) {
    _wifi.setCredentials(ssid, password);
    if (mode() == TransportMode::WIFI) {
        _wifi_activation_pending = true;
        _wifi_activation_started_ms = millis();
    }
}

void RuntimeStream::clearCredentials() {
    _wifi.clearCredentials();
    if (mode() == TransportMode::WIFI) {
        switchTo(TransportMode::USB);
        _fallback_pending = true;
    }
}

bool RuntimeStream::dataClientConnected() {
    return mode() == TransportMode::USB ? _usb.mounted() : _wifi.clientConnected();
}

bool RuntimeStream::mounted() {
    serviceTransport();
    return dataClientConnected();
}

bool RuntimeStream::hasError() const {
    return mode() == TransportMode::USB ? _usb.hasError() : _wifi.hasError();
}

size_t RuntimeStream::recv(uint8_t* buffer, size_t length) {
    size_t received = mode() == TransportMode::USB
        ? _usb.recv(buffer, length)
        : _wifi.recv(buffer, length);
    serviceTransport();
    return received;
}

bool RuntimeStream::send() {
    serviceTransport();
    return mode() == TransportMode::USB ? _usb.send() : _wifi.send();
}

void RuntimeStream::sendPingResponse(const SensorSnapshot& snapshot,
                                     const char* firmware,
                                     const char* hardware,
                                     const char* updated) {
    if (mode() == TransportMode::USB) {
        _usb.sendPingResponse(snapshot, firmware, hardware, updated);
    } else {
        _wifi.sendPingResponse(snapshot, firmware, hardware, updated);
    }
}

void RuntimeStream::serviceTransport() {
    if (mode() != TransportMode::WIFI || !_wifi_activation_pending) return;
    if (_wifi.wifiConnected()) {
        _wifi_activation_pending = false;
        return;
    }
    if (millis() - _wifi_activation_started_ms < WIFI_ACTIVATION_TIMEOUT_MS) return;

    _wifi.setEnabled(false);
    _usb.clearInput();
    _mode = TransportMode::USB;
    _wifi_activation_pending = false;
    _fallback_pending = true;
}

#define RUNTIME_SET_SCALAR(TYPE) \
    void RuntimeStream::set(const char* key, TYPE value) { \
        if (mode() == TransportMode::USB) _usb.set(key, value); \
        else _wifi.set(key, value); \
    }

RUNTIME_SET_SCALAR(uint32_t)
RUNTIME_SET_SCALAR(uint16_t)
RUNTIME_SET_SCALAR(uint8_t)
RUNTIME_SET_SCALAR(int32_t)
RUNTIME_SET_SCALAR(int16_t)
RUNTIME_SET_SCALAR(float)
RUNTIME_SET_SCALAR(bool)

#define RUNTIME_SET_ARRAY(TYPE) \
    void RuntimeStream::set(const char* key, const TYPE* values, size_t length) { \
        if (mode() == TransportMode::USB) _usb.set(key, values, length); \
        else _wifi.set(key, values, length); \
    }

RUNTIME_SET_ARRAY(uint32_t)
RUNTIME_SET_ARRAY(uint16_t)
RUNTIME_SET_ARRAY(int32_t)
RUNTIME_SET_ARRAY(int16_t)
RUNTIME_SET_ARRAY(float)
RUNTIME_SET_ARRAY(bool)
