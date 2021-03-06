#include <entities/space_element.h>
#include <glm/gtc/matrix_inverse.hpp>
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

	space_element_entity::space_element_entity(const mstring& name, render_node* node, const mat4f& transform, const mstring& animFile)
		: scene_entity(name), m_node(node), m_initialTransform(transform), m_animFile(animFile)
	{
	}

	space_element_entity::~space_element_entity() {
	}

	void space_element_entity::onInitialize() {
		transform_sys::get()->addComponentTo(this);
		transform->transform = m_initialTransform;

		mesh_sys::get()->addComponentTo(this);
		mesh->set_node(m_node);

		animation_sys::get()->addComponentTo(this);
		load_anim(m_animFile, this);

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