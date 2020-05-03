#include <ui/keyframe_editor.h>
using namespace ImGui;
using namespace std;

namespace kf {
	KeyframeEditorInterface::~KeyframeEditorInterface() {
		for (auto i = m_tracks.begin();i != m_tracks.end();i++) {
			delete i->second;
		}
	}

	void KeyframeEditorInterface::RemoveTrack(const string& track) {
		auto i = m_tracks.find(track);
		if (i == m_tracks.end()) return;

		for (auto vi = m_contiguousTracks.begin();vi != m_contiguousTracks.end();vi++) {
			if (i->second == *vi) {
				m_contiguousTracks.erase(vi);
				break;
			}
		}

		delete i->second;
		m_tracks.erase(i);
	}

	void KeyframeEditor(KeyframeEditorInterface* data, const ImVec2& size) {
		static const float track_padding = 5.0f;
		static const float track_margin = 2.5f;
		static const float space_between_name_and_keyframes = 2.0f;
		static const ImU32 track_bg_color = ImColor(1.0f, 1.0f, 1.0f, 0.1f);

		PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(track_margin, track_margin));

		BeginChild("##_kfe", size, true);
			ImDrawList* dl = GetWindowDrawList();
			ImVec2 o = GetCursorScreenPos();
			ImVec2 max_track_text_size = ImVec2(0, 0);
			size_t track_count = data->TrackCount();
			for (size_t i = 0;i < track_count;i++) {
				ImVec2 ts = CalcTextSize(data->Track(i)->name.c_str());
				if (ts.x > max_track_text_size.x) max_track_text_size.x = ts.x;
				if (ts.y > max_track_text_size.y) max_track_text_size.y = ts.y;
			}
			ImVec2 sz = ImVec2(
				GetWindowContentRegionWidth(),
				((max_track_text_size.y + (track_padding * 2.0f) + (track_margin * 2.0f)) * float(track_count))
			);
			if (size.x != 0.0f) sz.x = size.x;
			if (size.y != 0.0f) sz.y = size.y;
			sz.x -= track_margin * 2.0f;
			sz.y -= track_margin * 2.0f;

			float track_height = (track_padding * 2.0f) + (track_margin * 2.0f) + max_track_text_size.y;
			auto _max = [](float a, float b) { return a > b ? a : b; };
			for (size_t i = 0;i < track_count;i++) {
				KeyframeTrackBase* t = data->Track(i);
				ImVec2 track_tl = ImVec2(
					o.x + track_margin,
					o.y + (track_height * float(i)) + track_margin
				);
				ImVec2 track_br = ImVec2(track_tl.x + (sz.x - (track_margin * 2.0f)), track_tl.y + (track_height - (track_margin * 2.0f)));
				ImVec2 track_name_br = ImVec2(track_tl.x + max_track_text_size.x + (track_padding * 2.0f), track_br.y);
				ImVec2 track_keyframes_tl = ImVec2(track_name_br.x + space_between_name_and_keyframes, track_tl.y);
				if (IsRectVisible(track_tl, track_br)) {
					dl->AddRectFilled(track_tl, track_name_br, track_bg_color, 5.0f, ImDrawCornerFlags_Left);
					dl->AddRectFilled(track_keyframes_tl, track_br, track_bg_color, 5.0f, ImDrawCornerFlags_Right);
					dl->AddText(ImVec2(track_tl.x + track_padding, track_tl.y + track_padding), 0xFFFFFFFF, t->name.c_str());
				}
			}
		EndChild();

		PopStyleVar();
	}
};