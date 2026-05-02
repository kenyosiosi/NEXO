#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <vector>
#include <string>
#include <map>

class Serializer
{
    public:
        std::vector<char> serialize(const std::map<std::string, std::string>& data);
        std::map<std::string, std::string> deserialize(const std::vector<char>& buffer);
};

#endif // SERIALIZER_H