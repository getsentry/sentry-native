# Examples

Either set `SENTRY_DSN` environment variable when running examples, or
uncomment `sentry_options_set_dsn` and replace the placeholder DSN.

- [`crash.c`](crash.c): Captures a crash event.
- [`enrich.c`](enrich.c): Enriches an event with data such as a user, tags, and contexts.
- [`feedback.c`](feedback.c): Captures an event and sends feedback for it.
- [`logs.c`](logs.c): Sends structured logs with attributes.
- [`message.c`](message.c): Captures a message event.
- [`metrics.c`](metrics.c): Records counter, gauge, and distribution metrics.
- [`scope.c`](scope.c): Captures an event with local scope data.
- [`tracing.c`](tracing.c): Traces an operation with child spans.

Build examples from the repository root:

```sh
cmake -B build -DSENTRY_BUILD_EXAMPLES=ON
cmake --build build
```

Then run individual examples from the build output directory.
