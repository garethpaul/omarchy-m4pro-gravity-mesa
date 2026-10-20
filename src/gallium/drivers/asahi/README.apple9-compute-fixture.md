# Offline Apple9 first-compute fixture

`agx_apple9_compute_fixture_tool --offline-template OUTPUT_PREFIX` compiles a
new NIR shader that writes 42 to each of 256 output words. It writes the shader,
entry image, resource record, launch program, direct CDM stream, reference output
and exact compiler-profile JSON. It opens no render node and issues no ioctl.
The virtual addresses in the JSON are unallocated template addresses.

Build the tool and `agx_apple9_compute_fixture_tests` with `build-tests=true` on
Linux. The normal Apple9 compiler, launcher and geometry tests remain applicable.
The tool and tests link only reachable pure encoder sections from the same
`agx_apple9.c` used by the runtime. The runtime device/model compatibility checks
remain in force. In particular, this does not add G16S runtime support.

The template builder calls the existing source encoders for the entry branch,
launch, resource geometry and CDM record; it includes no recorded shader or
firmware state. It supports one direct SSBO store profile with local size
256x1x1 and no shared memory, preamble, scratch, atomics or samplers. Unsupported
profiles fail without modifying the destination. The first entry occupies the
ordinary 0x340 header plus one 0xc0 block. The CDM file includes the eight-byte
terminator; the UAPI bound mirrors `agx_flush_compute` and includes its first
four bytes.

`agx_apple9_build_compute_fixture` takes explicit addresses and the complete
shader byte length so a caller can
rebuild the immutable records after allocating real VM mappings. A caller must
validate the complete mappings, shader length, permissions, ownership, fences,
CPU/GPU cache synchronization and output/page canaries before submission through
the ordinary unchanged Asahi UAPI. Building a template does not qualify any of
those runtime requirements. The 1-TiB USC placement follows this branch's G16
path and remains untested on G16S.

First hardware acceptance is a completed ordinary compute fence, exactly 256
words equal to 42, unchanged surrounding canaries, no GPU faults or arbitrary
page writes, and a repeat after a fresh qualified boot. No such G16S result is
claimed by the tool or its host unit tests.
