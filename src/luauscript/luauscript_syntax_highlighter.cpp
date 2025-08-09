
#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/text_edit.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/templates/list.hpp>
#include "luauscript_syntax_highlighter.h"
#include "luau_constants.h"
#include "nobind.h"

using namespace godot;

Dictionary LuauSyntaxHighlighter::_get_line_syntax_highlighting(int32_t p_line) const {
    Dictionary color_map;
    
    TextEdit* text_edit = get_text_edit();
    if (!text_edit) {
        return color_map;
    }
    
    String line_text = text_edit->get_line(p_line);
    if (line_text.is_empty()) {
        return color_map;
    }
    
    Color current_color = text_color;
    bool in_string = false;
    bool in_multiline_string = false;
    bool in_comment = false;
    bool in_multiline_comment = false;
    char32_t string_delimiter = 0;
    
    // Track multiline state from previous lines
    static HashMap<int, bool> multiline_comment_state;
    static HashMap<int, bool> multiline_string_state;
    
    if (p_line > 0) {
        if (multiline_comment_state.has(p_line - 1)) {
            in_multiline_comment = multiline_comment_state[p_line - 1];
        }
        if (multiline_string_state.has(p_line - 1)) {
            in_multiline_string = multiline_string_state[p_line - 1];
        }
        if (in_multiline_comment) {
            in_comment = true;
            current_color = comment_color;
        } else if (in_multiline_string) {
            in_string = true;
            current_color = string_color;
        }
    }
    
    int32_t line_length = line_text.length();
    
    for (int32_t j = 0; j < line_length; j++) {
        Dictionary highlighter_info;
        Color color = current_color;
        
        char32_t current_char = line_text[j];
        char32_t next_char = (j + 1 < line_length) ? line_text[j + 1] : 0;
        char32_t prev_char = (j > 0) ? line_text[j - 1] : 0;
        
        // Handle multiline comment end first
        if (in_multiline_comment) {
            color = comment_color;
            if (current_char == ']' && next_char == ']') {
                highlighter_info["color"] = color;
                color_map[j] = highlighter_info;
                j++; // Skip next ]
                Dictionary end_info;
                end_info["color"] = comment_color;
                color_map[j] = end_info;
                in_multiline_comment = false;
                in_comment = false;
                current_color = text_color;
                continue;
            }
            highlighter_info["color"] = color;
            color_map[j] = highlighter_info;
            continue;
        }
        
        // Handle multiline string end
        if (in_multiline_string && !in_comment) {
            color = string_color;
            if (current_char == ']' && next_char == ']') {
                highlighter_info["color"] = color;
                color_map[j] = highlighter_info;
                j++; // Skip next ]
                Dictionary end_info;
                end_info["color"] = string_color;
                color_map[j] = end_info;
                in_multiline_string = false;
                in_string = false;
                current_color = text_color;
                continue;
            }
            highlighter_info["color"] = color;
            color_map[j] = highlighter_info;
            continue;
        }
        
        // Handle regular string end
        if (in_string && !in_multiline_string && !in_comment) {
            color = string_color;
            // Check for escape sequence
            if (prev_char == '\\' && j > 1 && line_text[j - 2] != '\\') {
                highlighter_info["color"] = string_color;
                color_map[j] = highlighter_info;
                continue;
            }
            if (current_char == string_delimiter) {
                highlighter_info["color"] = string_color;
                color_map[j] = highlighter_info;
                in_string = false;
                current_color = text_color;
                continue;
            }
            highlighter_info["color"] = color;
            color_map[j] = highlighter_info;
            continue;
        }
        
        // Check for comment start
        if (!in_string && current_char == '-' && next_char == '-') {
            if (j + 3 < line_length && line_text[j + 2] == '[' && line_text[j + 3] == '[') {
                // Multiline comment --[[
                in_multiline_comment = true;
                in_comment = true;
                color = comment_color;
                for (int k = 0; k < 4 && j < line_length; k++, j++) {
                    Dictionary comment_info;
                    comment_info["color"] = comment_color;
                    color_map[j] = comment_info;
                }
                j--; // Adjust for loop increment
                current_color = comment_color;
                continue;

            } else if (j + 2 < line_length && line_text[j + 2] == '-') {
                // This is a --- comment, might be an annotation
                int comment_start = j;
                
                // First, color all three dashes
                Dictionary dash_info;
                dash_info["color"] = comment_color;
                color_map[j] = dash_info;
                color_map[j + 1] = dash_info;
                color_map[j + 2] = dash_info;
                
                j += 3;
                
                while (j < line_length && line_text[j] == ' ') {
                    Dictionary space_info;
                    space_info["color"] = comment_color;
                    color_map[j] = space_info;
                    j++;
                }
                
                if (j < line_length && line_text[j] == '@') {
                    int annotation_start = j;
                    j++;
                    
                    // Read the annotation name (letters only)
                    while (j < line_length && is_ascii_identifier_char(line_text[j])) {
                        j++;
                    }
                    
                    String annotation = line_text.substr(annotation_start, j - annotation_start);
                    
                    Color new_annotation_color = keyword_color;
                    
                    if (!type_color.is_equal_approx(Color(0, 0, 0, 0))) {
                        new_annotation_color = type_color;
                    }
                    else if (!function_definition_color.is_equal_approx(Color(0, 0, 0, 0))) {
                        new_annotation_color = function_definition_color;
                    }
                    else if (new_annotation_color.is_equal_approx(comment_color)) {
                        new_annotation_color = annotation_color;
                    }
                    
                    for (int k = annotation_start; k < j; k++) {
                        Dictionary annotation_info;
                        annotation_info["color"] = new_annotation_color;
                        color_map[k] = annotation_info;
                    }
                    
                    if (annotation == "@extends") {
                        // Skip any spaces after @extends
                        while (j < line_length && line_text[j] == ' ') {
                            Dictionary space_info;
                            space_info["color"] = comment_color;
                            color_map[j] = space_info;
                            j++;
                        }
                        
                        if (j < line_length && is_ascii_identifier_char(line_text[j])) {
                            int class_name_start = j;
                            
                            // Read the class name
                            while (j < line_length && is_ascii_identifier_char(line_text[j])) {
                                j++;
                            }
                            
                            // Apply type color to the class name
                            Color class_color = built_in_type_color;
                            if (class_color.is_equal_approx(Color(0, 0, 0, 0))) {
                                class_color = type_color;
                            }
                            if (class_color.is_equal_approx(Color(0, 0, 0, 0))) {
                                class_color = Color(0.4, 0.8, 0.8, 1.0); // Cyan fallback
                            }
                            
                            for (int k = class_name_start; k < j; k++) {
                                Dictionary class_info;
                                class_info["color"] = class_color;
                                color_map[k] = class_info;
                            }
                        }
                    }
                    
                    // Color the rest of the line as a comment
                    while (j < line_length) {
                        Dictionary rest_info;
                        rest_info["color"] = comment_color;
                        color_map[j] = rest_info;
                        j++;
                    }
                    
                    in_comment = true;
                    j--;
                    continue;
                } else {
                    // Not an annotation, just a --- comment
                    while (j < line_length) {
                        Dictionary rest_info;
                        rest_info["color"] = comment_color;
                        color_map[j] = rest_info;
                        j++;
                    }
                    in_comment = true;
                    j--;
                    continue;
                }
            } else {
                // Regular -- comment
                in_comment = true;
                color = comment_color;
                highlighter_info["color"] = color;
                color_map[j] = highlighter_info;
                current_color = comment_color;
                continue;
            }
        }
        
        // Check for string literals
        if (!in_string && !in_comment) {
            // Check for multiline string [[
            if (current_char == '[' && next_char == '[') {
                in_string = true;
                in_multiline_string = true;
                color = string_color;
                highlighter_info["color"] = color;
                color_map[j] = highlighter_info;
                j++; // Skip next [
                Dictionary bracket_info;
                bracket_info["color"] = string_color;
                color_map[j] = bracket_info;
                current_color = string_color;
                continue;
            }
            // Check for regular strings
            else if (current_char == '"' || current_char == '\'' || current_char == '`') {
                in_string = true;
                string_delimiter = current_char;
                color = string_color;
                highlighter_info["color"] = color;
                color_map[j] = highlighter_info;
                current_color = string_color;
                continue;
            }
        }
        
        // Check for numbers
        if (!in_string && !in_comment) {
            if (is_digit(current_char) || (current_char == '.' && j + 1 < line_length && is_digit(line_text[j + 1]))) {
                // Don't highlight numbers that are part of identifiers
                if (j > 0 && is_ascii_identifier_char(line_text[j - 1])) {
                    color = current_color;
                } else {
                    color = number_color;
                    int number_start = j;
                    
                    // Check for hex numbers (0x)
                    if (current_char == '0' && next_char == 'x') {
                        j += 2;
                        while (j < line_length && is_hex_digit(line_text[j])) {
                            j++;
                        }
                    }
                    // Check for binary numbers (0b)
                    else if (current_char == '0' && next_char == 'b') {
                        j += 2;
                        while (j < line_length && is_binary_digit(line_text[j])) {
                            j++;
                        }
                    }
                    // Regular number with optional decimal and exponent
                    else {
                        bool has_dot = false;
                        bool has_exp = false;
                        while (j < line_length) {
                            if (is_digit(line_text[j])) {
                                j++;
                            } else if (!has_dot && line_text[j] == '.') {
                                has_dot = true;
                                j++;
                            } else if (!has_exp && (line_text[j] == 'e' || line_text[j] == 'E')) {
                                has_exp = true;
                                j++;
                                if (j < line_length && (line_text[j] == '+' || line_text[j] == '-')) {
                                    j++;
                                }
                            } else {
                                break;
                            }
                        }
                    }
                    
                    // Color all number characters
                    for (int k = number_start; k < j; k++) {
                        Dictionary num_info;
                        num_info["color"] = number_color;
                        color_map[k] = num_info;
                    }
                    j--; // Adjust for loop increment
                    current_color = text_color;
                    continue;
                }
            }
        }
        
        // Check for identifiers and keywords
        if (!in_string && !in_comment && is_ascii_identifier_char(current_char) && !is_digit(current_char)) {
            int identifier_start = j;
            while (j < line_length && is_ascii_identifier_char(line_text[j])) {
                j++;
            }
            
            String identifier = line_text.substr(identifier_start, j - identifier_start);
            
            if (identifier == "self") {
                color = self_keyword_color;
            } else if (control_flow_keywords.has(identifier)) {
                color = control_flow_keyword_color;
            } else if (keywords.has(identifier)) {
                color = keyword_color;
            } else if (built_in_types.has(identifier)) {
                color = type_color;
            } else if (built_in_functions.has(identifier)) {
                color = global_function_color;
            } else if (is_constant_identifier(identifier)) {
                color = constant_color;
            } else {
                int next_non_space = j;
                while (next_non_space < line_length && line_text[next_non_space] == ' ') {
                    next_non_space++;
                }
                
                if (next_non_space < line_length && line_text[next_non_space] == '(') {
                    color = function_color;
                } else if (identifier_start > 0 && (line_text[identifier_start - 1] == '.' || line_text[identifier_start - 1] == ':')) {
                    color = member_variable_color;
                } else {
                    color = text_color;
                }
            }
            
            // Color all identifier characters
            for (int k = identifier_start; k < j; k++) {
                Dictionary id_info;
                id_info["color"] = color;
                color_map[k] = id_info;
            }
            j--; // Adjust for loop increment
            current_color = text_color;
            continue;
        }
        
        if (!in_string && !in_comment && is_symbol(current_char)) {
            color = symbol_color;
            highlighter_info["color"] = color;
            color_map[j] = highlighter_info;
            current_color = text_color;
            continue;
        }
        
        if (!color_map.has(j)) {
            highlighter_info["color"] = color;
            color_map[j] = highlighter_info;
        }
    }
    
    // Store multiline state for next line
    multiline_comment_state[p_line] = in_multiline_comment;
    multiline_string_state[p_line] = in_multiline_string;
    
    return color_map;
}

void LuauSyntaxHighlighter::_clear_highlighting_cache() {
    keywords.clear();
    control_flow_keywords.clear();
    built_in_types.clear();
    built_in_functions.clear();
    color_regions.clear();
}

void LuauSyntaxHighlighter::_update_cache() {
    Ref<EditorSettings> settings = nobind::EditorInterface::get_singleton()->get_editor_settings();
    
    settings->set("text_editor/theme/highlighting/background_color", Color("#1f1f1f"));
    
    text_color = Color("#9cdcfe");
    if (!settings->has_setting("text_editor/theme/highlighting/luauscript/text_color")) {
        settings->set("text_editor/theme/highlighting/luauscript/text_color", text_color);
    }
    text_color = settings->get_setting("text_editor/theme/highlighting/luauscript/text_color");

    symbol_color = Color("#C8C8C8");
    if (!settings->has_setting("text_editor/theme/highlighting/luauscript/symbol_color")) {
        settings->set("text_editor/theme/highlighting/luauscript/symbol_color", symbol_color);
    }
    symbol_color = settings->get_setting("text_editor/theme/highlighting/luauscript/symbol_color");

    number_color = Color("#b5cea8");
    if (!settings->has_setting("text_editor/theme/highlighting/luauscript/number_color")) {
        settings->set("text_editor/theme/highlighting/luauscript/number_color", number_color);
    }
    number_color = settings->get_setting("text_editor/theme/highlighting/luauscript/number_color");

    function_color = Color("#DCDCAA");
    if (!settings->has_setting("text_editor/theme/highlighting/luauscript/function_color")) {
        settings->set("text_editor/theme/highlighting/luauscript/function_color", function_color);
    }
    function_color = settings->get_setting("text_editor/theme/highlighting/luauscript/function_color");

    global_function_color = Color("#DCDCAA");
    if (!settings->has_setting("text_editor/theme/highlighting/luauscript/global_function_color")) {
        settings->set("text_editor/theme/highlighting/luauscript/global_function_color", global_function_color);
    }
    global_function_color = settings->get_setting("text_editor/theme/highlighting/luauscript/global_function_color");

    built_in_type_color = Color("#4EC9B0");
    if (!settings->has_setting("text_editor/theme/highlighting/luauscript/built_in_type_color")) {
        settings->set("text_editor/theme/highlighting/luauscript/built_in_type_color", built_in_type_color);
    }
    built_in_type_color = settings->get_setting("text_editor/theme/highlighting/luauscript/built_in_type_color");

    keyword_color = Color("#C586C0");
    if (!settings->has_setting("text_editor/theme/highlighting/luauscript/keyword_color")) {
        settings->set("text_editor/theme/highlighting/luauscript/keyword_color", keyword_color);
    }
    keyword_color = settings->get_setting("text_editor/theme/highlighting/luauscript/keyword_color");

    control_flow_keyword_color = Color("#b999ef");
    if (!settings->has_setting("text_editor/theme/highlighting/luauscript/control_flow_keyword_color")) {
        settings->set("text_editor/theme/highlighting/luauscript/control_flow_keyword_color", control_flow_keyword_color);
    }
    control_flow_keyword_color = settings->get_setting("text_editor/theme/highlighting/luauscript/control_flow_keyword_color");

    type_color = Color("#4EC9B0");
    if (!settings->has_setting("text_editor/theme/highlighting/luauscript/type_color")) {
        settings->set("text_editor/theme/highlighting/luauscript/type_color", type_color);
    }
    type_color = settings->get_setting("text_editor/theme/highlighting/luauscript/type_color");

    string_color = Color("#ce9178");
    if (!settings->has_setting("text_editor/theme/highlighting/luauscript/string_color")) {
        settings->set("text_editor/theme/highlighting/luauscript/string_color", string_color);
    }
    string_color = settings->get_setting("text_editor/theme/highlighting/luauscript/string_color");

    constant_color = Color("#569cd6");
    if (!settings->has_setting("text_editor/theme/highlighting/luauscript/constant_color")) {
        settings->set("text_editor/theme/highlighting/luauscript/constant_color", constant_color);
    }
    constant_color = settings->get_setting("text_editor/theme/highlighting/luauscript/constant_color");

    self_keyword_color = Color("#7c9ed1");
    if (!settings->has_setting("text_editor/theme/highlighting/luauscript/self_keyword_color")) {
        settings->set("text_editor/theme/highlighting/luauscript/self_keyword_color", self_keyword_color);
    }
    self_keyword_color = settings->get_setting("text_editor/theme/highlighting/luauscript/self_keyword_color");

    comment_color = Color("#6A9955");
    if (!settings->has_setting("text_editor/theme/highlighting/luauscript/comment_color")) {
        settings->set("text_editor/theme/highlighting/luauscript/comment_color", comment_color);
    }
    comment_color = settings->get_setting("text_editor/theme/highlighting/luauscript/comment_color");

    member_variable_color = Color("#9cdcfe");
    if (!settings->has_setting("text_editor/theme/highlighting/luauscript/member_variable_color")) {
        settings->set("text_editor/theme/highlighting/luauscript/member_variable_color", member_variable_color);
    }
    member_variable_color = settings->get_setting("text_editor/theme/highlighting/luauscript/member_variable_color");

    function_definition_color = Color("#DCDCAA");
    if (!settings->has_setting("text_editor/theme/highlighting/luauscript/function_definition_color")) {
        settings->set("text_editor/theme/highlighting/luauscript/function_definition_color", function_definition_color);
    }
    function_definition_color = settings->get_setting("text_editor/theme/highlighting/luauscript/function_definition_color");

    annotation_color = Color("#7eb366ff");
    if (!settings->has_setting("text_editor/theme/highlighting/luauscript/annotation_color")) {
        settings->set("text_editor/theme/highlighting/luauscript/annotation_color", annotation_color);
    }
    annotation_color = settings->get_setting("text_editor/theme/highlighting/luauscript/annotation_color");

    // Initialize Luau keywords
    keywords.clear();
    keywords.insert("and");
    keywords.insert("do");
    keywords.insert("else");
    keywords.insert("elseif");
    keywords.insert("end");
    keywords.insert("for");
    keywords.insert("function");
    keywords.insert("if");
    keywords.insert("in");
    keywords.insert("local");
    keywords.insert("nil");
    keywords.insert("not");
    keywords.insert("or");
    keywords.insert("repeat");
    keywords.insert("then");
    keywords.insert("until");
    keywords.insert("while");
    keywords.insert("export");
    keywords.insert("type");
    keywords.insert("typeof");
    
    // Control flow keywords (subset of keywords with special coloring)
    control_flow_keywords.clear();
    control_flow_keywords.insert("break");
    control_flow_keywords.insert("continue");
    control_flow_keywords.insert("return");
    control_flow_keywords.insert("false");
    control_flow_keywords.insert("true");
    
    // Luau built-in types
    built_in_types.clear();
    built_in_types.insert("any");
    built_in_types.insert("boolean");
    built_in_types.insert("number");
    built_in_types.insert("string");
    built_in_types.insert("thread");
    built_in_types.insert("table");
    built_in_types.insert("userdata");
    built_in_types.insert("vector");
    built_in_types.insert("buffer");
    
    // Add all Godot registered classes
    PackedStringArray class_list = nobind::ClassDB::get_singleton()->get_class_list();
    for (const String &class_name : class_list) {
        built_in_types.insert(class_name);
    }

    // Common Luau built-in functions
    built_in_functions.clear();
    built_in_functions.insert("assert");
    built_in_functions.insert("collectgarbage");
    built_in_functions.insert("error");
    built_in_functions.insert("getfenv");
    built_in_functions.insert("getmetatable");
    built_in_functions.insert("ipairs");
    built_in_functions.insert("load");
    built_in_functions.insert("loadstring");
    built_in_functions.insert("next");
    built_in_functions.insert("pairs");
    built_in_functions.insert("pcall");
    built_in_functions.insert("print");
    built_in_functions.insert("rawequal");
    built_in_functions.insert("rawget");
    built_in_functions.insert("rawset");
    built_in_functions.insert("require");
    built_in_functions.insert("select");
    built_in_functions.insert("setfenv");
    built_in_functions.insert("setmetatable");
    built_in_functions.insert("tonumber");
    built_in_functions.insert("tostring");
    built_in_functions.insert("type");
    built_in_functions.insert("unpack");
    built_in_functions.insert("xpcall");
    built_in_functions.insert("warn");
    
    // Godot-specific functions that might be exposed
    built_in_functions.insert("load");
    built_in_functions.insert("preload");
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

// Helper method implementations
bool LuauSyntaxHighlighter::is_symbol(char32_t c) const {
    return (c >= '!' && c <= '/') || 
           (c >= ':' && c <= '@') || 
           (c >= '[' && c <= '`') || 
           (c >= '{' && c <= '~');
}

bool LuauSyntaxHighlighter::is_ascii_identifier_char(char32_t c) const {
    return (c >= 'a' && c <= 'z') || 
           (c >= 'A' && c <= 'Z') || 
           (c >= '0' && c <= '9') || 
           c == '_';
}

bool LuauSyntaxHighlighter::is_hex_digit(char32_t c) const {
    return (c >= '0' && c <= '9') || 
           (c >= 'a' && c <= 'f') || 
           (c >= 'A' && c <= 'F');
}

bool LuauSyntaxHighlighter::is_digit(char32_t c) const {
    return c >= '0' && c <= '9';
}

bool LuauSyntaxHighlighter::is_binary_digit(char32_t c) const {
    return c == '0' || c == '1';
}

bool LuauSyntaxHighlighter::is_constant_identifier(const String& identifier) const {
    if (identifier.is_empty()) {
        return false;
    }
    
    // Check if all characters are uppercase letters, digits, or underscores
    // and at least one uppercase letter exists
    bool has_uppercase = false;
    for (int i = 0; i < identifier.length(); i++) {
        char32_t c = identifier[i];
        if (c >= 'A' && c <= 'Z') {
            has_uppercase = true;
        } else if (c >= 'a' && c <= 'z') {
            // Has lowercase letter, not a constant
            return false;
        } else if (c != '_' && !(c >= '0' && c <= '9')) {
            // Has character that's not underscore or digit
            return false;
        }
    }
    
    return has_uppercase;
}
