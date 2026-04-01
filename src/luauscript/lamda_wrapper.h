#ifndef LAMDA_WRAPPER_H
#define LAMDA_WRAPPER_H

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/object.hpp>
#include <functional>


namespace godot {
    class LambdaWrapper : public godot::Object {
        GDCLASS(LambdaWrapper, godot::Object);

    private:
        std::function<void()> func;

    protected:
        static void _bind_methods() {
            godot::ClassDB::bind_method(godot::D_METHOD("execute"), &LambdaWrapper::execute);
        }

    public:
        void set_function(std::function<void()> p_func) { func = p_func; }
        
        void execute() {
            if (func) func();
        }
    };
}

#endif // LAMDA_WRAPPER_H