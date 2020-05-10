#include <entities/space_light_element.h>
#include <systems/control.h>
using namespace r2;

namespace rosen {
	inline bool load_anim(const mstring& path, scene_entity* entity) {
		if (r2engine::files()->exists(path)) {
			data_container* file = r2engine::files()->open(path, DM_BINARY, entity->name() + " (animation)");
			if (file) {
				char hdr[4] = { 0 };
				if (!file->read_data(hdr, 4)) {
					r2engine::files()->destroy(file);
					return false;
				}

				if (hdr[0] != 'A' || hdr[1] != 'N' || hdr[2] != 'I' || hdr[3] != 'M') {
					r2Error("'%s' is not a valid .anim file", path.c_str());
					r2engine::files()->destroy(file);
					return false;
				}

				u8 anim_count = 0;
				if (!file->read(anim_count)) {
					r2engine::files()->destroy(file);
					return false;
				}

				for (u8 i = 0;i < anim_count;i++) {
					try {
						entity->animation->animations.push(new animation_group(file, entity));
					} catch (std::exception& e) {
						r2Error(e.what());
					}
				}

				r2engine::files()->destroy(file);
			}
		}
	}

	space_light_element_entity::space_light_element_entity(const mstring& name, const light_props& props, const mstring& animFile)
		: scene_entity(name), m_initial_props(props), m_animFile(animFile)
	{
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

		animation_sys::get()->addComponentTo(this);
		load_anim(m_animFile, this);

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