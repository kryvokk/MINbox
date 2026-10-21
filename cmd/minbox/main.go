package main

import (
	"fmt"
	"os"

	"github.com/kryvokk/MINbox/internal/cli"
)

func main() {
	if err := cli.Run(os.Args[1:]); err != nil {
		fmt.Fprintln(os.Stderr, "minbox:", err)
		os.Exit(1)
	}
}
