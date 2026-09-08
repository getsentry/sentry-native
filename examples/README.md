# Examples

Set the `SENTRY_DSN` environment variable.

Build from the repository root:

```sh
cmake -S . -B build -DSENTRY_BUILD_EXAMPLES=ON
cmake --build build --config RelWithDebInfo
```

Run `sentry_example_<name>` from the build output directory.

- [`feedback`](feedback.c): Sends feedback for an existing event.
  Usage: `sentry_example_feedback <envelope> <message> [email] [name] [attachment ...]`
