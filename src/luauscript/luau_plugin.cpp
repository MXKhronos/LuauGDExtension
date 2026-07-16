#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/script_editor.hpp>
#include <godot_cpp/classes/script_editor_base.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/code_edit.hpp>

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

    _ensure_luau_completion_prefixes(nullptr);
    script_editor->connect(
        "editor_script_changed", 
        callable_mp(this, &LuauPlugin::_ensure_luau_completion_prefixes)
    );
}

void LuauPlugin::_exit_tree() {
    ScriptEditor* script_editor = get_editor_interface()->get_script_editor();
    if (script_editor != nullptr) {
        script_editor->disconnect(
            "editor_script_changed", 
            callable_mp(this, &LuauPlugin::_ensure_luau_completion_prefixes)
        );
    }
}

void LuauPlugin::_ensure_luau_completion_prefixes(class Script *p_script) {
    ScriptEditor* script_editor = get_editor_interface()->get_script_editor();
    if (script_editor == nullptr) {
        return;
    }

    ScriptEditorBase* base = script_editor->get_current_editor();
    if (base == nullptr) {
        return;
    }

    Control* control = base->get_base_editor();
    CodeEdit* code_edit = Object::cast_to<CodeEdit>(control);
    if (code_edit == nullptr) {
        return;
    }

    TypedArray<String> prefixes = code_edit->get_code_completion_prefixes();
    if (!prefixes.has(":")) {
        prefixes.append(":");
        code_edit->set_code_completion_prefixes(prefixes);
    }

}