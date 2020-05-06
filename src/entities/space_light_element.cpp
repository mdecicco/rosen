#include <entities/space_light_element.h>
#include <systems/control.h>
using namespace r2;

namespace rosen {
	space_light_element_entity::space_light_element_entity(const mstring& name, const light_props& props) : scene_entity(name), m_initial_props(props) {
	}

	space_light_element_entity::~space_light_element_entity() {
	}

	void space_light_element_entity::onInitialize() {
		transform_sys::get()->addComponentTo(this);
		transform->transform = m_initial_props.transform;

		lighting_sys::get()->addComponentTo(this);
		lighting->type = m_initial_props.type;
		lighting->color = m_initial_props.color;
		lighting->coneInnerAngle = m_initial_props.coneInnerAngle;
		lighting->coneOuterAngle = m_initial_props.coneOuterAngle;
		lighting->constantAttenuation = m_initial_props.constantAtt;
		lighting->linearAttenuation = m_initial_props.linearAtt;
		lighting->quadraticAttenuation = m_initial_props.quadraticAtt;

		//setUpdateFrequency(60.0f);
		stop_periodic_updates();
	}

	void space_light_element_entity::onUpdate(f32 frameDt, f32 updateDt) {
	}

	void space_light_element_entity::onEvent(event* evt) {
	}

	void space_light_element_entity::willBeDestroyed() {
	}

	void space_light_element_entity::belowFrequencyWarning(f32 percentLessThanDesired, f32 desiredFreq, f32 timeSpentLowerThanDesired) {
	}
};