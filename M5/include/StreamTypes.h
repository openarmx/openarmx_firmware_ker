// Copyright 2026 Enactic, Inc.
// Copyright 2026 原初智能机器人有限公司
// Licensed under the Apache License, Version 2.0.

#pragma once

#include <Arduino.h>

enum class Type {
    UINT32,
    UINT16,
    UINT8,
    INT32,
    INT16,
    FLOAT,
    BOOL,
};

struct FieldDef {
    const char* key;
    Type        type;
    size_t      count;
    size_t      offset;
};

using CommandCallback = bool (*)(const uint8_t* buf, size_t len);

