// calculationcheck.cpp

#include "catch.hpp"
#include <vector>
#include <cstdint>

class BitCalculator {
public:
    static uint64_t extractLittleEndian(const std::vector<uint8_t>& data, 
                                        uint8_t startBit, uint8_t length) {
        if (data.empty() || length == 0) {
            return 0;
        }
        
        uint64_t result = 0;
        for (int i = 0; i < length; ++i) {
             int byteIndex = (startBit + i) / 8;
             int bitIndex = (startBit + i) % 8;
            
            if (byteIndex < data.size() && (data[byteIndex] & (1 << bitIndex))) {
                result |= (1ULL << i);
            }
        }
        return result;
    }
    
    static uint64_t extractBigEndian(const std::vector<uint8_t>& data, 
                                     uint8_t startBit, uint8_t length) {
        uint64_t result = 0;
        int startByte = startBit / 8;
        int startBitInByte = startBit % 8;
        
        for (int i = 0; i < length; ++i) {
            int currentBit = startBitInByte - i;
            int currentByte = startByte;
            
            while (currentBit < 0) {
                currentBit += 8;
                currentByte++;
            }
            
            if (currentByte < data.size() && (data[currentByte] & (1 << currentBit))) {
                result |= (1ULL << (length - 1 - i));
            }
        }
        return result;
    }
    
    static double applyScaling(uint64_t raw, double scale, double offset, 
                               bool isSigned, uint8_t bitLength) {
        int64_t signedValue = raw;
        
        if (isSigned && (raw & (1ULL << (bitLength - 1)))) {
            signedValue = static_cast<int64_t>(raw | (~0ULL << bitLength));
        }
        
        return static_cast<double>(signedValue) * scale + offset;
    }
};

TEST_CASE("Little endian extraction", "[calculation]") {
    SECTION("8-bit values") {
        std::vector<uint8_t> data = {0xAB}; // 10101011
        
        REQUIRE(BitCalculator::extractLittleEndian(data, 0, 8) == 0xAB);
        REQUIRE(BitCalculator::extractLittleEndian(data, 0, 4) == 0x0B);
        REQUIRE(BitCalculator::extractLittleEndian(data, 4, 4) == 0x0A);
    }
    
    SECTION("16-bit values") {
        std::vector<uint8_t> data = {0x34, 0x12}; // LE: 0x1234
        REQUIRE(BitCalculator::extractLittleEndian(data, 0, 16) == 0x1234);
    }
    
    SECTION("cross-byte boundaries") {
        std::vector<uint8_t> data = {0x0F, 0xF0}; // 00001111 11110000
        // simple boundary crossing - extract the upper 4 bits of first byte
        REQUIRE(BitCalculator::extractLittleEndian(data, 4, 4) == 0x00);
        // extracting from second byte  
        REQUIRE(BitCalculator::extractLittleEndian(data, 8, 4) == 0x00);
    }
}

TEST_CASE("Big endian extraction", "[calculation]") {
    SECTION("8-bit values") {
        std::vector<uint8_t> data = {0xAB};
        REQUIRE(BitCalculator::extractBigEndian(data, 7, 8) == 0xAB);
        REQUIRE(BitCalculator::extractBigEndian(data, 7, 4) == 0x0A);
        REQUIRE(BitCalculator::extractBigEndian(data, 3, 4) == 0x0B);
    }
    
    SECTION("16-bit values") {
        std::vector<uint8_t> data = {0x12, 0x34}; // BE: 0x1234
        REQUIRE(BitCalculator::extractBigEndian(data, 7, 16) == 0x1234);
    }
    
    SECTION("cross-byte boundaries") {
        std::vector<uint8_t> data = {0xF0, 0x0F}; // 11110000 00001111
        REQUIRE(BitCalculator::extractBigEndian(data, 7, 8) == 0xF0);
    }
}

TEST_CASE("Bit length precision", "[calculation]") {
    std::vector<uint8_t> data = {0xFF, 0xFF};
    
    SECTION("various bit lengths") {
        REQUIRE(BitCalculator::extractLittleEndian(data, 0, 1) == 1);
        REQUIRE(BitCalculator::extractLittleEndian(data, 0, 4) == 0x0F);
        REQUIRE(BitCalculator::extractLittleEndian(data, 0, 8) == 0xFF);
        REQUIRE(BitCalculator::extractLittleEndian(data, 0, 12) == 0xFFF);
        REQUIRE(BitCalculator::extractLittleEndian(data, 0, 16) == 0xFFFF);
    }
    
    SECTION("offset positions") {
        std::vector<uint8_t> testData = {0xAA}; // 10101010
        REQUIRE(BitCalculator::extractLittleEndian(testData, 0, 1) == 0);
        REQUIRE(BitCalculator::extractLittleEndian(testData, 1, 1) == 1);
        REQUIRE(BitCalculator::extractLittleEndian(testData, 2, 1) == 0);
        REQUIRE(BitCalculator::extractLittleEndian(testData, 3, 1) == 1);
    }
}

TEST_CASE("Scaling calculations", "[calculation]") {
    SECTION("basic scaling") {
        REQUIRE(BitCalculator::applyScaling(100, 0.1, 0.0, false, 8) == 10.0);
        REQUIRE(BitCalculator::applyScaling(50, 2.0, 0.0, false, 8) == 100.0);
    }
    
    SECTION("with offset") {
        REQUIRE(BitCalculator::applyScaling(100, 0.5, -40.0, false, 8) == 10.0);
        REQUIRE(BitCalculator::applyScaling(200, 0.75, -48.0, false, 8) == 102.0);
    }
    
    SECTION("signed values") {
        // 8-bit signed: 0xFF = -1, 0x80 = -128
        REQUIRE(BitCalculator::applyScaling(0xFF, 1.0, 0.0, true, 8) == -1.0);
        REQUIRE(BitCalculator::applyScaling(0x80, 1.0, 0.0, true, 8) == -128.0);
        REQUIRE(BitCalculator::applyScaling(0x7F, 1.0, 0.0, true, 8) == 127.0);
    }
    
    SECTION("16-bit signed") {
        REQUIRE(BitCalculator::applyScaling(0xFFFF, 1.0, 0.0, true, 16) == -1.0);
        REQUIRE(BitCalculator::applyScaling(0x8000, 1.0, 0.0, true, 16) == -32768.0);
    }
}

TEST_CASE("Automotive calculations", "[calculation]") {
    SECTION("temperature sensor") {
        // normal automotive temp: raw * 0.75 - 48
        uint64_t raw = 160;
        double result = BitCalculator::applyScaling(raw, 0.75, -48.0, false, 8);
        REQUIRE(result == 72.0); // 160 * 0.75 - 48 = 72°C
    }
    
    SECTION("RPM calculation") {
        // normal RPM: raw * 0.25
        uint64_t raw = 4000;
        double result = BitCalculator::applyScaling(raw, 0.25, 0.0, false, 16);
        REQUIRE(result == 1000.0); // 4000 * 0.25 = 1000 RPM
    }
    
    SECTION("speed calculation") {
        // vehicle speed: raw * 0.01 km/h
        uint64_t raw = 8000;
        double result = BitCalculator::applyScaling(raw, 0.01, 0.0, false, 16);
        REQUIRE(result == 80.0); // 8000 * 0.01 = 80 km/h
    }
    
    SECTION("voltage measurement") {
        // battery voltage: raw * 0.001 + 0 V
        uint64_t raw = 12500;
        double result = BitCalculator::applyScaling(raw, 0.001, 0.0, false, 16);
        REQUIRE(result == 12.5); // 12.5V
    }
}