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

#include "asylo/identity/provisioning/sgx/internal/platform_provisioning.h"

#include <limits>

#include "absl/strings/str_cat.h"
#include "asylo/crypto/util/trivial_object_util.h"
#include "asylo/identity/provisioning/sgx/internal/platform_provisioning.pb.h"
#include "asylo/identity/sgx/identity_key_management_structs.h"
#include "asylo/util/status_macros.h"

namespace asylo {
namespace sgx {

const uint32_t kPceSvnMaxValue = std::numeric_limits<uint16_t>::max();

const uint32_t kPceIdMaxValue = std::numeric_limits<uint16_t>::max();

Status ValidateConfigurationId(const ConfigurationId &id) {
  if (!id.has_value()) {
    return Status(error::GoogleError::INVALID_ARGUMENT,
                  "ConfigurationId does not have a \"value\" field");
  }

  return Status::OkStatus();
}

Status ValidatePpid(const Ppid &ppid) {
  if (!ppid.has_value()) {
    return Status(error::GoogleError::INVALID_ARGUMENT,
                  "Ppid does not have a \"value\" field");
  }

  if (ppid.value().size() != kPpidSize) {
    return Status(
        error::GoogleError::INVALID_ARGUMENT,
        absl::StrCat(
            "Ppid's \"value\" field has an invalid size (must be exactly ",
            kPpidSize, " bytes)"));
  }

  return Status::OkStatus();
}

Status ValidateCpuSvn(const CpuSvn &cpu_svn) {
  if (!cpu_svn.has_value()) {
    return Status(error::GoogleError::INVALID_ARGUMENT,
                  "CpuSvn does not have a \"value\" field");
  }

  if (cpu_svn.value().size() != kCpusvnSize) {
    return Status(
        error::GoogleError::INVALID_ARGUMENT,
        absl::StrCat(
            "CpuSvn's \"value\" field has an invalid size (must be exactly ",
            kCpusvnSize, " bytes)"));
  }

  return Status::OkStatus();
}

Status ValidatePceSvn(const PceSvn &pce_svn) {
  if (!pce_svn.has_value()) {
    return Status(error::GoogleError::INVALID_ARGUMENT,
                  "PceSvn does not have a \"value\" field");
  }

  if (pce_svn.value() > kPceSvnMaxValue) {
    return Status(
        error::GoogleError::INVALID_ARGUMENT,
        absl::StrCat(
            "PceSvn's \"value\" field is too large (must be less than ",
            kPceSvnMaxValue, ")"));
  }

  return Status::OkStatus();
}

Status ValidatePceId(const PceId &pce_id) {
  if (!pce_id.has_value()) {
    return Status(error::GoogleError::INVALID_ARGUMENT,
                  "PceId does not have a \"value\" field");
  }

  if (pce_id.value() > kPceIdMaxValue) {
    return Status(
        error::GoogleError::INVALID_ARGUMENT,
        absl::StrCat("PceId's \"value\" field is too large (must be less than ",
                     kPceIdMaxValue, ")"));
  }

  return Status::OkStatus();
}

Status ValidateFmspc(const Fmspc &fmspc) {
  if (!fmspc.has_value()) {
    return Status(error::GoogleError::INVALID_ARGUMENT,
                  "Fmspc does not have a \"value\" field");
  }

  if (fmspc.value().size() != kFmspcSize) {
    return Status(
        error::GoogleError::INVALID_ARGUMENT,
        absl::StrCat("Fmspc's \"value\" has an invalid size (must be exactly ",
                     kFmspcSize, " bytes)"));
  }

  return Status::OkStatus();
}

Status ValidateReportProto(const ReportProto &report_proto) {
  return ConvertReportProtoToHardwareReport(report_proto).status();
}

Status ValidateTargetInfoProto(const TargetInfoProto &target_info_proto) {
  return ConvertTargetInfoProtoToTargetinfo(target_info_proto).status();
}

StatusOr<Report> ConvertReportProtoToHardwareReport(
    const ReportProto &report_proto) {
  if (!report_proto.has_value()) {
    return Status(error::GoogleError::INVALID_ARGUMENT,
                  "ReportProto does not have a \"value\" field");
  }

  Report report;
  ASYLO_RETURN_IF_ERROR(
      SetTrivialObjectFromBinaryString<Report>(report_proto.value(), &report));
  return report;
}

StatusOr<Targetinfo> ConvertTargetInfoProtoToTargetinfo(
    const TargetInfoProto &target_info_proto) {
  if (!target_info_proto.has_value()) {
    return Status(error::GoogleError::INVALID_ARGUMENT,
                  "TargetInfoProto does not have a \"value\" field");
  }

  Targetinfo targetinfo;
  ASYLO_RETURN_IF_ERROR(SetTrivialObjectFromBinaryString<Targetinfo>(
      target_info_proto.value(), &targetinfo));
  return targetinfo;
}

StatusOr<CpuSvn> CpuSvnFromReportProto(const ReportProto &report_proto) {
  Report report;
  ASYLO_ASSIGN_OR_RETURN(report,
                         ConvertReportProtoToHardwareReport(report_proto));
  CpuSvn cpu_svn;
  cpu_svn.set_value(ConvertTrivialObjectToBinaryString(report.body.cpusvn));
  return cpu_svn;
}

}  // namespace sgx
}  // namespace asylo
