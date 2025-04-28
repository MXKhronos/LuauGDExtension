
#include <godot_cpp/godot.hpp>
#include "luauscript_syntax_highlighter.h"
#include "luau_constants.h"

using namespace godot;

Dictionary godot::LuauSyntaxHighlighter::_get_line_syntax_highlighting(int32_t p_line) const {
    return Dictionary();
}

void godot::LuauSyntaxHighlighter::_clear_highlighting_cache() {
}

void LuauSyntaxHighlighter::_update_cache() {
}

String LuauSyntaxHighlighter::_get_name() const { 
    return luau::LUAUSCRIPT_NAME; 
};

PackedStringArray LuauSyntaxHighlighter::_get_supported_languages() const { 
    PackedStringArray languages;
    languages.push_back(luau::LUAUSCRIPT_NAME);
    return languages;
}

Ref<EditorSyntaxHighlighter> LuauSyntaxHighlighter::_create() const {
    Ref<LuauSyntaxHighlighter> highlighter;
    highlighter.instantiate();
    return highlighter;
};