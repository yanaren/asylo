/*
 *
 * Copyright 2017 Asylo authors
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

// This file implements bit-wise AND, equality, and inequality operations
// for Attributes protobuf.

#include "asylo/identity/sgx/attributes_util.h"

#include <google/protobuf/util/message_differencer.h>
#include "asylo/identity/sgx/attributes.pb.h"

namespace asylo {
namespace sgx {

Attributes operator&(const Attributes &lhs, const Attributes &rhs) {
  Attributes result;
  result.set_flags(lhs.flags() & rhs.flags());
  result.set_xfrm(lhs.xfrm() & rhs.xfrm());
  return result;
}

bool operator==(const Attributes &lhs, const Attributes &rhs) {
  return ::google::protobuf::util::MessageDifferencer::Equivalent(lhs, rhs);
}

bool operator!=(const Attributes &lhs, const Attributes &rhs) {
  return (!(lhs == rhs));
}

}  // namespace sgx
}  // namespace asylo
