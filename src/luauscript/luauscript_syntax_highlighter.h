#ifndef LUAU_SYNTAX_HIGHLIGHTER_H
#define LUAU_SYNTAX_HIGHLIGHTER_H

#include <godot_cpp/classes/script_language.hpp>
#include <godot_cpp/classes/editor_syntax_highlighter.hpp>
#include <godot_cpp/classes/code_highlighter.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/classes/script_editor.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/hash_map.hpp>

namespace godot {
#ifdef TOOLS_ENABLED

class LuauSyntaxHighlighter : public EditorSyntaxHighlighter {
    GDCLASS(LuauSyntaxHighlighter, EditorSyntaxHighlighter);

protected:
    static void _bind_methods() {}

private:
    struct ColorRegion {
        Color color;
        String start_key;
        String end_key;
        bool line_only = false;
    };

    Ref<CodeHighlighter> highlighter;
    ScriptLanguage *script_language = nullptr;

    // Colors from theme
	Color text_color;
	Color symbol_color;
	Color number_color;
	Color function_color;
	Color global_function_color;
	Color function_definition_color;
	Color built_in_type_color;
    Color member_variable_color;
    Color keyword_color;
    Color control_flow_keyword_color;
    Color comment_color;
    Color string_color;
    Color type_color;
    Color constant_color;
    Color self_keyword_color;
    Color annotation_color;
    
    // Language features
    HashSet<String> keywords;
    HashSet<String> control_flow_keywords;
    HashSet<String> built_in_types;
    HashSet<String> built_in_functions;
    Vector<ColorRegion> color_regions;
    
    // Helper methods
    bool is_symbol(char32_t c) const;
    bool is_ascii_identifier_char(char32_t c) const;
    bool is_hex_digit(char32_t c) const;
    bool is_digit(char32_t c) const;
    bool is_binary_digit(char32_t c) const;
    bool is_constant_identifier(const String& identifier) const;

public:
    Dictionary _get_line_syntax_highlighting(int32_t p_line) const override;
    void _clear_highlighting_cache() override;
    void _update_cache() override;

    String _get_name() const override;
    PackedStringArray _get_supported_languages() const override;
    Ref<EditorSyntaxHighlighter> _create() const;
};


#endif //TOOLS_ENABLED
}
#endif