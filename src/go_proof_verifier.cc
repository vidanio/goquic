#include "go_proof_verifier.h"
#include "go_functions.h"

#include "base/logging.h"

namespace net {

GoProofVerifier::GoProofVerifier(void* go_proof_verifier) :
  go_proof_verifier_(go_proof_verifier) { }

GoProofVerifier::~GoProofVerifier() {
  // TODO(hodduc) free go_proof_verifier
}

QuicAsyncStatus GoProofVerifier::VerifyProof(const std::string& hostname,
                                             const std::string& server_config,
                                             const std::vector<std::string>& certs,
                                             const std::string& cert_sct,
                                             const std::string& signature,
                                             const ProofVerifyContext* context,
                                             std::string* error_details,
                                             scoped_ptr<ProofVerifyDetails>* details,
                                             ProofVerifierCallback* callback) {
  // XXX(hodduc): Should we implement verifying on go-side asynchronously?

  scoped_ptr<GoProofVerifyDetails> verify_details_;
  verify_details_.reset(new GoProofVerifyDetails);

  if (certs.empty()) {
    *error_details = "Failed to create certificate chain. Certs are empty.";
    DLOG(WARNING) << *error_details;
//    verify_details_->cert_verify_result.cert_status = CERT_STATUS_INVALID;
    *details = verify_details_.Pass();
    return QUIC_FAILURE;
  }

  // Convery certs to X509Certificate.
  void* job = NewProofVerifyJob_C(
      go_proof_verifier_,
      (char*)(hostname.c_str()), (size_t)(hostname.length()),
      (char*)(server_config.c_str()), (size_t)(server_config.length()),
      (char*)(cert_sct.c_str()), (size_t)(cert_sct.length()),
      (char*)(signature.c_str()), (size_t)(signature.length()));

  for (auto it = certs.begin(); it != certs.end(); it++) {
    ProofVerifyJobAddCert_C(job, (char*)it->c_str(), (size_t)it->length());
  }

  // TODO(hodduc) detailed error msg
  int ret = ProofVerifyJobVerifyProof_C(job);

  if (ret == 1) {
    *details = verify_details_.Pass();
    return QUIC_SUCCESS;
  } else {
    *error_details = "Failed to verify proof";
    DLOG(WARNING) << *error_details;
    *details = verify_details_.Pass();
    return QUIC_FAILURE;
  }
}

} // namespace net