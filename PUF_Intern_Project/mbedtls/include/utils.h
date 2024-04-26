/*
 * utils.h
 *
 *  Created on: 13 Jun 2023
 *      Author: muhammad
 */

#ifndef INCLUDE_UTILS_H_
#include <enrollment.h>
#include "mbedtls/bignum.h"
#include "constants.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/sha256.h"
#define INCLUDE_UTILS_H_

int calculate_exp_mod(mbedtls_mpi mpiValue_g, mbedtls_mpi mpiValue_R1,
		mbedtls_mpi mpiValue_h, mbedtls_mpi mpiValue_R2, mbedtls_mpi mpiValue_p,
		mbedtls_mpi *mpiResult);
int update_var_for_exp_mod(mbedtls_mpi *mpiValue_g, mbedtls_mpi *mpiValue_R1,
		mbedtls_mpi *mpiValue_h, mbedtls_mpi *mpiValue_R2,
		mbedtls_mpi *mpiValue_p, mbedtls_mpi *mpiValue_n);
int myrand(void *rng_state, unsigned char *output, size_t len);
int calculate_SHA256(mbedtls_mpi mpiValue_p, mbedtls_mpi mpiValue_n, unsigned char sha256_result[32]);
int copyBufferToMPI(const unsigned char *buffer, size_t buffer_len,
		mbedtls_mpi *mpi);
#endif /* INCLUDE_UTILS_H_ */

//	char mpi_str[1024];
//	size_t t;
//	mbedtls_mpi_write_string(&result_v, 16, mpi_str, sizeof(mpi_str), &t);
//	PRINTF("MPI Value: %s\n", mpi_str);
