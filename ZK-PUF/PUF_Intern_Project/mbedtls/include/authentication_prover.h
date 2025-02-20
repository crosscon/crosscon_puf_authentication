/*
 * authentication_prover.h
 *
 *  Created on: 14 Jun 2023
 *      Author: muhammad
 */

#ifndef INCLUDE_AUTHENTICATION_H_
#include "utils.h"
#include "constants.h"
#include "mbedtls/ecp.h"
#define INCLUDE_AUTHENTICATION_H_

#define NUM_RANDOM_NUMBERS 2
#define RANDOM_NUMBER_SIZE_BYTES 32

int start_auth_prover();
int generateRandomNumbers(mbedtls_mpi *num, mbedtls_mpi *num2);
mbedtls_mpi get_Pover_Result_P();
mbedtls_mpi get_Pover_Result_c();
mbedtls_mpi get_Pover_Result_v();
mbedtls_mpi get_Pover_Result_w();
mbedtls_mpi get_Pover_n();

void init_ECC(mbedtls_ecp_group *grp, mbedtls_ecp_point *h, mbedtls_ecp_point *C);
int enroll_ECC(mbedtls_ecp_group *grp, mbedtls_ecp_point *h, mbedtls_ecp_point *C);
int authenticate_ECC(mbedtls_ecp_group *grp, mbedtls_ecp_point *g, mbedtls_ecp_point *h, mbedtls_ecp_point *proof, mbedtls_ecp_point *C, mbedtls_mpi *result_v, mbedtls_mpi *result_w, mbedtls_mpi *nonce);


#endif /* INCLUDE_AUTHENTICATION_H_ */
