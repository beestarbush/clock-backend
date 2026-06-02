# communication

Shared C++/Qt communication library for app/backend protocol code.

## Goals

- Centralize JSON-RPC wire contract (methods/topics/frames).
- Keep app-style architecture and naming (`Services::<Domain>::Service`).
- Let backend and app consume the same protocol implementation.

## Modules

- `services/websocket/Types.h`
  - Shared enums and wire string conversion helpers.
- `services/websocket/Frame.*`
  - JSON-RPC frame builders and parsers.
- `services/websocket/Service.*`
  - Reusable Qt WebSocket client service with request correlation.
- `services/websocket/SubscriptionHelper.*`
  - Shared startup/bootstrap + topic subscription helper.
- `services/configuration/DeviceConfiguration.*`
  - Shared configuration schema mapping and serialization helpers.
- `services/protocol/MethodCatalog.*`
  - Canonical method/topic lists for completeness checks.

## Build

```bash
cmake -S . -B build
cmake --build build -j
```

## Notes

- This library is intentionally communication-only.
- Hardware access and backend runtime behavior stay outside this module.
- Next migration step is backend-first adoption, then app migration.
