#include <entities/space_element.h>
#include <glm/gtc/matrix_inverse.hpp>
using namespace r2;

namespace rosen {
	space_element_entity::space_element_entity(const mstring& name, render_node* node, const mat4f& transform) : scene_entity(name), m_node(node), m_initialTransform(transform) {
	}

	space_element_entity::~space_element_entity() {
	}

	void space_element_entity::onInitialize() {
		transform_sys::get()->addComponentTo(this);
		transform->transform = m_initialTransform;

		mesh_sys::get()->addComponentTo(this);
		mesh->set_node(m_node);

		setUpdateFrequency(60.0f);
	}

	void space_element_entity::onUpdate(f32 frameDt, f32 updateDt) {
		mat4f t = transform->transform;

		struct instance_data {
			mat4f transform;
			mat3f normal_transform;
			i32 entity_id;
		} instance = { t, glm::inverseTranspose(mat3f(t)), (i32)id() };
		
		mesh->set_instance_data<instance_data>(instance);
	}

	void space_element_entity::onEvent(event* evt) {
	}

	void space_element_entity::willBeDestroyed() {
	}

	void space_element_entity::belowFrequencyWarning(f32 percentLessThanDesired, f32 desiredFreq, f32 timeSpentLowerThanDesired) {
	}
};