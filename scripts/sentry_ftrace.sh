#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'EOF'
Usage: sentry_ftrace.sh COMMAND [ARG...]

Run a command in an isolated ftrace instance and write the captured trace when
the command exits. The wrapped application must be built with
SENTRY_FTRACE=ON to emit Sentry ftrace markers.

Environment variables:
  SENTRY_FTRACE_OUTPUT
      Output file. Defaults to <executable>.<pid>.ftrace in the current working
      directory.
  SENTRY_FTRACE_BUFFER_SIZE_KB
      Per-CPU trace buffer size. The kernel default is used when unset.
  SENTRY_FTRACE_EVENTS
      Whitespace-separated additional events. Defaults to:
      sched/sched_switch sched/sched_wakeup sched/sched_waking
  SENTRY_FTRACE_TRACEFS
      Tracefs mount point. Auto-detected when unset.
  SENTRY_FTRACE_INSTANCE
      Trace instance name. Defaults to a unique name for this wrapper process.
      An existing instance is reused but not removed.
  SENTRY_FTRACE_PRIVILEGE_COMMAND
      Command used for privileged tracefs operations. Defaults to sudo for
      non-root users. Set to an empty value for a pre-provisioned tracefs.
EOF
}

die() {
    printf 'sentry_ftrace: %s\n' "$*" >&2
    exit 1
}

if (( $# == 0 )); then
    usage >&2
    exit 2
fi
if [[ "$1" == "-h" || "$1" == "--help" ]]; then
    usage
    exit 0
fi
if [[ "$1" == "--" ]]; then
    shift
    if (( $# == 0 )); then
        usage >&2
        exit 2
    fi
fi

trace_output="${SENTRY_FTRACE_OUTPUT-}"
trace_buffer_size_kb="${SENTRY_FTRACE_BUFFER_SIZE_KB-}"
default_trace_events="sched/sched_switch sched/sched_wakeup sched/sched_waking"
trace_events="${SENTRY_FTRACE_EVENTS-$default_trace_events}"
trace_root="${SENTRY_FTRACE_TRACEFS-}"
instance_name="${SENTRY_FTRACE_INSTANCE:-sentry-$$}"
trace_events="${trace_events//$'\n'/ }"
trace_events="${trace_events//$'\t'/ }"
trace_event_list=()
if [[ -n "$trace_events" ]]; then
    read -r -a trace_event_list <<<"$trace_events"
fi

if [[ -n "$trace_buffer_size_kb"
    && ! "$trace_buffer_size_kb" =~ ^[1-9][0-9]*$ ]]; then
    die "SENTRY_FTRACE_BUFFER_SIZE_KB must be a positive integer"
fi
if [[ ! "$instance_name" =~ ^[A-Za-z0-9_.-]+$ ]]; then
    die "SENTRY_FTRACE_INSTANCE contains invalid characters"
fi

privilege_command=()
if [[ ${SENTRY_FTRACE_PRIVILEGE_COMMAND+x} ]]; then
    if [[ -n "$SENTRY_FTRACE_PRIVILEGE_COMMAND" ]]; then
        read -r -a privilege_command \
            <<<"$SENTRY_FTRACE_PRIVILEGE_COMMAND"
    fi
elif (( EUID != 0 )); then
    privilege_command=(sudo)
fi
if (( ${#privilege_command[@]} > 0 )); then
    if ! command -v "${privilege_command[0]}" >/dev/null 2>&1; then
        die "privilege command not found: ${privilege_command[0]}"
    fi
    if [[ "$(basename -- "${privilege_command[0]}")" == "sudo" ]]; then
        "${privilege_command[@]}" -v
    fi
fi

privileged() {
    if (( ${#privilege_command[@]} > 0 )); then
        "${privilege_command[@]}" "$@"
    else
        "$@"
    fi
}

if [[ -z "$trace_root" ]]; then
    if privileged test -d /sys/kernel/tracing/instances; then
        trace_root=/sys/kernel/tracing
    elif privileged test -d /sys/kernel/debug/tracing/instances; then
        trace_root=/sys/kernel/debug/tracing
    else
        die "could not find a tracefs mount with instance support"
    fi
fi
if ! privileged test -d "$trace_root/instances"; then
    die "tracefs instances directory not found at $trace_root/instances"
fi

validate_output() {
    local output_dir
    output_dir="$(dirname -- "$trace_output")"
    if [[ ! -d "$output_dir" ]]; then
        die "output directory does not exist: $output_dir"
    fi
    if [[ -e "$trace_output" && ! -w "$trace_output" ]]; then
        die "output file is not writable: $trace_output"
    fi
    if [[ ! -e "$trace_output" && ! -w "$output_dir" ]]; then
        die "output directory is not writable: $output_dir"
    fi
}

if [[ -n "$trace_output" ]]; then
    validate_output
elif [[ ! -w "$PWD" ]]; then
    die "current working directory is not writable: $PWD"
fi

trace_dir="$trace_root/instances/$instance_name"
instance_owned=0
trace_ready=0
trace_started=0
child_pid=0
child_running=0
trace_root_execute_added=0
instances_execute_added=0
instance_execute_added=0

trace_path_exists() {
    privileged test -e "$1"
}

trace_path_is_dir() {
    privileged test -d "$1"
}

other_execute_is_set() {
    local mode
    mode="$(privileged stat -c '%a' "$1")"
    case "${mode: -1}" in
        1 | 3 | 5 | 7) return 0 ;;
        *) return 1 ;;
    esac
}

trace_write() {
    local relative_path="$1"
    local value="$2"
    if (( ${#privilege_command[@]} > 0 )); then
        printf '%s\n' "$value" \
            | "${privilege_command[@]}" tee \
                "$trace_dir/$relative_path" >/dev/null
    else
        printf '%s\n' "$value" >"$trace_dir/$relative_path"
    fi
}

trace_enable_event() {
    local event="$1"
    if [[ ! "$event" =~ ^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$ ]]; then
        die "invalid ftrace event: $event"
    fi
    if trace_path_exists "$trace_dir/events/$event/enable"; then
        trace_write "events/$event/enable" 1
    else
        printf 'sentry_ftrace: event is unavailable: %s\n' "$event" >&2
    fi
}

trace_export() {
    if (( ${#privilege_command[@]} > 0 )); then
        "${privilege_command[@]}" cat "$trace_dir/trace" >"$trace_output"
    else
        cat "$trace_dir/trace" >"$trace_output"
    fi
}

cleanup() {
    local status=$?
    local cleanup_status=0
    trap - EXIT
    set +e

    if (( child_running )); then
        kill -TERM "$child_pid" 2>/dev/null
        wait "$child_pid" 2>/dev/null
        child_running=0
    fi
    if (( trace_started )); then
        trace_write tracing_on 0 || cleanup_status=1
        trace_started=0
    fi
    if (( trace_ready )); then
        if trace_export; then
            printf 'Trace written to %s\n' "$trace_output" >&2
        else
            printf 'sentry_ftrace: failed to write trace to %s\n' \
                "$trace_output" >&2
            cleanup_status=1
        fi
        if trace_path_exists "$trace_dir/events/enable"; then
            trace_write events/enable 0 || cleanup_status=1
        fi
    fi
    if (( instance_execute_added )) && trace_path_exists "$trace_dir"; then
        privileged chmod o-x "$trace_dir" || cleanup_status=1
        instance_execute_added=0
    fi
    if (( instance_owned )); then
        privileged rmdir "$trace_dir" || cleanup_status=1
    fi
    if (( instances_execute_added )); then
        privileged chmod o-x "$trace_root/instances" || cleanup_status=1
        instances_execute_added=0
    fi
    if (( trace_root_execute_added )); then
        privileged chmod o-x "$trace_root" || cleanup_status=1
        trace_root_execute_added=0
    fi

    if (( status == 0 && cleanup_status != 0 )); then
        status=$cleanup_status
    fi
    exit "$status"
}
trap cleanup EXIT

if trace_path_exists "$trace_dir"; then
    if ! trace_path_is_dir "$trace_dir"; then
        die "trace instance path is not a directory: $trace_dir"
    fi
else
    privileged mkdir "$trace_dir"
    instance_owned=1
fi

for control_file in tracing_on trace trace_marker events/enable; do
    if ! trace_path_exists "$trace_dir/$control_file"; then
        die "trace instance is missing $control_file"
    fi
done

if ! other_execute_is_set "$trace_root"; then
    privileged chmod o+x "$trace_root"
    trace_root_execute_added=1
fi
if ! other_execute_is_set "$trace_root/instances"; then
    privileged chmod o+x "$trace_root/instances"
    instances_execute_added=1
fi
if ! other_execute_is_set "$trace_dir"; then
    privileged chmod o+x "$trace_dir"
    instance_execute_added=1
fi
if (( instance_owned && EUID != 0 )); then
    privileged chown "$(id -u):$(id -g)" "$trace_dir/trace_marker"
fi
if [[ ! -w "$trace_dir/trace_marker" ]]; then
    die "trace marker is not writable by the wrapped application"
fi

trace_write tracing_on 0
trace_write events/enable 0
trace_write trace ''
if [[ -n "$trace_buffer_size_kb" ]]; then
    trace_write buffer_size_kb "$trace_buffer_size_kb"
fi
trace_enable_event ftrace/print
for event in "${trace_event_list[@]}"; do
    trace_enable_event "$event"
done

export SENTRY_FTRACE_MARKER="$trace_dir/trace_marker"
trace_started=1
trace_write tracing_on 1

"$@" &
child_pid=$!
child_running=1

if [[ -z "$trace_output" ]]; then
    executable="$(basename -- "$1")"
    trace_output="$PWD/$executable.$child_pid.ftrace"
    validate_output
fi
trace_ready=1

if wait "$child_pid"; then
    child_status=0
else
    child_status=$?
fi
child_running=0
exit "$child_status"
