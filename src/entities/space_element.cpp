#include <entities/space_element.h>
using namespace r2;

namespace rosen {
	space_element_entity::space_element_entity(const mstring& name, render_node* node, const mat4f& transform) : scene_entity(name), m_node(node), m_initialTransform(transform) {
		node->uniforms()->uniform_mat4f("transform", transform);
	}

	space_element_entity::~space_element_entity() {
	}

	void space_element_entity::onInitialize() {
		transform_sys::get()->addComponentTo(this);
		transform->transform = m_initialTransform;
		setUpdateFrequency(1.0f);
	}

	void space_element_entity::onUpdate(f32 frameDt, f32 updateDt) {
		m_node->uniforms()->uniform_mat4f("transform", transform->transform);
	}

	void space_element_entity::onEvent(event* evt) {
	}

	void space_element_entity::willBeDestroyed() {
	}

	void space_element_entity::belowFrequencyWarning(f32 percentLessThanDesired, f32 desiredFreq, f32 timeSpentLowerThanDesired) {
	}
};