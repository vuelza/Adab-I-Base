#pragma once
#include <string>
#include "MuaseretDataTypes.hpp"

// CSV dosyasından okuyarak Muaseret senaryo girdilerini hazırlar.
MuaseretScenarioOutput prepareMuaseretInputs(const std::string& recordName);
