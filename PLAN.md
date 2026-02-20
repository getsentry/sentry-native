# TUS Upload for Large Attachments

## Context

Sentry envelopes can contain large attachments (especially minidumps, 10-500+ MB). Currently the entire envelope is serialized and POST'd in one request. Relay PR [#5638](https://github.com/getsentry/relay/pull/5638) adds `POST /api/{project_id}/upload/` using TUS "creation-with-upload" — the SDK should upload large attachments there, then send the envelope with lightweight `attachment_ref` placeholder items instead.

## Design

**Threshold**: attachments >= 100 MB use TUS upload.

**Attachment placeholders** ([docs PR #16483](https://github.com/getsentry/sentry-docs/pull/16483)): item type stays `"attachment"`, distinguished by content type `"application/vnd.sentry.attachment-ref"`. Headers: `attachment_length` (file size), `filename`, `attachment_type`. Location is the full value from the TUS `Location` response header. Payload states:
- Pre-upload: `{"path":"/absolute/path/to/file"}` (local-only, never sent to server)
- Post-upload: `{"location":"<Location-header-value>"}`
- On-disk (crash dump): `{"filename":"<name>"}` (relative to `<db>/attachments/<event-uuid>/`)

**Flow** (at send time, on the bgworker thread):
1. Scan envelope items for `attachment_ref` items with `"path"` in payload
2. For each: stream file to `/upload/` with TUS headers → get `Location` back
3. Replace payload with `{"location":"<Location-header-value>"}`
4. Serialize and send the (now smaller) envelope normally

**Streaming upload**: large attachments are never fully loaded into memory. The file is streamed directly from disk to the HTTP request:
- **curl**: `CURLOPT_READFUNCTION` + `CURLOPT_READDATA` with file descriptor
- **WinHTTP**: `WinHttpSendRequest` (headers only) + `WinHttpWriteData` loop

**Crash envelopes**: with attachment_ref, large files are NOT embedded inline. The original file must be preserved separately to survive crash + restart:
- **Copy** the file to `<db>/attachments/<event-uuid>/<filename>` (NEVER rename user-provided files — they're not ours to take)
- Update `attachment_ref` payload to `{"filename":"<name>"}`
- Serialized envelope stays small
- Special case: breakpad minidumps are written to `<run_path>/` and deleted after envelope serialization (line 204). For these SDK-owned temp files, rename is acceptable and avoids copying large data in signal handler.
- On restart, `sentry__process_old_runs` loads envelopes via `sentry__envelope_from_path()`. Lazy materialization (via `envelope_materialize()`) transparently parses them on first access to headers/items. For envelopes with an attachments dir, iterate items, find attachment_ref items with `"filename"` in payload, replace with full paths. Normal TUS flow handles them.

**Fallback**: if TUS upload fails (404, network error), drop the attachment (it's too large for inline). If server returns 404, disable TUS for the transport lifetime.

## Attachment Placeholder Wire Format (per [docs PR #16483](https://github.com/getsentry/sentry-docs/pull/16483))

Item header:
```json
{"type":"attachment","content_type":"application/vnd.sentry.attachment-ref","attachment_length":104857600,"filename":"crash.dmp","attachment_type":"event.minidump","length":N}
```

Payload:
```json
{"location":"/api/42/upload/019c7a950dd376a1817a9ced5cb7c4b5/?length=212341234&signature=Zct1IMmM3BIJrzDOwG3tUn5AlrLhqIJFBu8Kd59dXCz"}
```

`type` = `"attachment"`, `content_type` distinguishes from inline. `attachment_length` = file size. `length` = payload size. Headers preserve attachment metadata for Relay reassembly.

## Steps

### 1. Extend request struct for file streaming ✅

**`src/transports/sentry_http_transport.h`**:
- Add to `sentry_prepared_http_request_t`:
  ```c
  sentry_path_t *body_path;  // file to stream (alternative to body pointer)
  size_t body_path_len;       // expected file size (= Upload-Length)
  ```
- Add `char *location` to `sentry_http_response_t`

When `body_path` is set, backends stream from file instead of using `body` pointer.

### 2. Curl: streaming file upload ✅

**`src/transports/sentry_http_transport_curl.c`**:
- Add file read callback:
  ```c
  static size_t file_read_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
      FILE *f = userdata;
      return fread(buffer, size, nitems, f);
  }
  ```
- In `curl_send_task`: if `req->body_path` is set:
  - `fopen` the file
  - Use `CURLOPT_UPLOAD` + `CURLOPT_READFUNCTION` + `CURLOPT_READDATA` instead of `CURLOPT_POSTFIELDS`
  - Set `CURLOPT_INFILESIZE_LARGE` to `body_path_len`
  - `fclose` after `curl_easy_perform`
- Capture `Location` response header in `header_callback`

### 3. WinHTTP: streaming file upload ✅

**`src/transports/sentry_http_transport_winhttp.c`**:
- In `winhttp_send_task`: if `req->body_path` is set:
  - Open file with `CreateFile` / `_wfopen`
  - Call `WinHttpSendRequest(request, headers, -1, NULL, 0, totalLen, 0)` — headers only
  - Loop: read file in ~1 MB chunks, `WinHttpWriteData(request, chunk, chunkSize, &written)`
  - `WinHttpReceiveResponse` after all data written
  - Close file handle
- Query `Location` response header

### 4. Free new fields in request/response cleanup ✅

**`src/transports/sentry_http_transport.c`**:
- `sentry__prepared_http_request_free`: free `body_path` if set
- `http_send_request`: handle `resp.location` (extract before freeing)

### 5. Upload URL builder ✅

**`src/sentry_utils.c`** + **`src/sentry_utils.h`** — add `sentry__dsn_get_upload_url(dsn)`:
```
{scheme}://{host}:{port}{path}/api/{project_id}/upload/?sentry_key={public_key}
```

### 6. Envelope item accessors ✅

**`src/sentry_envelope.h`** + **`src/sentry_envelope.c`**:
- Un-guard from `#ifdef SENTRY_UNITTEST`:
  - `sentry__envelope_get_item_count()`
  - `sentry__envelope_get_item()` / `sentry__envelope_get_item_mut()`
  - `sentry__envelope_item_get_header()` / `sentry__envelope_item_get_payload()`
  - `sentry__envelope_item_set_payload(item, buf, len)` — replaces payload, updates `length` header

### 7. Large attachment → attachment placeholder at construction ✅

**`src/sentry_envelope.c`** — modify `sentry__envelope_add_attachment()`:
- Before calling `sentry__envelope_add_from_path()`, check `sentry__path_get_size()`
- If >= 100 MB: create item via `envelope_add_item()`, set type `"attachment"`, content_type `"application/vnd.sentry.attachment-ref"`, `attachment_length` header = file size, preserve filename/attachment_type headers, set payload to JSON `{"path":"<absolute-path>"}` — file NOT read
- If < 100 MB: read file inline as before

### 8. TUS request preparation ✅

**`src/transports/sentry_http_transport.c`** — add `prepare_tus_request(path, file_size, dsn, user_agent)`:
- method: `"POST"`
- url: `sentry__dsn_get_upload_url(dsn)`
- headers: `Tus-Resumable: 1.0.0`, `Upload-Length: <file_size>`, `Content-Type: application/offset+octet-stream`, `x-sentry-auth: <auth>`
- `body_path`: clone of the path
- `body_path_len`: file size
- No `body` pointer, no compression

### 9. TUS upload + envelope rewriting ✅

**`src/transports/sentry_http_transport.c`** — add `tus_upload_attachment_refs(state, envelope)`:
- Skip if `!state->has_tus`
- Iterate items via accessors
- For attachment items with content_type `"application/vnd.sentry.attachment-ref"`:
  - Parse payload JSON, extract `"path"` → local file path
  - `stat()` file to get size
  - Build TUS request via `prepare_tus_request`
  - Send via `state->send_func` (backend streams from file)
  - On 201 + Location: replace payload with `{"location":"<Location-header-value>"}` (header values pre-trimmed in curl/winhttp backends)
  - On 404: set `state->has_tus = false`, return
  - On other failure: continue
- Free TUS response fields

Add `bool has_tus` to `http_transport_state_t` (initialized to `true`).

### 10. Integrate into send path ✅

**`src/transports/sentry_http_transport.c`** — in `http_send_envelope()`, call `tus_upload_attachment_refs(state, envelope)` before `sentry__prepare_http_request()`.

### 11. Crash dump: persist large attachments ✅

**`src/sentry_database.c`** — `sentry__run_write_envelope()` iterates envelope items, calls `write_large_attachment()` for each attachment_ref:
- **Inline buffers**: writes payload to `<db>/attachments/<event-uuid>/<filename>`
- **External files**: copies to `<db>/attachments/<event-uuid>/<filename>` (user files never renamed)
- **Run-owned files**: renamed to `<db>/attachments/<event-uuid>/<filename>` (SDK-owned temp files, survives run dir cleanup)
- Updates item via `sentry__envelope_item_set_attachment_ref(item, dst)` and clears `inline` header

### 12. Restart: restore large attachments ✅

**`src/sentry_database.c`**:
- Fix `write_large_attachment()`: run-owned files are renamed to `<db>/attachments/` (not skipped), so they survive run dir cleanup
- In `sentry__process_old_runs()`, when processing `.envelope` files:
  - Derive event-uuid from envelope filename (first 36 chars)
  - If `<db>/attachments/<event-uuid>/` exists: materialize envelope so `tus_upload_attachment_refs()` can iterate items
  - If no attachments dir: load as raw (unchanged)
  - All attachment_ref paths already point to `<db>/attachments/` — no rewriting needed

### 13. Unit tests ✅

**`tests/unit/test_envelopes.c`**:
- `tus_upload_url` — verify URL format ✅
- `tus_request_preparation` — verify headers and body_path ✅
- `attachment_ref_creation` — verify large file creates attachment placeholder item with path payload ✅
- `attachment_ref_copy` — verify write copies external file and updates payload ✅
- `attachment_ref_move` — verify SDK-owned files in run dir are renamed to db/attachments/ ✅
- `attachment_ref_restore` — verify restart materializes envelope and attachment_ref items are accessible ✅

Register in `tests/unit/tests.inc`. ✅

### 14. Integration test

**`tests/test_integration_tus.py`** (new):
- Mock `/api/123456/upload/` → returns 201 + Location header ✅
- Mock `/api/123456/envelope/` → accepts envelope ✅
- Example app creates 100+ MB temp file, attaches it ✅
- Verify: upload endpoint received correct TUS headers + raw body ✅
- Verify: envelope contains attachment placeholder item with `location`, not inline data ✅
- Test fallback: upload returns 404 → attachment dropped, envelope still sent ✅
- Test small attachment: no TUS upload, inline as usual ✅
- Test crash → restart → TUS upload (step 12)

### 15. Test helper update ✅

**`tests/__init__.py`** — parse attachment-ref content type payloads as JSON in `Item.deserialize_from`.

### 16. Example app ✅

**`examples/example.c`** — add `"large-attachment"` arg that creates a 100+ MB temp file and attaches it.

## File Summary

| File | Change |
|------|--------|
| `src/transports/sentry_http_transport.h` | `body_path`/`body_path_len` in request, `location` in response |
| `src/transports/sentry_http_transport.c` | TUS request prep, upload logic, envelope rewriting, send path |
| `src/transports/sentry_http_transport_curl.c` | File read callback, streaming upload, Location capture |
| `src/transports/sentry_http_transport_winhttp.c` | WinHttpWriteData streaming, Location capture |
| `src/sentry_envelope.h` | Item accessor declarations, `sentry__envelope_add_attachment_ref`, `sentry__envelope_item_set_payload` |
| `src/sentry_envelope.c` | Item accessors, attachment placeholder creation, large attachment threshold |
| `src/sentry_utils.h` | `sentry__dsn_get_upload_url()` decl |
| `src/sentry_utils.c` | `sentry__dsn_get_upload_url()` impl |
| `src/sentry_database.c` | Extract large attachments at dump, restore at restart |
| `tests/unit/test_envelopes.c` | Unit tests |
| `tests/unit/tests.inc` | Test registration |
| `tests/test_integration_tus.py` | Integration tests (new) |
| `tests/__init__.py` | `attachment_ref` in Item.deserialize_from |
| `examples/example.c` | `large-attachment` arg |

## Relay Protocol Details (from PR #5638)

- **No compression**: Relay strictly validates `Content-Type: application/offset+octet-stream`, no `Content-Encoding` support. TUS uploads are raw bytes.
- **Required headers**: `Tus-Resumable: 1.0.0`, `Upload-Length: <size>`, `Content-Type: application/offset+octet-stream`, `X-Sentry-Auth: Sentry sentry_key=<key>`
- **Success response**: 201 + `Location: /api/{project_id}/upload/{uuid_hex}/?length={N}&signature={S}` + `Upload-Offset: <size>` + `Tus-Resumable: 1.0.0`
- **UUID format**: simple hex, no hyphens (server generates `Uuid::now_v7().as_simple()`)
- **Feature flag**: `projects:relay-upload-endpoint` must be enabled server-side
- **Max upload**: 1 GiB default (`max_upload_size` in Relay config)
- **Body validation**: Relay validates body size matches `Upload-Length` exactly (rejects mismatches)

## Notes

- Copying large files in signal handler context (crash dump path) is slow but necessary. `mkdir()` and `rename()` are async-signal-safe. File copy (`read`/`write` loop) is also signal-safe but slow for large files. `sentry__run_write_envelope()` runs in signal handler context with inproc/breakpad backends.
- For SDK-owned temp files (e.g., breakpad minidumps in `<run_path>/`), rename to `<db>/attachments/` is acceptable and faster.
- Lazy materialization (`envelope_materialize()`) eliminates the need to differentiate between raw and structured envelopes. All accessors auto-parse on first use.
