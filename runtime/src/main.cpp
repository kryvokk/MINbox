#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sched.h>
#include <string>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

constexpr char cgroupRoot[] = "/sys/fs/cgroup";

bool writeControl(const std::string& path, const std::string& value) {
    const int fd = open(path.c_str(), O_WRONLY | O_CLOEXEC);
    if (fd == -1) {
        std::perror(("minbox-runtime: open " + path).c_str());
        return false;
    }

    const ssize_t written = write(fd, value.data(), value.size());
    const int writeError = errno;
    close(fd);
    if (written != static_cast<ssize_t>(value.size())) {
        errno = written == -1 ? writeError : EIO;
        std::perror(("minbox-runtime: write " + path).c_str());
        return false;
    }
    return true;
}

bool createCgroup(const std::string& path, const char* memoryLimit,
                  const char* pidsLimit) {
    std::string controllers;
    if (memoryLimit != nullptr) {
        controllers += "+memory";
    }
    if (pidsLimit != nullptr) {
        if (!controllers.empty()) {
            controllers += ' ';
        }
        controllers += "+pids";
    }

    if (!writeControl(std::string(cgroupRoot) + "/cgroup.subtree_control",
                      controllers)) {
        return false;
    }

    if (mkdir(path.c_str(), 0755) != 0) {
        std::perror(("minbox-runtime: mkdir " + path).c_str());
        return false;
    }

    if (memoryLimit != nullptr &&
        !writeControl(path + "/memory.max", memoryLimit)) {
        rmdir(path.c_str());
        return false;
    }
    if (pidsLimit != nullptr && !writeControl(path + "/pids.max", pidsLimit)) {
        rmdir(path.c_str());
        return false;
    }
    return true;
}

void removeCgroup(const std::string& path) {
    if (rmdir(path.c_str()) != 0 && errno != ENOENT) {
        std::perror(("minbox-runtime: remove cgroup " + path).c_str());
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    const char* rootfs = nullptr;
    const char* memoryLimit = nullptr;
    const char* pidsLimit = nullptr;
    int i = 1;

    while (i < argc && std::strcmp(argv[i], "--") != 0) {
        if (i + 1 >= argc) {
            break;
        }
        if (std::strcmp(argv[i], "--rootfs") == 0 && rootfs == nullptr) {
            rootfs = argv[i + 1];
        } else if (std::strcmp(argv[i], "--memory") == 0 &&
                   memoryLimit == nullptr) {
            memoryLimit = argv[i + 1];
        } else if (std::strcmp(argv[i], "--pids") == 0 && pidsLimit == nullptr) {
            pidsLimit = argv[i + 1];
        } else {
            break;
        }
        i += 2;
    }

    if (rootfs == nullptr || i >= argc || std::strcmp(argv[i], "--") != 0 ||
        i + 1 >= argc || argv[i + 1][0] == '\0') {
        std::fprintf(stderr,
                     "Usage: %s --rootfs <path> [--memory <bytes>] "
                     "[--pids <count>] -- <command> [args...]\n",
                     argv[0]);
        return 2;
    }

    std::string cgroupPath;
    const bool useCgroup = memoryLimit != nullptr || pidsLimit != nullptr;
    if (useCgroup) {
        cgroupPath = std::string(cgroupRoot) + "/minbox-" +
                     std::to_string(getpid());
        if (!createCgroup(cgroupPath, memoryLimit, pidsLimit)) {
            return 1;
        }
    }

    int gate[2] = {-1, -1};
    if (useCgroup && pipe2(gate, O_CLOEXEC) != 0) {
        std::perror("minbox-runtime: pipe2");
        removeCgroup(cgroupPath);
        return 1;
    }

    if (unshare(CLONE_NEWPID) != 0) {
        std::perror("minbox-runtime: unshare PID namespace");
        if (useCgroup) {
            close(gate[0]);
            close(gate[1]);
            removeCgroup(cgroupPath);
        }
        return 1;
    }

    const pid_t child = fork();
    if (child == -1) {
        std::perror("minbox-runtime: fork");
        if (useCgroup) {
            close(gate[0]);
            close(gate[1]);
            removeCgroup(cgroupPath);
        }
        return 1;
    }

    if (child == 0) {
        if (useCgroup) {
            close(gate[1]);
            char ready = 0;
            ssize_t count;
            do {
                count = read(gate[0], &ready, 1);
            } while (count == -1 && errno == EINTR);
            close(gate[0]);
            if (count != 1 || ready != '1') {
                _exit(1);
            }
        }

        if (unshare(CLONE_NEWNS | CLONE_NEWUTS) != 0) {
            std::perror("minbox-runtime: unshare mount/UTS namespaces");
            _exit(1);
        }

        constexpr char containerHostname[] = "minbox";
        if (sethostname(containerHostname, sizeof(containerHostname) - 1) != 0) {
            std::perror("minbox-runtime: sethostname");
            _exit(1);
        }

        if (mount(nullptr, "/", nullptr, MS_REC | MS_PRIVATE, nullptr) != 0) {
            std::perror("minbox-runtime: make mounts private");
            _exit(1);
        }

        if (chroot(rootfs) != 0) {
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

        execvp(argv[i + 1], &argv[i + 1]);
        std::perror("minbox-runtime: execvp");
        _exit(127);
    }

    if (useCgroup) {
        close(gate[0]);
        const std::string childPid = std::to_string(child);
        const bool moved = writeControl(cgroupPath + "/cgroup.procs", childPid);
        const char ready = moved ? '1' : '0';
        if (write(gate[1], &ready, 1) != 1) {
            std::perror("minbox-runtime: release child");
        }
        close(gate[1]);
        if (!moved) {
            int ignoredStatus = 0;
            waitpid(child, &ignoredStatus, 0);
            removeCgroup(cgroupPath);
            return 1;
        }
    }

    int status = 0;
    pid_t waited;
    do {
        waited = waitpid(child, &status, 0);
    } while (waited == -1 && errno == EINTR);

    if (useCgroup) {
        removeCgroup(cgroupPath);
    }
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
