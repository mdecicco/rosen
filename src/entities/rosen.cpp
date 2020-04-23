#include <entities/rosen.h>
using namespace r2;

#include <systems/speech.h>
#include <systems/control.h>
#include <managers/source_man.h>

namespace rosen {
	f32 random(f32 min, f32 max) {
		f32 r = f32(rand()) / f32(RAND_MAX);
		r *= max - min;
		return min + r;
	}
	f32 random(f32 range) {
		return random(-range, range);
	}

	rosen_entity::rosen_entity(const mstring& name, render_node* _node) : scene_entity(name) {
		controlled = false;
		texture = nullptr;
		node = _node;
		shirt_color_hsv = vec3f(random(0.0f, 1.0f), random(0.0f, 1.0f), random(1.0f, 5.0f));
	}

	rosen_entity::~rosen_entity() {
	}

	void rosen_entity::onInitialize() {
		texture = r2engine::current_scene()->create_texture();
		texture->create(1, 1, 3, tt_unsigned_byte);
		
		transform_sys::get()->addComponentTo(this);
		transform->transform = mat4f(1.0f);

		mesh_sys::get()->addComponentTo(this);
		node->material_instance()->uniforms()->uniform_vec3f("shirt_tint", shirt_color_hsv);
		mesh->set_node(node);
		mesh->set_instance_data(transform->transform);

		physics_sys::get()->addComponentTo(this);
		physics->set_mass(1.0f);
		physics->set_shape(new btSphereShape(0.5f));
		physics->rigidBody()->setAngularFactor(btVector3(0.0f, 1.0f, 0.0f));
		physics->rigidBody()->setSpinningFriction(5.0f);
		physics->rigidBody()->setActivationState(DISABLE_DEACTIVATION);

		speech_system::get()->addComponentTo(this);
		speech()->texture = texture;
		speech()->audio->setRolloffFactor(1.0f);
		speech()->audio->setPitch(1.0f);

		control_system::get()->addComponentTo(this);
		control()->control_enabled = controlled;
		control()->movement_speed = 20.0f;
		control()->jump_impulse = 5.0f;


		setUpdateFrequency(60.0f);

		speak_nonsense(5);
	}

	void rosen_entity::onUpdate(f32 frameDt, f32 updateDt) {
		speech()->audio->setPosition(transform->transform * vec4f(0.0f, 0.0f, 0.0f, 1.0f));
		mesh->set_instance_data(transform->transform);

		if (!speech()->audio->isPlaying()) speak_nonsense(5);
	}

	void rosen_entity::onEvent(event* evt) {
		if (evt->name() == "SPEECH_FINISHED") {
			speak_nonsense(rand() % 30);
		}
	}

	void rosen_entity::willBeDestroyed() {
		speech()->texture = nullptr;
		r2engine::current_scene()->destroy(texture);
	}

	void rosen_entity::belowFrequencyWarning(f32 percentLessThanDesired, f32 desiredFreq, f32 timeSpentLowerThanDesired) {
	}

	speech_component* rosen_entity::speech() {
		auto& speech = speech_system::get()->state();
		speech.enable();
		speech_component* comp = (speech_component*)speech->entity(id());
		speech.disable();
		return comp;
	}

	control_component* rosen_entity::control() {
		auto& control = control_system::get()->state();
		control.enable();
		control_component* comp = (control_component*)control->entity(id());
		control.disable();
		return comp;
	}

	void rosen_entity::speak(const r2::mstring& text) {
		speech_system* sys = speech_system::get();
		speech_component* comp = speech();

		if (comp->plan) delete comp->plan;
		if (comp->execution) delete comp->execution;

		comp->plan = sys->sources->plan_speech(text);
		comp->execution = new speech_execution_context(comp->plan);
	}

	void rosen_entity::speak_nonsense(u32 word_count) {
		if (word_count == 0) return;

		speech_system* sys = speech_system::get();

		speech_component* comp = speech();
		comp->plan = new speech_plan();

		struct possible {
			u32 srcIdx;
			u32 snipIdx;
			speech_plan* plan;
		};
		mvector<possible> all;

		for (u32 i = 0;i < sys->sources->source_count();i++) {
			source_content* src = sys->sources->source(i);
			for (u32 s = 0;s < src->snippets.size();s++) {
				all.push_back({ i, s, nullptr });
			}
		}

		for (u32 i = 0;i < sys->sources->mixedWords.size();i++) {
			all.push_back({ 0, 0, sys->sources->mixedWords[i].plan });
		}

		for (u32 w = 0;w < word_count;w++) {
			possible& p = all[rand() % all.size()];
			if (p.plan) comp->plan->append(p.plan);
			else comp->plan->add(sys->sources->source(p.srcIdx), p.snipIdx);
		}

		comp->execution = new speech_execution_context(comp->plan);
	}
};