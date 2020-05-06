#include <entities/space_collision_element.h>
using namespace r2;

namespace rosen {
	space_collision_element_entity::space_collision_element_entity(const mstring& name, const mat4f& transform, btCollisionShape* shape) :
		scene_entity(name),
		m_initialTransform(transform),
		m_shape(shape)
	{
	}

	space_collision_element_entity::~space_collision_element_entity() {
	}

	void space_collision_element_entity::onInitialize() {
		transform_sys::get()->addComponentTo(this);
		transform->transform = m_initialTransform;

		physics_sys::get()->addComponentTo(this);
		physics->set_mass(0.0f);
		physics->set_shape(m_shape);

		stop_periodic_updates();
	}

	void space_collision_element_entity::onUpdate(f32 frameDt, f32 updateDt) {
	}

	void space_collision_element_entity::onEvent(event* evt) {
	}

	void space_collision_element_entity::willBeDestroyed() {
	}

	void space_collision_element_entity::belowFrequencyWarning(f32 percentLessThanDesired, f32 desiredFreq, f32 timeSpentLowerThanDesired) {
	}
};