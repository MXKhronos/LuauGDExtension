#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/script_editor.hpp>

#include "luau_plugin.h"

using namespace godot;

void LuauPlugin::_enter_tree() {
    ScriptEditor* script_editor = get_editor_interface()->get_script_editor();
    ERR_FAIL_COND_MSG(!script_editor, 
        "Failed to get script editor.");

    syntax_highlighter.instantiate();
    ERR_FAIL_COND_MSG(!syntax_highlighter.is_valid(), 
        "Failed to instantiate Luau syntax highlighter.");

    script_editor->register_syntax_highlighter(syntax_highlighter);
}