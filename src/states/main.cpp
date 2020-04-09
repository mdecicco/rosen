#include <states/main.h>
#include <utils/video.h>
#include <utils/audio.h>
#include <r2/managers/drivers/gl/driver.h>
#include <r2/engine.h>

namespace rosen {
	main_state::main_state() : state("main_state", MBtoB(200)) {
		// This state's memory has not been allocated yet. Any
		// allocations made here will be either in the global
		// scope, or the scope of the currently active state.
		// Be careful not to allocate here, or to explicitly
		// allocate in the global scope via:
		// memory_man::push_current(memory_man::global())
		// ...allocate stuff...
		// memory_man::pop_current()

		m_video = nullptr;
		m_audio = nullptr;
		m_currentTexture = nullptr;
	}

	main_state::~main_state() {
		// This state's memory should already be deallocated
		// by this point, unless the user deleted the state
		// while it was active (don't do that)
	}

	void main_state::onInitialize() {
		// The state's memory has been allocated, and is
		// currently active. From here until becameInactive
		// is called, all allocations made in this class
		// will be in the state's own memory scope (unless
		// explicitly allocated in the global scope using
		// memory_man::[push/pop]_current)
		setUpdateFrequency(25.0f);
	}

	void main_state::willBecomeActive() {
		// The state is going to become active. The previous
		// state is still active at this point, but will be
		// deactivated immediately after this function returns.
		// This state will be activated immediately after that

		m_video = new video_container("Me and My Brother", getScene());
		m_audio = new audio_container("Me and My Brother");
		r2engine::audio()->setListener(mat4f(1.0f));
	}

	void main_state::becameActive() {
		// The state was activated completely, and the previous
		// state (if any) has been cleared
	}

	void main_state::willBecomeInactive() {
		// Becoming inactive means, among other things, that
		// the entire block of memory used by this state will
		// either be cleared or temporarily deallocated.
		// You don't _have_ to destroy entities here, or
		// deallocate anything allocated within this state's
		// memory, but you should...

		if (m_video) delete m_video; m_video = nullptr;
		if (m_audio) delete m_audio; m_audio = nullptr;
		if (m_currentTexture) getScene()->destroy(m_currentTexture); m_currentTexture = nullptr;
	}

	void main_state::becameInactive() {
		// Anything allocated here will be deallocated immediately
		// after this function returns, unless it's explicitly
		// allocated in the global scope via:
		// memory_man::push_current(memory_man::global())
		// ...allocate stuff...
		// memory_man::pop_current()
	}

	void main_state::willBeDestroyed() {
		// By the time this function is called, unless the state is
		// currently active (should never happen unless the user
		// deletes the state for some reason), the state's memory
		// will already be deallocated, and any allocations made here
		// will be made either in the global scope, or the currently
		// active state's scope. Be careful to not allocate here, or
		// to specify which scope the allocation _should_ be made in.
		// This includes allocations made internally by std containers
	}

	void main_state::onUpdate(f32 frameDt, f32 updateDt) {
		// Will be called as frequently as the user specifies with
		// setUpdateFrequency(frequency in Hz), as long as the
		// application is able to achieve that frequency.
		// Rendering probably can't be done here, as it's called
		// outside of the context of the frame.

		printf("TestState::onUpdate(%.2f ms, %.2f ms)\n", frameDt * 1000.0f, updateDt * 1000.0f);
		if (m_audio->audio()->isPlaying()) {
			f32 ppos = m_audio->audio()->playPosition();
			u32 frameIdx = f32(m_video->info.framerate) * ppos;
			m_currentTexture = m_video->frame(frameIdx, m_currentTexture);
		}
	}

	void main_state::onRender() {
		// Will be called once per frame
		// ImGui::Text("Memory: %s / %s", format_size(getUsedMemorySize()), format_size(getMaxMemorySize()));
		ImGui::Text("FPS: %.2f", r2engine::get()->fps());
		if (ImGui::Button("Reset State", ImVec2(190, 20))) {
			r2engine::get()->activate_state("main_state");
		}
		if (m_currentTexture) {
			ImGui::Image((void*)((gl_render_driver*)r2engine::renderer()->driver())->get_texture_id(m_currentTexture), ImVec2(480, 270));
		}

		if (!m_audio->audio()->isPlaying()) m_audio->audio()->play();
		else {
			f32 ppos = m_audio->audio()->playPosition();
			f32 pitch = m_audio->audio()->pitch();
			if (ImGui::DragFloat("pitch", &pitch, 0.01f)) m_audio->audio()->setPitch(pitch);
			ImGui::Text("Pos: %.2f", ppos);
		}
	}

	void main_state::onEvent(event* evt) {
		// Will be called whenever a non-internal event is fired,
		// or in the case of deferred events, at the beginning of
		// the engine tick (before onUpdate would be called)
	}
};