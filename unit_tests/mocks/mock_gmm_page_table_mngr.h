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

#pragma once

#include "runtime/gmm_helper/page_table_mngr.h"
#include "gmock/gmock.h"

namespace OCLRT {
class MockGmmPageTableMngr : public GmmPageTableMngr {
  public:
    MockGmmPageTableMngr() = default;

    MockGmmPageTableMngr(GMM_DEVICE_CALLBACKS_INT *deviceCb, unsigned int translationTableFlags, GMM_TRANSLATIONTABLE_CALLBACKS *translationTableCb)
        : deviceCb(*deviceCb), translationTableFlags(translationTableFlags), translationTableCb(*translationTableCb){};

    MOCK_METHOD2(initContextAuxTableRegister, GMM_STATUS(HANDLE initialBBHandle, GMM_ENGINE_TYPE engineType));

    MOCK_METHOD2(initContextTRTableRegister, GMM_STATUS(HANDLE initialBBHandle, GMM_ENGINE_TYPE engineType));

    MOCK_METHOD1(updateAuxTable, GMM_STATUS(const GMM_DDI_UPDATEAUXTABLE *ddiUpdateAuxTable));

    GMM_DEVICE_CALLBACKS_INT deviceCb = {};
    GMM_TRANSLATIONTABLE_CALLBACKS translationTableCb = {};
    unsigned int translationTableFlags = 0;
};

} // namespace OCLRT
