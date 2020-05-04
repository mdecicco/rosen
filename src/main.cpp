#include <r2/engine.h>
#include <r2/managers/drivers/gl/driver.h>

#include <states/main.h>
#include <states/initial_loading.h>
#include <states/editor.h>

#include <managers/source_man.h>

#include <systems/speech.h>
#include <systems/control.h>

using namespace r2;

int main(int argc, char** argv) {
	r2engine::register_system(rosen::speech_system::create());
	r2engine::register_system(rosen::control_system::create());
	r2engine::create(argc, argv);
	auto eng = r2engine::get();

	eng->open_window(1920, 980, "Rosen", true);
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF("./resources/font/arial.ttf", 16.0f);

	eng->renderer()->set_driver(new gl_render_driver(eng->renderer()));

	rosen::source_man* sourceMgr = new rosen::source_man();
	rosen::speech_system::get()->sources = sourceMgr;

	eng->states()->register_state(new rosen::initial_loading_state(sourceMgr));
	eng->states()->register_state(new rosen::editor_state(sourceMgr));
	//eng->states()->register_state(new rosen::main_state(sourceMgr));
	eng->states()->activate("initial_loading_state");

	int ret = eng->run();

	delete sourceMgr;

	eng->shutdown();
	return ret;
}
