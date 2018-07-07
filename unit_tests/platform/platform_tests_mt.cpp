/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "runtime/helpers/options.h"
#include "unit_tests/fixtures/platform_fixture.h"
#include "gtest/gtest.h"

using namespace OCLRT;

struct PlatformTestMt : public ::testing::Test {
    void SetUp() override { pPlatform.reset(new Platform); }

    static void initThreadFunc(Platform *pP) {
        pP->initialize();
    }

    cl_int retVal = CL_SUCCESS;
    std::unique_ptr<Platform> pPlatform;
};

static void callinitPlatform(Platform *plt, bool *ret) {
    *ret = plt->initialize();
}

TEST_F(PlatformTestMt, initialize) {
    std::thread threads[10];
    bool ret[10];
    for (int i = 0; i < 10; ++i) {
        threads[i] = std::thread(callinitPlatform, pPlatform.get(), &ret[i]);
    }

    for (auto &th : threads)
        th.join();

    for (int i = 0; i < 10; ++i)
        EXPECT_TRUE(ret[i]);

    EXPECT_TRUE(pPlatform->isInitialized());

    pPlatform.reset(new Platform());

    for (int i = 0; i < 10; ++i) {
        threads[i] = std::thread(callinitPlatform, pPlatform.get(), &ret[i]);
    }

    for (auto &th : threads)
        th.join();

    for (int i = 0; i < 10; ++i)
        EXPECT_TRUE(ret[i]);

}

TEST_F(PlatformTestMt, mtSafeTest) {
    size_t devNum = pPlatform->getNumDevices();
    EXPECT_EQ(0u, devNum);

    std::thread t1(PlatformTestMt::initThreadFunc, pPlatform.get());
    std::thread t2(PlatformTestMt::initThreadFunc, pPlatform.get());

    t1.join();
    t2.join();

    devNum = pPlatform->getNumDevices();
    EXPECT_EQ(numPlatformDevices, devNum);
}
