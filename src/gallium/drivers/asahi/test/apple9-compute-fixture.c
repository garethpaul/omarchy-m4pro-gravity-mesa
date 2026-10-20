/* Copyright 2026 Asahi Linux Contributors
 * SPDX-License-Identifier: MIT
 * Offline first-job fixture: no render node, ioctl, firmware or trace replay.
 */
#include "agx_apple9.h"
#include "asahi/compiler/agx_compile_apple9.h"
#include "compiler/nir/nir_builder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool
write_part(const char *prefix, const char *suffix, const void *data, size_t size)
{
   char path[4096];
   int length = snprintf(path, sizeof(path), "%s.%s", prefix, suffix);
   if (length < 0 || length >= sizeof(path))
      return false;
   FILE *file = fopen(path, "wb");
   if (!file)
      return false;
   bool ok = fwrite(data, size, 1, file) == 1;
   return fclose(file) == 0 && ok;
}

int
main(int argc, char **argv)
{
   if (argc != 3 || strcmp(argv[1], "--offline-template")) {
      fprintf(stderr, "usage: %s --offline-template OUTPUT_PREFIX\n", argv[0]);
      return EXIT_FAILURE;
   }
   nir_builder b = nir_builder_init_simple_shader(
      MESA_SHADER_COMPUTE, &agx_nir_options, "apple9_first_compute_constant42");
   b.shader->info.workgroup_size[0] = 256;
   b.shader->info.workgroup_size[1] = b.shader->info.workgroup_size[2] = 1;
   b.shader->info.num_ssbos = 1;
   nir_def *gid = nir_channel(&b, nir_load_global_invocation_id(&b, 32), 0);
   nir_store_ssbo(&b, nir_imm_int(&b, 42), nir_imm_int(&b, 0),
                  nir_imul_imm(&b, gid, 4), .align_mul = 4);
   struct agx_shader_part shader = {0};
   struct agx_apple9_compute_profile profile = {0};
   const char *reason = NULL;
   if (!agx_compile_apple9_tiny(b.shader, &shader, &profile, &reason)) {
      fprintf(stderr, "compile failed: %s\n", reason);
      ralloc_free(b.shader);
      return EXIT_FAILURE;
   }
   const struct agx_apple9_compute_fixture_addresses addresses = {
      .usc_base = UINT64_C(0x10000000000),
      .shader_body = UINT64_C(0x10000010000),
      .resource = UINT64_C(0x10000020000),
      .launch = UINT64_C(0x10000020400),
      .output = UINT64_C(0x1100010000),
      .shader_size = shader.info.binary_size,
   };
   struct agx_apple9_compute_fixture fixture;
   bool ok = agx_apple9_build_compute_fixture(&fixture, &addresses, &profile);
   uint32_t expected[256];
   for (unsigned i = 0; i < 256; ++i)
      expected[i] = 42;
   ok = ok && write_part(argv[2], "shader.bin", shader.binary, shader.info.binary_size) &&
        write_part(argv[2], "entries.bin", fixture.entries, sizeof(fixture.entries)) &&
        write_part(argv[2], "resources.bin", fixture.resources, sizeof(fixture.resources)) &&
        write_part(argv[2], "launch.bin", fixture.launch, sizeof(fixture.launch)) &&
        write_part(argv[2], "cdm.bin", fixture.cdm, sizeof(fixture.cdm)) &&
        write_part(argv[2], "expected.bin", expected, sizeof(expected));
   char json[4096];
   int length = snprintf(json, sizeof(json),
      "{\n  \"schema\":1,\n  \"offline_template\":true,\n"
      "  \"hardware_execution\":\"UNTESTED\",\n"
      "  \"virtual_addresses_are_allocated\":false,\n"
      "  \"shader_binary_size\":%u,\n"
      "  \"profile\":{\"abi\":%u,\"resource_binding_count\":%u,"
      "\"resource_binding\":[%u],\"resource_kind\":[%u],"
      "\"resource_read_mask\":%u,\"resource_write_mask\":%u,"
      "\"writes_global\":%s,\"variable_local_size\":%s,\"local_size\":[%u,%u,%u],"
      "\"required_threadgroup_memory_bytes\":%u,\"scratch_size\":%u,"
      "\"preamble_offset\":%u,\"preamble_size\":%u,"
      "\"atomic_frame_size\":%u,\"publication_count\":%u},\n"
      "  \"addresses\":{\"usc_base\":\"0x10000000000\","
      "\"shader_body\":\"0x10000010000\",\"resource\":\"0x10000020000\","
      "\"launch\":\"0x10000020400\",\"output\":\"0x1100010000\","
      "\"cdm\":\"0x1100000000\"},\n"
      "  \"cmd_compute\":{\"cdm_ctrl_stream_base\":\"0x1100000000\","
      "\"cdm_ctrl_stream_end\":\"0x1100000030\",\"flags\":0,\"sampler_count\":0,"
      "\"sampler_heap\":0,\"helper_binary\":0,\"helper_cfg\":0,\"helper_data\":0},\n"
      "  \"expected_output\":{\"u32_count\":256,\"value\":42},\n"
      "  \"warning\":\"No G16S runtime validation. Relocate/map every buffer and qualify kernel firmware/job path before submission.\"\n}\n",
      shader.info.binary_size, profile.abi, profile.resource_binding_count,
      profile.resource_binding[0], profile.resource_kind[0],
      profile.resource_read_mask, profile.resource_write_mask,
      profile.writes_global ? "true" : "false", profile.variable_local_size ? "true" : "false",
      profile.local_size[0], profile.local_size[1], profile.local_size[2],
      profile.required_threadgroup_memory_bytes, profile.scratch_size,
      profile.preamble_offset, profile.preamble_size, profile.atomic_frame_size,
      profile.publication_count);
   ok = ok && length > 0 && length < sizeof(json) && write_part(argv[2], "json", json, length);
   free(shader.binary);
   ralloc_free(b.shader);
   if (!ok)
      fprintf(stderr, "fixture generation failed; no hardware work was submitted\n");
   return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
