#pragma once

namespace TypesCommon {

    enum Status {
        ENUM_STATUS_NA = 0,
        ENUM_STATUS_NO_TRACK = 1,
        ENUM_STATUS_TRACK = 2,
        ENUM_STATUS_COAST = 3,
        ENUM_STATUS_INITIALIZATION = 4,
    };

    enum SystemType {
        ENUM_SYSTEM_TEST = 0,
        ENUM_SYSTEM_AIC = 1,
        ENUM_SYSTEM_SSA = 2,
        ENUM_SYSTEM_KORHAN = 3,
        ENUM_SYSTEM_GOKDENIZ = 4,
        ENUM_SYSTEM_GOKER = 5,
    };

}

