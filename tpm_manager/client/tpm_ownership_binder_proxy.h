//
// Copyright (C) 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef TPM_MANAGER_CLIENT_TPM_OWNERSHIP_BINDER_PROXY_H_
#define TPM_MANAGER_CLIENT_TPM_OWNERSHIP_BINDER_PROXY_H_

#include <base/macros.h>

#include "android/tpm_manager/ITpmOwnership.h"
#include "tpm_manager/common/export.h"
#include "tpm_manager/common/tpm_ownership_interface.h"

namespace tpm_manager {

// An implementation of TpmOwnershipInterface that forwards requests to
// tpm_managerd using Binder.
class TPM_MANAGER_EXPORT TpmOwnershipBinderProxy
    : public TpmOwnershipInterface {
 public:
  TpmOwnershipBinderProxy() = default;
  explicit TpmOwnershipBinderProxy(android::tpm_manager::ITpmOwnership* binder);
  ~TpmOwnershipBinderProxy() override = default;

  // Initializes the client. Returns true on success.
  bool Initialize();

  // TpmOwnershipInterface methods. All assume a message loop and binder watcher
  // have already been configured elsewhere.
  void GetTpmStatus(const GetTpmStatusRequest& request,
                    const GetTpmStatusCallback& callback) override;
  void TakeOwnership(const TakeOwnershipRequest& request,
                     const TakeOwnershipCallback& callback) override;
  void RemoveOwnerDependency(
      const RemoveOwnerDependencyRequest& request,
      const RemoveOwnerDependencyCallback& callback) override;

 private:
  android::sp<android::tpm_manager::ITpmOwnership> default_binder_;
  android::tpm_manager::ITpmOwnership* binder_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(TpmOwnershipBinderProxy);
};

}  // namespace tpm_manager

#endif  // TPM_MANAGER_CLIENT_TPM_OWNERSHIP_BINDER_PROXY_H_
