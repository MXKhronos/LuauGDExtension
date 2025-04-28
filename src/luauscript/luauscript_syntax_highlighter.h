#ifndef LUAU_SYNTAX_HIGHLIGHTER_H
#define LUAU_SYNTAX_HIGHLIGHTER_H

#include <godot_cpp/classes/script_language.hpp>
#include <godot_cpp/classes/editor_syntax_highlighter.hpp>
#include <godot_cpp/classes/code_highlighter.hpp>

namespace godot {

#ifdef TOOLS_ENABLED
    class LuauSyntaxHighlighter : public EditorSyntaxHighlighter {
        GDCLASS(LuauSyntaxHighlighter, EditorSyntaxHighlighter);

    protected:
        static void _bind_methods() {}

    private:
        Ref<CodeHighlighter> highlighter;
        ScriptLanguage *script_language = nullptr;

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