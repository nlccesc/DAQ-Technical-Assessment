// canframecheck.cpp

#include "catch.hpp"
#include <vector>
#include <string>
#include <cstdint>
#include <cstdio>
#include <cstring>

struct CANFrame {
    double timestamp;
    std::string interface;
    uint32_t id;
    std::vector<uint8_t> data;
};

class CanFrameParser {
public:
    static bool parseLine(const std::string& line, CANFrame& frame) {
        if (line.empty()) {
            return false;
        }
        
        char interface[10];
        char id_hex[10];
        char data_hex[20] = "";
        
        int result = sscanf(line.c_str(), "(%lf) %s %[^#]#%s", 
                           &frame.timestamp, interface, id_hex, data_hex);
        
        if (result < 3) {
            return false;
        }
        
        frame.interface = interface;
        
        try {
            frame.id = std::stoul(id_hex, nullptr, 16);
        } catch (...) { return false; }
        
        frame.data.clear();
        for (size_t i = 0; i < strlen(data_hex); i += 2) {
            if (i + 1 >= strlen(data_hex)) break;
            try {
                char byte[3] = {data_hex[i], data_hex[i+1], '\0'};
                frame.data.push_back(std::stoul(byte, nullptr, 16));
            } catch (...) { return false; }
        }
        
        return true;
    }
    
    static bool isValidId(uint32_t id, bool extended = true) {
        return extended ? (id <= 0x1FFFFFFF) : (id <= 0x7FF);
    }
};

TEST_CASE("CAN frame parsing basic", "[canframe]") {
    SECTION("valid standard format") {
        std::string line = "(1641234567.123) can0 180#DEADBEEF";
        CANFrame frame;
        
        REQUIRE(CanFrameParser::parseLine(line, frame));
        REQUIRE(frame.timestamp == 1641234567.123);
        REQUIRE(frame.interface == "can0");
        REQUIRE(frame.id == 0x180);
        REQUIRE(frame.data.size() == 4);
        REQUIRE(frame.data[0] == 0xDE);
        REQUIRE(frame.data[3] == 0xEF);
    }
    
    SECTION("different interfaces") {
        std::vector<std::string> lines = {
            "(1641234567.1) can0 181#AB",
            "(1641234567.2) can1 182#CD",
            "(1641234567.3) can2 183#EF"
        };
        
        for (size_t i = 0; i < lines.size(); ++i) {
            CANFrame frame;
            REQUIRE(CanFrameParser::parseLine(lines[i], frame));
            REQUIRE(frame.interface == ("can" + std::to_string(i)));
            REQUIRE(frame.id == (0x181 + i));
        }
    }
    
    SECTION("empty data") {
        CANFrame frame;
        REQUIRE(CanFrameParser::parseLine("(1641234567.1) can0 123#", frame));
        REQUIRE(frame.data.size() == 0);
    }
    
    SECTION("maximum data") {
        CANFrame frame;
        REQUIRE(CanFrameParser::parseLine("(1641234567.1) can0 123#0123456789ABCDEF", frame));
        REQUIRE(frame.data.size() == 8);
    }
}

TEST_CASE("CAN ID validation", "[canframe]") {
    SECTION("standard CAN IDs") {
        REQUIRE(CanFrameParser::isValidId(0x0, false));
        REQUIRE(CanFrameParser::isValidId(0x7FF, false));
        REQUIRE_FALSE(CanFrameParser::isValidId(0x800, false));
    }
    
    SECTION("extended CAN IDs") {
        REQUIRE(CanFrameParser::isValidId(0x800, true));
        REQUIRE(CanFrameParser::isValidId(0x1FFFFFFF, true));
        REQUIRE_FALSE(CanFrameParser::isValidId(0x20000000, true));
    }
}

TEST_CASE("CAN parsing errors", "[canframe]") {
    CANFrame frame;
    
    SECTION("malformed lines") {
        REQUIRE_FALSE(CanFrameParser::parseLine("", frame));
        REQUIRE_FALSE(CanFrameParser::parseLine("invalid", frame));
        REQUIRE_FALSE(CanFrameParser::parseLine("(123) can0", frame));
        REQUIRE_FALSE(CanFrameParser::parseLine("123 can0 180#AB", frame));
    }
    
    SECTION("invalid hex data") {
        REQUIRE_FALSE(CanFrameParser::parseLine("(1641234567.1) can0 XYZ#AB", frame));
        REQUIRE_FALSE(CanFrameParser::parseLine("(1641234567.1) can0 180#XY", frame));
    }
}