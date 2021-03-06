/*
 * Copyright (c) 2014, Siemens AG. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <embb_mtapi_network_test_task.h>

#include <embb/mtapi/c/mtapi_ext.h>
#include <embb/mtapi/c/mtapi_network.h>
#include <embb/base/c/internal/unused.h>


#define MTAPI_CHECK_STATUS(status) PT_ASSERT(MTAPI_SUCCESS == status)

#define NETWORK_DOMAIN 1
#define NETWORK_LOCAL_NODE 3
#define NETWORK_LOCAL_JOB 3
#define NETWORK_REMOTE_NODE 3
#define NETWORK_REMOTE_JOB 4


static void test(
  void const * arguments,
  mtapi_size_t arguments_size,
  void * result_buffer,
  mtapi_size_t result_buffer_size,
  void const * node_local_data,
  mtapi_size_t node_local_data_size,
  mtapi_task_context_t * context) {
  EMBB_UNUSED(context);
  EMBB_UNUSED(result_buffer_size);
  EMBB_UNUSED(node_local_data_size);
  int elements = static_cast<int>(arguments_size / sizeof(float) / 2);
  float const * a = reinterpret_cast<float const *>(arguments);
  float const * b = reinterpret_cast<float const *>(arguments)+elements;
  float * c = reinterpret_cast<float*>(result_buffer);
  float const * d = reinterpret_cast<float const *>(node_local_data);
  for (int ii = 0; ii < elements; ii++) {
    c[ii] = a[ii] + b[ii] + d[0];
  }
}


NetworkTaskTest::NetworkTaskTest() {
  CreateUnit("mtapi network task test").Add(&NetworkTaskTest::TestBasic, this);
}

void NetworkTaskTest::TestBasic() {
  mtapi_status_t status;
  mtapi_job_hndl_t job;
  mtapi_task_hndl_t task;
  mtapi_action_hndl_t network_action, local_action;

  const int kElements = 64;
  float arguments[kElements * 2];
  float results[kElements];

  for (int ii = 0; ii < kElements; ii++) {
    arguments[ii] = static_cast<float>(ii);
    arguments[ii + kElements] = static_cast<float>(ii);
  }

  mtapi_initialize(
    NETWORK_DOMAIN,
    NETWORK_LOCAL_NODE,
    MTAPI_NULL,
    MTAPI_NULL,
    &status);
  MTAPI_CHECK_STATUS(status);

  mtapi_network_plugin_initialize("127.0.0.1", 12345, 5,
    kElements * 4 * 3 + 32, &status);
  MTAPI_CHECK_STATUS(status);

  float node_remote = 1.0f;
  local_action = mtapi_action_create(
    NETWORK_REMOTE_JOB,
    test,
    &node_remote, sizeof(float),
    MTAPI_DEFAULT_ACTION_ATTRIBUTES,
    &status);
  MTAPI_CHECK_STATUS(status);

  network_action = mtapi_network_action_create(
    NETWORK_DOMAIN,
    NETWORK_LOCAL_JOB,
    NETWORK_REMOTE_JOB,
    "127.0.0.1", 12345,
    &status);
  MTAPI_CHECK_STATUS(status);

  status = MTAPI_ERR_UNKNOWN;
  job = mtapi_job_get(NETWORK_LOCAL_JOB, NETWORK_DOMAIN, &status);
  MTAPI_CHECK_STATUS(status);

  task = mtapi_task_start(
    MTAPI_TASK_ID_NONE,
    job,
    arguments, kElements * 2 * sizeof(float),
    results, kElements*sizeof(float),
    MTAPI_DEFAULT_TASK_ATTRIBUTES,
    MTAPI_GROUP_NONE,
    &status);
  MTAPI_CHECK_STATUS(status);

  mtapi_task_wait(task, MTAPI_INFINITE, &status);
  MTAPI_CHECK_STATUS(status);

  for (int ii = 0; ii < kElements; ii++) {
    PT_EXPECT_EQ(results[ii], ii * 2 + 1);
  }

  mtapi_action_delete(network_action, MTAPI_INFINITE, &status);
  MTAPI_CHECK_STATUS(status);

  mtapi_action_delete(local_action, MTAPI_INFINITE, &status);
  MTAPI_CHECK_STATUS(status);

  mtapi_network_plugin_finalize(&status);
  MTAPI_CHECK_STATUS(status);

  mtapi_finalize(&status);
  MTAPI_CHECK_STATUS(status);
}
