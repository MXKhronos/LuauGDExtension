#ifndef LUAUSCRIPT_RESOURCE_FORMAT_H
#define LUAUSCRIPT_RESOURCE_FORMAT_H 

#include <godot_cpp/classes/resource_format_loader.hpp>
#include <godot_cpp/classes/resource_format_saver.hpp>
#include "luau_constants.h"

namespace godot {

    class ResourceFormatLoaderLuau : public ResourceFormatLoader {
        GDCLASS(ResourceFormatLoaderLuau, ResourceFormatLoader);

    protected:
        static void _bind_methods() {}

    public:
        static String get_resource_type(const String &p_path);

        bool _recognize_path(const String &p_path, const StringName &p_type) const;
        PackedStringArray _get_recognized_extensions() const override;
        bool _handles_type(const StringName &p_type) const override;
        String _get_resource_type(const String &p_path) const override;
        Variant _load(const String &p_path, const String &p_original_path, bool p_use_sub_threads, int32_t p_cache_mode) const override;
        // virtual String _get_resource_script_class(const String &p_path) const;
        // virtual int64_t _get_resource_uid(const String &p_path) const;
        // virtual PackedStringArray _get_dependencies(const String &p_path, bool p_add_types) const;
        // virtual Error _rename_dependencies(const String &p_path, const Dictionary &p_renames) const;
        // virtual bool _exists(const String &p_path) const;
        // virtual PackedStringArray _get_classes_used(const String &p_path) const;
    };


    class ResourceFormatSaverLuau : public ResourceFormatSaver {
        GDCLASS(ResourceFormatSaverLuau, ResourceFormatSaver);

    protected:
        static void _bind_methods() {};
        
    public:
        Error _save(const Ref<Resource> &p_resource, const String &p_path, uint32_t p_flags) override;
        //Error _set_uid(const String &p_path, int64_t p_uid) override;
        bool _recognize(const Ref<Resource> &p_resource) const override;
        PackedStringArray _get_recognized_extensions(const Ref<Resource> &p_resource) const override;
        bool _recognize_path(const Ref<Resource> &p_resource, const String &p_path) const override;
    };

}

#endif
