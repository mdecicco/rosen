#include <r2/engine.h>
#include <r2/managers/drivers/gl/driver.h>

#include <states/main.h>

using namespace r2;

int main(int argc, char** argv) {
	r2engine::create(argc, argv);
	auto eng = r2engine::get();

	eng->open_window(512, 512, "Rosen", true);
	eng->renderer()->set_driver(new gl_render_driver(eng->renderer()));

	eng->states()->register_state(new rosen::main_state());
	eng->states()->activate("main_state");

	int ret = eng->run();

	eng->shutdown();
	return ret;
}
