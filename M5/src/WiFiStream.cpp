// Copyright 2026 原初智能机器人有限公司
// Licensed under the Apache License, Version 2.0.

#include "WiFiStream.h"

#include <ESPmDNS.h>

WiFiStream::WiFiStream()
    : _server_started(false)
    , _mdns_started(false)
    , _enabled(false)
    , _error(false)
    , _last_wifi_attempt_ms(0)
    , _last_heartbeat_ms(0)
    , _remote_ip(0, 0, 0, 0)
    , _remote_port(0)
    , _command_ip(0, 0, 0, 0)
    , _command_port(0)
    , _session_active(false)
    , _on_command(nullptr)
    , _field_count(0)
    , _bin_total(0)
    , _command_size(0)
{
    memset(_bin_buf, 0, sizeof(_bin_buf));
    memset(_command_buf, 0, sizeof(_command_buf));
}

bool WiFiStream::add(const char* key, Type type, size_t count) {
    if (_field_count >= MAX_FIELDS) { _error = true; return false; }
    size_t required = _bin_total + _typeSize(type) * count;
    if (required > MAX_BUF) { _error = true; return false; }
    FieldDef& field = _fields[_field_count++];
    field.key = key;
    field.type = type;
    field.count = count;
    field.offset = _bin_total;
    _bin_total = required;
    return true;
}

void WiFiStream::begin(const String& ssid, const String& password, bool enabled) {
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setHostname(KER_WIFI_HOSTNAME);
    _ssid = ssid;
    _password = password;
    setEnabled(enabled);
}

void WiFiStream::setEnabled(bool enabled) {
    if (_enabled == enabled) return;
    resetNetworkServices();
    _enabled = enabled;
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setHostname(KER_WIFI_HOSTNAME);
    WiFi.disconnect(false);
    if (_enabled && !_ssid.isEmpty()) {
        WiFi.begin(_ssid.c_str(), _password.c_str());
    }
    _last_wifi_attempt_ms = millis();
}

void WiFiStream::setCredentials(const String& ssid, const String& password) {
    resetNetworkServices();
    WiFi.disconnect(false);
    _ssid = ssid;
    _password = password;
    if (_enabled && !_ssid.isEmpty()) {
        WiFi.begin(_ssid.c_str(), _password.c_str());
    }
    _last_wifi_attempt_ms = millis();
}

void WiFiStream::clearCredentials() {
    resetNetworkServices();
    WiFi.disconnect(false);
    _ssid = "";
    _password = "";
}

void WiFiStream::resetSession() {
    _command_size = 0;
    _session_active = false;
    _remote_port = 0;
    _command_port = 0;
}

void WiFiStream::resetNetworkServices() {
    resetSession();
    if (_server_started) {
        _udp.stop();
        _server_started = false;
    }
    if (_mdns_started) {
        MDNS.end();
        _mdns_started = false;
    }
}

void WiFiStream::maintainNetwork() {
    if (!_enabled) return;
    if (WiFi.status() != WL_CONNECTED) {
        resetNetworkServices();
        if (_ssid.isEmpty()) return;
        if (millis() - _last_wifi_attempt_ms >= 5000) {
            WiFi.disconnect();
            WiFi.begin(_ssid.c_str(), _password.c_str());
            _last_wifi_attempt_ms = millis();
        }
        return;
    }

    // UDP has no connection to accept.  Bind once to the KER port so command
    // datagrams (PING/START/heartbeat) can be received from any PC.
    if (!_server_started) {
        _udp.begin(KER_WIFI_PORT);
        _server_started = true;
    }
    if (!_mdns_started) {
        _mdns_started = MDNS.begin(KER_WIFI_HOSTNAME);
        if (_mdns_started) {
            MDNS.addService("openarm-ker", "udp", KER_WIFI_PORT);
        }
    }

    if (_session_active && millis() - _last_heartbeat_ms > HEARTBEAT_TIMEOUT_MS) {
        // A long, configurable grace period avoids false stops on weak Wi-Fi,
        // while still revoking an abandoned PC's stream authorization.
        uint8_t standby[1] = {0x01};
        if (_on_command) _on_command(standby, 1);
        _session_active = false;
        _remote_port = 0;
    }
}

bool WiFiStream::mounted() {
    maintainNetwork();
    return clientConnected();
}

bool WiFiStream::wifiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool WiFiStream::clientConnected() {
    return _session_active && _remote_port != 0 && wifiConnected();
}

String WiFiStream::ipAddress() {
    return wifiConnected() ? WiFi.localIP().toString() : String("--");
}

int32_t WiFiStream::rssi() {
    return wifiConnected() ? WiFi.RSSI() : 0;
}

void WiFiStream::onCommand(CommandCallback cb) {
    _on_command = cb;
}

FieldDef* WiFiStream::_find(const char* key) {
    for (size_t i = 0; i < _field_count; ++i) {
        if (strcmp(_fields[i].key, key) == 0) return &_fields[i];
    }
    return nullptr;
}

void WiFiStream::set(const char* key, uint32_t value) {
    FieldDef* field = _find(key); if (field) memcpy(_bin_buf + field->offset, &value, 4);
}
void WiFiStream::set(const char* key, uint16_t value) {
    FieldDef* field = _find(key); if (field) memcpy(_bin_buf + field->offset, &value, 2);
}
void WiFiStream::set(const char* key, uint8_t value) {
    FieldDef* field = _find(key); if (field) _bin_buf[field->offset] = value;
}
void WiFiStream::set(const char* key, int32_t value) {
    FieldDef* field = _find(key); if (field) memcpy(_bin_buf + field->offset, &value, 4);
}
void WiFiStream::set(const char* key, int16_t value) {
    FieldDef* field = _find(key); if (field) memcpy(_bin_buf + field->offset, &value, 2);
}
void WiFiStream::set(const char* key, float value) {
    FieldDef* field = _find(key); if (field) memcpy(_bin_buf + field->offset, &value, 4);
}
void WiFiStream::set(const char* key, bool value) {
    FieldDef* field = _find(key); if (field) _bin_buf[field->offset] = value ? 1 : 0;
}
void WiFiStream::set(const char* key, const uint32_t* array, size_t len) {
    FieldDef* field = _find(key); if (field) memcpy(_bin_buf + field->offset, array, 4 * len);
}
void WiFiStream::set(const char* key, const uint16_t* array, size_t len) {
    FieldDef* field = _find(key); if (field) memcpy(_bin_buf + field->offset, array, 2 * len);
}
void WiFiStream::set(const char* key, const int32_t* array, size_t len) {
    FieldDef* field = _find(key); if (field) memcpy(_bin_buf + field->offset, array, 4 * len);
}
void WiFiStream::set(const char* key, const int16_t* array, size_t len) {
    FieldDef* field = _find(key); if (field) memcpy(_bin_buf + field->offset, array, 2 * len);
}
void WiFiStream::set(const char* key, const float* array, size_t len) {
    FieldDef* field = _find(key); if (field) memcpy(_bin_buf + field->offset, array, 4 * len);
}
void WiFiStream::set(const char* key, const bool* array, size_t len) {
    FieldDef* field = _find(key);
    if (!field) return;
    for (size_t i = 0; i < len; ++i) _bin_buf[field->offset + i] = array[i] ? 1 : 0;
}

uint8_t WiFiStream::_checksum() const {
    uint8_t checksum = 0;
    for (size_t i = 0; i < _bin_total; ++i) checksum ^= _bin_buf[i];
    return checksum;
}

bool WiFiStream::sendDatagram(const uint8_t* data, size_t len) {
    if (!clientConnected() || len == 0) return false;
    if (!_udp.beginPacket(_remote_ip, _remote_port)) return false;
    size_t written = _udp.write(data, len);
    return written == len && _udp.endPacket() == 1;
}

bool WiFiStream::send() {
    maintainNetwork();
    if (_error || !clientConnected()) return false;

    uint8_t out[MAX_BUF + 3];
    out[0] = 0xA5;
    out[1] = 0x5A;
    memcpy(out + 2, _bin_buf, _bin_total);
    out[_bin_total + 2] = _checksum();

    const size_t frame_size = _bin_total + 3;

    return sendDatagram(out, frame_size);
}

void WiFiStream::parseCommands() {
    while (_command_size >= 2) {
        size_t header = 0;
        while (header + 1 < _command_size &&
               !(_command_buf[header] == 0xA5 && _command_buf[header + 1] == 0x43)) {
            ++header;
        }
        if (header > 0) {
            memmove(_command_buf, _command_buf + header, _command_size - header);
            _command_size -= header;
        }
        if (_command_size < COMMAND_FRAME_SIZE) return;

        uint8_t checksum = _command_buf[2] ^ _command_buf[3] ^ _command_buf[4];
        if (checksum != _command_buf[5]) {
            memmove(_command_buf, _command_buf + 1, --_command_size);
            continue;
        }
        const uint8_t command = _command_buf[2];
        const bool peer_matches = _session_active &&
            _command_ip == _remote_ip && _command_port == _remote_port;

        // PING is the explicit UDP handshake.  It selects the destination
        // endpoint, refreshes its lease, and lets main.cpp return the schema.
        // A reconnecting PC simply sends PING again from its new UDP port.
        if (command == 0x00) {
            _remote_ip = _command_ip;
            _remote_port = _command_port;
            _session_active = true;
            _last_heartbeat_ms = millis();
            if (_on_command) _on_command(_command_buf + 2, 3);
        } else if (peer_matches) {
            // Heartbeats prove the selected PC is still present.  They do
            // not change the streaming state, whereas START/STOP and the
            // existing calibration commands retain their old semantics.
            _last_heartbeat_ms = millis();
            if (command != 0x05 && _on_command) {
                _on_command(_command_buf + 2, 3);
            }
        }
        memmove(_command_buf, _command_buf + COMMAND_FRAME_SIZE,
                _command_size - COMMAND_FRAME_SIZE);
        _command_size -= COMMAND_FRAME_SIZE;
    }
}

size_t WiFiStream::recv(uint8_t* /*buf*/, size_t /*len*/) {
    maintainNetwork();
    size_t received = 0;
    int packet_size;
    while ((packet_size = _udp.parsePacket()) > 0) {
        if (packet_size > static_cast<int>(COMMAND_BUFFER_SIZE)) {
            uint8_t discard[COMMAND_BUFFER_SIZE];
            _udp.read(discard, sizeof(discard));
            continue;
        }
        _command_ip = _udp.remoteIP();
        _command_port = _udp.remotePort();
        _command_size = _udp.read(_command_buf, COMMAND_BUFFER_SIZE);
        received += _command_size;
        parseCommands();
    }
    return received;
}

void WiFiStream::sendPingResponse(const SensorSnapshot& snapshot,
                                  const char* fw, const char* hw,
                                  const char* updated) {
    maintainNetwork();
    if (!clientConnected()) return;
    static constexpr size_t PING_BUFFER_SIZE =
        2 + 16 + 16 + 12 + 1 + MAX_FIELDS * 18 + NUM_SENSORS * 5;
    uint8_t out[PING_BUFFER_SIZE] = {};
    size_t pos = 0;
    out[pos++] = 0xA5;
    out[pos++] = 0x50;
    strncpy(reinterpret_cast<char*>(out) + pos, fw, 16); pos += 16;
    strncpy(reinterpret_cast<char*>(out) + pos, hw, 16); pos += 16;
    strncpy(reinterpret_cast<char*>(out) + pos, updated, 12); pos += 12;
    out[pos++] = static_cast<uint8_t>(_field_count);
    for (size_t i = 0; i < _field_count; ++i) {
        strncpy(reinterpret_cast<char*>(out) + pos, _fields[i].key, 16); pos += 16;
        out[pos++] = static_cast<uint8_t>(_fields[i].type);
        out[pos++] = static_cast<uint8_t>(_fields[i].count);
    }
    for (int i = 0; i < NUM_SENSORS; ++i) {
        memcpy(out + pos, &snapshot.sensors[i].angle, 4); pos += 4;
        out[pos++] = snapshot.sensors[i].error ? 0 : 1;
    }
    sendDatagram(out, pos);
}

size_t WiFiStream::_typeSize(Type type) {
    switch (type) {
        case Type::UINT32: return 4;
        case Type::UINT16: return 2;
        case Type::UINT8: return 1;
        case Type::INT32: return 4;
        case Type::INT16: return 2;
        case Type::FLOAT: return 4;
        case Type::BOOL: return 1;
        default: return 0;
    }
}
