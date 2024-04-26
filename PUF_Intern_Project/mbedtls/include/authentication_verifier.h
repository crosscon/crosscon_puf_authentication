/*
 * authentication_verifier.h
 *
 *  Created on: 14 Jun 2023
 *      Author: muhammad
 */

#ifndef INCLUDE_AUTHENTICATION_VERIFIER_H_

#include "utils.h"
#include "authentication_prover.h"

#define INCLUDE_AUTHENTICATION_VERIFIER_H_
int start_auth_verifier();
int verify_ECC(mbedtls_ecp_group *grp, mbedtls_ecp_point *g, mbedtls_ecp_point *h, mbedtls_ecp_point *proof, mbedtls_ecp_point *C, mbedtls_mpi *result_v, mbedtls_mpi *result_w, mbedtls_mpi *nonce);

#endif /* INCLUDE_AUTHENTICATION_VERIFIER_H_ */
