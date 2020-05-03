#pragma once
#include <r2/utilities/imgui/imgui.h>

#include <string>
#include <list>
#include <unordered_map>
namespace kf {
	template <typename T>
	struct Keyframe {
		float time;
		T value;
	};

	struct KeyframeTrackBase {
		virtual ~KeyframeTrackBase() { }
		std::string name;
		ImColor color;
	};

	template <typename T>
	struct KeyframeTrack : public KeyframeTrackBase {
		typedef T (*InterpolatorCallback)(const T&, const T&, float);
		static inline T DefaultInterpolator(const T& a, const T& b, float w) { return a + ((b - a) * w); }
		InterpolatorCallback interpolator;
		std::list<Keyframe<T>> keyframes;
		T initial_value;

		inline T ValueAtTime(float time) {
			for (auto i = keyframes.begin();i != keyframes.end();i++) {
				auto n = std::next(i);
				if (i->time <= time) {
					if (n == keyframes.end()) return i->value;
					else return interpolator(i->value, n->value, (time - i->time) / (n->time - i->time));
				}
			}

			if (keyframes.size() > 0) {
				auto& kfr = *keyframes.begin();
				return interpolator(initial_value, kfr.value, time / kfr.time);
			}
			return initial_value;
		}

		inline void AddKeyframe(const T& value, float time) {
			for (auto i = keyframes.begin();i != keyframes.end();i++) {
				if (i->time > time + 0.0001f) {
					keyframes.insert(i, { time, value });
					return;
				} else if (i->time < time + 0.0001f && i->time > time - 0.0001f) {
					i->value = value;
					return;
				}
			}

			keyframes.push_back({ time, value });
		}
	};

	class KeyframeEditorInterface {
		public:
			KeyframeEditorInterface() { }
			~KeyframeEditorInterface();

			template <typename T>
			inline void AddTrack(const std::string& name, const T& initial, const ImColor& color = ImColor(1.0f, 1.0f, 1.0f), typename KeyframeTrack<T>::InterpolatorCallback interpolator = KeyframeTrack<T>::DefaultInterpolator) {
				KeyframeTrack<T>* track = new KeyframeTrack<T>;
				track->name = name;
				track->color = color;
				track->interpolator = interpolator ? interpolator : KeyframeTrack<T>::DefaultInterpolator;
				track->initial_value = initial;
				m_tracks[name] = track;
				m_contiguousTracks.push_back(track);
			}

			void RemoveTrack(const std::string& name);

			template <typename T>
			inline void SetKeyframe(const std::string& track, const T& value, float time) {
				auto it = m_tracks.find(track);
				assert(it != m_tracks.end());
				((KeyframeTrack<T>*)it->second)->AddKeyframe(value, time);
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
			float ScaleX;
			float ScrollX;
		protected:
			std::unordered_map<std::string, KeyframeTrackBase*> m_tracks;
			std::vector<KeyframeTrackBase*> m_contiguousTracks;
	};

	void KeyframeEditor(KeyframeEditorInterface* data, const ImVec2& size = ImVec2(0, 0));
};