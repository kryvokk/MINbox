package runtimeclient

import (
	"os"
	"os/exec"
	"os/signal"
	"strconv"
	"syscall"
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

	signals := make(chan os.Signal, 4)
	signal.Notify(signals, os.Interrupt, syscall.SIGTERM, syscall.SIGHUP, syscall.SIGQUIT)
	defer signal.Stop(signals)

	if err := cmd.Start(); err != nil {
		return err
	}

	done := make(chan struct{})
	go func() {
		for {
			select {
			case received := <-signals:
				_ = cmd.Process.Signal(received)
			case <-done:
				return
			}
		}
	}()

	err := cmd.Wait()
	close(done)
	return err
}
