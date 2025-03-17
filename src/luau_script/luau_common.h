
#include <luau_script/luauscript.h>

using namespace godot;

namespace LuauCommon {
    static const String get_name() {
        return "Luau";
    }
    
    static const String get_primary_extension() {
        return "luau";
    }

    static bool is_luau_file(const String &path) {
        String ext = path.get_extension().to_lower();
        return ext == "luau" || ext == "lua";
    }
    
    static PackedStringArray get_extensions() {
        PackedStringArray exts;
        exts.append("luau");
        exts.append("lua");
        return exts;
    }
}
