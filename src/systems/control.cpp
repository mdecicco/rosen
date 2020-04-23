#include <systems/control.h>

#include <r2/engine.h>
using namespace r2;

namespace rosen {
	control_component::control_component() {
		movement_speed = 1.0f;
		jump_impulse = 1.0f;
		control_enabled = false;
		on_ground = true;
	}

	control_component::~control_component() {
	}


	control_system* control_system::instance = nullptr;

	control_system::control_system() {
		analog_deadzone = 0.1f;
	}

	control_system::~control_system() {
	}

	control_system* control_system::create() {
		if (instance) return instance;
		instance = new control_system();
		return instance;
	}

	control_system* control_system::get() {
		return instance;
	}

	void control_system::initialize() {
		initialize_event_receiver();
		initialize_periodic_update();
		setUpdateFrequency(60);
		start_periodic_updates();
	}

	void control_system::deinitialize() {
		destroy_event_receiver();
		destroy_periodic_update();
	}

	void control_system::initialize_entity(r2::scene_entity* entity) {
		if (!entity->is_scripted()) return;
		entity->bind(this, "add_control_component", [](entity_system* system, scene_entity* entity, v8Args args) {
			system->addComponentTo(entity);
		});
	}

	void control_system::deinitialize_entity(r2::scene_entity* entity) {
		if (!entity->is_scripted()) return;
		auto s = state();
		s.enable();
		if (!s->contains_entity(entity->id())) {
			entity->unbind("add_control_component");
		} else entity->unbind("remove_control_component");
		s.disable();
	}

	r2::scene_entity_component* control_system::create_component(r2::entityId id) {
		auto s = state();
		s.enable();
		auto out = s->create<control_component>(id);
		s.disable();
		return out;
	}

	void control_system::bind(r2::scene_entity_component* component, r2::scene_entity* entity) {
		using c = control_component;
		if (entity->is_scripted()) {
			entity->unbind("add_control_component");
			entity->bind(component, "control_enabled", &control_component::control_enabled);
			entity->bind(component, "movement_speed", &control_component::movement_speed);
			entity->bind(this, "remove_control_component", [](entity_system* system, scene_entity* entity, v8Args args) {
				system->removeComponentFrom(entity);
			});
		}
	}

	void control_system::unbind(r2::scene_entity* entity) {
		if (entity->is_scripted()) {
			entity->unbind("control_enabled");
			entity->unbind("movement_speed");
			entity->bind(this, "add_control_component", [](entity_system* system, scene_entity* entity, v8Args args) {
				system->addComponentTo(entity);
			});
		}
	}

	void control_system::tick(r2::f32 dt) {
		update(dt);
	}

	void control_system::handle(r2::event* evt) {
	}

	void control_system::doUpdate(r2::f32 frameDelta, r2::f32 updateDelta) {
		vec2f xzMovement = vec2f(0, 0);

		bool jumpPressed = false;

		if (r2engine::input()->joystick_count() > 0) {
			auto js = r2engine::input()->joystick(0);
			const auto& jState = js->getJoyStickState();

			vec2f lstick = vec2f(f32(jState.mAxes[1].abs) / 32767.0f, f32(jState.mAxes[0].abs) / 32767.0f);
			//vec2f rstick = vec2f(f32(jState.mAxes[3].abs) / 32767.0f, f32(jState.mAxes[2].abs) / 32767.0f);


			jumpPressed = jState.mButtons[10];

			xzMovement = lstick * vec2f(1.0f, -1.0f);
			if (glm::length(lstick) < analog_deadzone) xzMovement = vec2f(0.0f, 0.0f);
		}

		scene* cur_scene = r2engine::current_scene();
		vec3f cam_pos;
		if (cur_scene && cur_scene->camera) {
			mat4f cam = glm::inverse(cur_scene->camera->transform->transform);
			cam_pos = cam * vec4f(0, 0, 0, 1);
		}

		auto& pstate = physics_sys::get()->physState();
		pstate.enable();
		btDynamicsWorld* pworld = pstate->world;
		pstate.disable();

		auto& state = this->state();
		state.enable();
		state->for_each<control_component>([this, pworld, updateDelta, cam_pos, xzMovement, jumpPressed](control_component* comp) {
			if (!comp->control_enabled) return true;

			scene_entity* entity = comp->entity();
			transform_component* tc = entity->transform.get();
			if (!tc) return true;
			physics_component* pc = entity->physics.get();

			vec3f pos = tc->transform * vec4f(0.0f, 0.0f, 0.0f, 1.0f);
			if (isnan(pos.x) || isnan(pos.y) || isnan(pos.z)) return true;

			vec3f forward = glm::normalize(pos - cam_pos);
			vec3f right = glm::normalize(glm::cross(forward, vec3f(0, 1, 0)));
			forward.y = 0.0f;
			right.y = 0.0f;

			vec3f trans = forward * xzMovement.y * comp->movement_speed;
			trans += right * xzMovement.x * comp->movement_speed;

			mat4f t = glm::translate(tc->transform, trans);

			if (pc) {
				if (xzMovement.x != 0.0f || xzMovement.y != 0.0f) {
					btVector3 vel = pc->rigidBody()->getLinearVelocity();
					pc->rigidBody()->setLinearVelocity(btVector3(trans.x * 0.1f, vel.y(), trans.z * 0.1f));
				}

				btVector3 from = btVector3(pos.x, pos.y, pos.z);
				btVector3 to = from - btVector3(0.0f, 1000.0f, 0.0f);
				btCollisionWorld::AllHitsRayResultCallback result(from, to);
				pworld->rayTest(from, to, result);
				btRigidBody* rb = pc->rigidBody();

				if (result.hasHit()) {
					u32 hitIdx = UINT32_MAX;
					for (u32 i = 0;i < result.m_collisionObjects.size();i++) {
						if (result.m_collisionObjects[i] != rb) {
							hitIdx = i;
							break;
						}
					}

					if (hitIdx != UINT32_MAX) {
						if ((from - result.m_hitPointWorld[hitIdx]).length() > 0.1f) {
							comp->on_ground = false;
						} else if (jumpPressed && (this->last_jump_press.stopped() || this->last_jump_press.elapsed() > 0.5f)) {
							comp->on_ground = true;
							this->last_jump_press.reset();
							this->last_jump_press.start();
							pc->rigidBody()->applyCentralImpulse(btVector3(0.0f, comp->jump_impulse, 0.0f));
						} else comp->on_ground = true;
					}
				}
			} else tc->transform = glm::translate(tc->transform, trans);;

			return true;
		});
		state.disable();
	}
};