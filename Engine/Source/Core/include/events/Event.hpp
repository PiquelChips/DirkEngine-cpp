#pragma once

#include <typeindex>

namespace dirk {

#define DEFINE_EVENT_TYPE(type)                                          \
    static std::type_index getStaticType() { return typeid(type); } \
    std::type_index getType() const override { return getStaticType(); }

class Event {
public:
    virtual ~Event() = default;
    virtual std::type_index getType() const = 0;
};

} // namespace dirk
