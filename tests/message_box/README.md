# Native debug message box

`RE::DebugMessageBox(const char*)` queues a message with the `DEBUG` title and the game's localized OK button. A null string is treated as empty. It has no callback or custom-button API. Call it on the game thread after game data has loaded, for example from an SFSE task scheduled upon `kPostDataLoad`.

## Evidence

Investigated with Ghidra MCP against Steam Starfield **1.16.244.0**, image base `0x140000000`. Unpacked executable SHA-256: `a100fcf3982047e29ae79d5f04a4cb0011641e08292f3431ccb9ac81fc63d3ac`.

The `Debug` / `MessageBox` Papyrus binding in `0x142004940` registers callback `0x1420035D0`. That callback constructs a native message request and calls the one-button manager overload. This implementation recreates that native request and calls the manager directly; it never invokes the Papyrus callback or invents VM arguments.

| Purpose | Address Library ID | VA in 1.16.244 |
| --- | ---: | --- |
| Manager singleton pointer | 938019 | `0x1461DE720` |
| Native one-button Create | 114231 | `0x141ED62B0` |
| Native string assignment | 36345 | `0x1402D31D0` |
| Request destructor | 43998 | `0x1404DF060` |

The IDs were matched against `versionlib-1-16-244-0.bin`. Addresses above are research evidence only; implementation uses named `REL::ID` constants.

The request consists of a callback smart pointer at `0x00`, three 16-byte strings at `0x08`, `0x18`, and `0x28`, and a 32-bit warning category at `0x38`. Debug.MessageBox uses a null callback, `DEBUG`, the message body, an empty third string, and category 5. The meaning of the third string has not been established, so it remains unnamed. The request occupies an eight-byte-aligned `0x40` bytes.

String assignment accesses a 12-byte inline buffer / heap pointer at `0x00`, capacity at `0x0C`, and length at `0x0E`. Capacity includes the terminator; values above 12 select heap storage. Reflection function `0x1427C9950`, associated with `BSReflection::BSStringType<BSStringT<char, 1, DynamicMemoryManagementPol>>`, uses the same assignment and layout. The existing public `BSStringT` has a different layout, so this contribution keeps the verified representation private and uses native assignment and destruction.

Create at `0x141ED62B0` builds one localized OK button, moves the callback and three strings out of its input, and submits the expanded request to `0x141ED53A0`. Its third argument becomes the uniqueness flag at expanded-request offset `0x59`; Debug.MessageBox passes false. The move helper at `0x140392E00` resets source capacity to 1 and length to zero. The destructor at `0x1404DF060` frees only strings with capacity above 12 and releases any callback reference. These observations establish safe cleanup after the native call. The input request must not be copied.

## Offline tests

From the repository root:

```powershell
xmake f -P tests/message_box -m releasedbg -y
xmake -P tests/message_box message-box-tests
xmake run -P tests/message_box message-box-tests
```

The tests link the actual library implementation and `REL::Relocation`. Only the address database is substituted, binding the four IDs to independent native-function stand-ins. They check request offsets and contents, empty/null/UTF-8/long strings, the 11/12-character inline boundary, missing-manager behavior, and cleanup with and without ownership transfer. These tests validate request construction and cleanup calls, not execution of the real engine allocator or UI.

## Runtime validation status

The author tested the native implementation in **Starfield 1.16.244** on September 28, 2026, using an SFSE consumer built against this contribution. The startup message appeared with the expected text and OK button; the author supplied a screenshot and confirmed that clicking OK closed it normally. This consumer calls `RE::DebugMessageBox` directly after `kPostDataLoad` through an SFSE game-thread task.

The tested DLL's SHA-256 is `feedc7e96950a0949d8cd99a56b317f76903ed71e63f80159c1be1ca5c66bcac`. This establishes basic in-game display and dismissal for that build, not exhaustive memory/lifetime testing. Other executable versions have not been investigated; Address Library indirection alone does not establish ABI compatibility.
