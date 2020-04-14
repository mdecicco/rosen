#include <entities/rosen.h>
using namespace r2;

#include <systems/speech.h>
#include <managers/source_man.h>

#define TEST_POS_DIMENSION_MAX 5.0f

namespace rosen {
	f32 random(f32 min, f32 max) {
		f32 r = f32(rand()) / f32(RAND_MAX);
		r *= max - min;
		return min + r;
	}
	f32 random(f32 range) {
		return random(-range, range);
	}

	rosen_entity::rosen_entity(const mstring& name, render_node* _node) : scene_entity(name), pos(vec3f(random(TEST_POS_DIMENSION_MAX), random(TEST_POS_DIMENSION_MAX), random(TEST_POS_DIMENSION_MAX)), 5.0f, r2::interpolate::easeInOutCubic) {
		texture = nullptr;
		node = _node;
		shirt_color_hsv = vec3f(random(0.0f, 1.0f), random(0.0f, 1.0f), random(1.0f, 5.0f));
	}

	rosen_entity::~rosen_entity() {
	}

	void rosen_entity::onInitialize() {
		texture = r2engine::current_scene()->create_texture();
		texture->create(1, 1, 3, tt_unsigned_byte);

		speech_system::get()->addComponentTo(this);
		transform_sys::get()->addComponentTo(this);
		mesh_sys::get()->addComponentTo(this);
		mesh->set_node(node);

		speech()->texture = texture;
		speech()->audio->setPosition(pos);
		speech()->audio->setRolloffFactor(5.0f);

		transform->transform = glm::translate(mat4f(1.0f), (vec3f)pos);
		mesh->set_instance_data(transform->transform);
		mesh->get_node()->material_instance()->uniforms()->uniform_vec3f("shirt_tint", shirt_color_hsv);

		setUpdateFrequency(60.0f);

		speak_nonsense(5);
	}

	void rosen_entity::onUpdate(f32 frameDt, f32 updateDt) {
		if (pos.stopped()) {
			pos = vec3f(random(TEST_POS_DIMENSION_MAX), random(TEST_POS_DIMENSION_MAX), random(TEST_POS_DIMENSION_MAX));
		}

		transform->transform = glm::translate(mat4f(1.0f), (vec3f)pos);
		speech()->audio->setPosition(pos);
		mesh->set_instance_data(transform->transform);
	}

	void rosen_entity::onEvent(event* evt) {
		if (evt->name() == "SPEECH_FINISHED") {
			speak_nonsense(rand() % 30);
			speech()->audio->setPitch(random(0.5f, 1.5f));
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