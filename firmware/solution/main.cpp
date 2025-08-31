#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <map>
#include <vector>
#include <iomanip>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include "dbcppp/Network.h"

// hold parsed CAN frame data
struct CANFrame {
    double timestamp;
    std::string interface;
    uint32_t id;
    std::vector<uint8_t> data;
};

// function prototypes
CANFrame parseLine(const std::string& line);
bool initializeNetworks(std::map<std::string, 
                        std::unique_ptr<dbcppp::INetwork>>& networks);
void processFrame(const CANFrame& frame, 
                  const std::map<std::string, 
                  std::unique_ptr<dbcppp::INetwork>>& networks,
                  std::vector<std::string>& results);
void processCANDump(const std::map<std::string, 
                    std::unique_ptr<dbcppp::INetwork>>& networks,
                    std::vector<std::string>& results);
void writeOutput(const std::vector<std::string>& results);

int main() {
    std::map<std::string, std::unique_ptr<dbcppp::INetwork>> networks;
    std::vector<std::string> results;
    
    if (!initializeNetworks(networks)) {
        std::cerr << "Failed to initialize decoder\n";
        return 1;
    }
    
    processCANDump(networks, results);
    writeOutput(results);
    
    std::cout << "Processed " << results.size() << " signals\n";
    return 0;
}

bool initializeNetworks(std::map<std::string, 
                        std::unique_ptr<dbcppp::INetwork>>& networks) {
    // load
    std::ifstream control("/app/dbc-files/ControlBus.dbc");
    std::ifstream sensor("/app/dbc-files/SensorBus.dbc");
    std::ifstream tractive("/app/dbc-files/TractiveBus.dbc");
    
    if (!control || !sensor || !tractive) {
        std::cerr << "Failed to open DBC files\n";
        return false;
    }
    
    // map
    networks["can0"] = dbcppp::INetwork::LoadDBCFromIs(control);
    networks["can1"] = dbcppp::INetwork::LoadDBCFromIs(sensor);
    networks["can2"] = dbcppp::INetwork::LoadDBCFromIs(tractive);
    
    return true;
}

CANFrame parseLine(const std::string& line) {
    CANFrame frame;
    char interface[10];
    char id_hex[10];
    char data_hex[20] = "";
    
    // parse format: (timestamp) interface id#data
    sscanf(line.c_str(), "(%lf) %s %[^#]#%s", 
           &frame.timestamp, interface, id_hex, data_hex);
    
    frame.interface = interface;
    frame.id = std::stoul(id_hex, nullptr, 16);
    
    for (size_t i = 0; i < strlen(data_hex); i += 2) {
        char byte[3] = {data_hex[i], data_hex[i+1], '\0'};
        frame.data.push_back(std::stoul(byte, nullptr, 16));
    }
    
    return frame;
}

void processFrame(const CANFrame& frame,
                  const std::map<std::string, 
                  std::unique_ptr<dbcppp::INetwork>>& networks,
                  std::vector<std::string>& results) {
    auto it = networks.find(frame.interface);
    if (it == networks.end()) {
        return;
    }
    
    // search for matching message definition in DBC
    for (const auto& msg : it->second->Messages()) {
        if (msg.Id() != frame.id) {
            continue; 
        }
        
        // pad data to 8 bytes
        std::vector<uint8_t> data(8, 0);
        std::copy(frame.data.begin(), 
                 std::min(frame.data.end(), frame.data.begin() + 8), 
                 data.begin());
        
        // decode
        for (const auto& sig : msg.Signals()) {
            const auto raw_value = sig.Decode(data.data());
            const auto phys_value = sig.RawToPhys(raw_value);
            
            // output string format
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1)
                << "(" << std::setprecision(6) << frame.timestamp << "): " 
                << sig.Name() << ": " << phys_value;
            results.push_back(oss.str());
        }
        
        return;
    }
}

void processCANDump(const std::map<std::string, 
                    std::unique_ptr<dbcppp::INetwork>>& networks,
                    std::vector<std::string>& results) {
                    std::ifstream input("/app/dump.log");
    if (!input) {
        std::cerr << "Failed to open dump.log\n";
        return;
    }
    
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty()) {
            try {
                CANFrame frame = parseLine(line);
                processFrame(frame, networks, results);
            } catch (...) {
                // skip bad lines
            }
        }
    }
    
    std::sort(results.begin(), results.end());
}

void writeOutput(const std::vector<std::string>& results) {
    // write decoded signals to output file
    std::ofstream output("/app/output.txt");
    if (!output) {
        std::cerr << "Failed to create output.txt\n";
        return;
    }
    
    for (const auto& result : results) {
        output << result << "\n";
    }
}