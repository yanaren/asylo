/*
 *
 * Copyright 2019 Asylo authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include "asylo/platform/primitives/sgx/fork.h"

#include "asylo/enclave.pb.h"
#include "asylo/platform/core/status_serializer.h"
#include "asylo/platform/core/trusted_global_state.h"
#include "asylo/platform/posix/threading/thread_manager.h"
#include "asylo/platform/primitives/sgx/fork_internal.h"
#include "asylo/platform/primitives/sgx/fork.pb.h"
#include "asylo/platform/primitives/trusted_primitives.h"
#include "asylo/util/status.h"

namespace asylo {
namespace {

Status VerifyOutputArguments(char **output, size_t *output_len) {
  if (!output || !output_len) {
    Status status =
        Status(error::GoogleError::INVALID_ARGUMENT,
               "Invalid input parameter passed to trusted handlers");
    primitives::TrustedPrimitives::DebugPuts(status.ToString().c_str());
    return status;
  }
  return Status::OkStatus();
}

}  // namespace

int TakeSnapshot(char **output, size_t *output_len) {
  Status status = VerifyOutputArguments(output, output_len);
  if (!status.ok()) {
    return 1;
  }
  EnclaveOutput enclave_output;
  // Take snapshot should not change any enclave states. Call
  // UntrustedLocalAlloc directly to create the StatusSerializer.
  StatusSerializer<EnclaveOutput> status_serializer(
      &enclave_output, enclave_output.mutable_status(), output, output_len,
      &primitives::TrustedPrimitives::UntrustedLocalAlloc);

  asylo::StatusOr<const asylo::EnclaveConfig *> config_result =
      asylo::GetEnclaveConfig();

  if (!config_result.ok()) {
    return status_serializer.Serialize(config_result.status());
  }

  const asylo::EnclaveConfig *config = config_result.ValueOrDie();
  if (!config->has_enable_fork() || !config->enable_fork()) {
    status = Status(error::GoogleError::FAILED_PRECONDITION,
                    "Insecure fork not enabled");
    return status_serializer.Serialize(status);
  }

  SnapshotLayout snapshot_layout;
  status = TakeSnapshotForFork(&snapshot_layout);
  *enclave_output.MutableExtension(snapshot) = snapshot_layout;
  return status_serializer.Serialize(status);
}

int Restore(const char *snapshot_layout, size_t snapshot_layout_len,
            char **output, size_t *output_len) {
  Status status = VerifyOutputArguments(output, output_len);
  if (!status.ok()) {
    return 1;
  }

  StatusSerializer<StatusProto> status_serializer(
      output, output_len, &primitives::TrustedPrimitives::UntrustedLocalAlloc);

  asylo::StatusOr<const asylo::EnclaveConfig *> config_result =
      asylo::GetEnclaveConfig();

  if (!config_result.ok()) {
    return status_serializer.Serialize(config_result.status());
  }

  const asylo::EnclaveConfig *config = config_result.ValueOrDie();
  if (!config->has_enable_fork() || !config->enable_fork()) {
    status = Status(error::GoogleError::FAILED_PRECONDITION,
                    "Insecure fork not enabled");
    return status_serializer.Serialize(status);
  }

  // |snapshot_layout| contains a serialized SnapshotLayout. We pass it to
  // RestoreForFork() without deserializing it because this proto requires
  // heap-allocated memory. Since restoring for fork() requires use of
  // a separate heap, we must take care to invoke this protos's allocators and
  // deallocators using the same heap. Consequently, we wait to deserialize this
  // message until after switching heaps in RestoreForFork().
  status = RestoreForFork(snapshot_layout, snapshot_layout_len);

  if (!status.ok()) {
    // Finalize the enclave as this enclave shouldn't be entered again.
    ThreadManager *thread_manager = ThreadManager::GetInstance();
    thread_manager->Finalize();
  }

  return status_serializer.Serialize(status);
}

int TransferSecureSnapshotKey(const char *input, size_t input_len,
                              char **output, size_t *output_len) {
  Status status = VerifyOutputArguments(output, output_len);
  if (!status.ok()) {
    return 1;
  }

  StatusSerializer<StatusProto> status_serializer(
      output, output_len, &primitives::TrustedPrimitives::UntrustedLocalAlloc);

  asylo::ForkHandshakeConfig fork_handshake_config;
  if (!fork_handshake_config.ParseFromArray(input, input_len)) {
    status = Status(error::GoogleError::INVALID_ARGUMENT,
                    "Failed to parse HandshakeInput");
    return status_serializer.Serialize(status);
  }

  status = TransferSecureSnapshotKey(fork_handshake_config);
  return status_serializer.Serialize(status);
}

}  // namespace asylo
