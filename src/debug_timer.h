#pragma once

#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>

class DebugTimer {
public:
	explicit DebugTimer(int frame_goal = 60) :
			m_frame_goal(frame_goal), m_last_reset(std::chrono::steady_clock::now()) {}

	void start(const std::string &group, const std::string &key) {
		m_groups[group][key].t_start = std::chrono::steady_clock::now();
	}

	void stop(const std::string &group, const std::string &key) {
		const auto now = std::chrono::steady_clock::now();
		auto &e = m_groups[group][key];
		e.total += now - e.t_start;
		e.count++;
	}

	// Calls callback(key, report) for every entry in the group.
	// If report_total_line is true, a group-total line is emitted first (keyed by the group name)
	template <typename F>
	void report_group(const std::string &group, F &&callback, bool report_total_line) const {
		const auto it = m_groups.find(group);
		if (it == m_groups.end()) {
			return;
		}
		const double number_of_frames = m_frame_goal * elapsed_seconds();
		double group_total_ms = 0.0;
		int group_count = 0;
		for (const auto &[key, e] : it->second) {
			group_total_ms += ms(e);
			group_count += e.count;
		}
		if (report_total_line) {
			callback(group, format_entry(group_total_ms, group_count, number_of_frames, ""));
		}
		const std::string prefix = report_total_line ? "    " : "";
		for (const auto &[key, e] : it->second) {
			callback(key, format_entry(ms(e), e.count, number_of_frames, prefix));
		}
	}

	// Emits one callback(key, report) with the sum across ALL groups.
	// Format: "1.234 ms/frame [12 %]" where % = fraction of the per-frame budget.
	template <typename F>
	void report_total(const std::string &key, F &&callback) const {
		const double number_of_frames = m_frame_goal * elapsed_seconds();
		const double budget_ms_per_frame = 1000.0 / m_frame_goal;
		double total_ms = 0.0;
		for (const auto &[group, entries] : m_groups) {
			for (const auto &[k, e] : entries) {
				total_ms += ms(e);
			}
		}
		const double ms_per_frame = number_of_frames > 0.0 ? total_ms / number_of_frames : 0.0;
		const double percent = ms_per_frame / budget_ms_per_frame * 100.0;
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(3) << ms_per_frame << " ms/frame ["
			<< std::setprecision(0) << std::round(percent) << " %]";
		callback(key, oss.str());
	}

	// Resets all accumulated values and records the current time as the start of the next interval.
	void reset_all() {
		m_last_reset = std::chrono::steady_clock::now();
		for (auto &[group, entries] : m_groups) {
			for (auto &[key, e] : entries) {
				e.total = {};
				e.count = 0;
			}
		}
	}

private:
	struct Entry {
		std::chrono::steady_clock::time_point t_start{};
		std::chrono::steady_clock::duration total{};
		int count = 0;
	};

	int m_frame_goal;
	std::chrono::steady_clock::time_point m_last_reset;
	std::unordered_map<std::string, std::unordered_map<std::string, Entry>> m_groups;

	double elapsed_seconds() const {
		return std::chrono::duration<double>(std::chrono::steady_clock::now() - m_last_reset).count();
	}

	static double ms(const Entry &e) {
		return std::chrono::duration<double, std::milli>(e.total).count();
	}

	static std::string format_entry(double total_ms, int count, double number_of_frames, const std::string &prefix) {
		const double ms_per_frame = number_of_frames > 0.0 ? total_ms / number_of_frames : 0.0;
		const double avg_ms = count > 0 ? total_ms / count : 0.0;
		std::ostringstream oss;
		oss << prefix << std::fixed << std::setprecision(3)
			<< ms_per_frame << " ms/frame (" << count << " x " << avg_ms << " ms)";
		return oss.str();
	}
};
