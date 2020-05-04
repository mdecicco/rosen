#include <ui/keyframe_editor.h>
using namespace ImGui;
using namespace std;

namespace kf {
	KeyframeEditorInterface::KeyframeEditorInterface() {
		Duration = 1.0f;
		CurrentTime = 0.0f;
		draw_data.scrubbing = false;
		draw_data.scrolling = false;
		draw_data.scale_x = 1.0f;
		draw_data.scroll_x = 0.0f;
		draw_data.scroll_start_x = 0.0f;
		draw_data.last_window_width = 0.0f;
	}

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


	inline float _max(float a, float b) { return a > b ? a : b; };
	inline float _min(float a, float b) { return a < b ? a : b; };

	bool KeyframeEditor(KeyframeEditorInterface* data, const ImVec2& size) {
		static const float track_padding = 5.0f;
		static const float track_margin = 5.5f;
		static const float space_between_name_and_keyframes = 2.0f;
		static const float scroll_bar_height = 16.0f;
		static const float scroll_bar_padding = 2.5f;
		static const float keyframe_width_in_seconds = 0.01f;
		static const float keyframe_min_width_in_pixels = 1.0f;
		static const float keyframe_max_width_in_pixels = 10.0f;
		static const float scroll_mult = 0.06f;
		static const ImU32 track_bg_color = ImColor(1.0f, 1.0f, 1.0f, 0.1f);
		static const ImU32 time_bar_bg_color = ImColor(1.0f, 1.0f, 1.0f, 0.1f);
		static const ImU32 time_bar_tick_color = ImColor(1.0f, 1.0f, 1.0f, 0.1f);
		static const ImU32 scrub_line_color = ImColor(1.0f, 1.0f, 1.0f, 0.1f);
		static const ImU32 scrub_line_highlight_color = ImColor(1.0f, 1.0f, 1.0f, 0.25f);
		static const ImU32 scroll_bg_color = ImColor(0.0f, 0.0f, 0.0f, 0.4f);
		static const ImU32 scroll_bar_color = ImColor(1.0f, 1.0f, 1.0f, 0.1f);
		static const ImU32 scroll_bar_highlight_color = ImColor(1.0f, 1.0f, 1.0f, 0.25f);

		bool keyframes_modified = false;

		PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(track_margin, track_margin));

		BeginChild("##_kfe", size, true, ImGuiWindowFlags_NoMove);
			ImDrawList* dl = GetWindowDrawList();
			ImVec2 o = GetCursorScreenPos();

			ImVec2 max_track_text_size = ImVec2(0, 0);
			size_t track_count = data->TrackCount();
			for (size_t i = 0;i < track_count;i++) {
				ImVec2 ts = CalcTextSize(data->Track(i)->name.c_str());
				if (ts.x > max_track_text_size.x) max_track_text_size.x = ts.x;
				if (ts.y > max_track_text_size.y) max_track_text_size.y = ts.y;
			}
			float track_height = (track_padding * 2.0f) + (track_margin * 2.0f) + max_track_text_size.y;
			ImVec2 cr_mn = GetWindowContentRegionMin();
			ImVec2 cr_mx = GetWindowContentRegionMax();
			ImVec2 sz = ImVec2(
				cr_mx.x - cr_mn.x,
				cr_mx.y - cr_mn.y
			);
			if (size.x != 0.0f) sz.x = size.x;
			if (size.y != 0.0f) sz.y = size.y;
			if (data->draw_data.last_window_width == 0.0f) data->draw_data.last_window_width = sz.x;

			ImVec2 scroll_bg_tl = ImVec2(
				o.x + max_track_text_size.x + (track_padding * 2.0f) + space_between_name_and_keyframes + track_margin,
				o.y + (sz.y - scroll_bar_height) + (track_margin - 1.0f)
			);
			ImVec2 scroll_bg_br = ImVec2(o.x + sz.x - track_margin, o.y + sz.y + (track_margin - 1.0f));
			ImVec2 scroll_bar_tl = ImVec2(scroll_bg_tl.x + scroll_bar_padding, scroll_bg_tl.y + scroll_bar_padding);
			ImVec2 scroll_bar_br = ImVec2(scroll_bg_br.x - scroll_bar_padding, scroll_bg_br.y - scroll_bar_padding);

			ImVec2 time_bar_tl = ImVec2(
				o.x + track_margin + max_track_text_size.x + (track_padding * 2.0f) + space_between_name_and_keyframes,
				o.y + track_margin
			);
			ImVec2 time_bar_br = ImVec2(o.x + track_margin + (sz.x - (track_margin * 2.0f)), time_bar_tl.y + (track_height - (track_margin * 2.0f)));
			auto sec_to_x = [time_bar_tl, time_bar_br, data](float t) {
				float scroll_offset = data->draw_data.scroll_x * data->draw_data.scale_x;
				float fac = (t / data->Duration) * data->draw_data.scale_x;
				return time_bar_tl.x + ((time_bar_br.x - time_bar_tl.x) * fac) - scroll_offset;
			};
			auto x_to_sec = [time_bar_tl, time_bar_br, data](float x) {
				float scroll_offset = data->draw_data.scroll_x * data->draw_data.scale_x;
				float fac = (((x + scroll_offset) - time_bar_tl.x) / (time_bar_br.x - time_bar_tl.x)) / data->draw_data.scale_x;
				return fac * data->Duration;
			};

			ImVec2 mp = GetMousePos();
			if (mp.x > (o.x + cr_mn.x) && mp.x < (o.x + cr_mx.x) && mp.y > (o.y + cr_mn.y) && mp.y < (o.y + cr_mx.y)) {
				float scale_delta = GetIO().MouseWheel * scroll_mult * data->draw_data.scale_x;
				float sec_before = x_to_sec(mp.x);
				data->draw_data.scale_x += scale_delta;
				if (data->draw_data.scale_x < 1.0f) data->draw_data.scale_x = 1.0f;
				float sec_after = x_to_sec(mp.x);
				float x_delta = sec_to_x(sec_after) - sec_to_x(sec_before);
				data->draw_data.scroll_x -= (x_delta / data->draw_data.scale_x);
			}

			if (sz.x < (data->draw_data.last_window_width - 0.001f) || sz.x > (data->draw_data.last_window_width + 0.001f)) {
				float dw = sz.x - data->draw_data.last_window_width;
				data->draw_data.last_window_width = sz.x;
				data->draw_data.scroll_x += (dw / data->draw_data.scale_x);
			}

			float max_scroll_width = scroll_bar_br.x - scroll_bar_tl.x;
			float scroll_bar_width = max_scroll_width / data->draw_data.scale_x;
			if (data->draw_data.scroll_x > (max_scroll_width - scroll_bar_width)) data->draw_data.scroll_x = (max_scroll_width - scroll_bar_width);
			else if (data->draw_data.scroll_x < 0.0f) data->draw_data.scroll_x = 0.0f;
			scroll_bar_tl.x += data->draw_data.scroll_x;
			scroll_bar_br.x = scroll_bar_tl.x + scroll_bar_width;

			if (IsRectVisible(time_bar_tl, time_bar_br) && time_bar_br.x > time_bar_tl.x) {
				dl->AddRectFilled(time_bar_tl, time_bar_br, time_bar_bg_color, 5.0f, ImDrawCornerFlags_Top | ImDrawCornerFlags_BotRight);
				dl->PushClipRect(time_bar_tl, time_bar_br);
				char tbuf[8] = { 0 };
				float increment = 0.01f;
				while ((sec_to_x(increment) - sec_to_x(0.0f)) < 80.0f) {
					increment *= 2.0f;
				}

				for (float t = increment;t < data->Duration;t += increment) {
					float x = sec_to_x(t);
					if (x < time_bar_tl.x || x > time_bar_br.x) continue;
					memset(tbuf, 0, 8);
					snprintf(tbuf, 8, "%.2fs", t);
					dl->AddLine(ImVec2(x, time_bar_tl.y), ImVec2(x, time_bar_br.y), time_bar_tick_color, 1.0f);
					dl->AddText(ImVec2(x + track_padding, time_bar_tl.y + track_padding), 0xFFFFFFFF, tbuf);
				}
				dl->PopClipRect();
			}

			char buf[2] = { 0 };
			for (size_t i = 0;i < track_count;i++) {
				KeyframeTrackBase* t = data->Track(i);
				ImVec2 track_tl = ImVec2(
					o.x + track_margin,
					o.y + (track_height * float(i + 1)) + track_margin
				);
				ImVec2 track_br = ImVec2(track_tl.x + (sz.x - (track_margin * 2.0f)), track_tl.y + (track_height - (track_margin * 2.0f)));
				ImVec2 track_name_br = ImVec2(track_tl.x + max_track_text_size.x + (track_padding * 2.0f), track_br.y);
				ImVec2 track_keyframes_tl = ImVec2(track_name_br.x + space_between_name_and_keyframes, track_tl.y);
				if (IsRectVisible(track_tl, track_br)) {
					dl->AddRectFilled(track_tl, track_name_br, track_bg_color, 5.0f, ImDrawCornerFlags_Left);
					dl->AddRectFilled(track_keyframes_tl, track_br, track_bg_color, 5.0f, ImDrawCornerFlags_Right);
					dl->AddText(ImVec2(track_tl.x + track_padding, track_tl.y + track_padding), 0xFFFFFFFF, t->name.c_str());

					PushClipRect(track_keyframes_tl, track_br, false);
					for (auto k = t->keyframes.begin();k != t->keyframes.end();k++) {
						float x = sec_to_x(k->time);
						float width = _max(_min(sec_to_x(keyframe_width_in_seconds) - sec_to_x(0.0f), keyframe_max_width_in_pixels), keyframe_min_width_in_pixels);
						if ((x + width) < track_keyframes_tl.x || x > track_br.x) continue;
						ImVec2 f_tl = ImVec2(x, track_tl.y + track_padding);
						ImVec2 f_br = ImVec2(x + width, track_br.y - track_padding);

						dl->AddRectFilled(f_tl, f_br, t->color);
					}
					PopClipRect();

					auto move_iter = t->keyframes.end();
					auto remove_iter = t->keyframes.end();
					bool found_dragged = false;
					for (auto k = t->keyframes.begin();k != t->keyframes.end();k++) {
						float x = sec_to_x(k->time);
						float width = _max(_min(sec_to_x(keyframe_width_in_seconds) - sec_to_x(0.0f), keyframe_max_width_in_pixels), keyframe_min_width_in_pixels);
						ImVec2 f_tl = ImVec2(x, track_tl.y + track_padding);
						ImVec2 f_br = ImVec2(x + width, track_br.y - track_padding);

						bool hovering = mp.x > f_tl.x && mp.x < f_br.x && mp.y > f_tl.y && mp.y < f_br.y;
						if (k->draw_data.context_window_open) {
							if (BeginPopupContextWindow("##_kfe_kctx")) {
								if (Selectable("Scrub To")) {
									k->draw_data.context_window_open = false;
									data->CurrentTime = k->time;
								}
								if (Selectable("Delete")) {
									k->draw_data.context_window_open = false;
									remove_iter = k;
									keyframes_modified = true;
								}
								EndPopup();
							}
						}

						if (hovering && IsMouseClicked(1)) {
							for (auto ok = t->keyframes.begin();ok != t->keyframes.end();ok++) {
								ok->draw_data.context_window_open = false;
							}
							k->draw_data.context_window_open = true;
						} else if (!hovering && IsMouseClicked(1)) {
							k->draw_data.context_window_open = false;
						}

						if (!IsMouseDown(0)) {
							if (k->draw_data.dragging) {
								move_iter = k;
								k->draw_data.dragging = false;
								keyframes_modified = true;
							}
						} else {
							if (k->draw_data.dragging) {
								float ds = x_to_sec(mp.x) - k->draw_data.last_drag_sec;
								k->draw_data.last_drag_sec = x_to_sec(mp.x);
								k->time += ds;
								if (k->time < 0.0f) k->time = 0.0f;
								else if (k->time > data->Duration) k->time = data->Duration;
								keyframes_modified = true;
							}

							if (!found_dragged && hovering && IsMouseClicked(0)) {
								k->draw_data.dragging = true;
								k->draw_data.last_drag_sec = x_to_sec(mp.x);
								found_dragged = true;
							}
						}
					}

					if (move_iter != t->keyframes.end()) {
						bool found_place = false;
						for (auto nk = t->keyframes.begin();nk != t->keyframes.end();nk++) {
							if (nk->time > move_iter->time + 0.0001f) {
								t->keyframes.splice(nk, t->keyframes, move_iter);
								found_place = true;
								break;
							} else if (nk != move_iter && nk->time < move_iter->time + 0.0001f && nk->time > move_iter->time - 0.0001f) {
								t->keyframes.splice(nk, t->keyframes, move_iter);
								t->keyframes.erase(nk);
								found_place = true;
								break;
							}
						}

						if (!found_place) t->keyframes.splice(t->keyframes.end(), t->keyframes, move_iter);
					}

					if (remove_iter != t->keyframes.end()) {
						t->keyframes.erase(remove_iter);
					}
				}
			}

			ImVec2 scrub_line_t = ImVec2(_min(_max(sec_to_x(data->CurrentTime), time_bar_tl.x), time_bar_br.x), o.y + track_margin);
			ImVec2 scrub_line_b = ImVec2(_min(_max(sec_to_x(data->CurrentTime), time_bar_tl.x), time_bar_br.x), o.y + sz.y - scroll_bar_height);
			bool mouse_over_scrub_line = mp.x > (scrub_line_t.x - 5.0f) && mp.x < (scrub_line_t.x + 5.0f) && mp.y > scrub_line_t.y && mp.y < scrub_line_b.y;

			dl->AddLine(scrub_line_t, scrub_line_b, mouse_over_scrub_line || data->draw_data.scrubbing ? scrub_line_highlight_color : scrub_line_color, 2.0f);


			bool mouse_over_scroll_bar = mp.x > scroll_bar_tl.x && mp.x < scroll_bar_br.x && mp.y > scroll_bar_tl.y && mp.y < scroll_bar_br.y;

			if (IsMouseClicked(0)) {
				if (!data->draw_data.scrubbing && mouse_over_scrub_line) {
					data->draw_data.scrubbing = true;
				} else if (!data->draw_data.scrolling && mouse_over_scroll_bar) {
					data->draw_data.scrolling = true;
					data->draw_data.scroll_start_x = mp.x;
				} else if (mp.x > time_bar_tl.x && mp.x < time_bar_br.x && mp.y > time_bar_tl.y && mp.y < time_bar_br.y) {
					float p = x_to_sec(mp.x);
					if (p < 0.0f) p = 0.0f;
					else if (p > data->Duration) p = data->Duration;
					data->CurrentTime = p;
					data->draw_data.scrubbing = true;
				}
			}

			if (IsMouseDown(0)) {
				if (data->draw_data.scrubbing) {
					float p = x_to_sec(mp.x);
					if (p < 0.0f) p = 0.0f;
					else if (p > data->Duration) p = data->Duration;
					data->CurrentTime = p;
				}
				if (data->draw_data.scrolling) {
					float dx = mp.x - data->draw_data.scroll_start_x;
					data->draw_data.scroll_x += dx;
					data->draw_data.scroll_start_x = mp.x;
				}
			} else {
				data->draw_data.scrubbing = false;
				data->draw_data.scrolling = false;
			}

			dl->AddRectFilled(scroll_bg_tl, scroll_bg_br, scroll_bg_color);
			dl->AddRectFilled(scroll_bar_tl, scroll_bar_br, mouse_over_scroll_bar || data->draw_data.scrolling ? scroll_bar_highlight_color : scroll_bar_color, (scroll_bar_br.y - scroll_bar_tl.y) * 0.5f);
		EndChild();

		PopStyleVar();

		return keyframes_modified;
	}
};