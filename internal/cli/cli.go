package cli

import (
	"fmt"
	"strconv"

	"github.com/kryvokk/MINbox/internal/runtimeclient"
)

func Run(args []string) error {
	const usage = "usage: minbox run --rootfs <path> [--memory <bytes>] [--pids <count>] -- <command> [args...]"

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
			value, err := strconv.ParseUint(args[i+1], 10, 64)
			if err != nil || value == 0 {
				return fmt.Errorf("--memory must be a positive byte count")
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
