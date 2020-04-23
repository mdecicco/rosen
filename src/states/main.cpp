#include <states/main.h>
#include <managers/source_man.h>
#include <managers/ui_man.h>
#include <managers/space_man.h>
#include <entities/rosen.h>
#include <systems/speech.h>
#include <utils/physics_drawer.h>

#include <r2/managers/drivers/gl/driver.h>
#include <r2/engine.h>
#include <r2/utilities/fly_camera.h>
#include <r2/utilities/debug_drawer.h>

namespace rosen {
	render_node* gen_rosen_node(scene* s, shader_program* shader) {
		vertex_format* vfmt = new vertex_format();
		vfmt->add_attr(vat_vec3f);
		vfmt->add_attr(vat_vec2f);

		instance_format* ifmt = new instance_format();
		ifmt->add_attr(iat_mat4f, true);

		uniform_format* mfmt = new uniform_format();
		mfmt->add_attr("shirt_tint", uat_vec3f);

		mesh_construction_data* mesh = new mesh_construction_data(vfmt, it_unsigned_byte, ifmt);
		mesh->set_max_vertex_count(4);
		mesh->set_max_index_count(6);
		mesh->set_max_instance_count(1);

		struct vertex { vec3f pos; vec2f tex; };
		f32 width = 1.80555556f;
		f32 height = 1.0f;
		mesh->append_vertex<vertex>({ vec3f(-width * 0.5f,  0.5f, 0.0f), vec2f(0, 0) });
		mesh->append_vertex<vertex>({ vec3f( width * 0.5f,  0.5f, 0.0f), vec2f(1, 0) });
		mesh->append_vertex<vertex>({ vec3f( width * 0.5f, -0.5f, 0.0f), vec2f(1, 1) });
		mesh->append_vertex<vertex>({ vec3f(-width * 0.5f, -0.5f, 0.0f), vec2f(0, 1) });

		mesh->append_index<u8>(0);
		mesh->append_index<u8>(1);
		mesh->append_index<u8>(3);
		mesh->append_index<u8>(1);
		mesh->append_index<u8>(2);
		mesh->append_index<u8>(3);

		mesh->append_instance(mat4f(1.0f));

		render_node* node = s->add_mesh(mesh);
		node_material* mtrl = new node_material("u_material", mfmt);
		mtrl->set_shader(shader);
		node->set_material_instance(mtrl->instantiate(s));
		node->material_instance()->uniforms()->uniform_vec3f("shirt_tint", vec3f(0.3f, 0.5f, 2.0f));

		return node;
	};
	


	main_state::main_state(source_man* sourceMgr) : state("main_state", MBtoB(60)) {
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
		m_ui = new ui_man(m_sources, getScene());
		m_debugShader = getScene()->load_shader("./resources/shader/debug.glsl", "debug_shader");
		m_debugDraw = new debug_drawer(getScene(), m_debugShader, 131072 * 2, 8192 * 3);
		m_physicsDraw = new physics_drawer(m_debugDraw);

		auto& ps = physics_sys::get()->physState();
		ps.enable();
		//ps->world->setDebugDrawer(m_physicsDraw);
		ps.disable();

		r2engine::audio()->setListener(mat4f(1.0f));
		m_camera = new fly_camera_entity();

		for (u32 i = 0;i < 15;i++) {
			char a[4] = { 0 };
			snprintf(a, 4, "%d", i);
			rosen_entity* rosen = new rosen_entity("Michael_" + mstring(a), gen_rosen_node(getScene(), m_spaces->get_rosen_shader()));
			rosen->controlled = i == 0;
			m_rosens.push_back(rosen);
		}
	}

	void main_state::becameActive() {
		// The state was activated completely, and the previous
		// state (if any) has been cleared
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

		for (u32 i = 0;i < m_rosens.size();i++) m_rosens[i]->destroy();
		m_rosens.clear();

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
		static char msg[1024] = { 0 };
		ImGui::InputText("##say", msg, 1024);
		ImGui::SameLine(0.0f, 5.0f);
		if (ImGui::Button("Say")) {
			for (u32 i = 0;i < m_rosens.size();i++) m_rosens[i]->speak(msg);
		}
		if (ImGui::Button("Toggle Space Cam")) {
			if (m_camera->camera->is_active()) m_spaces->get_current()->set_current_camera(0, false);
			else camera_sys::get()->activate_camera(m_camera);
		}

		for (u8 i = 0;i < m_spaces->get_current()->light_count();i++) {
			char buf[32] = { 0 };
			snprintf(buf, 32, "##light%d", i);
			ImGui::BeginChild(buf, ImVec2(600, 300));

			ImGui::Text("Light %d", i);

			rosen_space::light_def* light = m_spaces->get_current()->light(i);
			if (ImGui::DragFloat3((mstring("Position") + buf).c_str(), &light->position.x)) m_spaces->get_current()->update_uniforms();
			if (ImGui::DragFloat3((mstring("Direction") + buf).c_str(), &light->direction.x)) m_spaces->get_current()->update_uniforms();
			if (ImGui::DragFloat3((mstring("Color") + buf).c_str(), &light->color.x)) m_spaces->get_current()->update_uniforms();
			if (ImGui::DragFloat((mstring("cosConeInnerAngle") + buf).c_str(), &light->cosConeInnerAngle, 0.01f)) m_spaces->get_current()->update_uniforms();
			if (ImGui::DragFloat((mstring("cosConeOuterAngle") + buf).c_str(), &light->cosConeOuterAngle, 0.01f)) m_spaces->get_current()->update_uniforms();
			if (ImGui::DragFloat((mstring("constantAtt") + buf).c_str(), &light->constantAtt, 0.01f)) m_spaces->get_current()->update_uniforms();
			if (ImGui::DragFloat((mstring("linearAtt") + buf).c_str(), &light->linearAtt, 0.01f)) m_spaces->get_current()->update_uniforms();
			if (ImGui::DragFloat((mstring("quadraticAtt") + buf).c_str(), &light->quadraticAtt, 0.01f)) m_spaces->get_current()->update_uniforms();

			ImGui::EndChild();
		}

		m_debugDraw->begin();
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