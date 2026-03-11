#include "doctest.h"

#include "nobind.h"

#include <godot_cpp/godot.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/thread.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/file_access.hpp>

#include "luauscript/luau_engine.h"
#include "luauscript/luau_script.h"

using namespace godot;

TEST_CASE("REQUIRE Engine") {
    Engine* engine = nobind::Engine::get_singleton();
    CHECK(engine != nullptr);
}

TEST_CASE("REQUIRE LuauLanguage") {
    LuauLanguage* luau_lang = LuauLanguage::get_singleton();
    CHECK(luau_lang != nullptr);
}

// TEST_CASE("Run sayhello.luau") {
//     LuauLanguage* luau_lang = LuauLanguage::get_singleton();
//     CHECK(luau_lang != nullptr);

//     MainLoop* main_loop = nobind::Engine::get_singleton()->get_main_loop();
//     CHECK(main_loop != nullptr);

//     SceneTree* root_scene = nobind::Object::cast_to<SceneTree>(main_loop);
//     CHECK(root_scene != nullptr);
    
//     String file_name = "res://luau_scripts/sayhello.luau";
//     CHECK(FileAccess::file_exists(file_name));
    
//     bool recognized = ResourceLoader::get_singleton()->get_recognized_extensions_for_type("Script").has("luau");
//     CHECK(recognized); 

//     Ref<LuauScript> scr;
//     scr.instantiate();

//     Error err_load_scr = scr->load_source_code(file_name);
//     CHECK(err_load_scr == OK);

//     Error err_load = scr->load(LuauScript::LOAD_FULL);
//     CHECK(err_load == OK);
    
//     SceneTree* new_node = memnew(SceneTree);
//     new_node->set_script(scr);
//     root_scene->set_current_scene(nobind::Object::cast_to<Node>(new_node));
// }