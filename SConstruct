#!/usr/bin/env python

env = SConscript("extern/godot-cpp/SConstruct") # type: ignore

# Add Luau include paths
luau_dir = "extern/luau/"
luau_subdirs = ["Ast", "VM", "Compiler", "CodeGen", "Common"]

luau_includes = [luau_dir + subdir + "/include" for subdir in luau_subdirs]
env.Append(CPPPATH=luau_includes)

# Build Luau static libraries
luau_env = env.Clone()

luau_env.Append(CPPPATH=luau_includes)
luau_env.Append(CPPPATH=[luau_dir + subdir + "/src" for subdir in luau_subdirs])

luau_ast = luau_env.StaticLibrary(
    "luauast",
    luau_env.Glob("extern/luau/Ast/src/*.cpp")
)

luau_vm = luau_env.StaticLibrary(
    "luauvm",
    luau_env.Glob("extern/luau/VM/src/*.cpp")
)

luau_compiler = luau_env.StaticLibrary(
    "luaucompiler",
    luau_env.Glob("extern/luau/Compiler/src/*.cpp")
)

luau_codegen = luau_env.StaticLibrary(
    "luaucodegen",
    luau_env.Glob("extern/luau/CodeGen/src/*.cpp")
)

# Add existing paths
env.Append(CPPPATH=["src/"])
sources = env.Glob("src/*.cpp")
sources += env.Glob("src/editor/*.cpp")
sources += env.Glob("src/luauscript/*.cpp")

# Get the godot-cpp library path
godot_cpp_lib = "extern/godot-cpp/bin/libgodot-cpp"
if env["platform"] == "windows":
    godot_cpp_lib += ".windows"
godot_cpp_lib += "." + env["target"] + "." + env["arch"] + env["LIBSUFFIX"]

# Define output directory
output_dir = "Z:/Workspace/Dev/K/LuauDev/bin/LuauGDExt/"

# Link against Luau libraries
library = env.SharedLibrary(
    output_dir + "LuauGDExt{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
    source=sources,
    LIBS=[luau_ast, luau_vm, luau_compiler, luau_codegen, env.File(godot_cpp_lib)]
)

# Copy the LuauGDExt.gdextension file
gdextension_copy = env.Command(
    output_dir + "LuauGDExt.gdextension",
    "LuauGDExt.gdextension",
    Copy("$TARGET", "$SOURCE") # type: ignore
)

# Copy the LuauScript.svg file
gdext_icon_copy = env.Command(
    output_dir + "LuauScript.svg",
    "LuauScript.svg",
    Copy("$TARGET", "$SOURCE") # type: ignore
)

env.Depends(library, gdextension_copy)
env.Depends(library, gdext_icon_copy)
env.Default(library)
