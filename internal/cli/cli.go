package cli

import (
	"fmt"

	"github.com/kryvokk/MINbox/internal/runtimeclient"
)

func Run(args []string) error {
	const usage = "usage: minbox run --rootfs <path> -- <command> [args...]"

	if len(args) < 5 || args[0] != "run" {
		return fmt.Errorf("%s", usage)
	}

	if args[1] != "--rootfs" || args[3] != "--" {
		return fmt.Errorf("%s", usage)
	}

	rootfs := args[2]
	command := args[4:]
	if rootfs == "" || command[0] == "" {
		return fmt.Errorf("%s", usage)
	}

	return runtimeclient.Run(rootfs, command)
}
