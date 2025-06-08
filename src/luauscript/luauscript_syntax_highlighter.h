#ifndef LUAU_SYNTAX_HIGHLIGHTER_H
#define LUAU_SYNTAX_HIGHLIGHTER_H

#include <godot_cpp/classes/script_language.hpp>
#include <godot_cpp/classes/editor_syntax_highlighter.hpp>
#include <godot_cpp/classes/code_highlighter.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/classes/script_editor.hpp>

namespace godot {
#ifdef TOOLS_ENABLED

class LuauSyntaxHighlighter : public EditorSyntaxHighlighter {
    GDCLASS(LuauSyntaxHighlighter, EditorSyntaxHighlighter);

protected:
    static void _bind_methods() {}

private:
    Ref<CodeHighlighter> highlighter;
    ScriptLanguage *script_language = nullptr;

	Color font_color;
	Color symbol_color;
	Color function_color;
	Color global_function_color;
	Color function_definition_color;
	Color built_in_type_color;
	Color number_color;

public:
    Dictionary _get_line_syntax_highlighting(int32_t p_line) const override;
    void _clear_highlighting_cache() override;
    void _update_cache() override;

    String _get_name() const override;
    PackedStringArray _get_supported_languages() const override;
    Ref<EditorSyntaxHighlighter> _create() const override;
};


#endif //TOOLS_ENABLED
}
#endif