#!/usr/bin/env python
import os
env = SConscript("extern/godot-cpp/SConstruct") # type: ignore

if 'SCONS_CACHE' in os.environ:
    env.CacheDir(os.environ['SCONS_CACHE'])

if env["platform"] == "windows":
    env.Append(CXXFLAGS=["/EHsc"])
    env.Append(LINKFLAGS=["/ignore:4099"])
elif env["platform"] == "linux":
    env.Append(CCFLAGS=["-fexceptions"])
    
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
sources += env.Glob("src/luauscript/*.cpp")
sources += env.Glob("src/luauscript/variant/*.cpp")

# Get the godot-cpp library path
print(f"target={env['target']}") # editor
print(f"arch={env['arch']}") # x86_64
print(f"LIBSUFFIX={env['LIBSUFFIX']}") #.lib / .a
print(f"suffix={env['suffix']}") # .windows.editor.dev.x86_64 / .linux.editor.dev.x86_64
print(f"SHLIBSUFFIX={env['SHLIBSUFFIX']}") #.dll / .so

godot_cpp_lib = "extern/godot-cpp/bin/libgodot-cpp"
godot_cpp_lib += env['suffix'] + env["LIBSUFFIX"]
godot_cpp_file = env.File(godot_cpp_lib)


# Build with doctest tests
run_tests = ARGUMENTS.get("tests", "false").lower() == "true"
if run_tests:
    print("Building with DOCTEST enabled...")
    env.Append(CPPPATH=["tests/"])
    sources += env.Glob("tests/*.cpp")


# Define output directory
output_dir = "./demo/bin/LuauGDExt/" #Z:/Workspace/Dev/K/LuauDev/bin/LuauGDExt/

# Link against Luau libraries
library = env.SharedLibrary(
    output_dir + "LuauGDExt{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
    source = sources,
    LIBS = [godot_cpp_file, luau_codegen, luau_compiler, luau_vm, luau_ast]
)

# Copy the LuauGDExt.gdextension file
gdextension_copy = env.Command(
    output_dir + "LuauGDExt.gdextension",
    "LuauGDExt.gdextension",
    Copy("$TARGET", "$SOURCE")
)

# Copy the LuauScript.svg file
gdext_icon_copy = env.Command(
    output_dir + "LuauScript.svg",
    "LuauScript.svg",
    Copy("$TARGET", "$SOURCE")
)

env.Depends(library, gdextension_copy)
env.Depends(library, gdext_icon_copy)
env.Default(library)