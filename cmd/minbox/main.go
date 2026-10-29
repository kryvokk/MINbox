package main

import (
	"errors"
	"fmt"
	"os"
	"os/exec"

	"github.com/kryvokk/MINbox/internal/cli"
)

func main() {
	if err := cli.Run(os.Args[1:]); err != nil {
		var exitErr *exec.ExitError
		if errors.As(err, &exitErr) {
			os.Exit(exitErr.ExitCode())
		}

		fmt.Fprintln(os.Stderr, "minbox:", err)
		os.Exit(1)
	}
}
