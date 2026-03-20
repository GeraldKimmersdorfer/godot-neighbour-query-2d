#pragma once

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>

class DebugTimer {
public:
	void start(const std::string &key) {
		m_entries[key].t_start = std::chrono::steady_clock::now();
	}

	void stop(const std::string &key) {
		const auto now = std::chrono::steady_clock::now();
		auto &e = m_entries[key];
		e.total += now - e.t_start;
		e.count++;
	}

	// Calls callback(key, report) for every key that has at least one recorded call. Resets all
	// entries afterwards. report is e.g. "0.223 ms/frame (100 x 0.002 ms)"
	template <typename F>
	void report_all_and_reset(F &&callback, int frame_goal, double report_interval, const std::string &sum_key = "") {
		const double expected_frames = std::max(1.0, static_cast<double>(frame_goal) * report_interval);
		double sum_total_ms = 0.0;
		int sum_count = 0;

		for (auto &[key, e] : m_entries) {
			if (e.count == 0) {
				continue;
			}
			const double total_ms = std::chrono::duration<double, std::milli>(e.total).count();
			const double avg_ms = total_ms / e.count;
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(3)
				<< (total_ms / expected_frames) << " ms/frame (" << e.count << " x " << avg_ms << " ms)";
			callback(key, oss.str());
			sum_total_ms += total_ms;
			sum_count += e.count;
			e.total = {};
			e.count = 0;
		}

		if (!sum_key.empty() && sum_count > 0) {
			const double sum_avg_ms = sum_total_ms / sum_count;
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(3)
				<< (sum_total_ms / expected_frames) << " ms/frame (" << sum_count << " x " << sum_avg_ms << " ms)";
			callback(sum_key, oss.str());
		}
	}

private:
	struct Entry {
		std::chrono::steady_clock::time_point t_start{};
		std::chrono::steady_clock::duration total{};
		int count = 0;
	};
	std::unordered_map<std::string, Entry> m_entries;
};
