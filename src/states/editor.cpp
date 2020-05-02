#include <states/editor.h>
#include <managers/source_man.h>
#include <managers/ui_man.h>
#include <managers/space_man.h>
#include <systems/speech.h>
#include <utils/physics_drawer.h>

#include <r2/managers/drivers/gl/driver.h>
#include <r2/engine.h>
#include <r2/utilities/fly_camera.h>
#include <r2/utilities/debug_drawer.h>

#include <utils/imguizmo.h>

namespace rosen {
	editor_state::editor_state(source_man* sourceMgr) : state("editor_state", MBtoB(64)) {
		// This state's memory has not been allocated yet. Any
		// allocations made here will be either in the global
		// scope, or the scope of the currently active state.
		// Be careful not to allocate here, or to explicitly
		// allocate in the global scope via:
		// memory_man::push_current(memory_man::global())
		// ...allocate stuff...
		// memory_man::pop_current()

		m_sources = sourceMgr;
		m_spaces = nullptr;
		m_camera = nullptr;
		m_ui = nullptr;
	}

	editor_state::~editor_state() {
		// This state's memory should already be deallocated
		// by this point, unless the user deleted the state
		// while it was active (don't do that)
	}

	void editor_state::onInitialize() {
		// The state's memory has been allocated, and is
		// currently active. From here until becameInactive
		// is called, all allocations made in this class
		// will be in the state's own memory scope (unless
		// explicitly allocated in the global scope using
		// memory_man::[push/pop]_current)
		setUpdateFrequency(60.0f);
	}

	void editor_state::willBecomeActive() {
		// The state is going to become active. The previous
		// state is still active at this point, but will be
		// deactivated immediately after this function returns.
		// This state will be activated immediately after that

		m_spaces = new space_man(getScene());
		m_ui = new ui_man(m_sources, getScene());
		m_debugShader = getScene()->load_shader("./resources/shader/debug.glsl", "debug_shader");
		m_debugDraw = new debug_drawer(getScene(), m_debugShader, 131072 * 2, 8192 * 3);
		m_physicsDraw = new physics_drawer(m_debugDraw);

		auto& ps = physics_sys::get()->physState();
		ps.enable();
		ps->world->setDebugDrawer(m_physicsDraw);
		ps.disable();

		r2engine::audio()->setListener(mat4f(1.0f));
		m_camera = new fly_camera_entity();

		getScene()->clearColor = vec4f(0, 0, 0, 1.0f);
	}

	void editor_state::becameActive() {
		// The state was activated completely, and the previous
		// state (if any) has been cleared

		r2engine::get()->initialize_new_entities();
	}

	void editor_state::willBecomeInactive() {
		// Becoming inactive means, among other things, that
		// the entire block of memory used by this state will
		// either be cleared or temporarily deallocated.
		// You don't _have_ to destroy entities here, or
		// deallocate anything allocated within this state's
		// memory, but you should...

		if (m_ui) delete m_ui; m_ui = nullptr;
		if (m_spaces) delete m_spaces; m_spaces = nullptr;

		m_camera->destroy(); m_camera = nullptr;
	}

	void editor_state::becameInactive() {
		// Anything allocated here will be deallocated immediately
		// after this function returns, unless it's explicitly
		// allocated in the global scope via:
		// memory_man::push_current(memory_man::global())
		// ...allocate stuff...
		// memory_man::pop_current()
	}

	void editor_state::willBeDestroyed() {
		// By the time this function is called, unless the state is
		// currently active (should never happen unless the user
		// deletes the state for some reason), the state's memory
		// will already be deallocated, and any allocations made here
		// will be made either in the global scope, or the currently
		// active state's scope. Be careful to not allocate here, or
		// to specify which scope the allocation _should_ be made in.
		// This includes allocations made internally by std containers
	}

	void editor_state::onUpdate(f32 frameDt, f32 updateDt) {
		// Will be called as frequently as the user specifies with
		// setUpdateFrequency(frequency in Hz), as long as the
		// application is able to achieve that frequency.
		// Rendering probably can't be done here, as it's called
		// outside of the context of the frame.

		m_ui->update(frameDt, updateDt);
		rosen_space* space = m_spaces->get_current();
		if (space) space->update(updateDt);
	}

	void editor_state::onRender() {
		ImGuizmo::BeginFrame();
		ImGuizmo::Enable(true);
		ImGuiIO& io = ImGui::GetIO();
		ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

		scene_entity* camera = getScene()->camera;
		if (camera) r2engine::audio()->setListener(glm::inverse(camera->transform->transform));
		m_ui->render();

		GLFWwindow* window = *r2engine::get()->window();
		char title[128] = { 0 };
		snprintf(title, 128, "Rosen | %6.2f FPS | %8s / %8s", r2engine::get()->fps(), format_size(getUsedMemorySize()), format_size(getMaxMemorySize()));
		glfwSetWindowTitle(window, title);

		m_debugDraw->begin();

		auto& ps = physics_sys::get()->physState();
		ps.enable();
		ps->world->debugDrawWorld();
		ps.disable();
		m_debugDraw->line(vec3f(0, 0, 0), vec3f(100, 0, 0), vec4f(1, 0, 0, 1));
		m_debugDraw->line(vec3f(0, 0, 0), vec3f(0, 100, 0), vec4f(0, 1, 0, 1));
		m_debugDraw->line(vec3f(0, 0, 0), vec3f(0, 0, 100), vec4f(0, 0, 1, 1));
		
		static mat4f test(1.0f);
		if (camera) {
			mat4f t = camera->transform->transform;
			mat4f p = camera->camera->projection;
			ImGuizmo::Manipulate(&t[0][0], &p[0][0], ImGuizmo::TRANSLATE, ImGuizmo::WORLD, &test[0][0]);
			if (ImGuizmo::IsUsing()) {
				printf("Transforming...\n");
			}
		}
		m_debugDraw->end();
	}

	void editor_state::onEvent(event* evt) {
		// Will be called whenever a non-internal event is fired,
		// or in the case of deferred events, at the beginning of
		// the engine tick (before onUpdate would be called)
	}
};