// sensorvaluecheck.cpp

#include "catch.hpp"
#include <vector>
#include <cstdint>

struct Signal {
    uint8_t startBit;
    uint8_t length;
    bool isSigned;
    double scale;
    double offset;
    bool isLittleEndian;
};

class SensorExtractor {
public:
    static uint64_t extractBits(const std::vector<uint8_t>& data, const Signal& sig) {
        if (data.empty() || sig.length == 0) {
            return 0;
        }
        
        uint64_t rawValue = 0;
        
        if (sig.isLittleEndian) {
            for (int i = 0; i < sig.length; ++i) {
                int byteIndex = (sig.startBit + i) / 8;
                int bitIndex = (sig.startBit + i) % 8;
                
                if (byteIndex < data.size() && (data[byteIndex] & (1 << bitIndex))) {
                    rawValue |= (1ULL << i);
                }
            }
        } else {
            // Big endian
            int startByte = sig.startBit / 8;
            int startBitInByte = sig.startBit % 8;
            
            for (int i = 0; i < sig.length; ++i) {
                int currentBit = startBitInByte - i;
                int currentByte = startByte;
                
                while (currentBit < 0) {
                    currentBit += 8;
                    currentByte++;
                }
                
                if (currentByte < data.size() && (data[currentByte] & (1 << currentBit))) {
                    rawValue |= (1ULL << (sig.length - 1 - i));
                }
            }
        }
        
        return rawValue;
    }
    
    static double convertToPhysical(uint64_t raw, const Signal& sig) {
        int64_t signedValue = raw;
        
        if (sig.isSigned && (raw & (1ULL << (sig.length - 1)))) {
            signedValue = static_cast<int64_t>(raw | (~0ULL << sig.length));
        }
        
        return static_cast<double>(signedValue) * sig.scale + sig.offset;
    }
    
    static double extractSensorValue(const std::vector<uint8_t>& data, const Signal& sig) {
        uint64_t raw = extractBits(data, sig);
        return convertToPhysical(raw, sig);
    }
};

TEST_CASE("Basic bit extraction", "[sensor]") {
    SECTION("single byte little endian") {
        std::vector<uint8_t> data = {0xAB}; // 10101011
        Signal sig = {0, 8, false, 1.0, 0.0, true};
        
        REQUIRE(SensorExtractor::extractBits(data, sig) == 0xAB);
    }
    
    SECTION("nibble extraction") {
        std::vector<uint8_t> data = {0xAB};
        
        Signal lowNibble = {0, 4, false, 1.0, 0.0, true};
        REQUIRE(SensorExtractor::extractBits(data, lowNibble) == 0x0B);
        
        Signal highNibble = {4, 4, false, 1.0, 0.0, true};
        REQUIRE(SensorExtractor::extractBits(data, highNibble) == 0x0A);
    }
    
    SECTION("multi-byte little endian") {
        std::vector<uint8_t> data = {0x34, 0x12}; // 0x1234 in LE
        Signal sig = {0, 16, false, 1.0, 0.0, true};
        
        REQUIRE(SensorExtractor::extractBits(data, sig) == 0x1234);
    }
    
    SECTION("big endian extraction") {
        std::vector<uint8_t> data = {0x12, 0x34}; // 0x1234 in BE
        Signal sig = {7, 16, false, 1.0, 0.0, false};
        
        REQUIRE(SensorExtractor::extractBits(data, sig) == 0x1234);
    }
}

TEST_CASE("Physical value conversion", "[sensor]") {
    SECTION("basic scaling") {
        Signal sig = {0, 8, false, 0.1, 0.0, true};
        REQUIRE(SensorExtractor::convertToPhysical(100, sig) == 10.0);
    }
    
    SECTION("scaling with offset") {
        Signal sig = {0, 8, false, 0.75, -40.0, true};
        REQUIRE(SensorExtractor::convertToPhysical(200, sig) == 110.0); // 200*0.75-40
    }
    
    SECTION("signed values") {
        Signal sig = {0, 8, true, 1.0, 0.0, true};
        REQUIRE(SensorExtractor::convertToPhysical(0xFF, sig) == -1.0);
        REQUIRE(SensorExtractor::convertToPhysical(0x80, sig) == -128.0);
        REQUIRE(SensorExtractor::convertToPhysical(0x7F, sig) == 127.0);
    }
}

TEST_CASE("Complete sensor extraction", "[sensor]") {
    SECTION("temperature sensor") {
        std::vector<uint8_t> data = {0xA0}; // 160
        Signal tempSig = {0, 8, false, 0.75, -48.0, true};
        
        double temp = SensorExtractor::extractSensorValue(data, tempSig);
        REQUIRE(temp == 72.0); // 160 * 0.75 - 48
    }
    
    SECTION("RPM sensor") {
        std::vector<uint8_t> data = {0x10, 0x00}; // 4096 in BE
        Signal rpmSig = {7, 16, false, 0.25, 0.0, false};
        
        double rpm = SensorExtractor::extractSensorValue(data, rpmSig);
        REQUIRE(rpm == 1024.0); // 4096 * 0.25
    }
    
    SECTION("speed sensor") {
        std::vector<uint8_t> data = {0x64, 0x00}; // 100 in LE
        Signal speedSig = {0, 16, false, 0.01, 0.0, true};
        
        double speed = SensorExtractor::extractSensorValue(data, speedSig);
        REQUIRE(speed == 1.0); // 100 * 0.01
    }
}