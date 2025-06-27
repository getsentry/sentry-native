#include "sentry_process.h"

#include "sentry_alloc.h"
#include "sentry_logger.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

struct sentry_process_s {
    sentry_path_t *executable;
    int argc;
    char **argv;
    char **envp;
};

static void
free_argv(char **argv, int argc)
{
    if (!argv) {
        return;
    }

    for (int i = 0; i < argc; i++) {
        sentry_free(argv[i]);
    }
    sentry_free(argv);
}

static void
free_envp(char **envp)
{
    if (!envp) {
        return;
    }

    for (int i = 0; envp[i]; i++) {
        sentry_free(envp[i]);
    }
    sentry_free(envp);
}

sentry_process_t *
sentry__process_new(const sentry_path_t *executable)
{
    sentry_process_t *process = SENTRY_MAKE(sentry_process_t);
    if (!process) {
        return NULL;
    }
    memset(process, 0, sizeof(sentry_process_t));

    process->executable = sentry__path_clone(executable);
    if (!process->executable) {
        sentry_free(process);
        return NULL;
    }

    process->argc = 1;
    process->argv = sentry_malloc(2 * sizeof(char *));
    if (!process->argv) {
        sentry__path_free(process->executable);
        sentry_free(process);
        return NULL;
    }

    process->argv[0] = sentry_malloc(strlen(process->executable->path) + 1);
    if (!process->argv[0]) {
        sentry_free(process->argv);
        sentry__path_free(process->executable);
        sentry_free(process);
        return NULL;
    }
    strcpy(process->argv[0], process->executable->path);
    process->argv[1] = NULL;

    return process;
}

void
sentry__process_free(sentry_process_t *process)
{
    if (!process) {
        return;
    }

    free_argv(process->argv, process->argc);
    free_envp(process->envp);
    sentry__path_free(process->executable);
    sentry_free(process);
}

void
sentry__process_set_env(sentry_process_t *process, const sentry_pathchar_t *key,
    const sentry_pathchar_t *value, ...)
{
    if (!process || !key || !value) {
        return;
    }

    // Count environment variables to add
    int extra_env_count = 1; // for the key=value pair
    va_list args;
    va_start(args, value);
    const sentry_pathchar_t *k, *v;
    while ((k = va_arg(args, const sentry_pathchar_t *)) != NULL
        && (v = va_arg(args, const sentry_pathchar_t *)) != NULL) {
        extra_env_count++;
    }
    va_end(args);

    // Count current environment variables
    int current_env_count = 0;
    extern char **environ;
    if (environ) {
        while (environ[current_env_count]) {
            current_env_count++;
        }
    }

    // Free existing environment
    free_envp(process->envp);

    // Allocate array for environment variables
    process->envp = sentry_malloc(
        (current_env_count + extra_env_count + 1) * sizeof(char *));
    if (!process->envp) {
        return;
    }

    int env_index = 0;

    // Add the new environment variables
    size_t key_len = strlen(key);
    size_t value_len = strlen(value);
    process->envp[env_index] = sentry_malloc(key_len + 1 + value_len + 1);
    if (process->envp[env_index]) {
        snprintf(process->envp[env_index], key_len + 1 + value_len + 1, "%s=%s",
            key, value);
        env_index++;
    }

    va_start(args, value);
    while ((k = va_arg(args, const sentry_pathchar_t *)) != NULL
        && (v = va_arg(args, const sentry_pathchar_t *)) != NULL) {
        size_t k_len = strlen(k);
        size_t v_len = strlen(v);
        process->envp[env_index] = sentry_malloc(k_len + 1 + v_len + 1);
        if (process->envp[env_index]) {
            snprintf(
                process->envp[env_index], k_len + 1 + v_len + 1, "%s=%s", k, v);
            env_index++;
        }
    }
    va_end(args);

    // Copy current environment
    if (environ) {
        for (int i = 0; i < current_env_count; i++) {
            process->envp[env_index] = sentry_malloc(strlen(environ[i]) + 1);
            if (process->envp[env_index]) {
                strcpy(process->envp[env_index], environ[i]);
                env_index++;
            }
        }
    }

    // Null-terminate the array
    process->envp[env_index] = NULL;
}

static bool
add_argument(sentry_process_t *process, const char *arg)
{
    if (!process || !arg) {
        return false;
    }

    // Reallocate argv to fit the new argument
    char **new_argv = sentry_malloc((process->argc + 2) * sizeof(char *));
    if (!new_argv) {
        return false;
    }

    // Copy existing arguments
    for (int i = 0; i < process->argc; i++) {
        new_argv[i] = process->argv[i];
    }

    // Add new argument
    new_argv[process->argc] = sentry_malloc(strlen(arg) + 1);
    if (!new_argv[process->argc]) {
        sentry_free(new_argv);
        return false;
    }
    strcpy(new_argv[process->argc], arg);

    // Null-terminate
    new_argv[process->argc + 1] = NULL;

    // Replace old argv
    sentry_free(process->argv);
    process->argv = new_argv;
    process->argc++;

    return true;
}

bool
sentry__process_spawn(sentry_process_t *process)
{
    if (!process || !process->executable || !process->argv) {
        return false;
    }

    // POSIX implementation using double fork to create a fully detached
    // subprocess This avoids zombie processes and ensures the child is
    // completely independent
    pid_t pid1 = fork();
    if (pid1 == -1) {
        SENTRY_ERRORF("first fork() failed: %s", strerror(errno));
        return false;
    }

    if (pid1 == 0) {
        // First child process
        // Create new session and process group to detach from parent
        if (setsid() == -1) {
            SENTRY_ERRORF("setsid() failed: %s", strerror(errno));
            _exit(1);
        }

        // Second fork to ensure the process is not a session leader
        // and cannot acquire a controlling terminal (fully detached)
        pid_t pid2 = fork();
        if (pid2 == -1) {
            SENTRY_ERRORF("second fork() failed: %s", strerror(errno));
            _exit(1);
        }

        if (pid2 == 0) {
            // Second child process - this will be the fully detached subprocess
            // Redirect stdin, stdout, stderr to /dev/null
            int dev_null = open("/dev/null", O_RDWR);
            if (dev_null != -1) {
                dup2(dev_null, STDIN_FILENO);
                dup2(dev_null, STDOUT_FILENO);
                dup2(dev_null, STDERR_FILENO);
                if (dev_null > STDERR_FILENO) {
                    close(dev_null);
                }
            }

            // Execute with environment
            if (process->envp) {
                execve(process->executable->path, process->argv, process->envp);
            } else {
                execv(process->executable->path, process->argv);
            }

            SENTRY_ERRORF("execv failed: %s", strerror(errno));
            _exit(1);
        } else {
            // First child exits immediately
            _exit(0);
        }
    } else {
        // Parent process - wait for first child to exit
        int status;
        if (waitpid(pid1, &status, 0) == -1) {
            SENTRY_ERRORF("waitpid() failed: %s", strerror(errno));
            return false;
        }

        // Check if first child exited successfully
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            return true;
        } else {
            SENTRY_ERRORF("child process failed with status %d", status);
            return false;
        }
    }
}

bool
sentry__process_spawn_with_args(
    sentry_process_t *process, const sentry_pathchar_t *arg, ...)
{
    if (!process || !arg) {
        return false;
    }

    // Add the first argument
    if (!add_argument(process, arg)) {
        return false;
    }

    // Add remaining arguments
    va_list args;
    va_start(args, arg);
    const sentry_pathchar_t *a;
    while ((a = va_arg(args, const sentry_pathchar_t *)) != NULL) {
        if (!add_argument(process, a)) {
            va_end(args);
            return false;
        }
    }
    va_end(args);

    return sentry__process_spawn(process);
}
