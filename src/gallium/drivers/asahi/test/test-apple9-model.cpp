/* Copyright 2026 Asahi Linux Contributors
 * SPDX-License-Identifier: MIT
 */
#include "asahi/lib/agx_apple9_layout.h"
#include <gtest/gtest.h>

#ifndef NDEBUG
#error This test must exercise the runtime gate with assertions compiled out.
#endif

TEST(Apple9RuntimeModel, QualifiedModelsRemainAvailable)
{
   EXPECT_TRUE(agx_apple9_runtime_model_supported(16, 'G'));
   EXPECT_TRUE(agx_apple9_runtime_model_supported(17, 'P'));
}

TEST(Apple9RuntimeModel, G16SRemainsRejectedWithoutAssertions)
{
   for (unsigned variant : {'S', 'C', 'D', 'P'})
      EXPECT_FALSE(agx_apple9_runtime_model_supported(16, variant));
   for (unsigned variant : {'G', 'S', 'C', 'D'})
      EXPECT_FALSE(agx_apple9_runtime_model_supported(17, variant));
}

TEST(Apple9RuntimeModel, DoesNotQualifyOtherGenerations)
{
   EXPECT_FALSE(agx_apple9_runtime_model_supported(15, 'G'));
   EXPECT_FALSE(agx_apple9_runtime_model_supported(18, 'G'));
}
