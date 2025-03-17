#pragma once

#include <luau_script/luauscript.h>

#include <godot_cpp/classes/resource_format_saver.hpp>
#include <godot_cpp/classes/resource_format_loader.hpp>

using namespace godot;

// MARK: LuauScriptLoader
class LuauScriptLoader : public ResourceFormatLoader {
	GDCLASS(LuauScriptLoader, ResourceFormatLoader)

    static LuauScriptLoader *singleton;

public:
    static LuauScriptLoader *get_singleton();

	PackedStringArray _get_recognized_extensions() const override;
	bool _recognize_path(const String &p_path, const StringName &p_type) const override;
	bool _handles_type(const StringName &p_type) const override;
	String _get_resource_type(const String &p_path) const override;
	String _get_resource_script_class(const String &p_path) const override;
	int64_t _get_resource_uid(const String &p_path) const override;
	// PackedStringArray _get_dependencies(const String &p_path, bool p_add_types) const override;
	// Error _rename_dependencies(const String &p_path, const Dictionary &p_renames) const override;
	// bool _exists(const String &p_path) const override;
	// PackedStringArray _get_classes_used(const String &p_path) const override;
	Variant _load(const String &p_path, const String &p_original_path, bool p_use_sub_threads, int32_t p_cache_mode) const override;

	LuauScriptLoader();
	~LuauScriptLoader();

protected:
    static void _bind_methods() {}
};

// MARK: LuauScriptSavor
class LuauScriptSaver : public ResourceFormatSaver {
	GDCLASS(LuauScriptSaver, ResourceFormatSaver)

    static LuauScriptSaver *singleton;

public:
    static LuauScriptSaver *get_singleton();

	Error _save(const Ref<Resource> &p_resource, const String &p_path, uint32_t p_flags) override;
	Error _set_uid(const String &p_path, int64_t p_uid) override;
	bool _recognize(const Ref<Resource> &p_resource) const override;
	PackedStringArray _get_recognized_extensions(const Ref<Resource> &p_resource) const override;
	bool _recognize_path(const Ref<Resource> &p_resource, const String &p_path) const override;
    
	LuauScriptSaver();
	~LuauScriptSaver();
	
protected:
    static void _bind_methods() {}
};