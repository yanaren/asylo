#
# Copyright 2019 Asylo authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# This packages contains libraries used to implement SGX local and remote
# attestation.

load("//asylo/bazel:asylo.bzl", "cc_test_and_cc_enclave_test")
load("//asylo/bazel:copts.bzl", "ASYLO_DEFAULT_COPTS")
load(
    "//asylo/bazel:sgx_rules.bzl",
    "sgx_cc_unsigned_enclave",
    "sgx_debug_sign_enclave",
    "sgx_enclave_test",
)
load("@rules_proto//proto:defs.bzl", "proto_library")
load("@linux_sgx//:sgx_sdk.bzl", "sgx")
load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("@rules_cc//cc:defs.bzl", "cc_proto_library")

licenses(["notice"])

package(default_visibility = ["//asylo/identity/attestation/sgx:__subpackages__"])

proto_library(
    name = "remote_assertion_proto",
    srcs = ["remote_assertion.proto"],
    visibility = [
        "//asylo/identity/attestation/sgx:__subpackages__",
        "//asylo/identity/sgx:__subpackages__",
    ],
    deps = [
        "//asylo/crypto:algorithms_proto",
        "//asylo/crypto:certificate_proto",
        "//asylo/crypto:keys_proto",
        "//asylo/identity/platform/sgx:sgx_identity_proto",
    ],
)

cc_proto_library(
    name = "remote_assertion_cc_proto",
    visibility = [
        "//asylo/identity/attestation/sgx:__subpackages__",
        "//asylo/identity/sgx:__subpackages__",
    ],
    deps = [":remote_assertion_proto"],
)

proto_library(
    name = "remote_assertion_generator_enclave_proto",
    srcs = ["remote_assertion_generator_enclave.proto"],
    visibility = ["//visibility:private"],
    deps = [
        ":remote_assertion_proto",
        "//asylo:enclave_proto",
        "//asylo/crypto:certificate_proto",
        "//asylo/crypto:keys_proto",
        "//asylo/identity:identity_proto",
        "//asylo/identity/platform/sgx:sgx_identity_proto",
        "//asylo/identity/provisioning/sgx/internal:platform_provisioning_proto",
        "//asylo/identity/sealing:sealed_secret_proto",
    ],
)

cc_proto_library(
    name = "remote_assertion_generator_enclave_cc_proto",
    visibility = ["//asylo:implementation"],
    deps = ["remote_assertion_generator_enclave_proto"],
)

proto_library(
    name = "remote_assertion_generator_test_util_enclave_proto",
    srcs = ["remote_assertion_generator_test_util_enclave.proto"],
    deps = [
        ":remote_assertion_proto",
        "//asylo:enclave_proto",
        "//asylo/crypto:certificate_proto",
        "//asylo/crypto:keys_proto",
        "//asylo/identity/provisioning/sgx/internal:platform_provisioning_proto",
        "//asylo/identity/sealing:sealed_secret_proto",
    ],
)

cc_proto_library(
    name = "remote_assertion_generator_test_util_enclave_cc_proto",
    deps = [":remote_assertion_generator_test_util_enclave_proto"],
)

proto_library(
    name = "report_oracle_enclave_proto",
    srcs = ["report_oracle_enclave.proto"],
    deps = [
        "//asylo:enclave_proto",
        "//asylo/identity/provisioning/sgx/internal:platform_provisioning_proto",
    ],
)

cc_proto_library(
    name = "report_oracle_enclave_cc_proto",
    deps = ["report_oracle_enclave_proto"],
)

proto_library(
    name = "attestation_key_proto",
    srcs = ["attestation_key.proto"],
    visibility = ["//asylo:implementation"],
    deps = ["//asylo/crypto:keys_proto"],
)

cc_proto_library(
    name = "attestation_key_cc_proto",
    visibility = ["//asylo:implementation"],
    deps = [":attestation_key_proto"],
)

proto_library(
    name = "attestation_key_certificate_proto",
    srcs = ["attestation_key_certificate.proto"],
    visibility = ["//asylo:implementation"],
    deps = [
        "//asylo/crypto:keys_proto",
        "//asylo/identity/provisioning/sgx/internal:platform_provisioning_proto",
    ],
)

cc_proto_library(
    name = "attestation_key_certificate_cc_proto",
    visibility = ["//asylo:implementation"],
    deps = [":attestation_key_certificate_proto"],
)

# Defines an interface for interacting with Intel Architectural Enclaves.
cc_library(
    name = "intel_architectural_enclave_interface",
    hdrs = ["intel_architectural_enclave_interface.h"],
    copts = ASYLO_DEFAULT_COPTS,
    visibility = [
        "//asylo/identity/attestation/sgx:__pkg__",
        "//asylo/identity/sgx:__pkg__",
    ],
    deps = [
        "//asylo/crypto:algorithms_cc_proto",
        "//asylo/crypto/util:bytes",
        "//asylo/identity/sgx:hardware_types",
        "//asylo/util:status",
        "@com_google_absl//absl/types:span",
    ],
)

cc_library(
    name = "fake_pce",
    testonly = 1,
    srcs = ["fake_pce.cc"],
    hdrs = ["fake_pce.h"],
    copts = ASYLO_DEFAULT_COPTS,
    visibility = [
        "//asylo/identity/attestation/sgx:__pkg__",
        "//asylo/identity/sgx:__pkg__",
        "//asylo/test/grpc:__pkg__",
    ],
    deps = [
        ":intel_architectural_enclave_interface",
        "//asylo/crypto:algorithms_cc_proto",
        "//asylo/crypto:ecdsa_p256_sha256_signing_key",
        "//asylo/crypto:keys_cc_proto",
        "//asylo/crypto:rsa_oaep_encryption_key",
        "//asylo/crypto:signing_key",
        "//asylo/crypto/util:byte_container_util",
        "//asylo/crypto/util:bytes",
        "//asylo/crypto/util:trivial_object_util",
        "//asylo/identity/provisioning/sgx/internal:fake_sgx_pki",
        "//asylo/identity/sgx:hardware_types",
        "//asylo/identity/sgx:pce_util",
        "//asylo/identity/sgx:sgx_identity_util",
        "//asylo/identity/sgx:sgx_identity_util_internal",
        "//asylo/util:status",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:span",
    ],
)

cc_test(
    name = "fake_pce_test",
    srcs = ["fake_pce_test.cc"],
    deps = [
        ":fake_pce",
        "//asylo/crypto:algorithms_cc_proto",
        "//asylo/crypto:asymmetric_encryption_key",
        "//asylo/crypto:ecdsa_p256_sha256_signing_key",
        "//asylo/crypto:keys_cc_proto",
        "//asylo/crypto:rsa_oaep_encryption_key",
        "//asylo/crypto:signing_key",
        "//asylo/crypto/util:bytes",
        "//asylo/crypto/util:trivial_object_util",
        "//asylo/identity/sgx:hardware_types",
        "//asylo/identity/sgx:pce_util",
        "//asylo/test/util:memory_matchers",
        "//asylo/test/util:status_matchers",
        "//asylo/test/util:test_main",
        "//asylo/util:cleansing_types",
        "@com_google_googletest//:gtest",
    ],
)

# Implementation of IntelArchitecturalEnclaveInterface using Intel's SGX Data
# Center Attestation Primitives (DCAP) library.
cc_library(
    name = "dcap_intel_architectural_enclave_interface",
    srcs = ["dcap_intel_architectural_enclave_interface.cc"],
    hdrs = ["dcap_intel_architectural_enclave_interface.h"],
    copts = ASYLO_DEFAULT_COPTS,
    visibility = ["//asylo/identity/attestation/sgx:__pkg__"],
    deps = [
        ":dcap_library_interface",
        ":intel_architectural_enclave_interface",
        "//asylo/crypto:algorithms_cc_proto",
        "//asylo/crypto/util:bytes",
        "//asylo/identity/sgx:hardware_types",
        "//asylo/identity/sgx:pce_util",
        "//asylo/util:proto_enum_util",
        "//asylo/util:status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:span",
        "@linux_sgx//:public",
        "@sgx_dcap//:pce_types",
        "@sgx_dcap//:quote_wrapper_common",
    ],
)

cc_library(
    name = "dcap_intel_architectural_enclave_path_setter",
    testonly = 1,
    srcs = ["dcap_intel_architectural_enclave_path_setter.cc"],
    hdrs = ["dcap_intel_architectural_enclave_path_setter.h"],
    copts = ASYLO_DEFAULT_COPTS,
    visibility = ["//asylo/identity/attestation/sgx:__pkg__"],
    deps = [
        ":dcap_intel_architectural_enclave_interface",
        ":host_dcap_library_interface",
        "//asylo/util:error_codes",
        "//asylo/util:status",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "dcap_library_interface",
    hdrs = ["dcap_library_interface.h"],
    copts = ASYLO_DEFAULT_COPTS,
    visibility = ["//asylo:implementation"],
    deps = [
        "@linux_sgx//:public",
        "@sgx_dcap//:pce_types",
        "@sgx_dcap//:quote_wrapper_common",
    ],
)

cc_library(
    name = "host_dcap_library_interface",
    srcs = ["host_dcap_library_interface.cc"],
    hdrs = ["host_dcap_library_interface.h"],
    copts = ASYLO_DEFAULT_COPTS,
    visibility = ["//asylo:implementation"],
    deps = [
        ":dcap_library_interface",
        "@linux_sgx//:public",
        "@sgx_dcap//:pce_types",
        "@sgx_dcap//:pce_wrapper",
        "@sgx_dcap//:quote_wrapper",
        "@sgx_dcap//:quote_wrapper_common",
    ],
)

cc_library(
    name = "enclave_dcap_library_interface",
    srcs = ["enclave_dcap_library_interface.cc"],
    hdrs = ["enclave_dcap_library_interface.h"],
    copts = ASYLO_DEFAULT_COPTS,
    tags = [
        "manual",
    ],
    visibility = ["//asylo/identity/attestation/sgx:__pkg__"],
    deps = [
        ":dcap_library_interface",
        "//asylo/platform/primitives/sgx:trusted_sgx",
        "@sgx_dcap//:pce_types",
        "@sgx_dcap//:quote_wrapper_common",
    ],
)

cc_library(
    name = "attestation_key_certificate_impl",
    srcs = ["attestation_key_certificate_impl.cc"],
    hdrs = ["attestation_key_certificate_impl.h"],
    copts = ASYLO_DEFAULT_COPTS,
    visibility = ["//asylo:implementation"],
    deps = [
        ":attestation_key_cc_proto",
        ":attestation_key_certificate_cc_proto",
        ":remote_assertion_generator_constants",
        "//asylo/crypto:algorithms_cc_proto",
        "//asylo/crypto:certificate_cc_proto",
        "//asylo/crypto:certificate_interface",
        "//asylo/crypto:ecdsa_p256_sha256_signing_key",
        "//asylo/crypto:keys_cc_proto",
        "//asylo/crypto:signing_key",
        "//asylo/crypto/util:byte_container_view",
        "//asylo/crypto/util:bytes",
        "//asylo/crypto/util:trivial_object_util",
        "//asylo/identity:additional_authenticated_data_generator",
        "//asylo/identity/platform/sgx:sgx_identity_cc_proto",
        "//asylo/identity/provisioning/sgx/internal:platform_provisioning",
        "//asylo/identity/provisioning/sgx/internal:platform_provisioning_cc_proto",
        "//asylo/identity/sgx:hardware_types",
        "//asylo/identity/sgx:sgx_identity_util_internal",
        "//asylo/util:proto_enum_util",
        "//asylo/util:status",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/types:optional",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "attestation_key_certificate_impl_test",
    srcs = ["attestation_key_certificate_impl_test.cc"],
    deps = [
        ":attestation_key_cc_proto",
        ":attestation_key_certificate_cc_proto",
        ":attestation_key_certificate_impl",
        "//asylo/crypto:algorithms_cc_proto",
        "//asylo/crypto:certificate_cc_proto",
        "//asylo/crypto:certificate_interface",
        "//asylo/crypto:ecdsa_p256_sha256_signing_key",
        "//asylo/crypto:fake_certificate",
        "//asylo/crypto:keys_cc_proto",
        "//asylo/crypto:signing_key",
        "//asylo/identity:additional_authenticated_data_generator",
        "//asylo/identity/platform/sgx:sgx_identity_cc_proto",
        "//asylo/identity/provisioning/sgx/internal:platform_provisioning",
        "//asylo/identity/sgx:hardware_types",
        "//asylo/test/util:proto_matchers",
        "//asylo/test/util:status_matchers",
        "//asylo/test/util:test_main",
        "//asylo/util:status",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
        "@com_google_protobuf//:protobuf",
    ],
)

sgx_cc_unsigned_enclave(
    name = "report_oracle_enclave_unsigned.so",
    srcs = ["report_oracle_enclave.cc"],
    copts = ASYLO_DEFAULT_COPTS,
    deps = [
        ":report_oracle_enclave_cc_proto",
        "//asylo:enclave_cc_proto",
        "//asylo:enclave_runtime",
        "//asylo/identity/provisioning/sgx/internal:platform_provisioning",
        "//asylo/identity/sgx:hardware_interface",
        "//asylo/util:status",
    ],
)

sgx.enclave_configuration(
    name = "report_oracle_enclave_config",
    provision_key = "1",
)

sgx_debug_sign_enclave(
    name = "report_oracle_enclave.so",
    config = ":report_oracle_enclave_config",
    unsigned = "report_oracle_enclave_unsigned.so",
)

sgx_enclave_test(
    name = "dcap_intel_architectural_enclave_interface_e2e_test",
    srcs = ["dcap_intel_architectural_enclave_interface_e2e_test.cc"],
    enclaves = {
        "report_oracle_enclave_path": ":report_oracle_enclave.so",
    },
    tags = ["noregression"],
    test_args = ["--report_oracle_enclave_path='{report_oracle_enclave_path}'"],
    deps = [
        ":dcap_intel_architectural_enclave_interface",
        ":dcap_intel_architectural_enclave_path_setter",
        ":host_dcap_library_interface",
        ":intel_ecdsa_quote",
        ":report_oracle_enclave_cc_proto",
        "//asylo:enclave_cc_proto",
        "//asylo:enclave_client",
        "//asylo/crypto:algorithms_cc_proto",
        "//asylo/crypto:keys_cc_proto",
        "//asylo/crypto:rsa_oaep_encryption_key",
        "//asylo/crypto/util:trivial_object_util",
        "//asylo/identity/platform/sgx:architecture_bits",
        "//asylo/identity/provisioning/sgx/internal:platform_provisioning",
        "//asylo/identity/provisioning/sgx/internal:platform_provisioning_cc_proto",
        "//asylo/identity/sgx:hardware_types",
        "//asylo/identity/sgx:pce_util",
        "//asylo/platform/primitives/sgx:loader_cc_proto",
        "//asylo/test/util:memory_matchers",
        "//asylo/test/util:status_matchers",
        "//asylo/test/util:test_main",
        "//asylo/util:cleansing_types",
        "//asylo/util:status",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/memory",
        "@com_google_googletest//:gtest",
        "@sgx_dcap//:pce_constants",
        "@sgx_dcap//:quote_constants",
    ],
)

# Since the DCAP interface is never used in an enclave, this test is not a
# "cc_test_and_cc_enclave_test" target.
cc_test(
    name = "dcap_intel_architectural_enclave_interface_test",
    srcs = ["dcap_intel_architectural_enclave_interface_test.cc"],
    copts = ASYLO_DEFAULT_COPTS,
    deps = [
        ":dcap_intel_architectural_enclave_interface",
        ":dcap_library_interface",
        "//asylo/crypto:algorithms_cc_proto",
        "//asylo/crypto/util:byte_container_util",
        "//asylo/crypto/util:byte_container_view",
        "//asylo/crypto/util:bytes",
        "//asylo/crypto/util:trivial_object_util",
        "//asylo/identity/sgx:hardware_types",
        "//asylo/identity/sgx:pce_util",
        "//asylo/test/util:memory_matchers",
        "//asylo/test/util:status_matchers",
        "//asylo/test/util:test_main",
        "//asylo/util:status",
        "@com_google_absl//absl/memory",
        "@com_google_googletest//:gtest",
        "@sgx_dcap//:pce_constants",
        "@sgx_dcap//:pce_types",
        "@sgx_dcap//:quote_wrapper_common",
    ],
)

cc_library(
    name = "mock_intel_architectural_enclave_interface",
    testonly = 1,
    hdrs = ["mock_intel_architectural_enclave_interface.h"],
    copts = ASYLO_DEFAULT_COPTS,
    visibility = [
        "//asylo/identity/attestation/sgx:__pkg__",
        "//asylo/identity/sgx:__pkg__",
    ],
    deps = [
        ":intel_architectural_enclave_interface",
        "//asylo/crypto:algorithms_cc_proto",
        "//asylo/crypto/util:bytes",
        "//asylo/identity/sgx:hardware_types",
        "//asylo/util:status",
        "@com_google_absl//absl/types:span",
        "@com_google_googletest//:gtest",
    ],
)

cc_library(
    name = "intel_ecdsa_quote",
    srcs = ["intel_ecdsa_quote.cc"],
    hdrs = ["intel_ecdsa_quote.h"],
    copts = ASYLO_DEFAULT_COPTS,
    deps = [
        "//asylo/crypto/util:byte_container_reader",
        "//asylo/crypto/util:byte_container_util",
        "//asylo/crypto/util:byte_container_view",
        "//asylo/crypto/util:bytes",
        "//asylo/identity:descriptions",
        "//asylo/identity:identity_cc_proto",
        "//asylo/identity/sgx:hardware_types",
        "//asylo/util:status",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "intel_ecdsa_quote_test",
    srcs = ["intel_ecdsa_quote_test.cc"],
    copts = ASYLO_DEFAULT_COPTS,
    deps = [
        ":intel_ecdsa_quote",
        "//asylo/crypto/util:byte_container_util",
        "//asylo/crypto/util:trivial_object_util",
        "//asylo/identity:identity_cc_proto",
        "//asylo/identity/sgx:code_identity_constants",
        "//asylo/test/util:memory_matchers",
        "//asylo/test/util:status_matchers",
        "//asylo/test/util:test_main",
        "@com_google_googletest//:gtest",
    ],
)

cc_library(
    name = "remote_assertion_generator_constants",
    srcs = ["remote_assertion_generator_constants.cc"],
    hdrs = ["remote_assertion_generator_constants.h"],
    copts = ASYLO_DEFAULT_COPTS,
    visibility = [
        "//asylo/identity/attestation/sgx:__subpackages__",
        "//asylo/identity/sgx:__subpackages__",
    ],
)

cc_library(
    name = "remote_assertion_generator_enclave_util",
    srcs = ["remote_assertion_generator_enclave_util.cc"],
    hdrs = ["remote_assertion_generator_enclave_util.h"],
    copts = ASYLO_DEFAULT_COPTS,
    deps = [
        ":attestation_key_cc_proto",
        ":remote_assertion_generator_constants",
        ":remote_assertion_generator_enclave_cc_proto",
        "//asylo/crypto:algorithms_cc_proto",
        "//asylo/crypto:certificate_cc_proto",
        "//asylo/crypto:certificate_interface",
        "//asylo/crypto:certificate_util",
        "//asylo/crypto:ecdsa_p256_sha256_signing_key",
        "//asylo/crypto:keys_cc_proto",
        "//asylo/crypto:signing_key",
        "//asylo/crypto/util:byte_container_util",
        "//asylo/crypto/util:trivial_object_util",
        "//asylo/grpc/auth:grpc++_security_enclave",
        "//asylo/grpc/auth:sgx_local_credentials_options",
        "//asylo/identity:additional_authenticated_data_generator",
        "//asylo/identity/sealing:sealed_secret_cc_proto",
        "//asylo/identity/sealing/sgx:sgx_local_secret_sealer",
        "//asylo/identity/sgx:hardware_types",
        "//asylo/identity/sgx:sgx_remote_assertion_generator_impl",
        "//asylo/util:cleansing_types",
        "//asylo/util:proto_enum_util",
        "//asylo/util:status",
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "remote_assertion_generator_enclave_util_test",
    srcs = ["remote_assertion_generator_enclave_util_test.cc"],
    copts = ASYLO_DEFAULT_COPTS,
    deps = [
        ":attestation_key_cc_proto",
        ":remote_assertion_generator_constants",
        ":remote_assertion_generator_enclave_cc_proto",
        ":remote_assertion_generator_enclave_util",
        "//asylo/crypto:certificate_cc_proto",
        "//asylo/crypto:certificate_interface",
        "//asylo/crypto:certificate_util",
        "//asylo/crypto:ecdsa_p256_sha256_signing_key",
        "//asylo/crypto:fake_certificate",
        "//asylo/crypto:fake_signing_key",
        "//asylo/crypto:keys_cc_proto",
        "//asylo/crypto/util:trivial_object_util",
        "//asylo/identity/sealing:sealed_secret_cc_proto",
        "//asylo/identity/sealing/sgx:sgx_local_secret_sealer",
        "//asylo/test/util:proto_matchers",
        "//asylo/test/util:status_matchers",
        "//asylo/test/util:test_main",
        "//asylo/util:cleansing_types",
        "//asylo/util:proto_enum_util",
        "@com_google_googletest//:gtest",
        "@com_google_protobuf//:protobuf",
    ],
)

sgx_cc_unsigned_enclave(
    name = "remote_assertion_generator_test_util_enclave_unsigned.so",
    testonly = 1,
    srcs = ["remote_assertion_generator_test_util_enclave.cc"],
    copts = ASYLO_DEFAULT_COPTS,
    deps = [
        ":remote_assertion_cc_proto",
        ":remote_assertion_generator_enclave_util",
        ":remote_assertion_generator_test_util_enclave_cc_proto",
        "//asylo:enclave_cc_proto",
        "//asylo:enclave_runtime",
        "//asylo/crypto:ecdsa_p256_sha256_signing_key",
        "//asylo/grpc/auth:grpc++_security_enclave",
        "//asylo/grpc/auth:sgx_local_credentials_options",
        "//asylo/identity/provisioning/sgx/internal:platform_provisioning",
        "//asylo/identity/sgx:hardware_types",
        "//asylo/identity/sgx:sgx_identity_util_internal",
        "//asylo/identity/sgx:sgx_remote_assertion_generator_client",
        "//asylo/util:status",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc_secure",
        "@com_google_protobuf//:protobuf_lite",
    ],
)

sgx_debug_sign_enclave(
    name = "remote_assertion_generator_test_util_enclave.so",
    testonly = 1,
    config = "//asylo/grpc/util:grpc_enclave_config",
    unsigned = "remote_assertion_generator_test_util_enclave_unsigned.so",
)

# AGE configuration for tests running on non-FLC hardware.
sgx.enclave_configuration(
    name = "remote_assertion_generator_enclave_no_flc_config",
    base = "//asylo/grpc/util:grpc_enclave_config",
)

# AGE configuration for tests running on FLC hardware. Flexible Launch Control
# (FLC) provides access to the PROVISION_KEY.
sgx.enclave_configuration(
    name = "remote_assertion_generator_enclave_config",
    base = ":remote_assertion_generator_enclave_no_flc_config",
    provision_key = "1",
)

sgx_cc_unsigned_enclave(
    name = "remote_assertion_generator_enclave_unsigned.so",
    srcs = [
        "remote_assertion_generator_enclave.cc",
        "remote_assertion_generator_enclave.h",
    ],
    copts = ASYLO_DEFAULT_COPTS,
    deps = [
        ":certificate_util",
        ":remote_assertion_generator_enclave_cc_proto",
        ":remote_assertion_generator_enclave_util",
        "//asylo:enclave_cc_proto",
        "//asylo:enclave_runtime",
        "//asylo/crypto:certificate_cc_proto",
        "//asylo/crypto:ecdsa_p256_sha256_signing_key",
        "//asylo/crypto:keys_cc_proto",
        "//asylo/crypto:signing_key",
        "//asylo/crypto/util:trivial_object_util",
        "//asylo/identity/platform/sgx:code_identity_cc_proto",
        "//asylo/identity/provisioning/sgx/internal:platform_provisioning",
        "//asylo/identity/sgx:hardware_interface",
        "//asylo/identity/sgx:hardware_types",
        "//asylo/identity/sgx:pce_util",
        "//asylo/identity/sgx:sgx_identity_util_internal",
        "//asylo/identity/sgx:sgx_remote_assertion_generator_impl",
        "//asylo/util:logging",
        "//asylo/util:mutex_guarded",
        "//asylo/util:status",
        "@com_google_absl//absl/memory",
    ],
)

# The AGE without the PROVISION_KEY bit. Suitable for use in tests that do not
# utilize Flexible Launch Control (i.e., tests that do not test interactions
# between the AGE and the PCE).
sgx_debug_sign_enclave(
    name = "remote_assertion_generator_enclave_no_flc.so",
    config = ":remote_assertion_generator_enclave_no_flc_config",
    unsigned = "remote_assertion_generator_enclave_unsigned.so",
    visibility = ["//asylo:implementation"],
)

# The AGE with the PROVISION_KEY bit. Suitable for use in tests and deployments
# that utilize Flexible Launch Control (i.e., the PCE is used to certify the
# AGE).
sgx_debug_sign_enclave(
    name = "remote_assertion_generator_enclave.so",
    config = ":remote_assertion_generator_enclave_config",
    unsigned = "remote_assertion_generator_enclave_unsigned.so",
)

sgx_enclave_test(
    name = "remote_assertion_generator_enclave_test",
    srcs = ["remote_assertion_generator_enclave_test.cc"],
    copts = ASYLO_DEFAULT_COPTS,
    enclaves = {
        "assertion_generator_enclave": ":remote_assertion_generator_enclave_no_flc.so",
        "test_util_enclave": ":remote_assertion_generator_test_util_enclave.so",
    },
    test_args = [
        "--remote_assertion_generator_enclave_path='{assertion_generator_enclave}'",
        "--remote_assertion_generator_test_util_enclave_path='{test_util_enclave}'",
    ],
    deps = [
        ":attestation_key_cc_proto",
        ":fake_pce",
        ":remote_assertion_cc_proto",
        ":remote_assertion_generator_constants",
        ":remote_assertion_generator_enclave_cc_proto",
        ":remote_assertion_generator_enclave_util",
        ":remote_assertion_generator_test_util_enclave_cc_proto",
        "//asylo:enclave_cc_proto",
        "//asylo:enclave_client",
        "//asylo/crypto:algorithms_cc_proto",
        "//asylo/crypto:certificate_cc_proto",
        "//asylo/crypto:ecdsa_p256_sha256_signing_key",
        "//asylo/crypto:keys_cc_proto",
        "//asylo/identity/provisioning/sgx/internal:fake_sgx_pki",
        "//asylo/identity/provisioning/sgx/internal:platform_provisioning",
        "//asylo/identity/provisioning/sgx/internal:platform_provisioning_cc_proto",
        "//asylo/identity/sealing:sealed_secret_cc_proto",
        "//asylo/identity/sgx:pce_util",
        "//asylo/identity/sgx:sgx_identity_util_internal",
        "//asylo/identity/sgx:sgx_infrastructural_enclave_manager",
        "//asylo/platform/primitives/sgx:loader_cc_proto",
        "//asylo/test/util:enclave_assertion_authority_configs",
        "//asylo/test/util:status_matchers",
        "//asylo/test/util:test_main",
        "//asylo/util:status",
        "//asylo/util:status_macros",
        "//asylo/util:thread",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/synchronization",
        "@com_google_googletest//:gtest",
    ],
)

cc_library(
    name = "remote_assertion_util",
    srcs = ["remote_assertion_util.cc"],
    hdrs = ["remote_assertion_util.h"],
    copts = ASYLO_DEFAULT_COPTS,
    visibility = [
        "//asylo/identity/attestation/sgx:__subpackages__",
        "//asylo/identity/sgx:__subpackages__",
    ],
    deps = [
        ":attestation_key_certificate_impl",
        ":remote_assertion_cc_proto",
        "//asylo/crypto:algorithms_cc_proto",
        "//asylo/crypto:certificate_cc_proto",
        "//asylo/crypto:certificate_interface",
        "//asylo/crypto:certificate_util",
        "//asylo/crypto:ecdsa_p256_sha256_signing_key",
        "//asylo/crypto:keys_cc_proto",
        "//asylo/crypto:signing_key",
        "//asylo/crypto:x509_certificate",
        "//asylo/crypto/util:byte_container_util",
        "//asylo/identity:identity_acl_cc_proto",
        "//asylo/identity:identity_acl_evaluator",
        "//asylo/identity:identity_cc_proto",
        "//asylo/identity/platform/sgx:machine_configuration_cc_proto",
        "//asylo/identity/platform/sgx:sgx_identity_cc_proto",
        "//asylo/identity/sgx:pck_certificate_util",
        "//asylo/identity/sgx:sgx_identity_expectation_matcher",
        "//asylo/identity/sgx:sgx_identity_util",
        "//asylo/util:status",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_absl//absl/types:optional",
        "@com_google_absl//absl/types:span",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test_and_cc_enclave_test(
    name = "remote_assertion_util_test",
    srcs = ["remote_assertion_util_test.cc"],
    backends = sgx.backend_labels,
    copts = ASYLO_DEFAULT_COPTS,
    deps = [
        ":remote_assertion_cc_proto",
        ":remote_assertion_util",
        "//asylo/crypto:asn1",
        "//asylo/crypto:certificate_cc_proto",
        "//asylo/crypto:certificate_interface",
        "//asylo/crypto:ecdsa_p256_sha256_signing_key",
        "//asylo/crypto:fake_signing_key",
        "//asylo/crypto:keys_cc_proto",
        "//asylo/crypto:sha256_hash_cc_proto",
        "//asylo/crypto:signing_key",
        "//asylo/crypto:x509_certificate",
        "//asylo/crypto/util:bssl_util",
        "//asylo/identity:identity_acl_cc_proto",
        "//asylo/identity/platform/sgx:code_identity_cc_proto",
        "//asylo/identity/platform/sgx:machine_configuration_cc_proto",
        "//asylo/identity/platform/sgx:sgx_identity_cc_proto",
        "//asylo/identity/sgx:pck_certificate_util",
        "//asylo/identity/sgx:sgx_identity_util",
        "//asylo/test/util:proto_matchers",
        "//asylo/test/util:status_matchers",
        "//asylo/test/util:test_main",
        "//asylo/util:status_macros",
        "@boringssl//:crypto",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "certificate_util",
    srcs = ["certificate_util.cc"],
    hdrs = ["certificate_util.h"],
    deps = [
        ":attestation_key_certificate_impl",
        "//asylo/crypto:certificate_cc_proto",
        "//asylo/crypto:certificate_util",
        "//asylo/crypto:x509_certificate",
    ],
)
