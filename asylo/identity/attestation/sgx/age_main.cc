/*
 *
 * Copyright 2020 Asylo authors
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

#include <ios>
#include <iostream>
#include <memory>
#include <string>

#include <google/protobuf/repeated_field.h>
#include <google/protobuf/text_format.h>
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/time/clock.h"
#include "asylo/client.h"
#include "asylo/crypto/certificate.pb.h"
#include "asylo/crypto/keys.pb.h"
#include "asylo/enclave.pb.h"
#include "asylo/identity/attestation/sgx/internal/dcap_intel_architectural_enclave_interface.h"
#include "asylo/identity/attestation/sgx/internal/fake_pce.h"
#include "asylo/identity/attestation/sgx/internal/host_dcap_library_interface.h"
#include "asylo/identity/attestation/sgx/internal/intel_architectural_enclave_interface.h"
#include "asylo/identity/enclave_assertion_authority_config.pb.h"
#include "asylo/identity/enclave_assertion_authority_configs.h"
#include "asylo/identity/provisioning/sgx/internal/fake_sgx_pki.h"
#include "asylo/identity/provisioning/sgx/internal/platform_provisioning.h"
#include "asylo/identity/sgx/sgx_infrastructural_enclave_manager.h"
#include "asylo/platform/primitives/sgx/loader.pb.h"
#include "asylo/util/proto_parse_util.h"
#include "asylo/util/status.h"
#include "asylo/util/status_macros.h"
#include "asylo/util/statusor.h"

//
// Option A: Run the Assertion Generator Enclave.
//

// Required for Option A.
ABSL_FLAG(bool, start_age, false,
          "Whether to start the AGE's attestation server");

// Required for Option A.
ABSL_FLAG(std::string, age_server_address,
          "unix:/tmp/assertion_generator_enclave",
          "The address to use for the AGE's attestation server");

ABSL_FLAG(absl::Duration, server_lifetime, absl::InfiniteDuration(),
          "The amount of time to run the AGE's attestation server before "
          "exiting");

ABSL_FLAG(bool, use_fake_pce, false,
          "Whether to interact with a fake (non-enclave) PCE when certifying "
          "the AGE and/or printing SGX platform info. When used with "
          "--start_age, attestations will be rooted in the Asylo Fake PKI.");

// Optional for Option A.
//
// This flag is ignored if --use_fake_pce is used.
ABSL_FLAG(asylo::CertificateChain, issuer_certificate_chain, {},
          "An X.509 issuer certificate chain for the PCK certificate provided "
          "to AGE");

// Optional for Option A and Option B.
ABSL_FLAG(std::string, intel_enclaves_path, "",
          "Path to the folder containing the libsgx_pce.signed.so binary");

// Required for Option A.
ABSL_FLAG(std::string, age_path, "", "Path to the AGE binary");

ABSL_FLAG(bool, is_debuggable_enclave, false,
          "Whether to run the AGE in debug mode");

//
// Option B: Print SGX platform info (encrypted PPID, CPU SVN, PCE SVN, and PCE
// ID) for provisioning the platform.
//

// Required for Option B.
ABSL_FLAG(bool, print_sgx_platform_info, false,
          "Whether to print SGX platform info (Encrypted PPID, CPU SVN, "
          "PCE SVN, PCE ID to STDOUT");

// Required for Option B.
ABSL_FLAG(
    asylo::AsymmetricEncryptionKeyProto, ppidek,
    asylo::ParseTextProto<asylo::AsymmetricEncryptionKeyProto>(
        R"pb(
          key_type: ENCRYPTION_KEY
          encoding: ASYMMETRIC_KEY_PEM
          encryption_scheme: RSA3072_OAEP
          key: "-----BEGIN PUBLIC KEY-----\n"
               "MIIBojANBgkqhkiG9w0BAQEFAAOCAY8AMIIBigKCAYEA05b5Q0MRABxpRJw7/e6P\n"
               "OM2Vza10CXyH8adlAkyHwVcwpcmmpMz5HWIYHgCmdCdYWcobHfUxDvLV4Xk3OZQ9\n"
               "PeJQkxLWA+UZOkjwrgw37uBXJ73sFxsPOYYGVCB0hDS+NPpxb6H1TJpSD8S8LXou\n"
               "F+Ndog7KOQeYqQUaNPuPYJw6HiYwC/PzSUDZ913L0b9XjeUtzphXNfGTwxkugFU3\n"
               "q41kCNrm3WS0YoONQ6rSe8Jjqpfe7QmS1ohWhs0IIwMnmnh89DYS9bHmHVSriGn/\n"
               "GE/ch+40pmixgWe2zgpwFLyz4Y12HHPeAKtBykBRU2MEw2MLymLaqpzlAbfAD34L\n"
               "sL7p+A2ztmT9zZUXnFeO7MSsizYBXkxtHiFJoB3eBDlrNGhE6gZ24I0fosAmBcyR\n"
               "vqMXyHVGhRA5FlCOAkOYMXBp2DRxgudIJs3BgtPrb+lY5wZ3EB/fSXYwp2hCsBbX\n"
               "2pJ11X8udUOsg7Afw5AZzqqU0C5abBNy56a1wEWB41MnAgMBAAE=\n"
               "-----END PUBLIC KEY-----\n"
        )pb")
        .ValueOrDie(),
    "The RSA-3072 PPID encryption key");

namespace asylo {

template <class T>
bool AbslParseFlag(absl::string_view text, T *flag, std::string *error) {
  if (!google::protobuf::TextFormat::ParseFromString(
          std::string(text.data(), text.size()), flag)) {
    *error =
        absl::StrCat("Failed to parse ", flag->GetDescriptor()->full_name());
    return false;
  }
  return true;
}

template <class T>
std::string AbslUnparseFlag(const T &flag) {
  std::string serialized_flag;
  CHECK(google::protobuf::TextFormat::PrintToString(flag, &serialized_flag));
  return serialized_flag;
}

}  // namespace asylo

namespace {

asylo::StatusOr<asylo::EnclaveClient *> GetAgeClient(
    std::string enclave_path, bool is_debuggable_enclave,
    std::string server_address) {
  asylo::EnclaveAssertionAuthorityConfig sgx_local_assertion_authority_config;
  ASYLO_ASSIGN_OR_RETURN(sgx_local_assertion_authority_config,
                         asylo::CreateSgxLocalAssertionAuthorityConfig());

  asylo::EnclaveLoadConfig load_config =
      asylo::SgxInfrastructuralEnclaveManager::GetAgeEnclaveLoadConfig(
          std::move(enclave_path), is_debuggable_enclave,
          std::move(server_address),
          std::move(sgx_local_assertion_authority_config));

  return asylo::SgxInfrastructuralEnclaveManager::GetAgeEnclaveClient(
      load_config);
}

asylo::Status GetSgxPlatformInfo(
    asylo::AsymmetricEncryptionKeyProto ppidek,
    asylo::SgxInfrastructuralEnclaveManager *manager,
    std::string *encrypted_ppid, asylo::sgx::CpuSvn *cpu_svn,
    asylo::sgx::PceSvn *pce_svn, asylo::sgx::PceId *pce_id) {
  asylo::sgx::TargetInfoProto pce_target_info;
  ASYLO_RETURN_IF_ERROR(manager->PceGetTargetInfo(&pce_target_info, pce_svn));

  auto report_result =
      manager->AgeGeneratePceInfoSgxHardwareReport(pce_target_info, ppidek);
  asylo::sgx::ReportProto report = report_result.ValueOrDie();
  ASYLO_ASSIGN_OR_RETURN(*cpu_svn, asylo::sgx::CpuSvnFromReportProto(report));

  asylo::SignatureScheme pck_signature_scheme;
  return manager->PceGetInfo(report, ppidek, pce_svn, pce_id,
                             &pck_signature_scheme, encrypted_ppid);
}

asylo::Status PrintSgxPlatformInfo(
    asylo::AsymmetricEncryptionKeyProto ppidek,
    asylo::SgxInfrastructuralEnclaveManager *manager) {
  std::string encrypted_ppid;
  asylo::sgx::CpuSvn cpu_svn;
  asylo::sgx::PceSvn pce_svn;
  asylo::sgx::PceId pce_id;

  ASYLO_RETURN_IF_ERROR(GetSgxPlatformInfo(std::move(ppidek), manager,
                                           &encrypted_ppid, &cpu_svn, &pce_svn,
                                           &pce_id));

  std::cout << "Encrypted PPID: 0x" << absl::BytesToHexString(encrypted_ppid)
            << std::endl;
  std::cout << "CPU SVN: 0x" << absl::BytesToHexString(cpu_svn.value())
            << std::endl;
  std::cout << "PCE SVN: 0x" << std::hex << pce_svn.value() << std::endl;
  std::cout << "PCE ID: 0x" << std::hex << pce_id.value() << std::endl;

  return asylo::Status::OkStatus();
}

}  // namespace

int main(int argc, char **argv) {
  absl::ParseCommandLine(argc, argv);

  std::string age_path = absl::GetFlag(FLAGS_age_path);
  LOG_IF(QFATAL, age_path.empty()) << "--age_path cannot be empty";

  asylo::StatusOr<asylo::EnclaveClient *> client_result =
      GetAgeClient(age_path, absl::GetFlag(FLAGS_is_debuggable_enclave),
                   absl::GetFlag(FLAGS_age_server_address));
  LOG_IF(QFATAL, !client_result.ok())
      << "Failed to load AGE: " << client_result.status();

  std::unique_ptr<asylo::sgx::IntelArchitecturalEnclaveInterface>
      intel_enclaves;

  bool use_fake_pce = absl::GetFlag(FLAGS_use_fake_pce);

  if (use_fake_pce) {
    LOG(WARNING) << "Using fake PCE and Asylo Fake SGX PKI";

    auto result = asylo::sgx::FakePce::CreateFromFakePki();
    LOG_IF(QFATAL, !result.ok())
        << "Failed to create FakePce: " << result.status();
    intel_enclaves = std::move(result).ValueOrDie();
  } else {
    intel_enclaves =
        absl::make_unique<asylo::sgx::DcapIntelArchitecturalEnclaveInterface>(
            absl::make_unique<asylo::sgx::HostDcapLibraryInterface>());
  }

  std::string intel_enclaves_path = absl::GetFlag(FLAGS_intel_enclaves_path);
  if (!intel_enclaves_path.empty()) {
    asylo::Status status = intel_enclaves->SetEnclaveDir(intel_enclaves_path);
    LOG_IF(QFATAL, !status.ok())
        << "Failed to set up Intel enclave dir: " << status;
  }

  std::unique_ptr<asylo::SgxInfrastructuralEnclaveManager>
      sgx_infra_enclave_manager =
          absl::make_unique<asylo::SgxInfrastructuralEnclaveManager>(
              std::move(intel_enclaves), client_result.ValueOrDie());

  if (absl::GetFlag(FLAGS_start_age) ==
      absl::GetFlag(FLAGS_print_sgx_platform_info)) {
    LOG(QFATAL)
        << "Must choose either --start_age or --print_platform_identifiers";
  }

  if (absl::GetFlag(FLAGS_start_age)) {
    asylo::CertificateChain age_certificate_chain;

    auto attestation_key_certificate_result =
        sgx_infra_enclave_manager->CertifyAge();
    LOG_IF(QFATAL, !attestation_key_certificate_result.ok())
        << "Failed to certify AGE";

    *age_certificate_chain.add_certificates() =
        std::move(attestation_key_certificate_result).ValueOrDie();

    if (use_fake_pce) {
      asylo::sgx::AppendFakePckCertificateChain(&age_certificate_chain);
    } else if (!absl::GetFlag(FLAGS_issuer_certificate_chain)
                    .certificates()
                    .empty()) {
      asylo::CertificateChain certificate_chain =
          absl::GetFlag(FLAGS_issuer_certificate_chain);
      std::move(certificate_chain.certificates().begin(),
                certificate_chain.certificates().end(),
                google::protobuf::RepeatedFieldBackInserter(
                    age_certificate_chain.mutable_certificates()));
    }

    asylo::Status status =
        sgx_infra_enclave_manager->AgeUpdateCerts({age_certificate_chain})
            .status();
    LOG_IF(QFATAL, !status.ok()) << "Failed to update AGE certs: " << status;

    status = sgx_infra_enclave_manager->AgeStartServer();
    LOG_IF(QFATAL, !status.ok()) << "Failed to start AGE server: " << status;

    absl::SleepFor(absl::GetFlag(FLAGS_server_lifetime));
  }

  if (absl::GetFlag(FLAGS_print_sgx_platform_info)) {
    asylo::AsymmetricEncryptionKeyProto ppidek = absl::GetFlag(FLAGS_ppidek);
    LOG_IF(QFATAL, ppidek.key().empty())
        << "Must provide valid PPID encryption key with --ppidek";

    asylo::Status status = PrintSgxPlatformInfo(
        std::move(ppidek), sgx_infra_enclave_manager.get());
    LOG_IF(QFATAL, !status.ok())
        << "Failed to get SGX platform info: " << status;
  }

  return 0;
}
