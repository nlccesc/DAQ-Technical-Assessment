// errorhandling.cpp

#include "catch.hpp"
#include <vector>
#include <string>
#include <cstdint>

class LogValidator {
public:
    enum class ErrorType {
        VALID = 0,
        EMPTY_LINE = 1,
        MALFORMED_FORMAT = 2,
        INVALID_TIMESTAMP = 3,
        INVALID_INTERFACE = 4,
        INVALID_CAN_ID = 5,
        INVALID_DATA = 6,
        DATA_TOO_LONG = 7
    };
    
    static ErrorType validateLogLine(const std::string& line) {
        if (line.empty()) {
            return ErrorType::EMPTY_LINE;
        }
        
        if (line[0] != '(') {
            return ErrorType::MALFORMED_FORMAT;
        }
        
        size_t closeParenPos = line.find(')');
        if (closeParenPos == std::string::npos) {
            return ErrorType::MALFORMED_FORMAT;
        }
        
        if (closeParenPos + 1 >= line.length() || 
            line[closeParenPos + 1] != ' ') {
            return ErrorType::MALFORMED_FORMAT;
        }
        
        size_t spacePos = line.find(' ', closeParenPos + 2);
        if (spacePos == std::string::npos) {
            return ErrorType::MALFORMED_FORMAT;
        }
        
        size_t hashPos = line.find('#', spacePos);
        if (hashPos == std::string::npos) {
            return ErrorType::MALFORMED_FORMAT;
        }
        
        // validate timestamp
        std::string timestampStr = line.substr(1, closeParenPos - 1);
        if (timestampStr.empty()) return ErrorType::INVALID_TIMESTAMP;
        
        for (char c : timestampStr) {
            if (!((c >= '0' && c <= '9') || c == '.' || c == '-')) {
                return ErrorType::INVALID_TIMESTAMP;
            }
        }
        
        try {
            double timestamp = std::stod(timestampStr);
            if (timestamp <= 0.0 || timestamp > 2147483647.999) {
                return ErrorType::INVALID_TIMESTAMP;
            }
        } catch (...) {
            return ErrorType::INVALID_TIMESTAMP;
        }
        
        // validate interface
        std::string interface = line.substr(closeParenPos + 2, 
                                            spacePos - closeParenPos - 2);
        if (interface != "can0" && 
            interface != "can1" && 
            interface != "can2") {
            return ErrorType::INVALID_INTERFACE;
        }
        
        // validate CAN ID
        std::string canIdStr = line.substr(spacePos + 1, hashPos - spacePos - 1);
        if (canIdStr.empty()) {
            return ErrorType::INVALID_CAN_ID;
        }
        
        for (char c : canIdStr) {
            if (!((c >= '0' && c <= '9') || 
                (c >= 'A' && c <= 'F') || 
                (c >= 'a' && c <= 'f'))) {
                return ErrorType::INVALID_CAN_ID;
            }
        }
        
        try {
            uint32_t canId = std::stoul(canIdStr, nullptr, 16);
            if (canId > 0x1FFFFFFF) {
                return ErrorType::INVALID_CAN_ID;
            }
        } catch (...) {
            return ErrorType::INVALID_CAN_ID;
        }
        
        // validate data
        std::string dataStr = line.substr(hashPos + 1);
        
        // check data length (max 8 bytes = 16 hex chars)
        if (dataStr.length() > 16) {
            return ErrorType::DATA_TOO_LONG;
        }
        
        // check if length is even for hex pairs
        if (dataStr.length() % 2 != 0) {
            return ErrorType::INVALID_DATA;
        }
        
        // check hex validity
        for (char c : dataStr) {
            if (!((c >= '0' && c <= '9') || 
                  (c >= 'A' && c <= 'F') || 
                  (c >= 'a' && c <= 'f'))) {
                return ErrorType::INVALID_DATA;
            }
        }
        
        return ErrorType::VALID;
    }
};

TEST_CASE("Valid log entries", "[errorhandling]") {
    SECTION("standard valid formats") {
        std::vector<std::string> validLines = {
            "(1641234567.123) can0 180#DEADBEEF",
            "(1641234567.124) can1 181#CAFE",
            "(1641234567.125) can2 7FF#",
            "(0.000001) can0 123#FF",
            "(1641234567.126) can0 1FFFFFFF#0123456789ABCDEF"
        };
        
        for (const auto& line : validLines) {
            REQUIRE(LogValidator::validateLogLine(line) == LogValidator::ErrorType::VALID);
        }
    }
    
    SECTION("boundary cases") {
        REQUIRE(LogValidator::validateLogLine("(0.000001) can0 0#") == LogValidator::ErrorType::VALID);
        REQUIRE(LogValidator::validateLogLine("(1641234567.999) can2 1FFFFFFF#0123456789ABCDEF") == LogValidator::ErrorType::VALID);
    }
}

TEST_CASE("Format errors", "[errorhandling]") {
    SECTION("basic format issues") {
        REQUIRE(LogValidator::validateLogLine("") == LogValidator::ErrorType::EMPTY_LINE);
        REQUIRE(LogValidator::validateLogLine("1641234567.123 can0 180#DEAD") == LogValidator::ErrorType::MALFORMED_FORMAT);
        REQUIRE(LogValidator::validateLogLine("(1641234567.123 can0 180#DEAD") == LogValidator::ErrorType::MALFORMED_FORMAT);
        REQUIRE(LogValidator::validateLogLine("(1641234567.123)can0 180#DEAD") == LogValidator::ErrorType::MALFORMED_FORMAT);
        REQUIRE(LogValidator::validateLogLine("(1641234567.123) can0 180DEAD") == LogValidator::ErrorType::MALFORMED_FORMAT);
    }
}

TEST_CASE("Timestamp errors", "[errorhandling]") {
    SECTION("invalid timestamps") {
        REQUIRE(LogValidator::validateLogLine("() can0 180#DEAD") == LogValidator::ErrorType::INVALID_TIMESTAMP);
        REQUIRE(LogValidator::validateLogLine("(abc) can0 180#DEAD") == LogValidator::ErrorType::INVALID_TIMESTAMP);
        REQUIRE(LogValidator::validateLogLine("(-1.0) can0 180#DEAD") == LogValidator::ErrorType::INVALID_TIMESTAMP);
        REQUIRE(LogValidator::validateLogLine("(0.0) can0 180#DEAD") == LogValidator::ErrorType::INVALID_TIMESTAMP);
        REQUIRE(LogValidator::validateLogLine("(164123abc7.123) can0 180#DEAD") == LogValidator::ErrorType::INVALID_TIMESTAMP);
    }
}

TEST_CASE("Interface errors", "[errorhandling]") {
    SECTION("invalid interfaces") {
        REQUIRE(LogValidator::validateLogLine("(1641234567.123) can3 180#DEAD") == LogValidator::ErrorType::INVALID_INTERFACE);
        REQUIRE(LogValidator::validateLogLine("(1641234567.123) vcan0 180#DEAD") == LogValidator::ErrorType::INVALID_INTERFACE);
        REQUIRE(LogValidator::validateLogLine("(1641234567.123) eth0 180#DEAD") == LogValidator::ErrorType::INVALID_INTERFACE);
    }
}

TEST_CASE("CAN ID errors", "[errorhandling]") {
    SECTION("invalid CAN IDs") {
        REQUIRE(LogValidator::validateLogLine("(1641234567.123) can0 #DEAD") == LogValidator::ErrorType::INVALID_CAN_ID);
        REQUIRE(LogValidator::validateLogLine("(1641234567.123) can0 XYZ#DEAD") == LogValidator::ErrorType::INVALID_CAN_ID);
        REQUIRE(LogValidator::validateLogLine("(1641234567.123) can0 20000000#DEAD") == LogValidator::ErrorType::INVALID_CAN_ID);
    }
}

TEST_CASE("Data errors", "[errorhandling]") {
    SECTION("invalid data") {
        REQUIRE(LogValidator::validateLogLine("(1641234567.123) can0 180#ABC") == LogValidator::ErrorType::INVALID_DATA);
        REQUIRE(LogValidator::validateLogLine("(1641234567.123) can0 180#XY") == LogValidator::ErrorType::INVALID_DATA);
        REQUIRE(LogValidator::validateLogLine("(1641234567.123) can0 180#123456789ABCDEF01") == LogValidator::ErrorType::DATA_TOO_LONG);
    }
}