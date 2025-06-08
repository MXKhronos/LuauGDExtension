
#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/text_edit.hpp>
#include "luauscript_syntax_highlighter.h"
#include "luau_constants.h"

using namespace godot;

Dictionary LuauSyntaxHighlighter::_get_line_syntax_highlighting(int32_t p_line) const {
    
    return Dictionary();
}

void LuauSyntaxHighlighter::_clear_highlighting_cache() {
}

void LuauSyntaxHighlighter::_update_cache() {
    TextEdit* text_edit = get_text_edit();

	font_color = text_edit->get_theme_color("text_editor/theme/highlighting/function_color"); //font_color
	symbol_color = text_edit->get_theme_color("text_editor/theme/highlighting/symbol_color");
	function_color = text_edit->get_theme_color("text_editor/theme/highlighting/function_color");
	number_color = text_edit->get_theme_color("text_editor/theme/highlighting/number_color");

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
}
