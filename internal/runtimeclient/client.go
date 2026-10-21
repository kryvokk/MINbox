package runtimeclient

import (
	"os"
	"os/exec"
)

func Run(rootfs string, command []string) error {
	args := []string{"--rootfs", rootfs, "--"}
	args = append(args, command...)

	cmd := exec.Command("minbox-runtime", args...)
	cmd.Stdin = os.Stdin
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr

	return cmd.Run()
}
