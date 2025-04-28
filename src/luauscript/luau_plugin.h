#ifndef LUAU_PLUGIN_H
#define LUAU_PLUGIN_H

#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include "luauscript_syntax_highlighter.h"

namespace godot {
    class LuauPlugin : public EditorPlugin {
        GDCLASS(LuauPlugin, EditorPlugin);

    protected:
        static void _bind_methods() {};

    public:
        Ref<LuauSyntaxHighlighter> syntax_highlighter;

        void _enter_tree() override;
    };
}

#endif