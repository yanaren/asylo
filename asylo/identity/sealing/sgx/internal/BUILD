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

load("//asylo/bazel:copts.bzl", "ASYLO_DEFAULT_COPTS")
load("@rules_proto//proto:defs.bzl", "proto_library")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("@rules_cc//cc:defs.bzl", "cc_proto_library")

licenses(["notice"])

package(default_visibility = ["//asylo/identity/sealing/sgx:__subpackages__"])

proto_library(
    name = "local_sealed_secret_proto",
    srcs = ["local_sealed_secret.proto"],
)

cc_proto_library(
    name = "local_sealed_secret_cc_proto",
    deps = [":local_sealed_secret_proto"],
)

proto_library(
    name = "local_secret_sealer_test_data_proto",
    srcs = ["local_secret_sealer_test_data.proto"],
    deps = [
        "//asylo/identity/platform/sgx:code_identity_proto",
        "//asylo/identity/platform/sgx:sgx_identity_proto",
        "//asylo/identity/sealing:sealed_secret_proto",
    ],
)

cc_proto_library(
    name = "local_secret_sealer_test_data_cc_proto",
    deps = ["local_secret_sealer_test_data_proto"],
)

cc_library(
    name = "local_secret_sealer_helpers",
    srcs = ["local_secret_sealer_helpers.cc"],
    hdrs = ["local_secret_sealer_helpers.h"],
    copts = ASYLO_DEFAULT_COPTS,
    deps = [
        ":local_sealed_secret_cc_proto",
        "//asylo/crypto:aead_cryptor",
        "//asylo/crypto:algorithms_cc_proto",
        "//asylo/crypto:sha256_hash",
        "//asylo/crypto/util:byte_container_util",
        "//asylo/crypto/util:byte_container_view",
        "//asylo/crypto/util:bytes",
        "//asylo/crypto/util:trivial_object_util",
        "//asylo/identity:identity_acl_cc_proto",
        "//asylo/identity:identity_cc_proto",
        "//asylo/identity/platform/sgx:code_identity_cc_proto",
        "//asylo/identity/platform/sgx:machine_configuration_cc_proto",
        "//asylo/identity/platform/sgx:sgx_identity_cc_proto",
        "//asylo/identity/sealing:sealed_secret_cc_proto",
        "//asylo/identity/sgx:hardware_interface",
        "//asylo/identity/sgx:hardware_types",
        "//asylo/identity/sgx:sgx_identity_util_internal",
        "//asylo/platform/common:singleton",
        "//asylo/util:cleansing_types",
        "//asylo/util:proto_enum_util",
        "//asylo/util:status",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/types:span",
    ],
)

cc_binary(
    name = "generate_local_secret_sealer_test_data",
    testonly = 1,
    srcs = ["generate_local_secret_sealer_test_data.cc"],
    copts = ASYLO_DEFAULT_COPTS,
    deps = [
        ":local_secret_sealer_test_data_cc_proto",
        "//asylo/crypto/util:trivial_object_util",
        "//asylo/identity/platform/sgx:code_identity_cc_proto",
        "//asylo/identity/sealing/sgx:sgx_local_secret_sealer",
        "//asylo/identity/sgx:fake_enclave",
        "//asylo/identity/sgx:hardware_types",
        "//asylo/identity/sgx:sgx_identity_util",
        "//asylo/util:logging",
        "//asylo/util:status",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@com_google_protobuf//:protobuf",
    ],
)
