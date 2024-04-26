/*
 * utils.c
 *
 *  Created on: 13 Jun 2023
 *      Author: muhammad
 */
#include "utils.h"

int myrand(void *rng_state, unsigned char *output, size_t len) {
	size_t use_len;
	int rnd;

	if (rng_state != NULL)
		rng_state = NULL;

	while (len > 0) {
		use_len = len;
		if (use_len > sizeof(int))
			use_len = sizeof(int);

		rnd = rand();
		memcpy(output, &rnd, use_len);
		output += use_len;
		len -= use_len;
	}

	return (0);
}

int calculate_exp_mod(mbedtls_mpi mpiValue_g, mbedtls_mpi mpiValue_R1,
		mbedtls_mpi mpiValue_h, mbedtls_mpi mpiValue_R2, mbedtls_mpi mpiValue_p,
		mbedtls_mpi *mpiResult) {

	mbedtls_mpi g_power_r1, h_power_r2;
	mbedtls_mpi_init(&g_power_r1);
	mbedtls_mpi_init(&h_power_r2);

	if (mbedtls_mpi_exp_mod(&g_power_r1, &mpiValue_g, &mpiValue_R1, &mpiValue_p,
	NULL) != 0) {
		return 1;
	}
	if (mbedtls_mpi_exp_mod(&h_power_r2, &mpiValue_h, &mpiValue_R2, &mpiValue_p,
	NULL) != 0) {
		return 1;
	}

	mbedtls_mpi_mul_mpi(mpiResult, &g_power_r1, &h_power_r2);
	mbedtls_mpi_mod_mpi(mpiResult, mpiResult, &mpiValue_p);

	mbedtls_mpi_free(&g_power_r1);
	mbedtls_mpi_free(&h_power_r2);
	return 0;
}

int update_var_for_exp_mod(mbedtls_mpi *mpiValue_g, mbedtls_mpi *mpiValue_R1,
		mbedtls_mpi *mpiValue_h, mbedtls_mpi *mpiValue_R2,
		mbedtls_mpi *mpiValue_p, mbedtls_mpi *mpiValue_n) {

	if (mbedtls_mpi_read_string(mpiValue_R1, 10, R1) != 0) {
		return 1;
	}

	if (mbedtls_mpi_read_string(mpiValue_R2, 10, R2) != 0) {
		return 1;
	}
	if (mbedtls_mpi_read_string(mpiValue_p, 10, p) != 0) {
		return 1;
	}
	if (mbedtls_mpi_read_string(mpiValue_g, 10, g_GENERATOR_OF_G) != 0) {
		return 1;
	}
	if (mbedtls_mpi_read_string(mpiValue_h, 10, h_GENERATOR_OF_G) != 0) {
		return 1;
	}

	if (mbedtls_mpi_read_string(mpiValue_n, 10, n) != 0) {
		return 1;
	}
	return 0;
}

void printHex(const unsigned char *data, size_t length) {
	for (size_t i = 0; i < length; i++) {
		PRINTF("%02X ", data[i]);
	}
	PRINTF("\n");
}

void sha256Hash(const unsigned char *data, size_t data_len,
		unsigned char *output) {
	mbedtls_sha256(data, data_len, output, 0);
}
int copyMPIToBuffer(const mbedtls_mpi *mpi, unsigned char *buffer,
		size_t buffer_len) {
	size_t mpi_len = mbedtls_mpi_size(mpi);
	size_t copy_len = (buffer_len < mpi_len) ? buffer_len : mpi_len;
	memset(buffer, 0, buffer_len);
	if (mbedtls_mpi_write_binary(mpi, buffer, copy_len) != 0) {
		return 1;
	}
	return 0;
}
int copyBufferToMPI(const unsigned char *buffer, size_t buffer_len,
		mbedtls_mpi *mpi) {
	if (mbedtls_mpi_read_binary(mpi, buffer, buffer_len) != 0) {

		return 1;
	}

	return 0;
}
int concatenateBuffers(const unsigned char *buffer1, size_t buffer1_len,
		const unsigned char *buffer2, size_t buffer2_len,
		const unsigned char *buffer3, size_t buffer3_len,
		unsigned char *destination, size_t destination_len) {
	if (buffer1_len + buffer2_len + buffer3_len > destination_len) {
		PRINTF("Insufficient destination buffer size.\n");
		return 1;
	}

	memcpy(destination, buffer1, buffer1_len);
	memcpy(destination + buffer1_len, buffer2, buffer2_len);
	memcpy(destination + buffer1_len + buffer2_len, buffer3, buffer3_len);

	return 0;
}
int calculate_SHA256(mbedtls_mpi mpiValue_p, mbedtls_mpi mpiValue_n, unsigned char sha256_result[32]) {
	size_t mpi_len_p = mbedtls_mpi_size(&mpiValue_p);
	size_t mpi_len_n = mbedtls_mpi_size(&mpiValue_n);

	unsigned char buffer_p[mpi_len_p];
	unsigned char buffer_n[mpi_len_n];

	if (copyMPIToBuffer(&mpiValue_p, buffer_p, mpi_len_p) != 0) {
		return 1;
	}
	if (copyMPIToBuffer(&mpiValue_n, buffer_n, mpi_len_n) != 0) {
		return 1;
	}

	size_t result_len = mpi_len_p + mpi_len_n;
	unsigned char result[result_len];
	/*
	if (concatenateBuffers(buffer_p, mpi_len_p, mpi_len_Da, buffer_n,
			mpi_len_n, result, result_len) != 0) {
		return 1;
	}
	*/
	memcpy(result, buffer_p, mpi_len_p);
	memcpy(result + mpi_len_p, buffer_n, mpi_len_p);
	sha256Hash(result, result_len, sha256_result);

	return 0;
}
