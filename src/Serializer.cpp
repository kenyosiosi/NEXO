#include "Serializer.h"
#include <cstdint>

std::vector<char> Serializer::serialize(const std::map<std::string, std::string>& data)
{
    std::vector<char> buffer;
    // 1. Número de llaves (2 bytes)
    uint16_t keyCount = data.size();
    buffer.push_back(keyCount & 0xFF);
    buffer.push_back((keyCount >> 8) & 0xFF);

    for (auto const& [key, val] : data)
    {
        // 2. Longitud Llave (1 byte, max 255 chars)
        buffer.push_back((uint8_t)key.length());
        // 3. Texto Llave
        buffer.insert(buffer.end(), key.begin(), key.end());
        // 4. Longitud Valor (1 byte)
        buffer.push_back((uint8_t)val.length());
        // 5. Texto Valor
        buffer.insert(buffer.end(), val.begin(), val.end());
    }
    return buffer;
}

std::map<std::string, std::string> Serializer::deserialize(const std::vector<char>& buffer)
{
    std::map<std::string, std::string> data;
    if (buffer.size() < 2) return data; // Buffer demasiado pequeño

    int pos = 0;
    uint16_t keyCount = (uint8_t)buffer[pos] | ((uint8_t)buffer[pos + 1] << 8);
    pos += 2;

    for (int i = 0; i < keyCount; ++i) 
    {
        if (pos >= buffer.size()) break;
        
        uint8_t keyLen = (uint8_t)buffer[pos++];
        if (pos + keyLen > buffer.size()) break; // Seguridad de límites
        std::string key(buffer.begin() + pos, buffer.begin() + pos + keyLen);
        pos += keyLen;

        if (pos >= buffer.size()) break;
        uint8_t valLen = (uint8_t)buffer[pos++];
        if (pos + valLen > buffer.size()) break;
        std::string val(buffer.begin() + pos, buffer.begin() + pos + valLen);
        pos += valLen;

        data[key] = val;
    }
    return data;
}