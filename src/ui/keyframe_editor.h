#pragma once
#include <r2/utilities/imgui/imgui.h>

#include <string>
#include <vector>
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
		typedef float (*InterpolatorCallback)(T a, T b, float w);
		static inline float DefaultInterpolator(T a, T b, float w) { return a + ((b - a) * w); }
		InterpolatorCallback interpolator;
		std::vector<Keyframe<T>> keyframes;
		T initial_value;

		T ValueAtTime(float time) {
			for (auto i = keyframes.begin();i != keyframes.end();i++) {
				auto n = std::next(i);
				if (i->time <= time) {
					if (n == keyframes.end()) return i->value;
					else return interpolator(i->value, n->value, (time - i->time) / (n->time - i->time));
				}
			}

			if (keyframes.size() > 0) return interpolator(initial_value, keyframes[0].value, time / keyframes[0].time);
			return initial_value;
		}
	};

	class KeyframeEditorInterface {
		public:
			KeyframeEditorInterface();
			~KeyframeEditorInterface();

			template <typename T>
			void AddTrack(const std::string& name, const T& initial, const ImColor& color = ImColor(1.0f, 1.0f, 1.0f), KeyframeTrack<T>::InterpolatorCallback interpolator = KeyframeTrack<T>::DefaultInterpolator) {
				KeyframeTrack<T>* track = new KeyframeTrack<T>;
				track->name = name;
				track->color = color;
				track->interpolator = interpolator ? interpolator : KeyframeTrack<T>::DefaultInterpolator;
				track->initial_value = initial;
				m_tracks[name] = track;
			}

			void RemoveTrack(const char* name);

			template <typename T>
			T ValueAtTime(const std::string& name, float time) {
				auto it = m_tracks.find(name);
				assert(it != m_tracks.end());
				return ((KeyframeTrack<T>*)*it)->ValueAtTime(time);
			}

		protected:
			std::unordered_map<std::string, KeyframeTrackBase*> m_tracks;
	};
	class KeyframeEditor {
		public:
			KeyframeEditor();
			~KeyframeEditor();

			void Render(const ImVec2& size);
	};
};