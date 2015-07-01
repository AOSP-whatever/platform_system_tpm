// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// trunks_client is a command line tool that supports various TPM operations. It
// does not provide direct access to the trunksd D-Bus interface.

#include <stdio.h>
#include <string>

#include <base/command_line.h>
#include <base/logging.h>
#include <chromeos/syslog_logging.h>

#include "trunks/error_codes.h"
#include "trunks/hmac_session.h"
#include "trunks/password_authorization_delegate.h"
#include "trunks/policy_session.h"
#include "trunks/scoped_key_handle.h"
#include "trunks/tpm_state.h"
#include "trunks/tpm_utility.h"
#include "trunks/trunks_client_test.h"
#include "trunks/trunks_factory_impl.h"

namespace {

void PrintUsage() {
  puts("Options:");
  puts("  --allocate_pcr - Configures PCR 0-15 under the SHA256 bank.");
  puts("  --clear - Clears the TPM. Use before initializing the TPM.");
#if defined SPI_OVER_FTDI
  puts("  --ftdi - Tries to communicate with a TPM2 chip over FTDI.");
#endif
  puts("  --help - Prints this message.");
  puts("  --init_tpm - Initializes a TPM as CrOS firmware does.");
  puts("  --own - Takes ownership of the TPM with the provided password.");
  puts("  --owner_password - used to provide an owner password");
  puts("  --regression_test - Runs some basic regression tests. If");
  puts("                      owner_password is supplied, it runs tests that");
  puts("                      need owner permissions.");
  puts("  --startup - Performs startup and self-tests.");
  puts("  --status - Prints TPM status information.");
  puts("  --stress_test - Runs some basic stress tests.");
}

int Startup(bool use_ftdi) {
  trunks::TrunksFactoryImpl factory(use_ftdi);
  factory.GetTpmUtility()->Shutdown();
  return factory.GetTpmUtility()->Startup();
}

int Clear(bool use_ftdi) {
  trunks::TrunksFactoryImpl factory(use_ftdi);
  return factory.GetTpmUtility()->Clear();
}

int InitializeTpm(bool use_ftdi) {
  trunks::TrunksFactoryImpl factory(use_ftdi);
  return factory.GetTpmUtility()->InitializeTpm();
}

int AllocatePCR(bool use_ftdi) {
  trunks::TrunksFactoryImpl factory(use_ftdi);
  trunks::TPM_RC result;
  result = factory.GetTpmUtility()->AllocatePCR("");
  if (result != trunks::TPM_RC_SUCCESS) {
    LOG(ERROR) << "Error allocating PCR:" << trunks::GetErrorString(result);
    return result;
  }
  factory.GetTpmUtility()->Shutdown();
  return factory.GetTpmUtility()->Startup();
}

int TakeOwnership(const std::string& owner_password, bool use_ftdi) {
  trunks::TrunksFactoryImpl factory(use_ftdi);
  trunks::TPM_RC rc;
  rc = factory.GetTpmUtility()->TakeOwnership(owner_password,
                                              owner_password,
                                              owner_password);
  if (rc) {
    LOG(ERROR) << "Error taking ownership: " << trunks::GetErrorString(rc);
    return rc;
  }
  return 0;
}

int DumpStatus(bool use_ftdi) {
  trunks::TrunksFactoryImpl factory(use_ftdi);
  scoped_ptr<trunks::TpmState> state = factory.GetTpmState();
  trunks::TPM_RC result = state->Initialize();
  if (result != trunks::TPM_RC_SUCCESS) {
    LOG(ERROR) << "Failed to read TPM state: "
               << trunks::GetErrorString(result);
    return result;
  }
  printf("Owner password set: %s\n",
         state->IsOwnerPasswordSet() ? "true" : "false");
  printf("Endorsement password set: %s\n",
         state->IsEndorsementPasswordSet() ? "true" : "false");
  printf("Lockout password set: %s\n",
         state->IsLockoutPasswordSet() ? "true" : "false");
  printf("Ownership status: %s\n",
         state->IsOwned() ? "true" : "false");
  printf("In lockout: %s\n",
         state->IsInLockout() ? "true" : "false");
  printf("Platform hierarchy enabled: %s\n",
         state->IsPlatformHierarchyEnabled() ? "true" : "false");
  printf("Storage hierarchy enabled: %s\n",
         state->IsStorageHierarchyEnabled() ? "true" : "false");
  printf("Endorsement hierarchy enabled: %s\n",
         state->IsEndorsementHierarchyEnabled() ? "true" : "false");
  printf("Is Tpm enabled: %s\n",
         state->IsEnabled() ? "true" : "false");
  printf("Was shutdown orderly: %s\n",
         state->WasShutdownOrderly() ? "true" : "false");
  printf("Is RSA supported: %s\n",
         state->IsRSASupported() ? "true" : "false");
  printf("Is ECC supported: %s\n",
         state->IsECCSupported() ? "true" : "false");
  return 0;
}

}  // namespace

int main(int argc, char **argv) {
  base::CommandLine::Init(argc, argv);
  chromeos::InitLog(chromeos::kLogToSyslog | chromeos::kLogToStderr);
  base::CommandLine *cl = base::CommandLine::ForCurrentProcess();
  bool use_ftdi = false;
#if defined SPI_OVER_FTDI
  use_ftdi  = cl->HasSwitch("ftdi");
#endif
  if (cl->HasSwitch("status")) {
    return DumpStatus(use_ftdi);
  }
  if (cl->HasSwitch("startup")) {
    return Startup(use_ftdi);
  }
  if (cl->HasSwitch("clear")) {
    return Clear(use_ftdi);
  }
  if (cl->HasSwitch("init_tpm")) {
    return InitializeTpm(use_ftdi);
  }
  if (cl->HasSwitch("allocate_pcr")) {
    return AllocatePCR(use_ftdi);
  }
  if (cl->HasSwitch("help")) {
    puts("Trunks Client: A command line tool to access the TPM.");
    PrintUsage();
    return 0;
  }
  if (cl->HasSwitch("own")) {
    return TakeOwnership(cl->GetSwitchValueASCII("owner_password"), use_ftdi);
  }
  if (cl->HasSwitch("regression_test")) {
    trunks::TrunksClientTest test;
    LOG(INFO) << "Running RNG test.";
    if (!test.RNGTest()) {
      LOG(ERROR) << "Error running RNGtest.";
      return -1;
    }
    LOG(INFO) << "Running RSA key tests.";
    if (!test.SignTest()) {
      LOG(ERROR) << "Error running SignTest.";
      return -1;
    }
    if (!test.DecryptTest()) {
      LOG(ERROR) << "Error running DecryptTest.";
      return -1;
    }
    if (!test.ImportTest()) {
      LOG(ERROR) << "Error running ImportTest.";
      return -1;
    }
    if (!test.AuthChangeTest()) {
      LOG(ERROR) << "Error running AuthChangeTest.";
      return -1;
    }
    LOG(INFO) << "Running PCR test.";
    if (!test.PCRTest()) {
      LOG(ERROR) << "Error running PCRTest.";
      return -1;
    }
    LOG(INFO) << "Running policy tests.";
    if (!test.PolicyAuthValueTest()) {
      LOG(ERROR) << "Error running PolicyAuthValueTest.";
      return -1;
    }
    if (!test.PolicyAndTest()) {
      LOG(ERROR) << "Error running PolicyAndTest.";
      return -1;
    }
    if (!test.PolicyOrTest()) {
      LOG(ERROR) << "Error running PolicyOrTest.";
      return -1;
    }
    if (cl->HasSwitch("owner_password")) {
      std::string owner_password = cl->GetSwitchValueASCII("owner_password");
      LOG(INFO) << "Running NVRAM test.";
      if (!test.NvramTest(owner_password)) {
        LOG(ERROR) << "Error running NvramTest.";
        return -1;
      }
    }
    LOG(INFO) << "All tests were run successfully.";
    return 0;
  }
  if (cl->HasSwitch("stress_test")) {
    LOG(INFO) << "Running stress tests.";
    trunks::TrunksClientTest test;
    if (!test.ManyKeysTest()) {
      LOG(ERROR) << "Error running ManyKeysTest.";
      return -1;
    }
    if (!test.ManySessionsTest()) {
      LOG(ERROR) << "Error running ManySessionsTest.";
      return -1;
    }
    return 0;
  }
  puts("Invalid options!");
  PrintUsage();
  return -1;
}
