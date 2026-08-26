#ifndef LUAU_BRIDGE_H
#define LUAU_BRIDGE_H

#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/templates/hash_map.hpp>

#include <lua.h>
#include <lualib.h>

#include <luauscript/luau_script.h>

namespace godot {
    class LuauScriptInstance;


const int UINT64_T_TAG = 1;

class LuauObject {
    friend class LuauScriptInstance;

    static HashMap<uint64_t, LuauObject*> list;

public:
    uint64_t obj_id;
    LuauScriptInstance* instance;

    operator Object*() {
        return ObjectDB::get_instance(obj_id);
    }

    void push_self(lua_State *p_M) {
        lua_State* L = instance->get_main_state();
        lua_getref(L, instance->get_self_ref());

        if (lua_isnil(L, -1)) {
            WARN_PRINT("self ref no longer exist?");
        }

        lua_xmove(L, p_M, 1);
    }

    static void register_object(Object* p_obj_ptr, LuauScriptInstance* p_scr_instance) {
        uint64_t obj_id = p_obj_ptr->get_instance_id();

        if (list.has(obj_id) && list[obj_id] != nullptr) {
            memdelete(list[obj_id]);
        }

        list[obj_id] = memnew(LuauObject(obj_id, p_scr_instance));
    }

    static void unregister_object(Object* p_obj_ptr) {
        uint64_t obj_id = p_obj_ptr->get_instance_id();
        if (!list.has(obj_id)) return;

        LuauObject* luau_obj = list[obj_id];
        if (luau_obj != nullptr) {
            memdelete(luau_obj);
        } else {
            list.erase(obj_id);
        }
    }

    static LuauObject* get_luau_object(Object* p_obj_ptr) {
        uint64_t obj_id = p_obj_ptr->get_instance_id();
        return list[obj_id];
    }

    static LuauObject* get_luau_object(uint64_t obj_id) {
        if (!list.has(obj_id)) return nullptr;
        return list[obj_id];
    }

    LuauObject(uint64_t p_obj_id, LuauScriptInstance* p_scr_instance);
    ~LuauObject();
};

//MARK: LuauBridge
class LuauBridge {
    public:
        static void *luaL_checkudata(lua_State *L, int p_index, const char *p_tname);
        static void *luaL_testudata(lua_State *L, int p_index, const char *p_tname);

        static void push_uint64_t(lua_State* L, const uint64_t &p_uint64_t);
        static void push_string(lua_State *L, const godot::String &p_str);
        static void push_dictionary(lua_State *L, const Dictionary &p_dict);
        static void push_array(lua_State *L, const Array &p_array);
        static void push_variant(lua_State *L, const Variant &p_var);

        static uint64_t get_uint64_t(lua_State* L, int p_index);
        static godot::String get_string(lua_State *L, int p_index);
        static Dictionary get_dictionary(lua_State *L, int p_index);
        static Array get_array(lua_State *L, int p_index);
        static Variant get_variant(lua_State *L, int p_index);

        static void protect_metatable(lua_State* thread, int index);
};

//MARK: Property name remapping
StringName resolve_prop_name(lua_State* L, const char* p_key);


//MARK: VariantBridge
template<class GDV, bool __eq = true>
class VariantBridge {
public:
    static const char* variant_name;

    static GDV* push_new(lua_State* L) {
        GDV* ud = (GDV*)lua_newuserdata(L, sizeof(GDV));
        new (ud) GDV();

        luaL_getmetatable(L, variant_name);
        lua_setmetatable(L, -2);

        return ud;
    }

    static GDV* push_from(lua_State* L, const Variant& v) {
        GDV* ud = (GDV*)lua_newuserdata(L, sizeof(GDV));
        new (ud) GDV(v.operator GDV());

        luaL_getmetatable(L, variant_name);
        lua_setmetatable(L, -2);

        return ud;
    }

    static GDV& get_object(lua_State* L, unsigned int index) {
        void *ud = LuauBridge::luaL_checkudata(L, index, variant_name);

        if (!ud) {
            luaL_error(L, "Invalid userdata");
        }

        return *reinterpret_cast<GDV*>(ud);
    }

    static void register_variant(lua_State* L);


    static int on_index(lua_State* L, const GDV& object, const char* key);
    static int on_newindex(lua_State* L, const GDV& object, const char* key);
    static int on_call(lua_State* L, bool& is_valid);

    static int on_gc(lua_State *L) {
		get_object(L, 1).~GDV();
		return 0;
	}

	static int on_tostring(lua_State *L) {
	    void *ud = LuauBridge::luaL_testudata(L, 1, "Object");
        if (ud) {
            uint64_t obj_id = *(uint64_t*)ud;
            Object* obj = ObjectDB::get_instance(obj_id);
            
            if (obj != nullptr) {
                LuauBridge::push_string(L, obj->to_string());
            } else {
                LuauBridge::push_string(L, "nil");
            }
            return 1;
        }

        Variant value = get_object(L, 1);
        LuauBridge::push_string(L, value.stringify());
        return 1;
	}

    // MARK: Variant __index
	static int on_index(lua_State *L) {
		const char* key = lua_tostring(L, 2);

        if (lua_istable(L, 1)) {
            WARN_PRINT(vformat("Variant on_index table key=%s", key));

            void *p = lua_touserdata(L, 1);
            if (p == NULL) {
                WARN_PRINT(vformat("Raw table NULL"));
            }
        }

        void *ud = LuauBridge::luaL_testudata(L, 1, "Object");
        if (ud) {
            uint64_t obj_id = *(uint64_t*)ud;
            Object* obj = ObjectDB::get_instance(obj_id);
            
            if (obj != nullptr) {
                return VariantBridge<Object*>::on_index(L, obj, key); 
            } else {
                lua_pushnil(L);
                return 1;
            }
        }


        // Variant::OBJECT should never reach here;
        Variant variant = get_object(L, 1);

        StringName prop_name = resolve_prop_name(L, key);

        bool valid;
        Variant value = variant.get(prop_name, &valid); //Get Variant GDV property

        // WARN_PRINT(vformat("value (%s) is: %s (%s) valid: %s", prop_name, value, value.get_type_name(value.get_type()), (valid? "true" : "false")));
        if (!valid) {
            lua_getglobal(L, variant_name);
            if (lua_isnil(L, -1)) {
                return 1;
            }

            lua_pushstring(L, String(prop_name).utf8().get_data());
            lua_rawget(L, -2);
            if (!lua_isnil(L, -1)) {
                // Pop global table
                lua_remove(L, -2);
                
                return 1;
            }
            lua_pop(L, 2); // Pop nil and global table

            luaL_error(L, ("Invalid property: " + String(variant_name) + "." + String(key)).utf8().get_data());
            return 1;
        }
        
        if (value.get_type() == Variant::CALLABLE) {
            // obj.prop_name is a method

            lua_pushstring(L, String(prop_name).utf8().get_data());
            lua_pushcclosure(L, [](lua_State *L) -> int {
                const char* key = lua_tostring(L, lua_upvalueindex(1));
                Variant obj = get_object(L, 1);

                StringName method_name(key);

                if (!obj.has_method(method_name)) {
                    luaL_error(L, vformat("Object does not have method: %s", method_name).utf8().get_data());
                    return 1;
                }

                const int argc = lua_gettop(L) -1;
                Variant* var_buffer = (Variant*)memalloc(sizeof(Variant) * argc);
                const Variant** ptrs = (const Variant**)memalloc(sizeof(Variant*) * argc);
                for (int i = 0; i < argc; i++) {
                    Variant v = LuauBridge::get_variant(L, i + 2);
                    new (&var_buffer[i]) Variant(v);
                    ptrs[i] = &var_buffer[i];
                }

                Variant result;
                GDExtensionCallError error;
                obj.callp(method_name, ptrs, argc, result, error);
                if (error.error != GDEXTENSION_CALL_OK) {
                    GDExtensionCallErrorType error_type = error.error;
                    switch (error_type) {
                        case GDEXTENSION_CALL_ERROR_INVALID_METHOD: {
                            luaL_error(L, vformat("Object does not have method: %s", method_name).utf8().get_data());
                            break;
                        };
                        case GDEXTENSION_CALL_ERROR_INVALID_ARGUMENT: {
                            luaL_error(L, vformat("Invalid argument for method: %s", method_name).utf8().get_data());
                            break;
                        };
                        case GDEXTENSION_CALL_ERROR_TOO_FEW_ARGUMENTS: {
                            luaL_error(L, vformat("Too few arguments for method: %s, expected at least %s, got %s", method_name, error.argument, argc).utf8().get_data());
                            break;
                        };
                        case GDEXTENSION_CALL_ERROR_TOO_MANY_ARGUMENTS: {
                            luaL_error(L, vformat("Too many arguments for method: %s, expected %s, got %s", method_name, error.argument, argc).utf8().get_data());
                            break;
                        };
                        case GDEXTENSION_CALL_ERROR_METHOD_NOT_CONST: {
                            luaL_error(L, vformat("Method is not const: %s", method_name).utf8().get_data());
                            break;
                        };
                        default: {
                            luaL_error(L, vformat("Failed to call method(%s), Unkown error.", method_name).utf8().get_data());
                            break;
                        };
                    }
                    
                    return 1;
                }
                LuauBridge::push_variant(L, result);

                // Free the memory
                for (int i = 0; i < argc; i++) {
                    var_buffer[i].~Variant();
                }
                memfree(var_buffer);
                memfree(ptrs);

                return 1;
            }, key, 1);

            return 1;

        } else if (value.get_type() != Variant::NIL) {
            // get variant property
            LuauBridge::push_variant(L, value);
            return 1;

        }

        return on_index(L, variant, key);
	}

	static int on_newindex(lua_State *L) {
        const char* key = lua_tostring(L, 2);

        void *obj_ud = LuauBridge::luaL_testudata(L, 1, "Object");
        if (obj_ud) {
            uint64_t obj_id = *(uint64_t*)obj_ud;
            Object* resolved = ObjectDB::get_instance(obj_id);
            if (resolved != nullptr) {
                return VariantBridge<Object*>::on_newindex(L, resolved, key);
            }
            WARN_PRINT("Cannot assign to a freed object");
            return 0;
        }

        Variant obj = get_object(L, 1);

        if (obj.get_type() == Variant::OBJECT) {
            return on_newindex(L, obj, key);
        }

        Variant value = LuauBridge::get_variant(L, 3);
        StringName prop_name = resolve_prop_name(L, key);

        // Try to set the property
        bool valid;
        obj.set(prop_name, value, &valid);
        if (!valid) {
            WARN_PRINT("Failed to set property: " + String(key));
            return 1;
        }

        // Get, modify, set
        void* ud = lua_touserdata(L, 1);
        if (ud) {
            GDV* ptr = static_cast<GDV*>(ud);
            *ptr = obj;
        }

        return 0;
	}

    static int on_call(lua_State *L) {
        bool is_valid = true;

        on_call(L, is_valid);

        if (!is_valid) {
            Variant s = GDV();
            String v_name = Variant::get_type_name(s.get_type());
            WARN_PRINT("Invalid constructor for: " + v_name);

            //throw error invalid constructor
            const int argc = lua_gettop(L)-1;

            Array arg_types;

            for (int i = 0; i < argc; i++) {
                Variant v = LuauBridge::get_variant(L, i + 2);
                arg_types.push_back(Variant::get_type_name(v.get_type()));
            }

            String signature = String("{0}({1})").format(Array::make(v_name, String(",").join(arg_types)));
            luaL_error(L, vformat("No constructor of %s matches the signature %s", v_name, signature).utf8().get_data());
        }

        return 1;
    }

    static int on_eq(lua_State *L) {
        Variant v1 = LuauBridge::get_variant(L, 1);
        Variant v2 = LuauBridge::get_variant(L, 2);

        Variant result;
        bool valid;
        Variant::evaluate(Variant::Operator::OP_EQUAL, v1, v2, result, valid);

        if (!valid) {
            luaL_error(L, "No equality operator for types: %s and %s", Variant::get_type_name(v1.get_type()), Variant::get_type_name(v2.get_type()));
            return 1;
        }
        LuauBridge::push_variant(L, result);
            
        return 1;
    }

    static int on_add(lua_State *L) {
        Variant v1 = LuauBridge::get_variant(L, 1);
        Variant v2 = LuauBridge::get_variant(L, 2);

        Variant result;
        bool valid;
        Variant::evaluate(Variant::Operator::OP_ADD, v1, v2, result, valid);

        if (!valid) {
            luaL_error(L, "No addition operator for types: %s and %s", Variant::get_type_name(v1.get_type()), Variant::get_type_name(v2.get_type()));
            return 1;
        }
        LuauBridge::push_variant(L, result);
            
        return 1;
    }

    static int on_sub(lua_State *L) {
        Variant v1 = LuauBridge::get_variant(L, 1);
        Variant v2 = LuauBridge::get_variant(L, 2);

        Variant result;
        bool valid;
        Variant::evaluate(Variant::Operator::OP_SUBTRACT, v1, v2, result, valid);

        if (!valid) {
            luaL_error(L, "No subtraction operator for types: %s and %s",
                Variant::get_type_name(v1.get_type()), 
                Variant::get_type_name(v2.get_type())
            );
            return 1;
        }
        LuauBridge::push_variant(L, result);
            
        return 1;
    }

    static int on_mul(lua_State *L) {
        Variant v1 = LuauBridge::get_variant(L, 1);
        Variant v2 = LuauBridge::get_variant(L, 2);

        Variant result;
        bool valid;
        Variant::evaluate(Variant::Operator::OP_MULTIPLY, v1, v2, result, valid);

        if (!valid) {
            luaL_error(L, "No multiplication operator for types: %s and %s", Variant::get_type_name(v1.get_type()), Variant::get_type_name(v2.get_type()));
            return 1;
        }
        LuauBridge::push_variant(L, result);
            
        return 1;
    }

    static int on_div(lua_State *L) {
        Variant v1 = LuauBridge::get_variant(L, 1);
        Variant v2 = LuauBridge::get_variant(L, 2);

        Variant result;
        bool valid;
        Variant::evaluate(Variant::Operator::OP_DIVIDE, v1, v2, result, valid);

        if (!valid) {
            luaL_error(L, "No division operator for types: %s and %s", Variant::get_type_name(v1.get_type()), Variant::get_type_name(v2.get_type()));
            return 1;
        }
        LuauBridge::push_variant(L, result);
            
        return 1;
    }

    static int on_mod(lua_State *L) {
        Variant v1 = LuauBridge::get_variant(L, 1);
        Variant v2 = LuauBridge::get_variant(L, 2);

        Variant result;
        bool valid;
        Variant::evaluate(Variant::Operator::OP_MODULE, v1, v2, result, valid);

        if (!valid) {
            luaL_error(L, "No modulo operator for types: %s and %s", Variant::get_type_name(v1.get_type()), Variant::get_type_name(v2.get_type()));
            return 1;
        }
        LuauBridge::push_variant(L, result);
            
        return 1;
    }

    static int on_pow(lua_State *L) {
        Variant v1 = LuauBridge::get_variant(L, 1);
        Variant v2 = LuauBridge::get_variant(L, 2);

        Variant result;
        bool valid;
        Variant::evaluate(Variant::Operator::OP_POWER, v1, v2, result, valid);

        if (!valid) {
            luaL_error(L, "No power operator for types: %s and %s", Variant::get_type_name(v1.get_type()), Variant::get_type_name(v2.get_type()));
            return 1;
        }
        LuauBridge::push_variant(L, result);
            
        return 1;
    }


};

}; // namespace godot

#endif // LUAU_BRIDGE_H












