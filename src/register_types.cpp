#include "register_types.h"

#include <gdextension_interface.h>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "neighbour.h"
#include "neighbourhood_server.h"

using namespace godot;

static NeighbourhoodServer *neighbourhood_server_singleton = nullptr;

void initialize_gdextension_types(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	GDREGISTER_CLASS(Neighbour);
	GDREGISTER_CLASS(NeighbourhoodServer);
	neighbourhood_server_singleton = memnew(NeighbourhoodServer);
	Engine::get_singleton()->register_singleton("NeighbourhoodServer", neighbourhood_server_singleton);
}

void uninitialize_gdextension_types(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	Engine::get_singleton()->unregister_singleton("NeighbourhoodServer");
	memdelete(neighbourhood_server_singleton);
}

extern "C"
{
	GDExtensionBool GDE_EXPORT neighbourhood_server_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization)
	{
		GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
		init_obj.register_initializer(initialize_gdextension_types);
		init_obj.register_terminator(uninitialize_gdextension_types);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

		return init_obj.init();
	}
}