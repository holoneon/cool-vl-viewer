
## Changes

### Fix GCC 14 build issue

- Fixes a GCC 14 `array-bounds` false positive in `LLVertexBuffer::listMissingBits()`.
- Allows the viewer to build on modern Linux distributions, including Debian 13.

### Add optional UTF-8 login name support

- Adds support for optional UTF-8 `FirstName` / `LastName` input on the login screen.
- Keeps the existing ASCII/printable login-name validation as the default.
- When `AllowUTF8LoginNames` is enabled, the login first/last name fields accept Unicode characters while still rejecting whitespace and control characters.
- This is useful for OpenSim grids using `utf8mb4` account tables and international account names.


