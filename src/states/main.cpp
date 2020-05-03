#include <states/main.h>
#include <managers/source_man.h>
#include <managers/ui_man.h>
#include <managers/space_man.h>
#include <systems/speech.h>
#include <utils/physics_drawer.h>

#include <r2/managers/drivers/gl/driver.h>
#include <r2/engine.h>
#include <r2/utilities/fly_camera.h>
#include <r2/utilities/debug_drawer.h>

namespace rosen {
	main_state::main_state(source_man* sourceMgr) : state("main_state", MBtoB(256)) {
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

	main_state::~main_state() {
		// This state's memory should already be deallocated
		// by this point, unless the user deleted the state
		// while it was active (don't do that)
	}

	void main_state::onInitialize() {
		// The state's memory has been allocated, and is
		// currently active. From here until becameInactive
		// is called, all allocations made in this class
		// will be in the state's own memory scope (unless
		// explicitly allocated in the global scope using
		// memory_man::[push/pop]_current)
		setUpdateFrequency(60.0f);
	}

	void main_state::willBecomeActive() {
		// The state is going to become active. The previous
		// state is still active at this point, but will be
		// deactivated immediately after this function returns.
		// This state will be activated immediately after that

		m_spaces = new space_man(getScene());
		m_ui = new ui_man(m_sources, m_spaces, getScene());
		m_debugShader = getScene()->load_shader("./resources/shader/debug.glsl", "debug_shader");
		m_debugDraw = new debug_drawer(getScene(), m_debugShader, 131072 * 2, 8192 * 3);
		m_physicsDraw = new physics_drawer(m_debugDraw);

		auto& ps = physics_sys::get()->physState();
		ps.enable();
		//ps->world->setDebugDrawer(m_physicsDraw);
		ps.disable();

		r2engine::audio()->setListener(mat4f(1.0f));
		m_camera = new fly_camera_entity();

		getScene()->clearColor = vec4f(0, 0, 0, 1.0f);
	}

	void main_state::becameActive() {
		// The state was activated completely, and the previous
		// state (if any) has been cleared

		r2engine::get()->initialize_new_entities();
		m_spaces->get_current()->initialize();
	}

	void main_state::willBecomeInactive() {
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

	void main_state::becameInactive() {
		// Anything allocated here will be deallocated immediately
		// after this function returns, unless it's explicitly
		// allocated in the global scope via:
		// memory_man::push_current(memory_man::global())
		// ...allocate stuff...
		// memory_man::pop_current()
	}

	void main_state::willBeDestroyed() {
		// By the time this function is called, unless the state is
		// currently active (should never happen unless the user
		// deletes the state for some reason), the state's memory
		// will already be deallocated, and any allocations made here
		// will be made either in the global scope, or the currently
		// active state's scope. Be careful to not allocate here, or
		// to specify which scope the allocation _should_ be made in.
		// This includes allocations made internally by std containers
	}

	void main_state::onUpdate(f32 frameDt, f32 updateDt) {
		// Will be called as frequently as the user specifies with
		// setUpdateFrequency(frequency in Hz), as long as the
		// application is able to achieve that frequency.
		// Rendering probably can't be done here, as it's called
		// outside of the context of the frame.

		//printf("TestState::onUpdate(%.2f ms, %.2f ms)\n", frameDt * 1000.0f, updateDt * 1000.0f);

		m_ui->update(frameDt, updateDt);
		m_spaces->get_current()->update(updateDt);
	}

	void main_state::onRender() {
		scene_entity* camera = getScene()->camera;
		if (camera) r2engine::audio()->setListener(glm::inverse(camera->transform->transform));
		m_ui->render();

		GLFWwindow* window = *r2engine::get()->window();
		char title[128] = { 0 };
		snprintf(title, 128, "Rosen | %6.2f FPS | %8s / %8s", r2engine::get()->fps(), format_size(getUsedMemorySize()), format_size(getMaxMemorySize()));
		glfwSetWindowTitle(window, title);

		//ImGui::InputFloat("lod ratio", &speech_system::get()->dist_lod_skip_mult, 0.001f, 0.01f, 3);
		if (ImGui::Button("Toggle Space Cam")) {
			if (m_camera->camera->is_active()) m_spaces->get_current()->set_current_camera(0, false);
			else camera_sys::get()->activate_camera(m_camera);
		}

		m_debugDraw->begin();
		//m_debugDraw->line(vec3f(0, 0, 0), vec3f(100, 0, 0), vec4f(1, 0, 0, 1));
		//m_debugDraw->line(vec3f(0, 0, 0), vec3f(0, 100, 0), vec4f(0, 1, 0, 1));
		//m_debugDraw->line(vec3f(0, 0, 0), vec3f(0, 0, 100), vec4f(0, 0, 1, 1));

		//m_spaces->get_current()->debug_draw(m_debugDraw);

		lighting_component* lights[16];
		size_t light_count = lighting_sys::get()->get_lights(16, lights);
		for (u8 i = 0;i < light_count;i++) {
			lighting_component* light = lights[i];
			char buf[32] = { 0 };
			snprintf(buf, 32, "##light%d", i);
			ImGui::BeginChild(buf, ImVec2(600, 300));

			ImGui::Text("Light %s", light->entity()->name().c_str());

			ImGui::DragFloat3((mstring("Color") + buf).c_str(), &light->color.x, 0.01f);
			if (ImGui::DragFloat((mstring("coneInnerAngle") + buf).c_str(), &light->coneInnerAngle, 0.01f)) {
				if (light->coneInnerAngle > light->coneOuterAngle) light->coneOuterAngle = light->coneInnerAngle;
				if (light->coneInnerAngle < 0.0f) light->coneInnerAngle = 0.0f;
			}
			if (ImGui::DragFloat((mstring("coneOuterAngle") + buf).c_str(), &light->coneOuterAngle, 0.01f)) {
				if (light->coneOuterAngle < light->coneInnerAngle) light->coneInnerAngle = light->coneOuterAngle;
				if (light->coneOuterAngle < 0.0f) light->coneOuterAngle = 0.0f;
			}
			ImGui::DragFloat((mstring("constantAtt") + buf).c_str(), &light->constantAttenuation, 0.01f);
			ImGui::DragFloat((mstring("linearAtt") + buf).c_str(), &light->linearAttenuation, 0.01f);
			ImGui::DragFloat((mstring("quadraticAtt") + buf).c_str(), &light->quadraticAttenuation, 0.01f);

			ImGui::EndChild();
			/*
			f32 md = 15.0f;
			f32 ri = md * tanf(glm::radians(light->coneInnerAngle));
			f32 ro = md * tanf(glm::radians(light->coneOuterAngle));

			mat4f gt = light->entity()->transform->transform;
			gt = glm::translate(gt, vec3f(0, -md * 0.5f, 0));
			btTransform t;
			t.setFromOpenGLMatrix(&gt[0][0]);
			m_physicsDraw->drawCone(ri, md, 1, t, btVector3(light->color.x, light->color.y, light->color.z));
			m_physicsDraw->drawCone(ro, md, 1, t, btVector3(light->color.x, light->color.y, light->color.z));
			m_physicsDraw->drawTransform(t, 5.0f);
			*/
		}

		auto& ps = physics_sys::get()->physState();
		ps.enable();
		ps->world->debugDrawWorld();
		ps.disable();
		m_debugDraw->end();
	}

	void main_state::onEvent(event* evt) {
		// Will be called whenever a non-internal event is fired,
		// or in the case of deferred events, at the beginning of
		// the engine tick (before onUpdate would be called)
	}
};