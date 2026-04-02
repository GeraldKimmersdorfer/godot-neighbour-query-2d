extends RefCounted
class_name Benchmark

const ROW_KEYS := ["get_closest", "get_all", "get_next", "get_random", "get_next_first", "refresh"]
const ATTR_KEYS := ["count", "placement", "movement", "time", "speed"]
const BENCHMARK_TIME_EACH := 5.0
const QUERY_NODE_SPEED_EACH := 3.0
const RUNS := [
	{"count": 5000, "placement": DotContainer.InitPlacementMode.DENSITY, "movement": Dot.MovementMode.NONE,     "time": BENCHMARK_TIME_EACH, "speed": QUERY_NODE_SPEED_EACH},
	{"count": 500,  "placement": DotContainer.InitPlacementMode.DENSITY, "movement": Dot.MovementMode.NONE,     "time": BENCHMARK_TIME_EACH, "speed": QUERY_NODE_SPEED_EACH},
	{"count": 0,    "placement": DotContainer.InitPlacementMode.UNIFORM,               "movement": Dot.MovementMode.NONE,     "time": BENCHMARK_TIME_EACH, "speed": QUERY_NODE_SPEED_EACH},
	{"count": 5000, "placement": DotContainer.InitPlacementMode.UNIFORM,               "movement": Dot.MovementMode.NONE,     "time": BENCHMARK_TIME_EACH, "speed": QUERY_NODE_SPEED_EACH},
	{"count": 5000, "placement": DotContainer.InitPlacementMode.UNIFORM,               "movement": Dot.MovementMode.STRAIGHT, "time": BENCHMARK_TIME_EACH, "speed": QUERY_NODE_SPEED_EACH},
]

static func run_benchmark(option_container: OptionContainer) -> void:
	var saved_state: Dictionary = option_container.get_state()

	# Split attrs into common (same across all runs) and varying
	var common := {}
	var varying := []
	for attr in ATTR_KEYS:
		var first = RUNS[0][attr]
		var all_same := true
		for run in RUNS:
			if run[attr] != first:
				all_same = false
				break
		if all_same:
			common[attr] = first
		else:
			varying.append(attr)

	# Build per-run labels from varying attrs only
	var labels: PackedStringArray = []
	for run in RUNS:
		var parts := []
		for attr in varying:
			parts.append(_attr_str(attr, run[attr]))
		labels.append(" ".join(parts))

	var results: Array[Dictionary] = []
	for run in RUNS:
		var report: String = await option_container.run_single_benchmark(run["count"], run["placement"], run["movement"], run["time"], run["speed"])
		results.append(_parse_report(report))

	option_container.restore_state(saved_state)

	var output := _build_description(common) + _build_table(labels, results)
	DisplayServer.clipboard_set(output)
	OS.alert("Benchmark complete. Results copied to clipboard.", "Benchmark")

static func _attr_str(attr: String, val) -> String:
	match attr:
		"count":     return str(val)
		"placement": return DotContainer.InitPlacementMode.keys()[val]
		"movement":  return Dot.MovementMode.keys()[val]
		"time":      return "%.0fs" % val
		"speed":     return "%.2f rad/s" % val
	return str(val)

static func _build_description(common: Dictionary) -> String:
	var dt := Time.get_datetime_dict_from_system()
	var build := "DEBUG" if OS.is_debug_build() else "RELEASE"
	var title := "## Benchmark %02d.%02d %s" % [dt["month"], dt["day"], build]
	var lines := [title]
	lines.append("**Time:** %02d:%02d:%02d" % [dt["hour"], dt["minute"], dt["second"]])
	lines.append("**CPU:** %s" % OS.get_processor_name())
	if not common.is_empty():
		var parts := []
		for attr in common:
			parts.append("%s=%s" % [attr, _attr_str(attr, common[attr])])
		lines.append("**Common:** %s" % ", ".join(parts))
	lines.append("")
	return "\n".join(lines)

## Parses avg times from a benchmark report string.
## Returns a dict of { key: "value unit" }
static func _parse_report(report: String) -> Dictionary:
	var out := {}
	var regex := RegEx.new()
	# Matches "    get_closest: 8.39 us/f (1200 x 8.33 us)"
	# Captures: (1) key name, (2) avg value + unit as one string
	regex.compile(r"^\s+(\w+):\s[\d.]+\s\S+(?:\s\[\d+\s%\])?\s+\(\d+\s+x\s+([\d.]+\s+(?:ns|us|ms|s))\)")
	for line in report.split("\n"):
		var m := regex.search(line)
		if m:
			out[m.get_string(1)] = m.get_string(2)
	return out

static func _build_table(labels: PackedStringArray, results: Array[Dictionary]) -> String:
	var header := "| metric |"
	var sep := "| --- |"
	for lbl in labels:
		header += " %s |" % lbl
		sep += " --- |"

	var lines := [header, sep]
	for key in ROW_KEYS:
		var row := "| %s |" % key
		for r in results:
			row += " %s |" % r.get(key, "-")
		lines.append(row)

	return "\n".join(lines)
