#include <systems/speech.h>
#include <r2/managers/audioman.h>
#include <r2/engine.h>
#include <managers/source_man.h>

namespace rosen {
	speech_component::speech_component() {
		plan = nullptr;
		execution = nullptr;
		texture = nullptr;
		audio = new audio_source(NO_AUDIO_BUFFER);
		lod_skip_interval = 0.0f;
		lod_skip_time = 0.0f;
		lod_falloff_start_dist = 5.0f;
		volume = 1.0f;
		pitch = 1.0f;
	}

	speech_component::~speech_component() {
		if (plan) delete plan; plan = nullptr;
		if (execution) delete execution; execution = nullptr;
		if (audio) delete audio; audio = nullptr;
	}

	void speech_component::speak(const r2::mstring& text) {
		speech_system* sys = speech_system::get();

		if (plan) delete plan;
		if (execution) delete execution;

		plan = sys->sources->plan_speech(text);
		execution = new speech_execution_context(plan);
	}

	void speech_component::speak_nonsense(u32 word_count) {
		if (word_count == 0) return;

		speech_system* sys = speech_system::get();

		plan = new speech_plan();

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
			if (p.plan) plan->append(p.plan);
			else plan->add(sys->sources->source(p.srcIdx), p.snipIdx);
		}

		execution = new speech_execution_context(plan);
	}


	speech_system* speech_system::instance = nullptr;

	speech_system::speech_system() {
		sources = nullptr;
	}

	speech_system::~speech_system() {
	}


	speech_system* speech_system::create() {
		if (instance) return instance;
		instance = new speech_system();
		return instance;
	}

	speech_system* speech_system::get() {
		return instance;
	}


	void speech_system::initialize() {
		initialize_event_receiver();
		initialize_periodic_update();
		setUpdateFrequency(25);
		start_periodic_updates();
		dist_lod_skip_mult = 0.001f;

		r2engine::register_entity_property<f32>("speech.volume");
		r2engine::register_entity_property<f32>("speech.pitch");
		r2engine::register_entity_property<f32>("speech.lod_falloff_dist");
	}

	void speech_system::deinitialize() {
		destroy_event_receiver();
		destroy_periodic_update();
	}


	void speech_system::initialize_entity(r2::scene_entity* entity) {
		if (!entity->is_scripted()) return;
		entity->bind(this, "add_speech_component", [](entity_system* system, scene_entity* entity, v8Args args) {
			system->addComponentTo(entity);
		});
	}


	void speech_system::deinitialize_entity(r2::scene_entity* entity) {
		if (!entity->is_scripted()) return;
		auto s = state();
		s.enable();
		if (!s->contains_entity(entity->id())) {
			entity->unbind("add_speech_component");
		} else entity->unbind("speech");
		s.disable();
	}


	r2::scene_entity_component* speech_system::create_component(r2::entityId id) {
		auto s = state();
		s.enable();
		auto out = s->create<speech_component>(id);
		s.disable();
		return out;
	}


	void speech_system::bind(r2::scene_entity_component* component, r2::scene_entity* entity) {
		using c = speech_component;
		if (entity->is_scripted()) {
			entity->unbind("add_speech_component");
			entity->bind(this, "speech", "remove", [](entity_system* system, scene_entity* entity, v8Args args) {
				system->removeComponentFrom(entity);
			});
			entity->bind(this, "speech", "speak", [](entity_system* system, scene_entity* entity, v8Args args) {
				if (args.Length() != 1 || !args[0]->IsString()) {
					r2Error("Expected speech text to be passed to entity.speak()");
					args.GetReturnValue().Set(v8pp::convert<f32>::to_v8(args.GetIsolate(), 0.0f));
					return;
				}

				mstring text = v8pp::convert<mstring>::from_v8(r2engine::isolate(), args[0]);

				speech_system* sys = (speech_system*)system;
				auto& state = system->state();
				state.enable();
				speech_component* comp = (speech_component*)state->entity(entity->id());
				comp->speak(text);
				comp->execution->update(comp->audio, comp->texture);
				state.disable();

				args.GetReturnValue().Set(v8pp::convert<f32>::to_v8(args.GetIsolate(), comp->plan->duration));
			});
			entity->bind(this, "speech", "cancel_speech", [](entity_system* system, scene_entity* entity, v8Args args) {
				speech_system* sys = (speech_system*)system;
				auto& state = system->state();
				state.enable();
				speech_component* comp = (speech_component*)state->entity(entity->id());
				if (comp->plan) delete comp->plan;
				if (comp->execution) delete comp->execution;
				comp->plan = nullptr;
				comp->execution = nullptr;
				comp->audio->stop();
				state.disable();
			});
			entity->bind(this, "speech", "grab_frame", [](entity_system* system, scene_entity* entity, v8Args args) {
				if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsNumber()) {
					r2Error("Expected source name, frame time to be passed to entity.grab_frame()");
					return;
				}

				mstring name = v8pp::convert<mstring>::from_v8(r2engine::isolate(), args[0]);
				speech_system* sys = (speech_system*)system;
				source_content* source = sys->sources->source(name);
				if (source) {
					f32 time = v8pp::convert<f32>::from_v8(r2engine::isolate(), args[1]);

					auto& state = system->state();
					state.enable();
					speech_component* comp = (speech_component*)state->entity(entity->id());

					source->frame(time, comp->texture);
					state.disable();
				}
			});
			entity->bind(this, "speech", "is_speaking", [](entity_system* system, scene_entity* entity, v8Args args) {
				speech_system* sys = (speech_system*)system;
				auto& state = system->state();
				state.enable();
				speech_component* comp = (speech_component*)state->entity(entity->id());
				bool speaking = comp->execution != nullptr;
				state.disable();

				args.GetReturnValue().Set(v8pp::convert<bool>::to_v8(args.GetIsolate(), speaking));
			});
			entity->bind_interpolatable(component, "speech", "volume", &c::volume);
			entity->bind_interpolatable(component, "speech", "pitch", &c::pitch);
			entity->bind_interpolatable(component, "speech", "lod_falloff_dist", &c::lod_falloff_start_dist);
		}
	}


	void speech_system::unbind(r2::scene_entity* entity) {
		if (entity->is_scripted()) {
			entity->unbind("speech");
			entity->bind(this, "add_speech_component", [](entity_system* system, scene_entity* entity, v8Args args) {
				system->addComponentTo(entity);
			});
		}
	}


	void speech_system::tick(r2::f32 dt) {
		update(dt);
	}


	void speech_system::handle(r2::event* evt) {
	}


	void speech_system::doUpdate(r2::f32 frameDelta, r2::f32 updateDelta) {
		scene* cur_scene = r2engine::current_scene();
		const camera_frustum* frustum = nullptr;
		vec3f cam_pos;
		if (cur_scene && cur_scene->camera) {
			frustum = &cur_scene->camera->camera->frustum();
			cam_pos = glm::inverse(cur_scene->camera->transform->transform) * vec4f(0, 0, 0, 1.0f);
		}

		auto& state = this->state();
		state.enable();
		state->for_each<speech_component>([this, frustum, updateDelta, cam_pos](speech_component* comp) {
			bool in_frame = true;
			if (comp->entity()->transform && frustum) {
				vec3f pos = comp->entity()->transform->transform * vec4f(0, 0, 0, 1.0f);
				f32 dist = glm::length(cam_pos - pos);
				comp->lod_skip_interval = dist > comp->lod_falloff_start_dist ? pow(dist - comp->lod_falloff_start_dist, 2.0f) * dist_lod_skip_mult : 0.0f;
				in_frame = frustum->contains(pos, 1.0f);
			} else comp->lod_skip_interval = 0.0f;

			if (in_frame) {
				if (comp->volume != comp->audio->gain()) comp->audio->setGain(comp->volume);
				if (comp->pitch != comp->audio->pitch()) comp->audio->setPitch(comp->pitch);
			}

			if (comp->execution) {
				if (!comp->audio->isPlaying()) comp->audio->play();

				bool do_update_tex = comp->lod_skip_interval == 0.0f;
				comp->lod_skip_time -= updateDelta;
				if (comp->lod_skip_time <= 0.0f) {
					comp->lod_skip_time = comp->lod_skip_interval;
					do_update_tex = true;
				}
				
				comp->execution->update(comp->audio, in_frame && do_update_tex ? comp->texture : nullptr);
				
				if (comp->execution->completed) {
					delete comp->execution; comp->execution = nullptr;
					delete comp->plan; comp->plan = nullptr;
					comp->audio->stop();
					event e = evt("SPEECH_FINISHED");
					comp->entity()->dispatch(&e);
				}

				if (in_frame) comp->entity()->mesh->get_node()->material_instance()->set_texture("tex", comp->texture);
			}
			return true;
		});
		state.disable();
	}

};