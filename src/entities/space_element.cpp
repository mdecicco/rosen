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

		animation_sys::get()->addComponentTo(this);
		animation_group* a = new animation_group("ball", 5.0f, true);
		a->add_track<mat4f>("bounce", [](void* entity) {
			scene_entity* e = (scene_entity*)entity;
			if (!e->transform) return (mat4f*)nullptr;
			return &e->transform->transform;
		}, [](const mat4f& a, const mat4f& b, f32 w) {
			return a + ((b - a) * interpolate::easeInOutCubic(w));
		}, this);
		a->set("bounce", glm::translate(transform->transform, vec3f(0.0f, -5.0f, 0.0f)), 1.0f);
		a->set("bounce", glm::translate(transform->transform, vec3f(0.0f, 2.5f, 0.0f)), 2.0f);
		a->set("bounce", glm::translate(transform->transform, vec3f(0.0f, -2.5f, 0.0f)), 3.0f);
		a->set("bounce", glm::translate(transform->transform, vec3f(0.0f, 5.0f, 0.0f)), 4.0f);
		a->set("bounce", transform->transform, 5.0f);
		a->loops(true);
		a->play();
		animation->animations.push(a);

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