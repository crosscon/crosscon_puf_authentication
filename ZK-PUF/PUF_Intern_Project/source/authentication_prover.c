/*
 * authentication.c
 *
 *  Created on: 14 Jun 2023
 *      Author: muhammad
 */
#include <authentication_prover.h>
#include <enrollment.h>
#include <stdio.h>
#include "mbedtls/bignum.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ecp.h"
#include "fsl_puf.h"

#define PUF_KEY_SIZE 16

mbedtls_mpi result_c, result_P, result_v, result_w;

int getPUFResponse(mbedtls_mpi *mpiValue_R, int pufSlot) {
	size_t keyCodeSize = PUF_GET_KEY_CODE_SIZE_FOR_KEY_SIZE(PUF_KEY_SIZE);
    uint8_t keyCode[keyCodeSize];

    if(pufSlot==0){
    	if(R1==NULL){
    		if (PUF_SetIntrinsicKey(PUF, pufSlot, PUF_KEY_SIZE, keyCode, sizeof(keyCode)) != kStatus_Success) {
    		        return 1;
    		    }
    		R1 = (char *)malloc(keyCodeSize);
    		memcpy(R1, keyCode, keyCodeSize);
    	}
    	else
    		memcpy(keyCode, R1, keyCodeSize);
    }
    else{
    	if(R2==NULL){
		if (PUF_SetIntrinsicKey(PUF, pufSlot, PUF_KEY_SIZE, keyCode, sizeof(keyCode)) != kStatus_Success) {
				return 1;
			}
		R2 = (char *)malloc(keyCodeSize);
		memcpy(R2, keyCode, keyCodeSize);
	}
	else
		memcpy(keyCode, R2, keyCodeSize);
    }

    if (mbedtls_mpi_read_binary(mpiValue_R, keyCode, PUF_KEY_SIZE) != 0) {
        return 1;
    }

    memset(keyCode, 0, sizeof(keyCode));

    return 0;
}

int generateRandomNumbers(mbedtls_mpi *num, mbedtls_mpi *num2) {

	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;

	mbedtls_entropy_init(&entropy);
	mbedtls_ctr_drbg_init(&ctr_drbg);

	const char *pers = "random_number_generation";

	if (mbedtls_ctr_drbg_seed(&ctr_drbg, myrand, &entropy,
			(const unsigned char*) pers, strlen(pers)) != 0) {
		return 1;
	}

	unsigned char randomBytes[NUM_RANDOM_NUMBERS][RANDOM_NUMBER_SIZE_BYTES];

	for (int i = 0; i < NUM_RANDOM_NUMBERS; i++) {
		mbedtls_ctr_drbg_random(&ctr_drbg, randomBytes[i],
		RANDOM_NUMBER_SIZE_BYTES);
	}

	if (mbedtls_mpi_read_binary(num, randomBytes[0], 32) != 0) { // was 16
		return 1;
	}
	if (mbedtls_mpi_read_binary(num2, randomBytes[1], 32) != 0) { // was 16
		return 1;
	}

	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_entropy_free(&entropy);

	return 0;
}

void concatenateMpiValues(const mbedtls_mpi *mpi1, const mbedtls_mpi *mpi2,
		const mbedtls_mpi *mpi3, unsigned char **output, size_t *output_len) {
	size_t mpi1_len = mbedtls_mpi_size(mpi1);
	size_t mpi2_len = mbedtls_mpi_size(mpi2);
	size_t mpi3_len = mbedtls_mpi_size(mpi3);

	*output_len = mpi1_len + mpi2_len + mpi3_len;
	*output = (unsigned char*) malloc(*output_len);

	memcpy(*output, mpi1->p, mpi1_len);
	memcpy(*output + mpi1_len, mpi2->p, mpi2_len);
	memcpy(*output + mpi1_len + mpi2_len, mpi3->p, mpi3_len);
}

void add_MPI(const mbedtls_mpi *mpi1, const mbedtls_mpi *mpi2,
		const mbedtls_mpi *mpi3, mbedtls_mpi *result) {
	mbedtls_mpi_add_mpi(result, mpi1, mpi2); // Add mpi1 and mpi2, store result in result
	mbedtls_mpi_add_mpi(result, result, mpi3); // Add the previous result with mpi3, store result in result
}

int add_mul_mod(mbedtls_mpi *mpiValue_1, mbedtls_mpi *mpiValue_2,
		mbedtls_mpi *mpiValue_R, mbedtls_mpi *mpiValue_p, mbedtls_mpi *result) {

	if (mbedtls_mpi_mul_mpi(result, mpiValue_2, mpiValue_R) != 0) {
		return 1;
	}
	if (mbedtls_mpi_add_mpi(result, result, mpiValue_1) != 0) {
		return 1;
	}
	if (mbedtls_mpi_mod_mpi(result, result, mpiValue_p) != 0) {
		return 1;
	}
	return 0;
}

void init_ECC(mbedtls_ecp_group *grp, mbedtls_ecp_point *h, mbedtls_ecp_point *C) {
	mbedtls_ecp_group_init(grp);
	mbedtls_ecp_point_init(h);
	mbedtls_ecp_point_init(C);
	int res;
	res = mbedtls_ecp_group_load(grp, MBEDTLS_ECP_DP_SECP256R1);
	mbedtls_mpi x;
	mbedtls_mpi_init(&x);
	res = mbedtls_mpi_lset(&x, 123456789);  // used to create a second group generator as part of the global parameters
	res = mbedtls_ecp_mul(grp, h, &x, &grp->G, myrand, NULL);

}

int enroll_ECC(mbedtls_ecp_group *grp, mbedtls_ecp_point *h, mbedtls_ecp_point *C) {

	mbedtls_mpi mpiValue_R1, mpiValue_R2;
	mbedtls_mpi_init(&mpiValue_R1);
	mbedtls_mpi_init(&mpiValue_R2);

	if (getPUFResponse(&mpiValue_R1, 0) != 0) {

		return 1;
	}

	if (getPUFResponse(&mpiValue_R2, 1) != 0) {
		return 1;
	}

	if(mbedtls_ecp_muladd(grp, C, &mpiValue_R1, &grp->G, &mpiValue_R2, h) != 0) { // C = g * R1 + h * R2
		return 1;
	}
	return 0;
}
int authenticate_ECC(mbedtls_ecp_group *grp, mbedtls_ecp_point *g, mbedtls_ecp_point *h, mbedtls_ecp_point *proof, mbedtls_ecp_point *C, mbedtls_mpi *result_v, mbedtls_mpi *result_w, mbedtls_mpi *nonce) {

	unsigned char sha256_result[32];
	int res = 0;
	mbedtls_mpi mpiValue_R1, mpiValue_R2, random_r, random_u, result_c;

	mbedtls_mpi_init(&mpiValue_R1);
	mbedtls_mpi_init(&mpiValue_R2);
	mbedtls_mpi_init(&random_r);
	mbedtls_mpi_init(&random_u);
	mbedtls_mpi_init(&result_c);

	if (generateRandomNumbers(&random_r, &random_u) != 0) {
		PRINTF("Error in Random Process\n\n");
		return 1;
	}

	if (getPUFResponse(&mpiValue_R1, 0) != 0) {
		return 1;
	}

	if (getPUFResponse(&mpiValue_R2, 1) != 0) {
		return 1;
	}

	if(mbedtls_ecp_muladd(grp, proof, &random_r, g, &random_u, h) != 0) { //proof
		return 1;
	}

	size_t olen;
	unsigned char buff[100];
	mbedtls_ecp_point_write_binary(grp, C, MBEDTLS_ECP_PF_UNCOMPRESSED, &olen, buff, sizeof(buff));
	unsigned char buff2[olen + mbedtls_mpi_size(nonce)];
	memcpy(buff2, buff, olen);
	mbedtls_mpi_write_binary(nonce, buff2 + olen, sizeof(buff2) - olen);
	sha256Hash(buff2, sizeof(buff2), sha256_result);
	mbedtls_mpi_read_string(&result_c, 16, sha256_result);

	add_mul_mod(&random_r, &result_c, &mpiValue_R1, &grp->P, result_v);
	add_mul_mod(&random_u, &result_c, &mpiValue_R2, &grp->P, result_w);

	return 0;

}


int start_auth_prover() {

	mbedtls_mpi mpiValue_R1;
	mbedtls_mpi mpiValue_R2;
	mbedtls_mpi mpiValue_p;
	mbedtls_mpi mpiValue_g;
	mbedtls_mpi mpiValue_h;
	mbedtls_mpi mpiValue_n;

	mbedtls_mpi_init(&mpiValue_R1);
	mbedtls_mpi_init(&mpiValue_R2);
	mbedtls_mpi_init(&mpiValue_p);
	mbedtls_mpi_init(&mpiValue_g);
	mbedtls_mpi_init(&mpiValue_h);
	mbedtls_mpi_init(&mpiValue_n);

	mbedtls_mpi random_r, random_u;

	mbedtls_mpi_init(&random_r);
	mbedtls_mpi_init(&random_u);

	mbedtls_mpi_init(&result_P);
	mbedtls_mpi_init(&result_v);
	mbedtls_mpi_init(&result_w);
	mbedtls_mpi_init(&result_c);

	//generate random r and u
	if (generateRandomNumbers(&random_r, &random_u) != 0) {
		PRINTF("Error in Random Process\n\n");
		return 1;
	}

	// init value in mpis
	if (update_var_for_exp_mod(&mpiValue_g, &mpiValue_R1, &mpiValue_h,
			&mpiValue_R2, &mpiValue_p, &mpiValue_n) != 0) {
		PRINTF("Error Calculating first mod \n\n");
		return 1;
	}

	//int res = mbedtls_ecp_muladd()

	//calculate value of P
	if (calculate_exp_mod(mpiValue_g, random_r, mpiValue_h, random_u,
			mpiValue_p, &result_P) != 0) {
		PRINTF("Error Calculating second mod. \n\n");
		return 1;
	}
	unsigned char sha256_result[32];
	// calculate value of c
	if (calculate_SHA256(result_P, mpiValue_n, sha256_result)
			!= 0) {
		PRINTF("Error Calculating SHA.\n\n");
		return 1;
	}

	if (copyBufferToMPI(sha256_result, 32, &result_c) != 0) {
		PRINTF("Error Copy Buffer.\n\n");
		return 1;
	}
//
//	char mpi_str[1024];
//	size_t t;
//	mbedtls_mpi_write_string(&result_c, 16, mpi_str, sizeof(mpi_str), &t);
//	PRINTF("MPI Value: %s\n", mpi_str);

	if (add_mul_mod(&random_r, &result_c, &mpiValue_R1, &mpiValue_p, &result_v)
			!= 0) {
		PRINTF("Error Calculating v.\n\n");
		return 1;
	}
	// calculate value of w
	if (add_mul_mod(&random_u, &result_c, &mpiValue_R2, &mpiValue_p, &result_w)
			!= 0) {
		PRINTF("Error Calculating w.\n\n");
		return 1;
	}

	mbedtls_mpi_free(&random_r);
	mbedtls_mpi_free(&random_u);
	mbedtls_mpi_free(&mpiValue_R1);
	mbedtls_mpi_free(&mpiValue_R2);
	mbedtls_mpi_free(&mpiValue_p);
	mbedtls_mpi_free(&mpiValue_g);
	mbedtls_mpi_free(&mpiValue_h);
	mbedtls_mpi_free(&mpiValue_n);

	return 0;
}
mbedtls_mpi get_Pover_Result_P() {
	return result_P;

}
mbedtls_mpi get_Pover_Result_c() {
	return result_c;
}
mbedtls_mpi get_Pover_Result_v() {
	return result_v;
}
mbedtls_mpi get_Pover_Result_w() {
	return result_w;
}

