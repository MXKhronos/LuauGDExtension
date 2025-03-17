#!/usr/bin/env python
import os

env = SConscript("extern/godot-cpp/SConstruct")

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# tweak this if you want to use different folders, or more folders, to store your source code in.
env.Append(CPPPATH=["src/"])
sources = Glob("src/*.cpp")
sources += Glob("src/**/*.cpp")

# Define output directory
extTitle = "LuauGDExtension"
output_dir = f"Y:/Repo/luau_gamedemo/addons/{extTitle}/"

library = env.SharedLibrary(
    output_dir + extTitle +"{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
    source=sources,
)

# Copy the .gdextension file to the library's directory
gdextension_copy = env.Command(
    output_dir + f"{extTitle}.gdextension",
    f"{extTitle}.gdextension",
    Copy("$TARGET", "$SOURCE")
)
# Make sure the gdextension file gets copied
Depends(library, gdextension_copy)

Default(library)
