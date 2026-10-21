
#include <cstdio>
#include <cstring>
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

    if (chroot(argv[2]) != 0) {
        std::perror("minbox-runtime: chroot");
        return 1;
    }

    if (chdir("/") != 0) {
        std::perror("minbox-runtime: chdir");
        return 1;
    }

    execvp(argv[4], &argv[4]);

    std::perror("minbox-runtime: execvp");
    return 127;
}
