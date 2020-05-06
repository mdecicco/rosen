#pragma once
#include <r2/utilities/imgui/imgui.h>

#include <string>
#include <list>
#include <unordered_map>
namespace kf {
	struct KeyframeBase {
		virtual ~KeyframeBase() { }
		float time;
		void* user_pointer;

		struct {
			bool context_window_open;
			bool dragging;
			float last_drag_sec;
		} draw_data;
	};

	template <typename T>
	struct Keyframe : public KeyframeBase {
		T value;
	};

	struct KeyframeTrackBase {
		virtual ~KeyframeTrackBase() { }
		std::string name;
		ImColor color;
		std::list<KeyframeBase*> keyframes;
		void* user_pointer;
	};

	template <typename T>
	struct KeyframeTrack : public KeyframeTrackBase {
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
						return interpolator(f->value, nf->value, (time - f->time) / (nf->time - f->time));
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
				assert(it != m_tracks.end());
				return ((KeyframeTrack<T>*)it->second);
			}

			template <typename T>
			inline KeyframeTrack<T>* Track(size_t idx) {
				return (KeyframeTrack<T>*)m_contiguousTracks[idx];
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