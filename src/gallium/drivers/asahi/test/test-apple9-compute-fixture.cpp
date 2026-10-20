/* Copyright 2026 Asahi Linux Contributors
 * SPDX-License-Identifier: MIT
 */
#include "agx_apple9.h"
#include "asahi/genxml/agx_pack.h"
#include <gtest/gtest.h>
#include <cstring>

static agx_apple9_compute_profile
profile()
{
   agx_apple9_compute_profile p = {};
   p.abi = AGX_APPLE9_COMPUTE_ABI_DIRECT_BUFFERS;
   p.resource_binding_count = 1;
   p.resource_write_mask = 1;
   p.local_size[0] = 256;
   p.local_size[1] = p.local_size[2] = 1;
   return p;
}

static agx_apple9_compute_fixture_addresses
addresses()
{
   return {UINT64_C(0x10000000000), UINT64_C(0x10000010000),
           UINT64_C(0x10000020000), UINT64_C(0x10000020400),
           UINT64_C(0x1100010000), 42};
}

static uint32_t
u32(const uint8_t *p)
{
   uint32_t v;
   memcpy(&v, p, sizeof(v));
   return v;
}

static uint64_t
u64(const uint8_t *p)
{
   uint64_t v;
   memcpy(&v, p, sizeof(v));
   return v;
}

TEST(Apple9ComputeFixture, PublishesOneSourceBuiltDirectJob)
{
   agx_apple9_compute_fixture out = {};
   auto a = addresses();
   auto p = profile();
   ASSERT_TRUE(agx_apple9_build_compute_fixture(&out, &a, &p));
   EXPECT_EQ(u32(out.entries), 0x340u);
   EXPECT_EQ(u32(out.entries + 0x340), 0xc0u);
   EXPECT_EQ(u64(out.resources), a.resource + 0xc0);
   EXPECT_EQ(u64(out.resources + 8), a.output);
   EXPECT_EQ(u32(out.resources + 0xc0), 1u);
   EXPECT_EQ(u32(out.resources + 0xc4), 1u);
   EXPECT_EQ(u32(out.resources + 0xc8), 1u);
   EXPECT_EQ(u32(out.cdm + 0x10), 256u);
   EXPECT_EQ(u32(out.cdm + 0x14), 1u);
   EXPECT_EQ(u32(out.cdm + 0x18), 1u);
   EXPECT_EQ(u32(out.cdm + 0x1c), 256u);
   EXPECT_EQ(u32(out.cdm + 0x20), 1u);
   EXPECT_EQ(u32(out.cdm + 0x24), 1u);
   EXPECT_EQ(u32(out.cdm + 0x2c) >> 29, AGX_CDM_BLOCK_TYPE_STREAM_TERMINATE);
   EXPECT_EQ(u32(out.cdm + 0x30), 0u);
}

TEST(Apple9ComputeFixture, RejectsUnsupportedProfileWithoutPublishing)
{
   auto a = addresses();
   for (unsigned test = 0; test < 7; ++test) {
      auto p = profile();
      if (test == 0) p.preamble_size = 16;
      if (test == 1) p.scratch_size = 16;
      if (test == 2) p.resource_binding_count = 2;
      if (test == 3) p.resource_kind[0] = AGX_APPLE9_COMPUTE_RESOURCE_UBO;
      if (test == 4) p.local_size[0] = 128;
      if (test == 5) p.required_threadgroup_memory_bytes = 16;
      if (test == 6) p.resource_write_mask = 0;
      agx_apple9_compute_fixture out, original;
      memset(&out, 0xa5, sizeof(out));
      original = out;
      EXPECT_FALSE(agx_apple9_build_compute_fixture(&out, &a, &p));
      EXPECT_EQ(memcmp(&out, &original, sizeof(out)), 0);
   }
}

TEST(Apple9ComputeFixture, RejectsWrongApertureOrAlignment)
{
   auto p = profile();
   for (unsigned test = 0; test < 20; ++test) {
      auto a = addresses();
      if (test == 0) a.usc_base = 0;
      if (test == 1) a.shader_body++;
      if (test == 2) a.resource++;
      if (test == 3) a.launch++;
      if (test == 4) a.output++;
      if (test == 5) a.shader_body = a.usc_base;
      if (test == 6) a.shader_body = a.usc_base + UINT64_C(0x100000000);
      if (test == 7) a.output = 0;
      if (test == 8) a.resource = 0;
      if (test == 9) a.launch = a.usc_base + UINT64_C(0x100000000);
      if (test == 10) a.launch = a.resource;
      if (test == 11) a.output = UINT64_MAX & ~3ull;
      if (test == 12) a.output = a.usc_base;
      if (test == 13) a.launch = a.resource + 64;
      if (test == 14) a.shader_body = a.resource + 64;
      if (test == 15) a.shader_body = a.launch + 64;
      if (test == 16) a.shader_size = 0;
      if (test == 17) { a.shader_body = a.resource - 64; a.shader_size = 128; }
      if (test == 18) { a.shader_body = a.launch - 64; a.shader_size = 128; }
      if (test == 19) { a.shader_body = a.usc_base + UINT64_C(0x100000000) - 64; a.shader_size = 128; }
      agx_apple9_compute_fixture out;
      EXPECT_FALSE(agx_apple9_build_compute_fixture(&out, &a, &p));
   }
}
