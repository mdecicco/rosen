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
	}

	speech_component::~speech_component() {
		if (plan) delete plan; plan = nullptr;
		if (execution) delete execution; execution = nullptr;
		if (audio) delete audio; audio = nullptr;
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
		} else entity->unbind("remove_speech_component");
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
			entity->bind(this, "remove_speech_component", [](entity_system* system, scene_entity* entity, v8Args args) {
				system->removeComponentFrom(entity);
			});
			entity->bind(this, "speak", [](entity_system* system, scene_entity* entity, v8Args args) {
				if (args.Length() != 1 || !args[0]->IsString()) {
					r2Error("Expected speech text to be passed to entity.speak()");
					return;
				}

				mstring text = v8pp::convert<mstring>::from_v8(r2engine::isolate(), args[0]);

				speech_system* sys = (speech_system*)system;
				auto& state = system->state();
				state.enable();
				speech_component* comp = (speech_component*)state->entity(entity->id());
				if (comp->plan) delete comp->plan;
				if (comp->execution) delete comp->execution;
				comp->plan = sys->sources->plan_speech(text);
				comp->execution = new speech_execution_context(comp->plan);
				state.disable();
			});
		}
	}


	void speech_system::unbind(r2::scene_entity* entity) {
		if (entity->is_scripted()) {
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
		auto& state = this->state();
		state.enable();
		state->for_each<speech_component>([](speech_component* comp) {
			if (comp->execution) {
				if (!comp->audio->isPlaying()) comp->audio->play();
				comp->execution->update(comp->audio, comp->texture);
				if (comp->execution->completed) {
					delete comp->execution; comp->execution = nullptr;
					delete comp->plan; comp->plan = nullptr;
					comp->audio->stop();
					event e = evt("SPEECH_FINISHED");
					comp->entity()->dispatch(&e);
				}
				comp->entity()->mesh->get_node()->material_instance()->set_texture("tex", comp->texture);
			}
			return true;
		});
		state.disable();
	}

};