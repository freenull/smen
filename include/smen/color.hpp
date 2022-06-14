#ifndef SMEN_COLOR_HPP
#define SMEN_COLOR_HPP

#include <memory>
#include <sstream>

namespace smen {
    struct Color4 {
        using ComponentType = uint8_t;
        ComponentType r;  
        ComponentType g;  
        ComponentType b;  
        ComponentType a;  

        Color4(ComponentType r, ComponentType g, ComponentType b, ComponentType a)
        : r(r), g(g), b(b), a(a) {}

        inline void write_string(std::ostream & s) const {
            s << "Color4(" << r << ", " << g << ", " << b << ", alpha: " << a << ")";
        }

        inline const std::string to_string() const {
            auto s = std::stringstream();
            write_string(s);
            return s.str();
        }
    };

    inline std::ostream & operator<<(std::ostream & s, const Color4 & color) {
        color.write_string(s);
        return s;
    }
}

#endif//SMEN_COLOR_HPP
