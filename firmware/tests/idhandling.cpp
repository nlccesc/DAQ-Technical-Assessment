// idhandling.cpp

#include "catch.hpp"
#include <map>
#include <vector>
#include <string>

struct ExampleSignal {
    std::string name;
    uint8_t startBit;
    uint8_t length;
    double scale;
    double offset;
};

struct ExampleMessage {
    uint32_t id;
    std::string name;
    std::vector<ExampleSignal> signals;
};

class DbcConflictHandler {
public:
    struct Conflict {
        uint32_t canId;
        std::vector<std::string> interfaces;
        bool hasOverlap;
        std::vector<std::string> details;
    };
    
    static std::vector<Conflict> findConflicts(
        const std::map<std::string, std::vector<ExampleMessage>>& networks) {
        
        std::vector<Conflict> conflicts;
        std::map<uint32_t, std::vector<std::pair<std::string, ExampleMessage>>> idMap;
        
        // make map of CAN ID to messages
        for (const auto& net : networks) {
            for (const auto& msg : net.second) {
                idMap[msg.id].push_back({net.first, msg});
            }
        }
        
        // conflict check
        for (const auto& entry : idMap) {
            if (entry.second.size() > 1) {
                Conflict conflict;
                conflict.canId = entry.first;
                conflict.hasOverlap = false;
                
                for (const auto& msgPair : entry.second) {
                    conflict.interfaces.push_back(msgPair.first);
                }
                
                checkSignalOverlap(entry.second, conflict);
                conflicts.push_back(conflict);
            }
        }
        
        return conflicts;
    }
    
    static std::string getDbcFile(const std::string& interface) {
        if (interface == "can0") return "ControlBus.dbc";
        if (interface == "can1") return "SensorBus.dbc";
        if (interface == "can2") return "TractiveBus.dbc";
        return "";
    }

private:
    static void checkSignalOverlap(
        const std::vector<std::pair<std::string, ExampleMessage>>& messages,
        Conflict& conflict) {
        
        for (size_t i = 0; i < messages.size(); ++i) {
            for (size_t j = i + 1; j < messages.size(); ++j) {
                const auto& msg1 = messages[i].second;
                const auto& msg2 = messages[j].second;
                
                for (const auto& sig1 : msg1.signals) {
                    for (const auto& sig2 : msg2.signals) {
                        if (signalsOverlap(sig1, sig2)) {
                            conflict.hasOverlap = true;
                            conflict.details.push_back(sig1.name + " vs " + sig2.name);
                        }
                    }
                }
            }
        }
    }
    
    static bool signalsOverlap(const ExampleSignal& sig1, const ExampleSignal& sig2) {
        uint8_t end1 = sig1.startBit + sig1.length - 1;
        uint8_t end2 = sig2.startBit + sig2.length - 1;
        return !(end1 < sig2.startBit || end2 < sig1.startBit);
    }
};

TEST_CASE("Interface mapping", "[idhandling]") {
    SECTION("valid mappings") {
        REQUIRE(DbcConflictHandler::getDbcFile("can0") == "ControlBus.dbc");
        REQUIRE(DbcConflictHandler::getDbcFile("can1") == "SensorBus.dbc");
        REQUIRE(DbcConflictHandler::getDbcFile("can2") == "TractiveBus.dbc");
    }
    
    SECTION("invalid interfaces") {
        REQUIRE(DbcConflictHandler::getDbcFile("can3") == "");
        REQUIRE(DbcConflictHandler::getDbcFile("") == "");
    }
}

TEST_CASE("No conflicts", "[idhandling]") {
    SECTION("different CAN IDs") {
        std::map<std::string, std::vector<ExampleMessage>> networks;
        
        networks["can0"] = {{0x180, "Engine", {{"RPM", 0, 16, 0.25, 0.0}}}};
        networks["can1"] = {{0x280, "Sensor", {{"Temp", 0, 16, 0.1, -40.0}}}};
        
        auto conflicts = DbcConflictHandler::findConflicts(networks);
        REQUIRE(conflicts.empty());
    }
    
    SECTION("same ID no overlap") {
        std::map<std::string, std::vector<ExampleMessage>> networks;
        
        networks["can0"] = {{0x100, "Msg1", {{"Sig1", 0, 8, 1.0, 0.0}}}};
        networks["can1"] = {{0x100, "Msg1", {{"Sig2", 16, 8, 1.0, 0.0}}}};
        
        auto conflicts = DbcConflictHandler::findConflicts(networks);
        REQUIRE(conflicts.size() == 1);
        REQUIRE_FALSE(conflicts[0].hasOverlap);
    }
}

TEST_CASE("Conflict detection", "[idhandling]") {
    SECTION("overlapping signals") {
        std::map<std::string, std::vector<ExampleMessage>> networks;
        
        networks["can0"] = {{0x200, "Test", {{"RPM", 0, 16, 0.25, 0.0}}}};
        networks["can2"] = {{0x200, "Test", {{"Speed", 0, 16, 1.0, 0.0}}}};
        
        auto conflicts = DbcConflictHandler::findConflicts(networks);
        REQUIRE(conflicts.size() == 1);
        REQUIRE(conflicts[0].canId == 0x200);
        REQUIRE(conflicts[0].hasOverlap);
        REQUIRE(conflicts[0].interfaces.size() == 2);
    }
    
    SECTION("three-way conflict") {
        std::map<std::string, std::vector<ExampleMessage>> networks;
        
        networks["can0"] = {{0x300, "A", {{"Sig1", 0, 8, 1.0, 0.0}}}};
        networks["can1"] = {{0x300, "B", {{"Sig2", 4, 8, 1.0, 0.0}}}};
        networks["can2"] = {{0x300, "C", {{"Sig3", 8, 8, 1.0, 0.0}}}};
        
        auto conflicts = DbcConflictHandler::findConflicts(networks);
        REQUIRE(conflicts.size() == 1);
        REQUIRE(conflicts[0].interfaces.size() == 3);
        REQUIRE(conflicts[0].hasOverlap);
    }
    
    SECTION("automotive scenario") {
        std::map<std::string, std::vector<ExampleMessage>> networks;
        
        // OEM interpretation
        networks["can0"] = {{0x2C0, "IC_info", {
            {"VehicleSpeed", 16, 16, 0.01, 0.0},
            {"EngineRPM", 0, 16, 0.25, 0.0}
        }}};
        
        // interpretation
        networks["can2"] = {{0x2C0, "ECU_Data", {
            {"WheelSpeed", 16, 16, 0.0625, 0.0},
            {"MotorRPM", 0, 16, 1.0, 0.0}
        }}};
        
        auto conflicts = DbcConflictHandler::findConflicts(networks);
        REQUIRE(conflicts.size() == 1);
        REQUIRE(conflicts[0].hasOverlap);
        REQUIRE(conflicts[0].details.size() >= 2);
    }
}