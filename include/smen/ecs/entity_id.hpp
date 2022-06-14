#ifndef SMEN_ECS_ENTITY_ID_HPP
#define SMEN_ECS_ENTITY_ID_HPP

#include <memory>
#include <iostream>
#include <sstream>

namespace smen {
    using EntityID = uint32_t;
    /* struct EntityID { */
    /* public: */
    /*     const uint32_t id; */

    /*     EntityID() = delete; */
    /*     inline EntityID(uint32_t id) : id(id) {} */

    /*     inline void write_string(std::ostream & s) const { */
    /*         s << "Entity(" << id << ")"; */
    /*     } */

    /*     inline std::string to_string() const { */
    /*         std::ostringstream s; */
    /*         write_string(s); */
    /*         return s.str(); */
    /*     } */
    /* }; */

    /* inline std::ostream & operator<<(std::ostream & s, const EntityID & ent) { */
    /*     ent.write_string(s); */
    /*     return s; */
    /* } */
}

#endif//SMEN_ECS_ENTITY_ID_HPP
