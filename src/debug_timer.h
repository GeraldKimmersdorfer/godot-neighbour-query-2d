#pragma once

#include <chrono>
#include <cmath>
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

	// Records elapsed time since start() and increments the call counter.
	void stop(const std::string &group, const std::string &key) {
		const auto now = std::chrono::steady_clock::now();
		auto &e = m_groups[group][key];
		e.total += now - e.t_start;
		e.count++;
	}

	std::string create_report() const {
		const double nframes = m_frame_goal * elapsed_seconds();
		const double frame_budget_ms = 1000.0 / m_frame_goal;

		double grand_ms = 0.0;
		std::ostringstream oss;

		for (const auto &[group, entries] : m_groups) {
			double group_ms = 0.0;
			int group_count = 0;
			for (const auto &[key, e] : entries) {
				group_ms += ms(e);
				group_count += e.count;
			}
			if (group_count == 0) { continue; }
			grand_ms += group_ms;

			oss << "\n";
			oss << "    " << group << ": " << format_entry(group_ms, group_count, nframes, frame_budget_ms);
			for (const auto &[key, e] : entries)
				if (e.count > 0)
					oss << "\n       " << key << ": " << format_entry(ms(e), e.count, nframes);
		}
		return "Total: " + format_entry(grand_ms, 0, nframes, frame_budget_ms) + oss.str();
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

	static double safe_div(double a, double b) {
		return b > 0.0 ? a / b : 0.0;
	}

	static std::string format_entry(double total_ms, int count, double number_of_frames, double frame_budget_ms = 0.0) {
		const double ms_per_frame = safe_div(total_ms, number_of_frames);
		const double avg_ms = count > 0 ? total_ms / count : 0.0;
		std::ostringstream oss;
		oss << std::fixed << std::setprecision(3) << ms_per_frame << " ms/f";
		if (frame_budget_ms > 0.0) {
			const double percent = ms_per_frame / frame_budget_ms * 100.0;
			oss << " [" << std::setprecision(0) << std::round(percent) << " %]";
		}
		if (count > 0) {
			oss << std::fixed << std::setprecision(3) << " (" << count << " x " << avg_ms << " ms)";
		}
		return oss.str();
	}
};
