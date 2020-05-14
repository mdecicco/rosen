#pragma once
#include <r2/utilities/imgui/imgui.h>

#include <string>
#include <list>
#include <unordered_map>
namespace kf {
	struct KeyframeBase {
		virtual ~KeyframeBase() { };
		float time;
		void* user_pointer;

		struct {
			bool context_window_open;
			bool dragging;
			float last_drag_sec;
			float time_before_drag;
		} draw_data;
	};

	template <typename T>
	struct Keyframe : public KeyframeBase {
		virtual ~Keyframe() { }
		T value;
	};

	struct KeyframeTrackBase {
		virtual ~KeyframeTrackBase() { };

		void AddKeyframe(float time, void* user_pointer);

		std::string name;
		ImColor color;
		std::list<KeyframeBase*> keyframes;
		void* user_pointer;
	};

	template <typename T>
	struct KeyframeTrack : public KeyframeTrackBase {
		virtual ~KeyframeTrack() {
			for (auto i = keyframes.begin();i != keyframes.end();i++) {
				Keyframe<T>* kf = (Keyframe<T>*)*i;
				delete kf;
			}
		}

		typedef T (*InterpolatorCallback)(const T&, const T&, float);
		static inline T DefaultInterpolator(const T& a, const T& b, float w) { return a + ((b - a) * w); }
		InterpolatorCallback interpolator;
		T initial_value;

		inline T ValueAtTime(float time) {
			for (auto i = keyframes.begin();i != keyframes.end();i++) {
				auto n = std::next(i);
				if (i->time <= time) {
					Keyframe<T>* f = (Keyframe<T>*)*i;
					if (n == keyframes.end()) return f->value;
					else {
						Keyframe<T>* nf = (Keyframe<T>*)*n;
						if (nf->time >= time) {
							return interpolator(f->value, nf->value, (time - f->time) / (nf->time - f->time));
						}
					}
				}
			}

			if (keyframes.size() > 0) {
				Keyframe<T>* kfr = (Keyframe<T>*)*keyframes.begin();
				return interpolator(initial_value, kfr->value, time / kfr->time);
			}

			return initial_value;
		}

		inline void AddKeyframe(const T& value, float time, void* user_pointer = nullptr) {
			for (auto i = keyframes.begin();i != keyframes.end();i++) {
				Keyframe<T>* kf = (Keyframe<T>*)*i;
				if (kf->time > time + 0.0001f) {
					Keyframe<T>* f = new Keyframe<T>;
					f->time = time;
					f->value = value;
					f->draw_data.context_window_open = false;
					f->draw_data.dragging = false;
					f->user_pointer = user_pointer;
					keyframes.insert(i, f);
					return;
				} else if (kf->time < time + 0.0001f && kf->time > time - 0.0001f) {
					kf->user_pointer = user_pointer;
					kf->value = value;
					return;
				}
			}

			Keyframe<T>* f = new Keyframe<T>;
			f->time = time;
			f->value = value;
			f->user_pointer = user_pointer;
			f->draw_data.context_window_open = false;
			f->draw_data.dragging = false;
			keyframes.push_back(f);
		}
	};


	/*
	 * Interface between the application and the keyframe editor
	 *
	 * Notes:
	 *    - If no callbacks are set, the editor will:
	 *       - Create empty keyframes when the keyframe button is pressed for any track
	 *       - Delete keyframes when the delete option is clicked in the keyframe context menu
	 *       - Delete the non-dragged keyframe if one keyframe is moved to within 0.0001 second of another
	 *    - If SetKeyframe is called two or more times for the same track / time pair, the existing keyframe
	 *      on the track at that time will be overwritten with the new one
	 *    - The ValueAtTime function only exists as a possible convenience to the application and isn't used
	 *      internally. You can add typeless/valueless tracks / keyframes if desired.
	 *    - If you use ValueAtTime on a typeless/valueless track the behavior is undefined.
	 *    - If you use ValueAtTime with a type that is different than the type the specified track was created
	 *      with then the behavior is undefined
	 */
	class KeyframeEditorInterface {
		public:
			KeyframeEditorInterface();
			~KeyframeEditorInterface();

			template <typename T>
			inline void AddTrack(const std::string& name, const T& initial, const ImColor& color = ImColor(1.0f, 1.0f, 1.0f), typename KeyframeTrack<T>::InterpolatorCallback interpolator = KeyframeTrack<T>::DefaultInterpolator, void* user_pointer = nullptr) {
				KeyframeTrack<T>* track = new KeyframeTrack<T>;
				track->name = name;
				track->color = color;
				track->interpolator = interpolator ? interpolator : KeyframeTrack<T>::DefaultInterpolator;
				track->initial_value = initial;
				track->user_pointer = user_pointer;
				m_tracks[name] = track;
				m_contiguousTracks.push_back(track);
			}

			template <typename T>
			inline void AddTrack(const std::string& name, const T& initial, void* user_pointer) {
				AddTrack<T>(name, initial, ImColor(1.0f, 1.0f, 1.0f), default_interpolator<T>, user_pointer);
			}

			void AddTrack(const std::string& name, const ImColor& color = ImColor(1.0f, 1.0f, 1.0f), void* user_pointer = nullptr);

			void RemoveTrack(const std::string& name);

			template <typename T>
			inline void SetKeyframe(const std::string& track, const T& value, float time, void* user_pointer = nullptr) {
				auto it = m_tracks.find(track);
				assert(it != m_tracks.end());
				((KeyframeTrack<T>*)it->second)->AddKeyframe(value, time, user_pointer);
			}

			template <typename T>
			inline KeyframeTrack<T>* Track(const std::string& track) {
				auto it = m_tracks.find(track);
				if (it != m_tracks.end()) return ((KeyframeTrack<T>*)it->second);
				return nullptr;
			}

			template <typename T>
			inline KeyframeTrack<T>* Track(size_t idx) {
				return (KeyframeTrack<T>*)m_contiguousTracks[idx];
			}
			
			inline KeyframeTrackBase* Track(const std::string& track) {
				auto it = m_tracks.find(track);
				if (it != m_tracks.end()) return it->second;
				return nullptr;
			}

			inline KeyframeTrackBase* Track(size_t idx) {
				return m_contiguousTracks[idx];
			}

			inline size_t TrackCount() const { return m_contiguousTracks.size(); }

			template <typename T>
			inline T ValueAtTime(const std::string& name, float time) {
				auto it = m_tracks.find(name);
				assert(it != m_tracks.end());
				return ((KeyframeTrack<T>*)*it)->ValueAtTime(time);
			}

			typedef bool (*create_keyframe_callback)(void* /* callback_userdata */, void* /* track_userdata */, float /* time */, void*& /* new_keyframe_user_data */);
			typedef bool (*set_keyframe_time_callback)(void* /* callback_userdata */, void* /* track_userdata */, void* /* keyframe_userdata */, float /* time */);
			typedef void (*seek_callback)(void* /* callback_userdata */, float /* time */);
			typedef bool (*delete_keyframe_callback)(void* /* callback_userdata */, void* /* track_userdata */, void* /* keyframe_userdata */);
			typedef void (*reorder_keyframes_callback)(void* /* callback_userdata */, void* /* track_userdata */, const std::vector<void*>& /* ordered_keyframe_userdatas */);
			typedef void (*keyframe_context_menu_callback)(void* /* callback_userdata */, void* /* track_userdata */, void* /* keyframe_userdata */);

			// Return true if keyframe created, false if not
			create_keyframe_callback on_create_keyframe;

			// Note:
			//	  If this function would result in the application setting the time for some keyframe (keyframe A) to the same time (+/- 0.0001 seconds) as
			//    some other keyframe (keyframe B), the keyframe editor's internal logic assumes that the application gets rid of keyframe B on its own. The
			//    on_delete_keyframe callback is not called for keyframe B since that callback implies that you don't have to delete it if that would not
			//    agree with the application. This keyframe editor does not let multiple keyframes exist for the same time. In the above scenario, the editor
			//    will automatically delete it's own record of keyframe B. If the application does not do this, the editor will no longer accurately reflect
			//    the animation being edited.
			//
			//    Alternatively to deleting keyframe B, the callback return false and the keyframe editor will put keyframe A back where it was before moving
			//    it.
			set_keyframe_time_callback on_set_keyframe_time;

			// Return true if keyframe is deleted (or otherwise removed from the animation), false if not
			delete_keyframe_callback on_delete_keyframe;

			// Application should reorder its keyframes according to the last parameter of this function
			reorder_keyframes_callback on_keyframe_reorder;

			// Current time changed
			seek_callback on_seek;

			struct keyframe_context_menu_item {
				const char* text;
				keyframe_context_menu_callback callback;
			};
			std::vector<keyframe_context_menu_item> keyframe_context_menu_items;

			void* callback_userdata;

			float CurrentTime;
			float Duration;

			struct {
				bool scrubbing;
				bool scrolling;
				bool v_scrolling;
				float scale_x;
				float scroll_x;
				float scroll_start_x;
				float scroll_y;
				float scroll_start_y;
				float last_window_width;
			} draw_data;
		protected:
			std::unordered_map<std::string, KeyframeTrackBase*> m_tracks;
			std::vector<KeyframeTrackBase*> m_contiguousTracks;
	};

	bool KeyframeEditor(KeyframeEditorInterface* data, const ImVec2& size = ImVec2(0, 0));
};