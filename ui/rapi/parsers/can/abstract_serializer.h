#ifndef ABSTRACT_SERIALIZER_H
#define ABSTRACT_SERIALIZER_H

#include "tools/rapi_names.hpp"

class AbstractSerializer
{
public:
    virtual std::vector<uint8_t> deserialize(const std::vector<uint8_t> &bytes) = 0;
    virtual std::vector<uint8_t> serialize(std::vector<uint8_t> data) = 0;

};

#endif // ABSTRACT_SERIALIZER_H
