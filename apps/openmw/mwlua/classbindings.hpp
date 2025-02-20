#ifndef MWLUA_CLASSBINDINGS_H
#define MWLUA_CLASSBINDINGS_H

#include <sol/forward.hpp>

#include "context.hpp"

namespace MWLua
{
    sol::table initCoreClassBindings(const Context& context);
}

#endif // MWLUA_CLASSBINDINGS_H
