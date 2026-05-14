#include "../../include/Storage/Serializer.h"
#include <cstdint>

std::vector<char> Serializer::serialize(const std::map<std::string, std::string>& data) {
    std::vector<char> buffer;
    uint16_t keyCount = data.size();
    buffer.push_back(keyCount & 0xFF);
    buffer.push_back((keyCount >> 8) & 0xFF);

    for (auto const& [key, val] : data) {
        // Guardar longitud de la llave en 4 bytes (uint32_t)
        uint32_t keyLen = key.length();
        buffer.push_back(keyLen & 0xFF);
        buffer.push_back((keyLen >> 8) & 0xFF);
        buffer.push_back((keyLen >> 16) & 0xFF);
        buffer.push_back((keyLen >> 24) & 0xFF);
        buffer.insert(buffer.end(), key.begin(), key.end());

        // Guardar longitud del valor en 4 bytes (uint32_t)
        uint32_t valLen = val.length();
        buffer.push_back(valLen & 0xFF);
        buffer.push_back((valLen >> 8) & 0xFF);
        buffer.push_back((valLen >> 16) & 0xFF);
        buffer.push_back((valLen >> 24) & 0xFF);
        buffer.insert(buffer.end(), val.begin(), val.end());
    }
    return buffer;
}

std::map<std::string, std::string> Serializer::deserialize(const std::vector<char>& buffer) {
    std::map<std::string, std::string> data;
    if (buffer.size() < 2) return data;

    size_t pos = 0;
    uint16_t keyCount = static_cast<uint8_t>(buffer[pos]) | (static_cast<uint8_t>(buffer[pos + 1]) << 8);
    pos += 2;

    for (uint16_t i = 0; i < keyCount; ++i) {
        if (pos >= buffer.size()) break;

        // Leer longitud de la llave (4 bytes)
        uint32_t keyLen = static_cast<uint8_t>(buffer[pos]) |
                          (static_cast<uint8_t>(buffer[pos+1]) << 8) |
                          (static_cast<uint8_t>(buffer[pos+2]) << 16) |
                          (static_cast<uint8_t>(buffer[pos+3]) << 24);
        pos += 4;
        
        std::string key(buffer.begin() + pos, buffer.begin() + pos + keyLen);
        pos += keyLen;

        // Leer longitud del valor (4 bytes)
        uint32_t valLen = static_cast<uint8_t>(buffer[pos]) |
                          (static_cast<uint8_t>(buffer[pos+1]) << 8) |
                          (static_cast<uint8_t>(buffer[pos+2]) << 16) |
                          (static_cast<uint8_t>(buffer[pos+3]) << 24);
        pos += 4;
        
        std::string val(buffer.begin() + pos, buffer.begin() + pos + valLen);
        pos += valLen;
        
        data[key] = val;
    }
    return data;
}