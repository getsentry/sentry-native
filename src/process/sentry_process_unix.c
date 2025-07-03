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
    char **argv;
    char **envp;
};

static char **
string_array_new(size_t len)
{
    char **array = sentry_malloc(len * sizeof(char *) + 1);
    if (array) {
        array[len] = NULL;
    }
    return array;
}

static bool
string_array_set_value(char **array, size_t index, const char *value)
{
    if (!array || !value) {
        return false;
    }

    size_t len = strlen(value);
    array[index] = sentry_malloc(len + 1);
    if (!array[index]) {
        return false;
    }
    strcpy(array[index], value);
    return true;
}

static bool
string_array_set_key_value(
    char **array, size_t index, const char *key, const char *value)
{
    if (!array || !key || !value) {
        return false;
    }

    size_t key_len = strlen(key);
    size_t value_len = strlen(value);
    array[index] = sentry_malloc(key_len + 1 + value_len + 1);
    if (!array[index]) {
        return false;
    }
    snprintf(array[index], key_len + 1 + value_len + 1, "%s=%s", key, value);
    return true;
}

static void
string_array_free(char **array)
{
    if (!array) {
        return;
    }

    for (int i = 0; array[i]; i++) {
        sentry_free(array[i]);
    }
    sentry_free(array);
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

    return process;
}

void
sentry__process_free(sentry_process_t *process)
{
    if (!process) {
        return;
    }

    string_array_free(process->argv);
    string_array_free(process->envp);
    sentry__path_free(process->executable);
    sentry_free(process);
}

void
sentry__process_set_env(
    sentry_process_t *process, const char *key0, const char *value0, ...)
{
    if (!process || !key0 || !value0) {
        return;
    }

    // current env + key0=value0 + variable key-value pairs
    int env_len = 1;
    extern char **environ;
    while (environ && environ[env_len - 1]) {
        env_len++;
    }
    va_list args;
    va_start(args, value0);
    const char *key, *value;
    while ((key = va_arg(args, const char *)) != NULL
        && (value = va_arg(args, const char *)) != NULL) {
        env_len++;
    }
    va_end(args);

    // key0=value0
    int i = 0;
    char **envp = string_array_new(env_len);
    if (!string_array_set_key_value(envp, i++, key0, value0)) {
        string_array_free(envp);
        return;
    }

    // variable key-value pairs
    va_start(args, value0);
    while ((key = va_arg(args, const char *)) != NULL
        && (value = va_arg(args, const char *)) != NULL) {
        if (!string_array_set_key_value(envp, i++, key, value)) {
            string_array_free(envp);
            va_end(args);
            return;
        }
    }
    va_end(args);

    // current env
    char **env = environ;
    while (env && *env) {
        if (!string_array_set_value(envp, i++, *env)) {
            string_array_free(envp);
            return;
        }
        env++;
    }

    string_array_free(process->envp);
    process->envp = envp;
}

/**
 * Spawns a new fully detached subprocess by double-forking.
 */
bool
spawn_process(sentry_process_t *process)
{
    pid_t pid1 = fork();
    if (pid1 == -1) {
        SENTRY_ERRORF("first fork() failed: %s", strerror(errno));
        return false;
    }

    if (pid1 == 0) {
        // first child process: create new session and process group to detach
        // from parent
        if (setsid() == -1) {
            SENTRY_ERRORF("setsid() failed: %s", strerror(errno));
            _exit(1);
        }

        // second fork to ensure the process is not a session leader and cannot
        // acquire a controlling terminal
        pid_t pid2 = fork();
        if (pid2 == -1) {
            SENTRY_ERRORF("second fork() failed: %s", strerror(errno));
            _exit(1);
        }

        if (pid2 == 0) {
            // second child process: redirect stdin/out/err to /dev/null
            int dev_null = open("/dev/null", O_RDWR);
            if (dev_null != -1) {
                dup2(dev_null, STDIN_FILENO);
                dup2(dev_null, STDOUT_FILENO);
                dup2(dev_null, STDERR_FILENO);
                if (dev_null > STDERR_FILENO) {
                    close(dev_null);
                }
            }

            // finally, execute the process
            if (process->envp) {
                execvpe(
                    process->executable->path, process->argv, process->envp);
            } else {
                execvp(process->executable->path, process->argv);
            }

            SENTRY_ERRORF("execv failed: %s", strerror(errno));
            _exit(1);
        } else {
            // the first child exits immediately
            _exit(0);
        }
    } else {
        // parent process: wait for the first child to exit
        int status;
        if (waitpid(pid1, &status, 0) == -1) {
            SENTRY_ERRORF("waitpid() failed: %s", strerror(errno));
            return false;
        }
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            SENTRY_ERRORF("child process failed with status %d", status);
            return false;
        }
        return true;
    }
}

bool
sentry__process_spawn(sentry_process_t *process)
{
    if (!process) {
        return false;
    }

    char **argv = string_array_new(1);
    if (!string_array_set_value(argv, 0, process->executable->path)) {
        string_array_free(argv);
        return NULL;
    }

    string_array_free(process->argv);
    process->argv = argv;
    return spawn_process(process);
}

bool
sentry__process_spawn_with_args(
    sentry_process_t *process, const char *arg0, ...)
{
    if (!process || !arg0) {
        return false;
    }

    // exe + arg0 + variable args
    int argc = 2;
    va_list args;
    va_start(args, arg0);
    while (va_arg(args, const char *) != NULL) {
        argc++;
    }
    va_end(args);

    // exe, arg0
    int i = 0;
    char **argv = string_array_new(argc);
    if (!string_array_set_value(argv, i++, process->executable->path)
        || !string_array_set_value(argv, i++, arg0)) {
        string_array_free(argv);
        return false;
    }

    // variable args
    va_start(args, arg0);
    while (va_arg(args, const char *) != NULL) {
        const char *argn = va_arg(args, const char *);
        if (!string_array_set_value(argv, i++, argn)) {
            va_end(args);
            string_array_free(argv);
            return false;
        }
    }
    va_end(args);

    string_array_free(process->argv);
    process->argv = argv;

    return spawn_process(process);
}
