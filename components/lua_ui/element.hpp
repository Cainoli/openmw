#ifndef OPENMW_LUAUI_ELEMENT
#define OPENMW_LUAUI_ELEMENT

#include "widget.hpp"

namespace LuaUi
{
    struct Element
    {
        static std::shared_ptr<Element> make(sol::table layout);

        template <class Callback>
        static void forEach(Callback callback)
        {
            for (auto& [e, _] : sAllElements)
                callback(e);
        }

        WidgetExtension* mRoot;
        sol::object mLayout;
        std::string mLayer;
        bool mUpdate;
        bool mDestroy;

        void create();

        void update();

        void destroy();

        friend void clearUserInterface();

    private:
        Element(sol::table layout);
        sol::table layout() { return LuaUtil::cast<sol::table>(mLayout); }
        static std::map<Element*, std::shared_ptr<Element>> sAllElements;
    };
}

#endif // !OPENMW_LUAUI_ELEMENT
