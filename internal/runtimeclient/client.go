package runtimeclient

import (
	"os"
	"os/exec"
	"strconv"
)

func Run(rootfs string, memoryBytes uint64, hasMemory bool, pidsLimit uint64, hasPids bool, command []string) error {
	args := []string{"--rootfs", rootfs}
	if hasMemory {
		args = append(args, "--memory", strconv.FormatUint(memoryBytes, 10))
	}
	if hasPids {
		args = append(args, "--pids", strconv.FormatUint(pidsLimit, 10))
	}
	args = append(args, "--")
	args = append(args, command...)

	cmd := exec.Command("minbox-runtime", args...)
	cmd.Stdin = os.Stdin
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr

	return cmd.Run()
}
