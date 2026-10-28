#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sched.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc < 5 ||
        std::strcmp(argv[1], "--rootfs") != 0 ||
        std::strcmp(argv[3], "--") != 0) {
        std::fprintf(
            stderr,
            "Usage: %s --rootfs <path> -- <command> [args...]\n",
            argv[0]
        );
        return 2;
    }

    if (unshare(CLONE_NEWPID) != 0) {
        std::perror("minbox-runtime: unshare PID namespace");
        return 1;
    }

    const pid_t child = fork();
    if (child == -1) {
        std::perror("minbox-runtime: fork");
        return 1;
    }

    if (child == 0) {
        if (unshare(CLONE_NEWNS) != 0) {
            std::perror("minbox-runtime: unshare mount namespace");
            _exit(1);
        }

        if (mount(nullptr, "/", nullptr, MS_REC | MS_PRIVATE, nullptr) != 0) {
            std::perror("minbox-runtime: make mounts private");
            _exit(1);
        }

        if (chroot(argv[2]) != 0) {
            std::perror("minbox-runtime: chroot");
            _exit(1);
        }

        if (chdir("/") != 0) {
            std::perror("minbox-runtime: chdir");
            _exit(1);
        }

        if (mount("proc", "/proc", "proc",
                  MS_NOSUID | MS_NODEV | MS_NOEXEC, nullptr) != 0) {
            std::perror("minbox-runtime: mount /proc");
            _exit(1);
        }

        execvp(argv[4], &argv[4]);

        std::perror("minbox-runtime: execvp");
        _exit(127);
    }

    int status = 0;
    pid_t waited;
    do {
        waited = waitpid(child, &status, 0);
    } while (waited == -1 && errno == EINTR);

    if (waited == -1) {
        std::perror("minbox-runtime: waitpid");
        return 1;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }

    return 1;
}
