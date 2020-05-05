#include <states/initial_loading.h>
#include <managers/source_man.h>

#include <ui/keyframe_editor.h>

#include <r2/engine.h>
#define INITIAL_LOADING_STATE_UPDATE_RATE 10.0f
namespace rosen {
	static kf::KeyframeEditorInterface* kfi = nullptr;

	initial_loading_state::initial_loading_state(source_man* sourceMgr) :
		state("initial_loading_state", MBtoB(15)), m_progress(0.0f, 1.0f / INITIAL_LOADING_STATE_UPDATE_RATE, interpolate::easeInOutCubic)
	{
		// This state's memory has not been allocated yet. Any
		// allocations made here will be either in the global
		// scope, or the scope of the currently active state.
		// Be careful not to allocate here, or to explicitly
		// allocate in the global scope via:
		// memory_man::push_current(memory_man::global())
		// ...allocate stuff...
		// memory_man::pop_current()

		m_sources = sourceMgr;
	}

	initial_loading_state::~initial_loading_state() {
		// This state's memory should already be deallocated
		// by this point, unless the user deleted the state
		// while it was active (don't do that)
	}

	void initial_loading_state::onInitialize() {
		// The state's memory has been allocated, and is
		// currently active. From here until becameInactive
		// is called, all allocations made in this class
		// will be in the state's own memory scope (unless
		// explicitly allocated in the global scope using
		// memory_man::[push/pop]_current)
		setUpdateFrequency(10.0f);
	}

	void initial_loading_state::willBecomeActive() {
		// The state is going to become active. The previous
		// state is still active at this point, but will be
		// deactivated immediately after this function returns.
		// This state will be activated immediately after that

		kfi = new kf::KeyframeEditorInterface();
		kfi->AddTrack<float>("test abc", 0.0f);
		kfi->AddTrack<float>("testy", 0.0f);
		kfi->AddTrack<float>("grumpo balls 0", 0.0f);
		kfi->AddTrack<float>("grumpo balls 1", 0.0f);
		kfi->AddTrack<float>("grumpo balls 2", 0.0f);
		kfi->AddTrack<float>("grumpo balls 3", 0.0f);
		kfi->AddTrack<float>("grumpo balls 4", 0.0f);
		kfi->AddTrack<float>("grumpo balls 5", 0.0f);
		kfi->AddTrack<float>("grumpo balls 6", 0.0f);
		kfi->AddTrack<float>("grumpo balls 7", 0.0f);
		kfi->SetKeyframe("test abc", 1.0f, 2.0f);
		kfi->SetKeyframe("testy", 1.0f, 3.0f);
		kfi->SetKeyframe("grumpo balls 0", 1.0f, 4.0f);
		kfi->SetKeyframe("grumpo balls 0", 1.0f, 4.0f);
		kfi->SetKeyframe("grumpo balls 0", 2.0f, 4.1f);
		kfi->SetKeyframe("grumpo balls 0", 3.0f, 4.2f);
		kfi->SetKeyframe("grumpo balls 0", 4.0f, 4.3f);
		kfi->Duration = 60.0f;
	}

	void initial_loading_state::becameActive() {
		// The state was activated completely, and the previous
		// state (if any) has been cleared
	}

	void initial_loading_state::willBecomeInactive() {
		// Becoming inactive means, among other things, that
		// the entire block of memory used by this state will
		// either be cleared or temporarily deallocated.
		// You don't _have_ to destroy entities here, or
		// deallocate anything allocated within this state's
		// memory, but you should...

		delete kfi;
	}

	void initial_loading_state::becameInactive() {
		// Anything allocated here will be deallocated immediately
		// after this function returns, unless it's explicitly
		// allocated in the global scope via:
		// memory_man::push_current(memory_man::global())
		// ...allocate stuff...
		// memory_man::pop_current()
	}

	void initial_loading_state::willBeDestroyed() {
		// By the time this function is called, unless the state is
		// currently active (should never happen unless the user
		// deletes the state for some reason), the state's memory
		// will already be deallocated, and any allocations made here
		// will be made either in the global scope, or the currently
		// active state's scope. Be careful to not allocate here, or
		// to specify which scope the allocation _should_ be made in.
		// This includes allocations made internally by std containers
	}

	void initial_loading_state::onUpdate(f32 frameDt, f32 updateDt) {
		// Will be called as frequently as the user specifies with
		// setUpdateFrequency(frequency in Hz), as long as the
		// application is able to achieve that frequency.
		// Rendering probably can't be done here, as it's called
		// outside of the context of the frame.
		f32 prog = m_sources->loadingProgress();
		m_progress = prog;
		if (prog == 1.0f && m_startDelayTimer.stopped()) {
			m_startDelayTimer.start();
		} else if (m_startDelayTimer > 2.0f) {
			r2engine::get()->activate_state("editor_state");
		}
	}

	void initial_loading_state::onRender() {
		ImGui::ProgressBar((f32)m_progress);

		kf::KeyframeEditor(kfi);
	}

	void initial_loading_state::onEvent(event* evt) {
		// Will be called whenever a non-internal event is fired,
		// or in the case of deferred events, at the beginning of
		// the engine tick (before onUpdate would be called)
	}
};