#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <vector>
#include <string>
#include <map>

class Serializer
{
    public:
        std::vector<char> serialize(const std::map<std::string, std::string>& data);
};

#endif // SERIALIZER_H