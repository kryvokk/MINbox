package cli

import (
	"fmt"
	"math"
	"strconv"
	"strings"

	"github.com/kryvokk/MINbox/internal/runtimeclient"
)

func Run(args []string) error {
	const usage = "usage: minbox run --rootfs <path> [--memory <size>] [--pids <count>] -- <command> [args...]"

	if len(args) < 4 || args[0] != "run" {
		return fmt.Errorf("%s", usage)
	}

	var rootfs string
	var memoryBytes uint64
	var pidsLimit uint64
	hasMemory := false
	hasPids := false
	i := 1
	for i < len(args) && args[i] != "--" {
		if i+1 >= len(args) {
			return fmt.Errorf("%s", usage)
		}
		switch args[i] {
		case "--rootfs":
			if rootfs != "" || args[i+1] == "" {
				return fmt.Errorf("%s", usage)
			}
			rootfs = args[i+1]
		case "--memory":
			if hasMemory {
				return fmt.Errorf("%s", usage)
			}
			value, err := parseMemorySize(args[i+1])
			if err != nil {
				return fmt.Errorf("invalid --memory value %q: %w", args[i+1], err)
			}
			memoryBytes, hasMemory = value, true
		case "--pids":
			if hasPids {
				return fmt.Errorf("%s", usage)
			}
			value, err := strconv.ParseUint(args[i+1], 10, 64)
			if err != nil || value == 0 {
				return fmt.Errorf("--pids must be a positive process count")
			}
			pidsLimit, hasPids = value, true
		default:
			return fmt.Errorf("%s", usage)
		}
		i += 2
	}
	if rootfs == "" || i >= len(args) || i+1 >= len(args) {
		return fmt.Errorf("%s", usage)
	}
	command := args[i+1:]
	if command[0] == "" {
		return fmt.Errorf("%s", usage)
	}

	return runtimeclient.Run(rootfs, memoryBytes, hasMemory, pidsLimit, hasPids, command)
}

func parseMemorySize(input string) (uint64, error) {
	numberEnd := 0
	for numberEnd < len(input) && input[numberEnd] >= '0' && input[numberEnd] <= '9' {
		numberEnd++
	}
	if numberEnd == 0 {
		return 0, fmt.Errorf("expected a positive integer with an optional unit (for example 64M or 128MiB)")
	}

	value, err := strconv.ParseUint(input[:numberEnd], 10, 64)
	if err != nil || value == 0 {
		return 0, fmt.Errorf("expected a positive integer with an optional unit (for example 64M or 128MiB)")
	}

	var multiplier uint64
	switch strings.ToUpper(input[numberEnd:]) {
	case "", "B":
		multiplier = 1
	case "K", "KIB":
		multiplier = 1 << 10
	case "M", "MIB":
		multiplier = 1 << 20
	case "G", "GIB":
		multiplier = 1 << 30
	case "T", "TIB":
		multiplier = 1 << 40
	case "KB":
		multiplier = 1000
	case "MB":
		multiplier = 1000 * 1000
	case "GB":
		multiplier = 1000 * 1000 * 1000
	case "TB":
		multiplier = 1000 * 1000 * 1000 * 1000
	default:
		return 0, fmt.Errorf("unknown unit; use B, K/M/G/T, KiB/MiB/GiB/TiB, or KB/MB/GB/TB")
	}

	if value > math.MaxUint64/multiplier {
		return 0, fmt.Errorf("size is too large")
	}
	return value * multiplier, nil
}
