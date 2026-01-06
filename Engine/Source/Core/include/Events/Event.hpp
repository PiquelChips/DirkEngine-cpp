#pragma once

#include <string_view>
#include <typeindex>

namespace dirk {

#define DEFINE_EVENT_TYPE(type)                                          \
public:                                                                  \
    static std::type_index getStaticType() { return typeid(type); }      \
    std::type_index getType() const override { return getStaticType(); } \
    std::string_view name() const override { return #type; }

class Event {
public:
    virtual ~Event() = default;
    virtual std::type_index getType() const = 0;
    virtual std::string_view name() const = 0;
};

} // namespace dirk
