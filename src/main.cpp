#include <r2/engine.h>
#include <r2/managers/drivers/gl/driver.h>

#include <states/main.h>
#include <managers/source_man.h>

using namespace r2;

int main(int argc, char** argv) {
	r2engine::create(argc, argv);
	auto eng = r2engine::get();

	eng->open_window(1024, 512, "Rosen", true);
	eng->renderer()->set_driver(new gl_render_driver(eng->renderer()));

	rosen::source_man* sourceMgr = new rosen::source_man();

	eng->states()->register_state(new rosen::main_state(sourceMgr));
	eng->states()->activate("main_state");

	int ret = eng->run();

	delete sourceMgr;

	eng->shutdown();
	return ret;
}
