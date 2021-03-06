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

#include "asylo/identity/attestation/sgx/sgx_intel_ecdsa_qe_remote_assertion_verifier.h"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "asylo/crypto/certificate.pb.h"
#include "asylo/crypto/ecdsa_p256_sha256_signing_key.h"
#include "asylo/crypto/keys.pb.h"
#include "asylo/crypto/sha256_hash.h"
#include "asylo/crypto/sha256_hash.pb.h"
#include "asylo/crypto/util/byte_container_util.h"
#include "asylo/crypto/util/byte_container_view.h"
#include "asylo/crypto/util/bytes.h"
#include "asylo/crypto/util/trivial_object_util.h"
#include "asylo/identity/additional_authenticated_data_generator.h"
#include "asylo/identity/attestation/enclave_assertion_verifier.h"
#include "asylo/identity/attestation/sgx/internal/intel_ecdsa_quote.h"
#include "asylo/identity/attestation/sgx/sgx_intel_ecdsa_qe_remote_assertion_authority_config.pb.h"
#include "asylo/identity/enclave_assertion_authority.h"
#include "asylo/identity/identity.pb.h"
#include "asylo/identity/platform/sgx/code_identity.pb.h"
#include "asylo/identity/platform/sgx/machine_configuration.pb.h"
#include "asylo/identity/sgx/code_identity_constants.h"
#include "asylo/identity/sgx/identity_key_management_structs.h"
#include "asylo/identity/sgx/intel_certs/intel_sgx_root_ca_cert.h"
#include "asylo/identity/sgx/secs_attributes.h"
#include "asylo/identity/sgx/sgx_identity_util.h"
#include "asylo/platform/common/static_map.h"
#include "asylo/test/util/memory_matchers.h"
#include "asylo/test/util/proto_matchers.h"
#include "asylo/test/util/status_matchers.h"
#include "asylo/util/error_codes.h"
#include "asylo/util/proto_parse_util.h"
#include "asylo/util/status.h"
#include "asylo/util/statusor.h"
#include "QuoteVerification/Src/AttestationLibrary/include/QuoteVerification/QuoteConstants.h"

namespace asylo {
namespace {

using ::testing::Eq;
using ::testing::HasSubstr;
using ::testing::IsTrue;
using ::testing::Test;

// clang-format off
constexpr char kValidAssertionDescriptionProto[] = R"pb(
    description: {
      identity_type: CODE_IDENTITY
      authority_type: "SGX Intel ECDSA QE"
    })pb";
// clang-format on

class SgxIntelEcdsaQeRemoteAssertionVerifierTest : public Test {
 protected:
  void SetUp() override {
    SgxIntelEcdsaQeRemoteAssertionAuthorityConfig config;
    *config.mutable_generator_info()
         ->mutable_pck_certificate_chain()
         ->add_certificates() = MakeIntelSgxRootCaCertificateProto();
    *config.mutable_verifier_info()->add_root_certificates() =
        MakeIntelSgxRootCaCertificateProto();
    ASSERT_TRUE(config.SerializeToString(&valid_config_));
  }

  sgx::IntelQeQuoteHeader GenerateValidQuoteHeader() const {
    // Pull the constants directly from the Intel spec instead of their
    // libraries so that we sanity check compliance with the written spec.
    constexpr int kVersion = 3;
    constexpr int kEcdsaP256 = 2;
    constexpr char kVendorId[] =
        "\x93\x9A\x72\x33\xF7\x9C\x4C\xA9\x94\x0A\x0D\xB3\x95\x7F\x06\x07";

    auto header = TrivialRandomObject<sgx::IntelQeQuoteHeader>();
    header.version = kVersion;
    header.algorithm = kEcdsaP256;
    header.qe_vendor_id.assign(kVendorId, sizeof(kVendorId) - 1);
    return header;
  }

  sgx::ReportBody GenerateValidQuoteBody(ByteContainerView user_data) const {
    auto body = TrivialRandomObject<sgx::ReportBody>();
    auto aad_generator =
        AdditionalAuthenticatedDataGenerator::CreateEkepAadGenerator();
    body.reportdata.data = aad_generator->Generate(user_data).ValueOrDie();
    return body;
  }

  void SignQuoteHeaderAndReport(sgx::IntelQeQuote *quote) const {
    auto signing_key = EcdsaP256Sha256SigningKey::Create().ValueOrDie();
    ByteContainerView data_to_sign(quote,
                                   sizeof(quote->header) + sizeof(quote->body));
    Signature signature;
    ASSERT_THAT(signing_key->Sign(data_to_sign, &signature), IsOk());
    ASSERT_THAT(signature.ecdsa_signature().r().size(), Eq(32));
    quote->signature.body_signature.replace(0, signature.ecdsa_signature().r());

    ASSERT_THAT(signature.ecdsa_signature().s().size(), Eq(32));
    quote->signature.body_signature.replace(32,
                                            signature.ecdsa_signature().s());

    EccP256CurvePoint key_bytes = signing_key->GetPublicKeyPoint().ValueOrDie();
    static_assert(sizeof(quote->signature.public_key) == sizeof(key_bytes),
                  "Key size mismatch");
    quote->signature.public_key.assign(&key_bytes, sizeof(key_bytes));
  }

  void SignQuotingEnclaveReport(sgx::IntelQeQuote *quote) const {
    Sha256Hash sha256;
    sha256.Update(quote->signature.public_key);
    sha256.Update(quote->qe_authn_data);

    std::vector<uint8_t> report_data;
    ASSERT_THAT(sha256.CumulativeHash(&report_data), IsOk());
    report_data.resize(sgx::kReportdataSize);
    quote->signature.qe_report.reportdata.data.assign(report_data);


    quote->cert_data.qe_cert_data_type =
        ::intel::sgx::qvl::constants::PCK_ID_PCK_CERT_CHAIN;
  }

  sgx::IntelQeQuote GenerateValidQuote(ByteContainerView user_data) const {
    sgx::IntelQeQuote quote;
    quote.header = GenerateValidQuoteHeader();
    quote.body = GenerateValidQuoteBody(user_data);
    SignQuoteHeaderAndReport(&quote);
    AppendTrivialObject(TrivialRandomObject<UnsafeBytes<123>>(),
                        &quote.qe_authn_data);
    SignQuotingEnclaveReport(&quote);

    return quote;
  }

  // Creates an assertion object that wraps the given quote, with the correct
  // description targeting the SGX Intel ECDSA QE assertion authority.
  Assertion CreateAssertion(sgx::IntelQeQuote quote) {
    Assertion assertion = ParseTextProtoOrDie(kValidAssertionDescriptionProto);

    std::vector<uint8_t> packed_quote = sgx::PackDcapQuote(quote);
    assertion.set_assertion(packed_quote.data(), packed_quote.size());
    return assertion;
  }

  std::string valid_config_;
};

TEST_F(SgxIntelEcdsaQeRemoteAssertionVerifierTest, VerifierFoundInStaticMap) {
  std::string authority_id;
  ASYLO_ASSERT_OK_AND_ASSIGN(
      authority_id,
      EnclaveAssertionAuthority::GenerateAuthorityId(
          CODE_IDENTITY, sgx::kSgxIntelEcdsaQeRemoteAssertionAuthority));

  ASSERT_NE(AssertionVerifierMap::GetValue(authority_id),
            AssertionVerifierMap::value_end());
}

TEST_F(SgxIntelEcdsaQeRemoteAssertionVerifierTest, IdentityType) {
  SgxIntelEcdsaQeRemoteAssertionVerifier verifier;
  EXPECT_EQ(verifier.IdentityType(), CODE_IDENTITY);
}

TEST_F(SgxIntelEcdsaQeRemoteAssertionVerifierTest, AuthorityType) {
  SgxIntelEcdsaQeRemoteAssertionVerifier verifier;
  EXPECT_EQ(verifier.AuthorityType(),
            sgx::kSgxIntelEcdsaQeRemoteAssertionAuthority);
}

TEST_F(SgxIntelEcdsaQeRemoteAssertionVerifierTest, InitializeSucceedsOnce) {
  SgxIntelEcdsaQeRemoteAssertionVerifier verifier;
  ASYLO_EXPECT_OK(verifier.Initialize(valid_config_));
  EXPECT_THAT(verifier.Initialize(valid_config_),
              StatusIs(error::GoogleError::FAILED_PRECONDITION));
}

TEST_F(SgxIntelEcdsaQeRemoteAssertionVerifierTest,
       IsInitializedReturnsFalsePriorToInitialize) {
  SgxIntelEcdsaQeRemoteAssertionVerifier verifier;
  EXPECT_FALSE(verifier.IsInitialized());
}

TEST_F(SgxIntelEcdsaQeRemoteAssertionVerifierTest,
       IsInitializedReturnsTrueAfterInitialize) {
  SgxIntelEcdsaQeRemoteAssertionVerifier verifier;
  ASYLO_EXPECT_OK(verifier.Initialize(valid_config_));
  EXPECT_TRUE(verifier.IsInitialized());
}

TEST_F(SgxIntelEcdsaQeRemoteAssertionVerifierTest,
       IsInitializedReturnsFalseWithoutVerifierInfo) {
  std::string serialized_config;
  SgxIntelEcdsaQeRemoteAssertionAuthorityConfig config;
  *config.mutable_generator_info()
       ->mutable_pck_certificate_chain()
       ->add_certificates() = MakeIntelSgxRootCaCertificateProto();
  ASSERT_TRUE(config.SerializeToString(&serialized_config));

  SgxIntelEcdsaQeRemoteAssertionVerifier verifier;
  ASYLO_EXPECT_OK(verifier.Initialize(serialized_config));
  EXPECT_FALSE(verifier.IsInitialized());
}

TEST_F(SgxIntelEcdsaQeRemoteAssertionVerifierTest,
       InitializeFailsWithUnparsableConfig) {
  SgxIntelEcdsaQeRemoteAssertionVerifier verifier;
  EXPECT_THAT(verifier.Initialize("!@#!@"),
              StatusIs(error::GoogleError::INVALID_ARGUMENT));
}

TEST_F(SgxIntelEcdsaQeRemoteAssertionVerifierTest,
       InitializeFailsWithInvalidConfig) {
  // There are separate tests for config validation, so there is no need to test
  // lots of permutations of bad configuration here.
  SgxIntelEcdsaQeRemoteAssertionAuthorityConfig config;
  std::string serialized_config;
  ASSERT_TRUE(config.SerializeToString(&serialized_config));

  SgxIntelEcdsaQeRemoteAssertionVerifier verifier;
  EXPECT_THAT(verifier.Initialize(serialized_config),
              StatusIs(error::GoogleError::INVALID_ARGUMENT));
}

TEST_F(SgxIntelEcdsaQeRemoteAssertionVerifierTest,
       CreateAssertionRequestFailsIfNotInitialized) {
  SgxIntelEcdsaQeRemoteAssertionVerifier verifier;

  AssertionRequest request;
  EXPECT_THAT(verifier.CreateAssertionRequest(&request),
              StatusIs(error::GoogleError::FAILED_PRECONDITION,
                       HasSubstr("CreateAssertionRequest")));
}

TEST_F(SgxIntelEcdsaQeRemoteAssertionVerifierTest,
       CreateAssertionRequestSuccess) {
  SgxIntelEcdsaQeRemoteAssertionVerifier verifier;
  ASYLO_ASSERT_OK(verifier.Initialize(valid_config_));

  AssertionRequest request;
  ASYLO_ASSERT_OK(verifier.CreateAssertionRequest(&request));

  const AssertionDescription &description = request.description();
  EXPECT_EQ(description.identity_type(), CODE_IDENTITY);
  EXPECT_EQ(description.authority_type(),
            sgx::kSgxIntelEcdsaQeRemoteAssertionAuthority);
}

TEST_F(SgxIntelEcdsaQeRemoteAssertionVerifierTest,
       CanVerifyFailsIfNotInitialized) {
  SgxIntelEcdsaQeRemoteAssertionVerifier verifier;

  AssertionOffer offer = ParseTextProtoOrDie(kValidAssertionDescriptionProto);
  EXPECT_THAT(verifier.CanVerify(offer),
              StatusIs(error::GoogleError::FAILED_PRECONDITION,
                       HasSubstr("CanVerify")));
}

TEST_F(SgxIntelEcdsaQeRemoteAssertionVerifierTest,
       CanVerifyFailsIfIncompatibleAssertionIdentityType) {
  SgxIntelEcdsaQeRemoteAssertionVerifier verifier;
  ASYLO_ASSERT_OK(verifier.Initialize(valid_config_));

  AssertionOffer offer = ParseTextProtoOrDie(R"pb(
    description: {
      identity_type: CODE_IDENTITY
      authority_type: "bad authority"
    })pb");
  EXPECT_THAT(verifier.CanVerify(offer),
              StatusIs(error::GoogleError::INVALID_ARGUMENT,
                       HasSubstr("Assertion description does not match")));
}

TEST_F(SgxIntelEcdsaQeRemoteAssertionVerifierTest,
       CanVerifyFailsIfIncompatibleAssertionAuthorityType) {
  SgxIntelEcdsaQeRemoteAssertionVerifier verifier;
  ASYLO_ASSERT_OK(verifier.Initialize(valid_config_));

  AssertionOffer offer = ParseTextProtoOrDie(R"pb(
    description: {
      identity_type: CERT_IDENTITY
      authority_type: "SGX Intel ECDSA QE"
    })pb");
  EXPECT_THAT(verifier.CanVerify(offer),
              StatusIs(error::GoogleError::INVALID_ARGUMENT,
                       HasSubstr("Assertion description does not match")));
}

TEST_F(SgxIntelEcdsaQeRemoteAssertionVerifierTest, CanVerifySuccess) {
  SgxIntelEcdsaQeRemoteAssertionVerifier verifier;
  ASYLO_ASSERT_OK(verifier.Initialize(valid_config_));
  AssertionOffer offer = ParseTextProtoOrDie(kValidAssertionDescriptionProto);
  EXPECT_THAT(verifier.CanVerify(offer), IsOkAndHolds(IsTrue()));
}

TEST_F(SgxIntelEcdsaQeRemoteAssertionVerifierTest,
       VerifyFailsIfNotInitialized) {
  SgxIntelEcdsaQeRemoteAssertionVerifier verifier;

  Assertion assertion = ParseTextProtoOrDie(kValidAssertionDescriptionProto);
  EnclaveIdentity identity;
  EXPECT_THAT(
      verifier.Verify("user data", assertion, &identity),
      StatusIs(error::GoogleError::FAILED_PRECONDITION, HasSubstr("Verify")));
}

TEST_F(SgxIntelEcdsaQeRemoteAssertionVerifierTest,
       VerifyFailsIfIncompatibleAssertionDescription) {
  SgxIntelEcdsaQeRemoteAssertionVerifier verifier;
  ASYLO_ASSERT_OK(verifier.Initialize(valid_config_));

  Assertion assertion = ParseTextProtoOrDie(R"pb(
    description: {
      identity_type: CERT_IDENTITY
      authority_type: "SGX Intel ECDSA QE"
    })pb");
  EnclaveIdentity identity;
  EXPECT_THAT(verifier.Verify("user data", assertion, &identity),
              StatusIs(error::GoogleError::INVALID_ARGUMENT,
                       HasSubstr("Assertion description does not match")));
}

TEST_F(SgxIntelEcdsaQeRemoteAssertionVerifierTest,
       VerifyFailsIfUnparseableAssertion) {
  SgxIntelEcdsaQeRemoteAssertionVerifier verifier;
  ASYLO_ASSERT_OK(verifier.Initialize(valid_config_));

  Assertion assertion = ParseTextProtoOrDie(kValidAssertionDescriptionProto);
  assertion.set_assertion("can't parse this");
  EnclaveIdentity identity;
  EXPECT_THAT(verifier.Verify("user data", assertion, &identity),
              StatusIs(error::GoogleError::INVALID_ARGUMENT));
}

TEST_F(SgxIntelEcdsaQeRemoteAssertionVerifierTest,
       VerifyFailsIfAssertionIsNotBoundToUserData) {
  SgxIntelEcdsaQeRemoteAssertionVerifier verifier;
  ASYLO_ASSERT_OK(verifier.Initialize(valid_config_));

  Assertion assertion = CreateAssertion(GenerateValidQuote("user data"));
  EnclaveIdentity identity;
  EXPECT_THAT(verifier.Verify("not the user data", assertion, &identity),
              StatusIs(error::GoogleError::INVALID_ARGUMENT,
                       HasSubstr("quote data does not match")));
}

TEST_F(SgxIntelEcdsaQeRemoteAssertionVerifierTest,
       VerifyFailsWithIncorrectAadGenerator) {
  auto wrong_generator =
      AdditionalAuthenticatedDataGenerator::CreateGetPceInfoAadGenerator();
  SgxIntelEcdsaQeRemoteAssertionVerifier verifier(std::move(wrong_generator));
  ASYLO_ASSERT_OK(verifier.Initialize(valid_config_));

  Assertion assertion =
      CreateAssertion(GenerateValidQuote("data to be quoted"));
  EnclaveIdentity identity;
  EXPECT_THAT(verifier.Verify("data to be quoted", assertion, &identity),
              StatusIs(error::GoogleError::INVALID_ARGUMENT,
                       HasSubstr("quote data does not match")));
}

TEST_F(SgxIntelEcdsaQeRemoteAssertionVerifierTest,
       VerifyFailsWithIncorrectQuoteVersion) {
  SgxIntelEcdsaQeRemoteAssertionVerifier verifier;
  ASYLO_ASSERT_OK(verifier.Initialize(valid_config_));

  sgx::IntelQeQuote quote = GenerateValidQuote("user data");
  quote.header.version ^= 1;

  Assertion assertion = CreateAssertion(quote);
  EnclaveIdentity identity;
  EXPECT_THAT(
      verifier.Verify("user data", assertion, &identity),
      StatusIs(error::GoogleError::INVALID_ARGUMENT, HasSubstr("version")));
}

TEST_F(SgxIntelEcdsaQeRemoteAssertionVerifierTest,
       VerifyFailsWithIncorrectQuoteAlgorithm) {
  SgxIntelEcdsaQeRemoteAssertionVerifier verifier;
  ASYLO_ASSERT_OK(verifier.Initialize(valid_config_));

  sgx::IntelQeQuote quote = GenerateValidQuote("user data");
  quote.header.algorithm ^= 1;

  Assertion assertion = CreateAssertion(quote);
  EnclaveIdentity identity;
  EXPECT_THAT(
      verifier.Verify("user data", assertion, &identity),
      StatusIs(error::GoogleError::INVALID_ARGUMENT, HasSubstr("algorithm")));
}

TEST_F(SgxIntelEcdsaQeRemoteAssertionVerifierTest,
       VerifyFailsWithIncorrectVendorId) {
  SgxIntelEcdsaQeRemoteAssertionVerifier verifier;
  ASYLO_ASSERT_OK(verifier.Initialize(valid_config_));

  sgx::IntelQeQuote quote = GenerateValidQuote("user data");
  quote.header.qe_vendor_id[0] ^= 1;

  Assertion assertion = CreateAssertion(quote);
  EnclaveIdentity identity;
  EXPECT_THAT(
      verifier.Verify("user data", assertion, &identity),
      StatusIs(error::GoogleError::INVALID_ARGUMENT, HasSubstr("vendor ID")));
}

TEST_F(SgxIntelEcdsaQeRemoteAssertionVerifierTest,
       VerifyFailsWithBadKeyQuoteSigningKey) {
  SgxIntelEcdsaQeRemoteAssertionVerifier verifier;
  ASYLO_ASSERT_OK(verifier.Initialize(valid_config_));

  sgx::IntelQeQuote quote = GenerateValidQuote("user data");
  quote.signature.public_key =
      "\xff\xff\xff\xff\xff\xff\xff\xff"
      "\xff\xff\xff\xff\xff\xff\xff\xff"
      "\xff\xff\xff\xff\xff\xff\xff\xff"
      "\xff\xff\xff\xff\xff\xff\xff\xff";

  Assertion assertion = CreateAssertion(quote);
  EnclaveIdentity identity;
  EXPECT_THAT(verifier.Verify("user data", assertion, &identity),
              StatusIs(error::GoogleError::INTERNAL,
                       HasSubstr("elliptic curve routines")));
}

TEST_F(SgxIntelEcdsaQeRemoteAssertionVerifierTest,
       VerifyFailsWithQuoteSignatureMismatch) {
  SgxIntelEcdsaQeRemoteAssertionVerifier verifier;
  ASYLO_ASSERT_OK(verifier.Initialize(valid_config_));

  sgx::IntelQeQuote quote = GenerateValidQuote("user data");
  quote.signature.body_signature[0] ^= 0xff;

  Assertion assertion = CreateAssertion(quote);
  EnclaveIdentity identity;
  EXPECT_THAT(
      verifier.Verify("user data", assertion, &identity),
      StatusIs(error::GoogleError::INTERNAL, HasSubstr("BAD_SIGNATURE")));
}

TEST_F(SgxIntelEcdsaQeRemoteAssertionVerifierTest, VerifySuccess) {
  SgxIntelEcdsaQeRemoteAssertionVerifier verifier;
  ASYLO_ASSERT_OK(verifier.Initialize(valid_config_));

  sgx::IntelQeQuote quote = GenerateValidQuote("important data");
  Assertion assertion = CreateAssertion(quote);
  EnclaveIdentity identity;
  EXPECT_THAT(verifier.Verify("important data", assertion, &identity), IsOk());

  SgxIdentity peer_identity;
  ASYLO_ASSERT_OK_AND_ASSIGN(peer_identity, ParseSgxIdentity(identity));

  EXPECT_EQ(quote.body.cpusvn.size(),
            peer_identity.machine_configuration().cpu_svn().value().size());
  EXPECT_THAT(peer_identity.machine_configuration().cpu_svn().value().data(),
              MemEq(quote.body.cpusvn.data(), quote.body.cpusvn.size()));

  sgx::SecsAttributeSet peer_attributes(
      peer_identity.code_identity().attributes());
  EXPECT_EQ(peer_attributes, quote.body.attributes);
  EXPECT_EQ(peer_identity.code_identity().miscselect(), quote.body.miscselect);

  EXPECT_EQ(quote.body.mrenclave.size(),
            peer_identity.code_identity().mrenclave().hash().size());
  EXPECT_THAT(peer_identity.code_identity().mrenclave().hash().data(),
              MemEq(quote.body.mrenclave.data(), quote.body.mrenclave.size()));

  EXPECT_EQ(quote.body.mrsigner.size(), peer_identity.code_identity()
                                            .signer_assigned_identity()
                                            .mrsigner()
                                            .hash()
                                            .size());
  EXPECT_THAT(peer_identity.code_identity()
                   .signer_assigned_identity()
                   .mrsigner()
                   .hash().data(),
              MemEq(quote.body.mrsigner.data(), quote.body.mrsigner.size()));

  EXPECT_EQ(
      peer_identity.code_identity().signer_assigned_identity().isvprodid(),
      quote.body.isvprodid);
  EXPECT_EQ(peer_identity.code_identity().signer_assigned_identity().isvsvn(),
            quote.body.isvsvn);
}

}  // namespace
}  // namespace asylo
