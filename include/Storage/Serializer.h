#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <vector>
#include <string>
#include <map>

class Serializer {
public:
    
    static std::vector<char> serialize(const std::map<std::string, std::string>& data);
    static std::map<std::string, std::string> deserialize(const std::vector<char>& buffer);
};

#endif