// doctest-based tests for the Luau GDExtension.

// Filtering examples (add them to the editor command line):
//   --run-extension-tests -ts="Script instances"
//   --run-extension-tests -ts=Benchmark
//   --run-extension-tests -ts=Benchmark=false   (exclude benchmarks)

#include "doctest.h"

#include "nobind.h"

#include <godot_cpp/godot.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <chrono>
#include <cmath>
#include <cstring>
#include <string>

#include <lua.h>
#include <lualib.h>

#include "Luau/Compiler.h"

#include "luauscript/luau_engine.h"
#include "luauscript/luau_script.h"
#include "luauscript/luau_bridge.h"
#include "luauscript/luau_cache.h"

using namespace godot;

namespace {

Ref<LuauScript> make_script(const String &p_source) {
	Ref<LuauScript> scr;
	scr.instantiate();
	scr->set_source_code(p_source);
	return scr;
}

uint64_t now_usec() {
	return uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(
							std::chrono::steady_clock::now().time_since_epoch())
							.count());
}

void report_bench(const char *p_name, int64_t p_ops, uint64_t p_usec_total) {
	const double us_per_op = double(p_usec_total) / double(p_ops);
	const double ops_per_sec = us_per_op > 0.0 ? 1.0e6 / us_per_op : 0.0;
	UtilityFunctions::print(
			"[Benchmark] ", p_name,
			" | ops=", int64_t(p_ops),
			" | total_ms=", p_usec_total / 1000.0,
			" | us_per_op=", us_per_op,
			" | ops_per_sec=", ops_per_sec);
}

//like loadstring
bool lua_run_chunk(lua_State *L, const char *p_source, int p_nresults = 0) {
	std::string bytecode = Luau::compile(p_source);
	if (luau_load(L, "=test_chunk", bytecode.c_str(), bytecode.size(), 0) != 0) {
		lua_pop(L, 1); // error message
		return false;
	}
	if (lua_pcall(L, 0, p_nresults, 0) != 0) {
		lua_pop(L, 1); // error message
		return false;
	}
	return true;
}

} // namespace

// ==============================================================================
// Core singletons
// ==============================================================================
TEST_SUITE("Core singletons")
{
	TEST_CASE("Engine singleton is available") {
		Engine *engine = nobind::Engine::get_singleton();
		REQUIRE(engine != nullptr);
	}

	TEST_CASE("LuauLanguage singleton is available") {
		LuauLanguage *luau_lang = LuauLanguage::get_singleton();
		REQUIRE(luau_lang != nullptr);
	}

	TEST_CASE("LuauEngine singleton is available") {
		LuauEngine *luau = LuauEngine::get_singleton();
		REQUIRE(luau != nullptr);
	}

	TEST_CASE("LuauCache singleton is available") {
		LuauCache *cache = LuauCache::get_singleton();
		REQUIRE(cache != nullptr);
	}
}

// ==============================================================================
// VM infrastructure
// ==============================================================================
TEST_SUITE("VM infrastructure")
{
	TEST_CASE("All VM states are created") {
		LuauEngine *luau = LuauEngine::get_singleton();
		REQUIRE(luau != nullptr);

		CHECK(luau->get_vm(LuauEngine::VM_SCRIPT_LOAD) != nullptr);
		CHECK(luau->get_vm(LuauEngine::VM_CORE) != nullptr);
		CHECK(luau->get_vm(LuauEngine::VM_USER) != nullptr);
	}

	TEST_CASE("Out of range VM types return nullptr") {
		LuauEngine *luau = LuauEngine::get_singleton();
		REQUIRE(luau != nullptr);

		CHECK(luau->get_vm(LuauEngine::VM_MAX) == nullptr);
		CHECK(luau->get_vm(static_cast<LuauEngine::VMType>(-1)) == nullptr);
	}

	TEST_CASE("Math constants are global in every VM") {
		LuauEngine *luau = LuauEngine::get_singleton();
		REQUIRE(luau != nullptr);

		for (int i = 0; i < LuauEngine::VM_MAX; i++) {
			lua_State *L = luau->get_vm(static_cast<LuauEngine::VMType>(i));
			REQUIRE(L != nullptr);

			CAPTURE(i);
			lua_getglobal(L, "PI");
			CHECK(lua_isnumber(L, -1) != 0);
			CHECK(doctest::Approx(lua_tonumber(L, -1)) == Math_PI);
			lua_pop(L, 1);

			lua_getglobal(L, "TAU");
			CHECK(doctest::Approx(lua_tonumber(L, -1)) == Math_TAU);
			lua_pop(L, 1);

			lua_getglobal(L, "INF");
			CHECK(lua_tonumber(L, -1) > 0.0);
			CHECK(std::isinf(lua_tonumber(L, -1)));
			lua_pop(L, 1);

			lua_getglobal(L, "NAN");
			CHECK(std::isnan(lua_tonumber(L, -1)));
			lua_pop(L, 1);
		}
	}

	TEST_CASE("Core VM exposes Godot helper functions and classes") {
		lua_State *L = LuauEngine::get_singleton()->get_vm(LuauEngine::VM_CORE);
		REQUIRE(L != nullptr);

		int top = lua_gettop(L);

		lua_getglobal(L, "print");
		CHECK_EQ(lua_type(L, -1), LUA_TFUNCTION);
		lua_pop(L, 1);

		lua_getglobal(L, "typeof");
		CHECK_EQ(lua_type(L, -1), LUA_TFUNCTION);
		lua_pop(L, 1);

		for (const char *fn : { "sin", "cos", "clamp", "lerp", "min", "max", "randf" }) {
			lua_getglobal(L, fn);
			CHECK_EQ(lua_type(L, -1), LUA_TFUNCTION);
			lua_pop(L, 1);
		}

		lua_getglobal(L, "Vector2");
		CHECK_EQ(lua_type(L, -1), LUA_TTABLE);
		lua_getfield(L, -1, "ZERO");
		CHECK(LuauBridge::luaL_testudata(L, -1, "Vector2") != nullptr);
		lua_pop(L, 1);
		lua_getfield(L, -1, "ONE");
		CHECK(LuauBridge::luaL_testudata(L, -1, "Vector2") != nullptr);
		lua_pop(L, 1);
		lua_pop(L, 1);

		CHECK_EQ(lua_gettop(L), top);
	}

	TEST_CASE("Enum global resolves registered Godot enums") {
		LuauEngine *luau = LuauEngine::get_singleton();
		lua_State *L = luau->get_vm(LuauEngine::VM_CORE);
		REQUIRE(L != nullptr);

		const HashMap<String, HashMap<String, int64_t>> &enums = LuauEngine::get_global_enums();
		CHECK(enums.size() > 0);

		for (const KeyValue<String, HashMap<String, int64_t>> &E : enums) {
			CHECK(E.value.size() > 0);
			break;
		}

		String first_enum;
		for (const KeyValue<String, HashMap<String, int64_t>> &E : enums) {
			first_enum = E.key;
			break;
		}

		if (!first_enum.is_empty()) {
			CharString enum_name = first_enum.utf8();

			lua_getglobal(L, "Enum");
			CHECK_EQ(lua_type(L, -1), LUA_TTABLE);

			lua_getfield(L, -1, enum_name.get_data());
			CHECK_EQ(lua_type(L, -1), LUA_TTABLE);

			String first_member;
			for (const KeyValue<String, int64_t> &E : enums[first_enum]) {
				first_member = E.key;
				break;
			}
			if (!first_member.is_empty()) {
				CharString member_name = first_member.utf8();
				int64_t expected = enums[first_enum][first_member];

				lua_getfield(L, -1, member_name.get_data());
				CHECK_EQ(lua_type(L, -1), LUA_TNUMBER);
				CHECK_EQ((int64_t)lua_tointeger(L, -1), expected);
				lua_pop(L, 1);
			}

			lua_pop(L, 2); // enum table + Enum table
		}
	}

	TEST_CASE("VM globals stay balanced across access") {
		lua_State *L = LuauEngine::get_singleton()->get_vm(LuauEngine::VM_CORE);
		REQUIRE(L != nullptr);

		int top = lua_gettop(L);
		for (int i = 0; i < 100; i++) {
			lua_getglobal(L, "PI");
			lua_pop(L, 1);
			lua_getglobal(L, "print");
			lua_pop(L, 1);
		}
		CHECK_EQ(lua_gettop(L), top);
	}
}

// ==============================================================================
// Language metadata
// ==============================================================================
TEST_SUITE("Language metadata")
{
	TEST_CASE("Language identity strings") {
		LuauLanguage *lang = LuauLanguage::get_singleton();
		REQUIRE(lang != nullptr);

		CHECK_EQ(lang->_get_name(), String("Luau"));
		CHECK_EQ(lang->_get_type(), String("LuauScript"));
		CHECK_EQ(lang->_get_extension(), String("luau"));
	}

	TEST_CASE("Reserved words are reported") {
		LuauLanguage *lang = LuauLanguage::get_singleton();
		REQUIRE(lang != nullptr);

		PackedStringArray words = lang->_get_reserved_words();
		CHECK(words.size() > 0);

		CHECK(words.has("function"));
		CHECK(words.has("local"));
		CHECK(words.has("end"));
		CHECK(words.has("then"));
		CHECK(words.has("nil"));
		CHECK(words.has("true"));

		CHECK_FALSE(words.has("definitely_not_a_keyword"));
	}

	TEST_CASE("Control flow keywords are classified") {
		LuauLanguage *lang = LuauLanguage::get_singleton();
		REQUIRE(lang != nullptr);

		CHECK(lang->_is_control_flow_keyword("if"));
		CHECK(lang->_is_control_flow_keyword("while"));
		CHECK(lang->_is_control_flow_keyword("for"));
		CHECK(lang->_is_control_flow_keyword("repeat"));
		CHECK(lang->_is_control_flow_keyword("return"));

		CHECK_FALSE(lang->_is_control_flow_keyword("then"));
		CHECK_FALSE(lang->_is_control_flow_keyword("function"));
		CHECK_FALSE(lang->_is_control_flow_keyword("local"));
		CHECK_FALSE(lang->_is_control_flow_keyword("print"));
	}

	TEST_CASE("Comment and string delimiters") {
		LuauLanguage *lang = LuauLanguage::get_singleton();
		REQUIRE(lang != nullptr);

		PackedStringArray comments = lang->_get_comment_delimiters();
		CHECK(comments.has("--"));

		PackedStringArray strings = lang->_get_string_delimiters();
		CHECK(strings.size() > 0);
	}

	TEST_CASE("Recognized extensions contain luau") {
		LuauLanguage *lang = LuauLanguage::get_singleton();
		REQUIRE(lang != nullptr);

		PackedStringArray exts = lang->_get_recognized_extensions();
		CHECK(exts.has("luau"));
	}

	TEST_CASE("Language creates LuauScript instances") {
		LuauLanguage *lang = LuauLanguage::get_singleton();
		REQUIRE(lang != nullptr);

		LuauScript *created = Object::cast_to<LuauScript>(lang->_create_script());
		REQUIRE(created != nullptr);
		CHECK(created->_get_language() == lang);
		memdelete(created);
	}

	TEST_CASE("Named global constants can be added and removed") {
		LuauLanguage *lang = LuauLanguage::get_singleton();
		REQUIRE(lang != nullptr);

		const StringName name = "TEST_GLOBAL_CONSTANT";
		CHECK_FALSE(lang->global_constants.has(name));

		lang->_add_named_global_constant(name, Variant(123));
		CHECK(lang->global_constants.has(name));
		CHECK_EQ(lang->global_constants[name], Variant(123));

		lang->_remove_named_global_constant(name);
		CHECK_FALSE(lang->global_constants.has(name));
	}
}

// ==============================================================================
// Script loading & compilation
// ==============================================================================
TEST_SUITE("Script loading & compilation")
{
	TEST_CASE("Valid script compiles and becomes valid") {
		LuauLanguage *lang = LuauLanguage::get_singleton();
		REQUIRE(lang != nullptr);

		Ref<LuauScript> scr = make_script(
				"--- @tool\n"
				"--- @extends Node\n"
				"Value = 1 :: number\n"
				"function GetValue()\n"
				"\treturn Value\n"
				"end\n");
		REQUIRE(scr.is_valid());

		CHECK_EQ(scr->load(LuauScript::LOAD_FULL), OK);
		CHECK(scr->_is_valid());
		CHECK(scr->_has_source_code());
		CHECK(scr->_get_language() == lang);
	}

	TEST_CASE("Loading the same stage twice is a no-op") {
		Ref<LuauScript> scr = make_script("Value = 1 :: number\n");
		REQUIRE(scr.is_valid());

		CHECK_EQ(scr->load(LuauScript::LOAD_COMPILE), OK);
		CHECK_EQ(scr->load(LuauScript::LOAD_COMPILE), OK);
		CHECK_EQ(scr->load(LuauScript::LOAD_FULL), OK);
		CHECK_EQ(scr->load(LuauScript::LOAD_FULL), OK);
	}

	TEST_CASE("Syntax error fails the full load at the parse stage") {
		Ref<LuauScript> scr = make_script(
				"Value = 1 :: number\n"
				"function broken(\n");
		REQUIRE(scr.is_valid());

		Error err = scr->load(LuauScript::LOAD_FULL);
		CHECK(err != OK);
		CHECK_EQ(err, ERR_PARSE_ERROR);

		CHECK(scr->_is_valid());
	}

	TEST_CASE("Empty source fails to load") {
		Ref<LuauScript> scr = make_script("");
		REQUIRE(scr.is_valid());

		CHECK_EQ(scr->load(LuauScript::LOAD_FULL), ERR_INVALID_DATA);
	}

	TEST_CASE("load_source_code reads existing files and fails on missing ones") {
		Ref<LuauScript> scr;
		scr.instantiate();
		REQUIRE(scr.is_valid());

		CHECK_EQ(scr->load_source_code("res://luau_scripts/hello_world.luau"), OK);
		CHECK(scr->_get_source_code().contains("Node2D"));

		Ref<LuauScript> missing;
		missing.instantiate();
		REQUIRE(missing.is_valid());

		CHECK(missing->load_source_code("res://luau_scripts/does_not_exist_abc123.luau") != OK);
	}
}

// ==============================================================================
// Script definition parsing
// ==============================================================================
TEST_SUITE("Script definition parsing")
{
	TEST_CASE("@extends sets the base class") {
		Ref<LuauScript> scr = make_script(
				"--- @extends Node2D\n"
				"Value = 1 :: number\n");
		REQUIRE(scr.is_valid());
		REQUIRE_EQ(scr->load(LuauScript::LOAD_FULL), OK);

		CHECK_EQ(scr->get_definition().extends, String("Node2D"));
	}

	TEST_CASE("Scripts without @extends default to RefCounted") {
		Ref<LuauScript> scr = make_script("Value = 1 :: number\n");
		REQUIRE(scr.is_valid());
		REQUIRE_EQ(scr->load(LuauScript::LOAD_FULL), OK);

		CHECK_EQ(scr->get_definition().extends, String("RefCounted"));
	}

	TEST_CASE("@tool is detected") {
		Ref<LuauScript> scr = make_script(
				"--- @tool\n"
				"--- @extends Node\n"
				"Value = 1 :: number\n");
		REQUIRE(scr.is_valid());
		REQUIRE_EQ(scr->load(LuauScript::LOAD_FULL), OK);

		CHECK(scr->_is_tool());
	}

	TEST_CASE("Typed globals become properties with parsed defaults") {
		Ref<LuauScript> scr = make_script(
				"Title = \"Hello\" :: string\n"
				"Speed = 300 :: number\n"
				"Ratio = 0.5 :: number\n"
				"Enabled = true :: boolean\n");
		REQUIRE(scr.is_valid());
		REQUIRE_EQ(scr->load(LuauScript::LOAD_FULL), OK);

		const GDClassDefinition &def = scr->get_definition();

		CHECK(def.property_indices.has("Title"));
		if (def.property_indices.has("Title")) {
			const GDClassProperty &p = def.properties[def.property_indices["Title"]];
			CHECK_EQ(p.property.type, GDEXTENSION_VARIANT_TYPE_STRING);
			CHECK_EQ((String)p.default_value, String("Hello"));
		}

		CHECK(def.property_indices.has("Speed"));
		if (def.property_indices.has("Speed")) {
			const GDClassProperty &p = def.properties[def.property_indices["Speed"]];
			CHECK_EQ(p.property.type, GDEXTENSION_VARIANT_TYPE_INT);
			CHECK_EQ(p.default_value, Variant(int64_t(300)));
		}

		CHECK(def.property_indices.has("Ratio"));
		if (def.property_indices.has("Ratio")) {
			const GDClassProperty &p = def.properties[def.property_indices["Ratio"]];
			CHECK_EQ(p.property.type, GDEXTENSION_VARIANT_TYPE_FLOAT);
			CHECK(doctest::Approx((double)(float)p.default_value) == 0.5);
		}

		CHECK(def.property_indices.has("Enabled"));
		if (def.property_indices.has("Enabled")) {
			const GDClassProperty &p = def.properties[def.property_indices["Enabled"]];
			CHECK_EQ(p.property.type, GDEXTENSION_VARIANT_TYPE_BOOL);
			CHECK((bool)p.default_value);
		}
	}

	TEST_CASE("ALL_CAPS globals become constants, not properties") {
		Ref<LuauScript> scr = make_script(
				"MAX_SPEED = 999 :: number\n"
				"Speed = 10 :: number\n");
		REQUIRE(scr.is_valid());
		REQUIRE_EQ(scr->load(LuauScript::LOAD_FULL), OK);

		const GDClassDefinition &def = scr->get_definition();

		CHECK(def.constants.has("MAX_SPEED"));
		CHECK_FALSE(def.property_indices.has("MAX_SPEED"));
		CHECK_EQ((int64_t)def.constants["MAX_SPEED"], 999);

		CHECK(def.property_indices.has("Speed"));

		Dictionary exposed = scr->_get_constants();
		CHECK(exposed.has("MAX_SPEED"));
	}

	TEST_CASE("Top-level functions become methods") {
		Ref<LuauScript> scr = make_script(
				"--- @tool\n"
				"--- @extends Node\n"
				"function AddScore(points: number)\n"
				"\treturn points\n"
				"end\n"
				"function _ready()\n"
				"end\n");
		REQUIRE(scr.is_valid());
		REQUIRE_EQ(scr->load(LuauScript::LOAD_FULL), OK);

		const GDClassDefinition &def = scr->get_definition();

		CHECK(def.methods.has("AddScore"));
		CHECK(def.methods.has("_ready"));
		CHECK_FALSE(def.methods.has("NotDefinedAnywhere"));

		CHECK(scr->_has_static_method("AddScore") == false);
	}

	TEST_CASE("Locals become members but are not exposed as properties") {
		Ref<LuauScript> scr = make_script(
				"local frames = 0\n"
				"local hp: number = 100\n"
				"public_value = 1 :: number\n");
		REQUIRE(scr.is_valid());
		REQUIRE_EQ(scr->load(LuauScript::LOAD_FULL), OK);

		const GDClassDefinition &def = scr->get_definition();

		CHECK(def.member_indices.has("frames"));
		CHECK(def.member_indices.has("hp"));
		CHECK_FALSE(def.property_indices.has("frames"));
		CHECK_FALSE(def.property_indices.has("hp"));

		CHECK(def.property_indices.has("public_value"));
		CHECK(def.member_indices.has("public_value"));

		TypedArray<StringName> members = scr->_get_members();
		bool found_hp = false;
		for (int i = 0; i < members.size(); i++) {
			if (String(StringName(members[i])) == String("hp")) {
				found_hp = true;
			}
		}
		CHECK(found_hp);
	}

	TEST_CASE("@export annotations map to property hints") {
		Ref<LuauScript> scr = make_script(
				"--- @export_range(0, 100)\n"
				"Speed = 50 :: number\n"
				"\n"
				"--- @export_enum(\"Sword\", \"Bow\")\n"
				"Weapon = \"Sword\" :: string\n");
		REQUIRE(scr.is_valid());
		REQUIRE_EQ(scr->load(LuauScript::LOAD_FULL), OK);

		const GDClassDefinition &def = scr->get_definition();

		REQUIRE(def.property_indices.has("Speed"));
		const GDClassProperty &speed = def.properties[def.property_indices["Speed"]];
		CHECK_EQ(speed.property.hint, PROPERTY_HINT_RANGE);
		CHECK_EQ((String)speed.property.hint_string, String("0,100"));

		REQUIRE(def.property_indices.has("Weapon"));
		const GDClassProperty &weapon = def.properties[def.property_indices["Weapon"]];
		CHECK_EQ(weapon.property.hint, PROPERTY_HINT_ENUM);
		CHECK_EQ((String)weapon.property.hint_string, String("Sword,Bow"));
	}
}

// ==============================================================================
// Script instances
// ==============================================================================
TEST_SUITE("Script instances")
{
	TEST_CASE("Tool script creates a real instance on a matching Node") {
		Ref<LuauScript> scr = make_script(
				"--- @tool\n"
				"--- @extends Node\n"
				"Score = 0 :: number\n"
				"Scored = signal(\"Scored\")\n"
				"function AddScore(points: number)\n"
				"\tScore += points\n"
				"\treturn Score\n"
				"end\n");
		REQUIRE(scr.is_valid());
		REQUIRE_EQ(scr->load(LuauScript::LOAD_FULL), OK);

		CHECK(scr->can_instantiate());

		Node *node = memnew(Node);
		REQUIRE(node != nullptr);
		node->set_script(scr);

		CHECK(scr->_instance_has(node));
		CHECK(scr->instance_has(node->get_instance_id()));

		Ref<Script> attached = node->get_script();
		CHECK(attached.is_valid());
		CHECK(Object::cast_to<LuauScript>(attached.ptr()) != nullptr);

		CHECK_EQ((int64_t)node->get("Score"), 0);

		node->set("Score", Variant(41));
		CHECK_EQ((int64_t)node->get("Score"), 41);

		CHECK(node->has_method("AddScore"));
		CHECK_FALSE(node->has_method("NonexistentMethod"));
		Variant result = node->call("AddScore", 8);
		CHECK_EQ((int64_t)node->get("Score"), 49);
		CHECK_EQ((int64_t)result, 49);

		CHECK(scr->_has_script_signal("Scored"));
		CHECK_FALSE(scr->_has_script_signal("NeverDeclared"));

		bool found_method = false;
		TypedArray<Dictionary> methods = scr->_get_script_method_list();
		for (int i = 0; i < methods.size(); i++) {
			Dictionary m = methods[i];
			if (m.has("name") && (String)m["name"] == String("AddScore")) {
				found_method = true;
			}
		}
		CHECK(found_method);

		bool found_prop = false;
		TypedArray<Dictionary> props = scr->_get_script_property_list();
		for (int i = 0; i < props.size(); i++) {
			Dictionary p = props[i];
			if (p.has("name") && (String)p["name"] == String("Score")) {
				found_prop = true;
			}
		}
		CHECK(found_prop);

		memdelete(node);
	}

	TEST_CASE("Default @extends RefCounted works with RefCounted owners") {
		Ref<LuauScript> scr = make_script(
				"--- @tool\n"
				"Value = 5 :: number\n"
				"function GetValue()\n"
				"\treturn Value\n"
				"end\n");
		REQUIRE(scr.is_valid());
		REQUIRE_EQ(scr->load(LuauScript::LOAD_FULL), OK);

		CHECK_EQ(scr->get_definition().extends, String("RefCounted"));

		RefCounted *obj = memnew(RefCounted);
		REQUIRE(obj != nullptr);
		obj->set_script(scr);

		CHECK(scr->_instance_has(obj));
		CHECK_EQ((int64_t)obj->get("Value"), 5);
		CHECK(obj->has_method("GetValue"));

		memdelete(obj);
	}

	TEST_CASE("Base class mismatch falls back to a placeholder instance") {
		Ref<LuauScript> scr = make_script(
				"--- @tool\n"
				"--- @extends Node2D\n"
				"Value = 7 :: number\n");
		REQUIRE(scr.is_valid());
		REQUIRE_EQ(scr->load(LuauScript::LOAD_FULL), OK);

		Node *node = memnew(Node);
		REQUIRE(node != nullptr);
		node->set_script(scr);

		CHECK_FALSE(scr->_instance_has(node));
		CHECK_EQ((int64_t)node->get("Value"), 7);

		memdelete(node);
	}
}

// ==============================================================================
// Variant bridge
// ==============================================================================
TEST_SUITE("Variant bridge")
{
	TEST_CASE("String push/get round-trip") {
		lua_State *L = LuauEngine::get_singleton()->get_vm(LuauEngine::VM_CORE);
		REQUIRE(L != nullptr);

		int top = lua_gettop(L);
		LuauBridge::push_string(L, "hello world");
		CHECK_EQ(lua_type(L, -1), LUA_TSTRING);
		CHECK_EQ(LuauBridge::get_string(L, -1), String("hello world"));
		lua_pop(L, 1);

		CHECK_EQ(lua_gettop(L), top);
	}

	TEST_CASE("Primitive variant round-trips") {
		lua_State *L = LuauEngine::get_singleton()->get_vm(LuauEngine::VM_CORE);
		REQUIRE(L != nullptr);

		int top = lua_gettop(L);

		// nil
		LuauBridge::push_variant(L, Variant());
		CHECK_EQ(lua_type(L, -1), LUA_TNIL);
		CHECK(LuauBridge::get_variant(L, -1).get_type() == Variant::NIL);
		lua_pop(L, 1);

		// bool
		LuauBridge::push_variant(L, Variant(true));
		CHECK_EQ(lua_type(L, -1), LUA_TBOOLEAN);
		{
			Variant back = LuauBridge::get_variant(L, -1);
			CHECK(back.get_type() == Variant::BOOL);
			CHECK((bool)back);
		}
		lua_pop(L, 1);

		// int (Lua numbers are doubles, so ints come back as floats)
		LuauBridge::push_variant(L, Variant(int64_t(300)));
		CHECK_EQ(lua_type(L, -1), LUA_TNUMBER);
		{
			Variant back = LuauBridge::get_variant(L, -1);
			CHECK(back.get_type() == Variant::FLOAT);
			CHECK_EQ((int64_t)(float)back, 300);
		}
		lua_pop(L, 1);

		// float
		LuauBridge::push_variant(L, Variant(2.5f));
		CHECK_EQ(lua_type(L, -1), LUA_TNUMBER);
		{
			Variant back = LuauBridge::get_variant(L, -1);
			CHECK(back.get_type() == Variant::FLOAT);
			CHECK(doctest::Approx((double)(float)back) == 2.5);
		}
		lua_pop(L, 1);

		// string
		LuauBridge::push_variant(L, Variant(String("bridge")));
		CHECK_EQ(lua_type(L, -1), LUA_TSTRING);
		{
			Variant back = LuauBridge::get_variant(L, -1);
			CHECK(back.get_type() == Variant::STRING);
			CHECK_EQ((String)back, String("bridge"));
		}
		lua_pop(L, 1);

		CHECK_EQ(lua_gettop(L), top);
	}

	TEST_CASE("Array round-trip") {
		lua_State *L = LuauEngine::get_singleton()->get_vm(LuauEngine::VM_CORE);
		REQUIRE(L != nullptr);

		int top = lua_gettop(L);

		Array arr;
		arr.append(1);
		arr.append("two");
		arr.append(true);

		LuauBridge::push_array(L, arr);
		CHECK_EQ(lua_type(L, -1), LUA_TTABLE);
		CHECK_EQ(lua_objlen(L, -1), 3);

		Array back = LuauBridge::get_array(L, -1);
		CHECK_EQ(back.size(), 3);
		CHECK_EQ((int64_t)(float)back[0], 1);
		CHECK_EQ((String)back[1], String("two"));
		CHECK((bool)back[2]);

		lua_pop(L, 1);
		CHECK_EQ(lua_gettop(L), top);
	}

	TEST_CASE("Dictionary round-trip") {
		lua_State *L = LuauEngine::get_singleton()->get_vm(LuauEngine::VM_CORE);
		REQUIRE(L != nullptr);

		int top = lua_gettop(L);

		Dictionary dict;
		dict["name"] = "bench";
		dict["level"] = 3;

		LuauBridge::push_dictionary(L, dict);
		CHECK_EQ(lua_type(L, -1), LUA_TTABLE);

		Dictionary back = LuauBridge::get_dictionary(L, -1);
		CHECK_EQ(back.size(), 2);
		CHECK(back.has("name"));
		CHECK_EQ((String)back["name"], String("bench"));
		CHECK(back.has("level"));
		CHECK_EQ((int64_t)(float)back["level"], 3);

		lua_pop(L, 1);
		CHECK_EQ(lua_gettop(L), top);
	}

	TEST_CASE("Vector3 userdata round-trip") {
		lua_State *L = LuauEngine::get_singleton()->get_vm(LuauEngine::VM_CORE);
		REQUIRE(L != nullptr);

		int top = lua_gettop(L);

		LuauBridge::push_variant(L, Variant(Vector3(1.5f, 2.5f, 3.5f)));
		CHECK_EQ(lua_type(L, -1), LUA_TUSERDATA);
		CHECK(LuauBridge::luaL_testudata(L, -1, "Vector3") != nullptr);

		Variant back = LuauBridge::get_variant(L, -1);
		CHECK(back.get_type() == Variant::VECTOR3);
		Vector3 v = back;
		CHECK(doctest::Approx((double)v.x) == 1.5);
		CHECK(doctest::Approx((double)v.y) == 2.5);
		CHECK(doctest::Approx((double)v.z) == 3.5);

		lua_pop(L, 1);
		CHECK_EQ(lua_gettop(L), top);
	}

	TEST_CASE("Lua table without metatable becomes a Dictionary") {
		lua_State *L = LuauEngine::get_singleton()->get_vm(LuauEngine::VM_CORE);
		REQUIRE(L != nullptr);

		int top = lua_gettop(L);

		REQUIRE(lua_run_chunk(L, "return { color = \"red\", count = 7 }", 1));

		Variant back = LuauBridge::get_variant(L, -1);
		CHECK(back.get_type() == Variant::DICTIONARY);
		Dictionary d = back;
		CHECK_EQ((String)d["color"], String("red"));
		CHECK_EQ((int64_t)(float)d["count"], 7);

		lua_pop(L, 1);
		CHECK_EQ(lua_gettop(L), top);
	}
}

// ==============================================================================
// Script cache
// ==============================================================================
TEST_SUITE("Script cache")
{
	TEST_CASE("Cached scripts are shared between requests") {
		LuauCache *cache = LuauCache::get_singleton();
		REQUIRE(cache != nullptr);

		Error err = OK;
		Ref<LuauScript> a = cache->get_script("res://luau_scripts/hello_world.luau", err);
		CHECK_EQ(err, OK);
		CHECK(a.is_valid());

		err = OK;
		Ref<LuauScript> b = cache->get_script("res://luau_scripts/hello_world.luau", err);
		CHECK_EQ(err, OK);
		CHECK(b.is_valid());

		CHECK(a == b);
	}

	TEST_CASE("Missing scripts report an error") {
		LuauCache *cache = LuauCache::get_singleton();
		REQUIRE(cache != nullptr);

		Error err = OK;
		Ref<LuauScript> scr = cache->get_script("res://luau_scripts/missing_file_xyz.luau", err);
		CHECK(err != OK);
	}
}

// ==============================================================================
// Resource format
// ==============================================================================
TEST_SUITE("Resource format")
{
	TEST_CASE("Script loader recognizes luau files") {
		PackedStringArray exts = ResourceLoader::get_singleton()->get_recognized_extensions_for_type("Script");
		CHECK(exts.has("luau"));
	}

	TEST_CASE("Resource saver recognizes loaded LuauScript") {
		Ref<LuauScript> scr = make_script("Value = 1 :: number\n");
		REQUIRE(scr.is_valid());

		PackedStringArray exts = ResourceSaver::get_singleton()->get_recognized_extensions(scr);
		CHECK(exts.has("luau"));
	}
}

// ==============================================================================
// File-based demo scripts
// ==============================================================================
TEST_SUITE("Demo scripts")
{
	TEST_CASE("Run sayhello.luau") {
		LuauLanguage *luau_lang = LuauLanguage::get_singleton();
		REQUIRE(luau_lang != nullptr);

		String file_name = "res://luau_scripts/sayhello.luau";
		CHECK(FileAccess::file_exists(file_name));

		Ref<LuauScript> scr;
		scr.instantiate();
		REQUIRE(scr.is_valid());

		Error err_load_scr = scr->load_source_code(file_name);
		CHECK_EQ(err_load_scr, OK);

		Error err_load = scr->load(LuauScript::LOAD_FULL);
		CHECK_EQ(err_load, OK);

		SceneTree *tree = memnew(SceneTree);
		REQUIRE(tree != nullptr);
		tree->set_script(scr);
		
		CHECK_FALSE(scr->_instance_has(tree));
		CHECK(scr->is_placeholder_fallback_enabled());

		memdelete(tree);
	}

	TEST_CASE("hello_world.luau parses methods, properties and members") {
		Ref<LuauScript> scr;
		scr.instantiate();
		REQUIRE(scr.is_valid());

		REQUIRE_EQ(scr->load_source_code("res://luau_scripts/hello_world.luau"), OK);
		REQUIRE_EQ(scr->load(LuauScript::LOAD_FULL), OK);

		const GDClassDefinition &def = scr->get_definition();
		CHECK_EQ(def.extends, String("Node2D"));

		CHECK(def.property_indices.has("Message"));
		CHECK(def.methods.has("_init"));
		CHECK(def.methods.has("_ready"));
		CHECK(def.methods.has("_process"));
		CHECK(def.member_indices.has("elapsed"));
		CHECK(def.member_indices.has("frames"));
	}
}

// ==============================================================================
// Benchmarks
// ==============================================================================
TEST_SUITE("Benchmark")
{
	TEST_CASE("Variant push/get round-trip") {
		lua_State *L = LuauEngine::get_singleton()->get_vm(LuauEngine::VM_CORE);
		REQUIRE(L != nullptr);

		int top = lua_gettop(L);

		// Integer variant round-trip.
		{
			const int64_t ops = 100000;
			
			LuauBridge::push_variant(L, Variant(int64_t(1))); // warmup
			lua_pop(L, 1);

			uint64_t start = now_usec();
			for (int64_t i = 0; i < ops; i++) {
				LuauBridge::push_variant(L, Variant(int64_t(i)));
				Variant back = LuauBridge::get_variant(L, -1);
				(void)back;
				lua_pop(L, 1);
			}
			uint64_t elapsed = now_usec() - start;
			report_bench("variant_int_roundtrip", ops, elapsed);
		}

		// String variant round-trip.
		{
			const int64_t ops = 50000;
			const String s = "benchmark-string-value";
			LuauBridge::push_variant(L, s); // warmup
			lua_pop(L, 1);

			uint64_t start = now_usec();
			for (int64_t i = 0; i < ops; i++) {
				LuauBridge::push_variant(L, s);
				Variant back = LuauBridge::get_variant(L, -1);
				(void)back;
				lua_pop(L, 1);
			}
			uint64_t elapsed = now_usec() - start;
			report_bench("variant_string_roundtrip", ops, elapsed);
		}

		// Vector3 userdata round-trip.
		{
			const int64_t ops = 50000;
			const Variant v = Variant(Vector3(1, 2, 3));
			LuauBridge::push_variant(L, v); // warmup
			lua_pop(L, 1);

			uint64_t start = now_usec();
			for (int64_t i = 0; i < ops; i++) {
				LuauBridge::push_variant(L, v);
				Variant back = LuauBridge::get_variant(L, -1);
				(void)back;
				lua_pop(L, 1);
			}
			uint64_t elapsed = now_usec() - start;
			report_bench("variant_vector3_roundtrip", ops, elapsed);
		}

		CHECK_EQ(lua_gettop(L), top);
	}

	TEST_CASE("Luau script compile + parse (LOAD_FULL)") {
		static const char *BENCH_SOURCE =
				"--- @tool\n"
				"--- @extends Node\n"
				"MaxSpeed = 300 :: number\n"
				"Boost = 0.5 :: number\n"
				"Title = \"Bench\" :: string\n"
				"local frames: number = 0\n"
				"local elapsed: number = 0\n"
				"Weights = { 0.5, 1.5, 2.5 } :: { number }\n"
				"function Reset()\n"
				"\tframes = 0\n"
				"\telapsed = 0\n"
				"end\n"
				"function Tick(delta: number)\n"
				"\telapsed += delta\n"
				"\tframes += 1\n"
				"\tif elapsed > 10 then\n"
				"\t\tReset()\n"
				"\tend\n"
				"\treturn frames\n"
				"end\n"
				"function Compute(values: { number })\n"
				"\tlocal total = 0\n"
				"\tfor i = 1, #values do\n"
				"\t\tlocal v = values[i] * MaxSpeed\n"
				"\t\tif v > 1000 then\n"
				"\t\t\tv = v * Boost\n"
				"\t\tend\n"
				"\t\ttotal += v\n"
				"\tend\n"
				"\treturn total\n"
				"end\n"
				"Score = 0 :: number\n"
				"Rank = \"Novice\" :: string\n"
				"Flags = 0 :: number\n";

		const int64_t ops = 100;

		// Warmup (also validates the source compiles).
		Ref<LuauScript> warm = make_script(BENCH_SOURCE);
		REQUIRE_EQ(warm->load(LuauScript::LOAD_FULL), OK);
		REQUIRE(warm->_is_valid());

		uint64_t start = now_usec();
		for (int64_t i = 0; i < ops; i++) {
			Ref<LuauScript> scr = make_script(BENCH_SOURCE);
			Error err = scr->load(LuauScript::LOAD_FULL);
			CHECK_EQ(err, OK);
		}
		uint64_t elapsed = now_usec() - start;
		report_bench("script_compile_full", ops, elapsed);
	}

	TEST_CASE("Lua VM arithmetic loop") {
		lua_State *L = LuauEngine::get_singleton()->get_vm(LuauEngine::VM_CORE);
		REQUIRE(L != nullptr);

		int top = lua_gettop(L);

		const char *chunk =
				"return function(n)\n"
				"\tlocal s = 0\n"
				"\tfor i = 1, n do\n"
				"\t\ts += i\n"
				"\tend\n"
				"\treturn s\n"
				"end\n";

		std::string bytecode = Luau::compile(chunk);
		REQUIRE(luau_load(L, "=bench_arith", bytecode.c_str(), bytecode.size(), 0) == 0);
		REQUIRE(lua_pcall(L, 0, 1, 0) == 0);
		REQUIRE(lua_isfunction(L, -1));

		int fn_ref = lua_ref(L, -1);
		lua_pop(L, 1);

		const int64_t ops = 2000;
		const int64_t n = 1000;

		// Warmup.
		lua_getref(L, fn_ref);
		lua_pushinteger(L, n);
		REQUIRE(lua_pcall(L, 1, 1, 0) == 0);
		lua_pop(L, 1);

		uint64_t start = now_usec();
		for (int64_t i = 0; i < ops; i++) {
			lua_getref(L, fn_ref);
			lua_pushinteger(L, n);
			lua_pcall(L, 1, 1, 0);
			lua_pop(L, 1);
		}
		uint64_t elapsed = now_usec() - start;
		report_bench("lua_arithmetic_sum_1000", ops, elapsed);

		// Sanity check (outside the timed loop): sum(1000) == 500500.
		lua_getref(L, fn_ref);
		lua_pushinteger(L, n);
		REQUIRE(lua_pcall(L, 1, 1, 0) == 0);
		CHECK_EQ((int64_t)lua_tointeger(L, -1), 500500);
		lua_pop(L, 1);

		lua_unref(L, fn_ref);
		CHECK_EQ(lua_gettop(L), top);
	}

	TEST_CASE("Script method call through an instance") {
		Ref<LuauScript> scr = make_script(
				"--- @tool\n"
				"--- @extends Node\n"
				"Score = 0 :: number\n"
				"function Add(points: number)\n"
				"\tScore += points\n"
				"\treturn Score\n"
				"end\n");
		REQUIRE(scr.is_valid());
		REQUIRE_EQ(scr->load(LuauScript::LOAD_FULL), OK);

		Node *node = memnew(Node);
		REQUIRE(node != nullptr);
		node->set_script(scr);
		REQUIRE(scr->_instance_has(node));

		const int64_t ops = 5000;

		Variant warm = node->call("Add", 1);
		REQUIRE_EQ((int64_t)warm, 1);

		uint64_t start = now_usec();
		for (int64_t i = 0; i < ops; i++) {
			node->call("Add", 1);
		}
		uint64_t elapsed = now_usec() - start;
		report_bench("instance_method_call", ops, elapsed);

		CHECK_EQ((int64_t)node->get("Score"), ops + 1);

		memdelete(node);
	}

	TEST_CASE("Script property set/get through an instance") {
		Ref<LuauScript> scr = make_script(
				"--- @tool\n"
				"--- @extends Node\n"
				"Value = 0 :: number\n");
		REQUIRE(scr.is_valid());
		REQUIRE_EQ(scr->load(LuauScript::LOAD_FULL), OK);

		Node *node = memnew(Node);
		REQUIRE(node != nullptr);
		node->set_script(scr);
		REQUIRE(scr->_instance_has(node));

		const int64_t ops = 20000;

		node->set("Value", Variant(1));
		REQUIRE_EQ((int64_t)node->get("Value"), 1);

		uint64_t start = now_usec();
		for (int64_t i = 0; i < ops; i++) {
			node->set("Value", Variant(i));
			node->get("Value");
		}
		uint64_t elapsed = now_usec() - start;
		report_bench("instance_property_set_get", ops, elapsed);

		CHECK_EQ((int64_t)node->get("Value"), ops - 1);

		memdelete(node);
	}
}
