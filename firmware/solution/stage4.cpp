#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <iomanip>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <regex>

// signal definition
struct Signal {
    std::string name;
    uint8_t start_bit;
    uint8_t bit_length;
    bool is_little_endian;
    double scale;
    double offset;
    std::string unit;
};

// msg definition
struct Message {
    uint32_t id;
    std::string name;
    std::vector<Signal> signals;
};

// dbc network containing all messages
struct DBCNetwork {
    std::vector<Message> messages;
};

// CAN frame data
struct CANFrame {
    double timestamp;
    std::string interface;
    uint32_t id;
    std::vector<uint8_t> data;
};

class DBCParser {
public:
    static DBCNetwork parseFile(const std::string& filename) {
        DBCNetwork network;
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Cannot open DBC file: " << filename << std::endl;
            return network;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#') {
                continue;
            }
            
            if (line.substr(0, 3) == "BO_") {
                Message msg = parseMessage(line, file);
                if (msg.id != 0) {
                    network.messages.push_back(msg);
                }
            }
        }
        
        return network;
    }

private:
    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(' ');
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(' ');
        return str.substr(first, (last - first + 1));
    }
    
    static Message parseMessage(const std::string& line, std::ifstream& file) {
    Message msg;
    
    std::regex msg_regex(R"(BO_\s+(\d+)\s+(\w+)\s*:\s*(\d+)\s+(\w+))");
    std::smatch match;
    
    if (std::regex_search(line, match, msg_regex)) {
        msg.id = std::stoul(match[1].str());
        msg.name = match[2].str();
        
        int signal_lines_found = 0;
        std::string signal_line;
        while (std::getline(file, signal_line)) {
            signal_line = trim(signal_line);
            if (signal_line.empty()) break;
            if (signal_line[0] != ' ') {
                file.seekg(-(long)signal_line.length() - 1, std::ios::cur);
                break;
            }
            
            if (signal_line.find("SG_") != std::string::npos) {
                signal_lines_found++;
                Signal signal = parseSignal(signal_line);
                if (!signal.name.empty()) {
                    msg.signals.push_back(signal);
                }
            }
        }
        
        if (msg.signals.empty() && signal_lines_found > 0) {
            std::cout << "Message " << msg.name << " found " << signal_lines_found 
                      << " SG_ lines but parsed 0 signals" << std::endl;
        }
    }
    
    return msg;
}
    
    static Signal parseSignal(const std::string& line) {
    Signal signal;
    
    // show lines im trying to parse
    static int signal_count = 0;
    signal_count++;
    if (signal_count <= 3) {
        std::cout << "Parsing signal line: '" << line << "'" << std::endl;
    }
    
    std::regex signal_regex(R"(SG_\s+(\w+)\s*:\s*(\d+)\|(\d+)@([01])([+-])\s*\(\s*([\d.-]+)\s*,\s*([\d.-]+)\s*\))");
    std::smatch match;
    
    if (std::regex_search(line, match, signal_regex)) {
        if (signal_count <= 3) {
            std::cout << " Signal matched: " << match[1].str() << std::endl;
        }
        signal.name = match[1].str();
        signal.start_bit = std::stoi(match[2].str());
        signal.bit_length = std::stoi(match[3].str());
        signal.is_little_endian = (match[4].str() == "1");
        signal.scale = std::stod(match[6].str());
        signal.offset = std::stod(match[7].str());
        
        // extract unit if present
        size_t unit_start = line.find('"');
        if (unit_start != std::string::npos) {
            size_t unit_end = line.find('"', unit_start + 1);
            if (unit_end != std::string::npos) {
                signal.unit = line.substr(unit_start + 1, unit_end - unit_start - 1);
            }
        }
    } else {
        if (signal_count <= 3) {
            std::cout << "  No match " << std::endl;
        }
    }
    
    return signal;
}
};

class CANDecoder {
public:
    static uint64_t extractBits(const uint8_t* data, const Signal& signal) {
        uint64_t result = 0;
        
        if (signal.is_little_endian) {
            // little endian byte order
            for (int i = 0; i < signal.bit_length; i++) {
                int bit_pos = signal.start_bit + i;
                int byte_idx = bit_pos / 8;
                int bit_idx = bit_pos % 8;
                
                if (byte_idx < 8 && (data[byte_idx] & (1 << bit_idx))) {
                    result |= (1ULL << i);
                }
            }
        } else {
            // big endian byte order
            int start_byte = signal.start_bit / 8;
            int start_bit_in_byte = signal.start_bit % 8;
            
            for (int i = 0; i < signal.bit_length; i++) {
                int current_bit = start_bit_in_byte - i;
                int current_byte = start_byte;
                
                if (current_bit < 0) {
                    current_byte++;
                    current_bit += 8;
                }
                
                if (current_byte < 8 && (data[current_byte] & (1 << current_bit))) {
                    result |= (1ULL << (signal.bit_length - 1 - i));
                }
            }
        }
        
        return result;
    }
    
    static double decodeSignal(const uint8_t* data, const Signal& signal) {
        uint64_t raw_value = extractBits(data, signal);
        
        // for signed values
        if (signal.bit_length < 64 && (raw_value & (1ULL << (signal.bit_length - 1)))) {
            // sign extend for negative values
            raw_value |= (~0ULL << signal.bit_length);
        }
        
        double physical_value = (int64_t)raw_value * signal.scale + signal.offset;
        return physical_value;
    }
};

// function prototypes
CANFrame parseLine(const std::string& line);
bool initializeNetworks(std::map<std::string, DBCNetwork>& networks);
void processFrame(const CANFrame& frame, const std::map<std::string, DBCNetwork>& networks, std::vector<std::string>& results);
void processCANDump(const std::map<std::string, DBCNetwork>& networks, std::vector<std::string>& results);
void writeOutput(const std::vector<std::string>& results);

int main() {
    std::map<std::string, DBCNetwork> networks;
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

bool initializeNetworks(std::map<std::string, DBCNetwork>& networks) {
    std::cout << "=== Parsing DBC files ===" << std::endl;
    
    networks["can0"] = DBCParser::parseFile("dbc-files/ControlBus.dbc");
    std::cout << "ControlBus parsed: " << networks["can0"].messages.size() << " messages" << std::endl;
    
    networks["can1"] = DBCParser::parseFile("dbc-files/SensorBus.dbc");
    std::cout << "SensorBus parsed: " << networks["can1"].messages.size() << " messages" << std::endl;
    
    networks["can2"] = DBCParser::parseFile("dbc-files/TractiveBus.dbc");
    std::cout << "TractiveBus parsed: " << networks["can2"].messages.size() << " messages" << std::endl;
    
    // first few message IDs for each network for debugging
    std::cout << "\n=== Sample Message IDs ===" << std::endl;
    for (const auto& network : networks) {
        std::cout << network.first << " message IDs: ";
        for (size_t i = 0; i < std::min(size_t(5), network.second.messages.size()); i++) {
            std::cout << "0x" << std::hex << network.second.messages[i].id << std::dec << " ";
        }
        std::cout << std::endl;
    }
    
    // check if at least one network loaded successfully
    for (const auto& net : networks) {
        if (!net.second.messages.empty()) {
            return true;
        }
    }
    
    std::cerr << "No DBC files loaded successfully\n";
    return false;
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
                  DBCNetwork>& networks, 
                  std::vector<std::string>& results) {
    static int frame_count = 0;
    frame_count++;
    
    // show first 5 frames
    if (frame_count <= 5) {
        std::cout << "Processing frame " << frame_count << ": " << frame.interface 
                  << " ID=0x" << std::hex << frame.id << std::dec << std::endl;
    }
    
    auto it = networks.find(frame.interface);
    if (it == networks.end()) {
        if (frame_count <= 5) std::cout << "  Interface " << frame.interface << " not found" << std::endl;
        return;
    }
    
    // find matching message
    bool found_message = false;
    for (const auto& msg : it->second.messages) {
        if (msg.id == frame.id) {
            found_message = true;
            if (frame_count <= 5) std::cout << "  Found matching message: " << msg.name << " with " << msg.signals.size() << " signals" << std::endl;
            
            // pad data to 8 bytes
            std::vector<uint8_t> data(8, 0);
            std::copy(frame.data.begin(), 
                     std::min(frame.data.end(), frame.data.begin() + 8), 
                     data.begin());
            
            // decode signals in this message
            for (const auto& signal : msg.signals) {
                double phys_value = CANDecoder::decodeSignal(data.data(), signal);
                
                // format output string
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(1)
                    << "(" << std::setprecision(6) << frame.timestamp << "): " 
                    << signal.name << ": " << phys_value;
                results.push_back(oss.str());
            }
            
            return;
        }
    }
    
    if (!found_message && frame_count <= 5) {
        std::cout << "  No message found for ID 0x" << std::hex << frame.id << std::dec << std::endl;
    }
}

void processCANDump(const std::map<std::string, DBCNetwork>& networks, std::vector<std::string>& results) {
    std::ifstream input("dump.log");
    if (!input) {
        std::cerr << "Failed to open dump.log\n";
        return;
    }
    
    std::cout << "\n=== Processing CAN frames ===" << std::endl;
    
    std::string line;
    int total_frames = 0;
    while (std::getline(input, line)) {
        if (!line.empty()) {
            total_frames++;
            try {
                CANFrame frame = parseLine(line);
                processFrame(frame, networks, results);
            } catch (...) {
                // skip bad lines
            }
        }
    }
    
    std::cout << "Total frames processed: " << total_frames << std::endl;
    std::sort(results.begin(), results.end());
}

void writeOutput(const std::vector<std::string>& results) {
    std::ofstream output("output.txt");
    if (!output) {
        std::cerr << "Failed to create output.txt\n";
        return;
    }
    
    for (const auto& result : results) {
        output << result << "\n";
    }
}