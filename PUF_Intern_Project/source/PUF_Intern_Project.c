/*
 * Copyright 2016-2023 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of NXP Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file    PUF_Intern_Project.c
 * @brief   Application entry point.
 */
#include <authentication_prover.h>
#include <authentication_verifier.h>
#include <enrollment.h>
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "LPC55S69_cm33_core0.h"
#include "fsl_debug_console.h"
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "mbedtls/timing.h"
#include "mbedtls/ecp.h"
#include "utils.h"
#include "constants.h"


#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/sha256.h"

//int myrand( void *rng_state, unsigned char *output, size_t len );


#define mbedtls_printf PRINTF

/* TODO: insert other include files here. */

/* TODO: insert other definitions and declarations here. */

/*
 * @brief   Application entry point.
 */

#define DWT_CONTROL             (*(volatile uint32_t *)0xE0001000)
#define DWT_CYCCNT              (*(volatile uint32_t *)0xE0001004)
#define DWT_CYCCNTENA_BIT       (1UL<<0)


void enableCycleCounter() {
	// Enable the DWT cycle counter
	DWT_CONTROL |= DWT_CYCCNTENA_BIT;
}

void resetCycleCounter() {
	// Reset the DWT cycle counter
	DWT_CYCCNT = 0;
}

unsigned long getCycleCount() {
	// Read the DWT cycle counter value
	return DWT_CYCCNT;
}

int add_mul_mod2(mbedtls_mpi *mpiValue_1, mbedtls_mpi *mpiValue_2,
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



/*
void init_ecc(mbedtls_ecp_group *grp, mbedtls_ecp_point *g, mbedtls_ecp_point *h) {
	mbedtls_ecp_group_init(grp);
	mbedtls_ecp_point_init(g);
	mbedtls_ecp_point_init(h);
	mbedtls_ecp_group_load(grp, MBEDTLS_ECP_DP_SECP256R1);
	mbedtls_mpi x;
	mbedtls_mpi_init(&x);
	mbedtls_mpi_lset(&x, 123456789);  // used to create a second group generator as part of the global parameters
	mbedtls_ecp_mul(grp, h, x, grp.G, myrand, NULL);
}
*/
int main(void) {

	/* Init board hardware. */	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();
	BOARD_InitDebugConsole();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
	/* Init FSL debug console. */

#endif

	uint32_t coreFrequency = SystemCoreClock;
	unsigned long start, end, cycles;
	BOARD_InitDebugConsole();
	// Enable and reset the cycle counter
	enableCycleCounter();
	resetCycleCounter();
	double time;
	// Start the timer
	int res;

	mbedtls_ecp_group grp;
	mbedtls_ecp_point h, C;
	init_ECC(&grp, &h, &C);
	res = enroll_ECC(&grp, &h, &C);
	mbedtls_ecp_point proof;
	mbedtls_ecp_point_init(&proof);
	mbedtls_mpi result_v, result_w, nonce;
	mbedtls_mpi_init(&result_v);
	mbedtls_mpi_init(&result_w);
	mbedtls_mpi_init(&nonce);


	res = authenticate_ECC(&grp, &grp.G, &h, &proof, &C, &result_v, &result_w, &nonce );


	res = verify_ECC(&grp, &grp.G, &h, &proof, &C, &result_v, &result_w, &nonce );

	/*
	start = getCycleCount();
	mbedtls_ecp_group grp;
	mbedtls_ecp_group_init(&grp);
	mbedtls_ecp_point H;
	mbedtls_ecp_point_init(&H);
	mbedtls_mpi mpiValue_R3;
	mbedtls_mpi_init(&mpiValue_R3);
	mbedtls_ecp_point H2;
	mbedtls_ecp_point_init(&H2);
	*/
/*
//--------ECDSA test
	int ret = 1;
	//int exit_code = MBEDTLS_EXIT_FAILURE;
	mbedtls_ecdsa_context ctx_sign, ctx_verify;
	mbedtls_ecp_point Q;
	mbedtls_ecp_point_init(&Q);
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	unsigned char message[100];
	unsigned char hash[32];
	unsigned char sig[MBEDTLS_ECDSA_MAX_LEN];
	size_t sig_len;
	const char *pers = "0338566bf1a12c4a6f57964ec37dce473408f73efb2ad914c6cb4fa3bc7a668e"; //seed

	mbedtls_mpi mpiValue_R1, mpiValue_R2, mpiValue_n;

	mbedtls_mpi random_r, random_u, result_v, result_w, result_c, result_P;

    mbedtls_ecdsa_init(&ctx_sign);
    mbedtls_ecdsa_init(&ctx_verify);

    mbedtls_mpi_init(&mpiValue_R1);
    mbedtls_mpi mpi_r, mpi_c;
    mbedtls_mpi_init(&result_v);
    mbedtls_mpi_init(&mpi_r);
    mbedtls_mpi_init(&mpi_c);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    memset(sig, 0, sizeof(sig));
    memset(message, 0x25, sizeof(message));

    mbedtls_entropy_init(&entropy);
    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *) pers, strlen(pers));

    res = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);

    unsigned char r[32];
    mbedtls_ctr_drbg_random(&ctr_drbg, r, 32);

	if (mbedtls_mpi_read_string(&mpiValue_R1, 10, R1) != 0) {
			return 1;
	}

	copyBufferToMPI(r, 32, &mpi_r);
    //mbedtls_mpi_read_string(&mpi_r, 10, r);

    res = mbedtls_ecp_mul(&grp, &H, &mpi_r, &grp.G,  mbedtls_ctr_drbg_random, NULL);

    mbedtls_sha256(message, sizeof(message), hash, 0);
    copyMPIToBuffer(&mpi_c, hash, 32);
    add_mul_mod2(&mpi_r, &mpi_c, &mpiValue_R1, &grp.P, &result_v);

    ret = mbedtls_ecdsa_genkey(&ctx_sign, MBEDTLS_ECP_DP_SECP256R1, mbedtls_ctr_drbg_random, &ctr_drbg);

    mbedtls_sha256(message, sizeof(message), hash, 0);
    ret = mbedtls_ecdsa_write_signature(&ctx_sign, MBEDTLS_MD_SHA256, hash, sizeof(hash), sig, &sig_len, mbedtls_ctr_drbg_random, &ctr_drbg);

    ret = mbedtls_ecdsa_read_signature(&ctx_sign,hash, sizeof(hash), sig, sig_len);

//---------------end

*/

/*
	size_t olen;
	unsigned char buff[66];
	if (mbedtls_mpi_read_string(&mpiValue_R3, 10, R1) != 0) {
		return 1;
	}
	res = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);

	res = mbedtls_ecp_mul(&grp, &H, &mpiValue_R3, &grp.G, myrand, NULL);

	end = getCycleCount();
	cycles = end - start;
	PRINTF("proof time: %lu \r\n", cycles);

	res = mbedtls_ecp_point_write_binary(&grp, &H, MBEDTLS_ECP_PF_UNCOMPRESSED, &olen, buff, 66);
// -- end preprocessing --

	start = getCycleCount();

	//res = mbedtls_ecp_mul(&grp, &H, &mpiValue_R3, &grp.G, myrand, NULL);


	mbedtls_mpi mpiValue_R1, mpiValue_R2, random_r, random_u, result_v, result_w, result_c, result_P, mpiValue_n;

	mbedtls_mpi_init(&mpiValue_R1);
	mbedtls_mpi_init(&mpiValue_R2);
	mbedtls_mpi_init(&random_r);
	mbedtls_mpi_init(&random_u);

	mbedtls_ecp_point C;
	mbedtls_ecp_point_init(&C);
	mbedtls_ecp_point proof;
	mbedtls_ecp_point_init(&proof);

	mbedtls_mpi_init(&result_v);
	mbedtls_mpi_init(&result_w);
	mbedtls_mpi_init(&result_c);
	mbedtls_mpi_init(&result_P);
	mbedtls_mpi_init(&mpiValue_n);
	if (generateRandomNumbers(&random_r, &random_u) != 0) {
		PRINTF("Error in Random Process\n\n");
		return 1;
	}

	if (mbedtls_mpi_read_string(&mpiValue_R1, 10, R1) != 0) {
			return 1;
	}

	if (mbedtls_mpi_read_string(&mpiValue_R2, 10, R2) != 0) {
			return 1;
	}


	if (mbedtls_mpi_read_string(&mpiValue_n, 10, n) != 0) { //nonce
			return 1;
	}
	res = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);

	res = mbedtls_ecp_point_read_binary(&grp, &H2, buff, olen);

	res = mbedtls_ecp_check_pubkey(&grp, &H2);

	//mbedtls_mpi_lset(&result_c, 123456789);

	//res = mbedtls_ecp_mul(&grp, &g, &mpiValue_R1, &grp.G, myrand, NULL); //R_1 * G

//	res = mbedtls_ecp_mul(&grp, &h, &mpiValue_R2, &H2, myrand, NULL);   //R_2 * H

	res = mbedtls_ecp_muladd(&grp, &C, &mpiValue_R1, &grp.G, &mpiValue_R2, &H2);
	resetCycleCounter();
	start = getCycleCount();
	 // C = R_1 * G + R_2 * H Enrollment

	res = mbedtls_ecp_muladd(&grp, &proof, &random_r, &grp.G, &random_u, &H2);  // proof = r * G + u * H

	char sha256_result[32];
		// calculate value of c

	calculate_SHA256(result_P, mpiValue_n, sha256_result);
	//copyBufferToMPI(sha256_result, 32, &result_c);
	//char test[] = "2CF24DBA5FB0A30E26E83B2AC5B9E29E";

	mbedtls_mpi_read_string(&result_c, 16, sha256_result);

	add_mul_mod2(&random_r, &result_c, &mpiValue_R1, &grp.P, &result_v);
	add_mul_mod2(&random_u, &result_c, &mpiValue_R2, &grp.P, &result_w);
	/*
	if (calculate_SHA256(result_P, mpiValue_n, sha256_result) != 0) { //proof should be hashed
		PRINTF("Error Calculating SHA.\n\n");
		return 1;
	}

	if (copyBufferToMPI(sha256_result, 32, &result_c) != 0) {
		PRINTF("Error Copy Buffer.\n\n");
		return 1;
	}


	if (add_mul_mod2(&random_r, &result_c, &mpiValue_R1, &grp.P, &result_v)
			!= 0) {
		PRINTF("Error Calculating v.\n\n");
		return 1;
	}
	// calculate value of w
	if (add_mul_mod2(&random_u, &result_c, &mpiValue_R2, &grp.P, &result_w)  // w = u - cR2
			!= 0) {
		PRINTF("Error Calculating w.\n\n");
		return 1;
	}

	*/
	//---- verify -----
	// calculate sha -> C
	// proof == C^c
	/*
	end = getCycleCount();
	cycles = end - start;
	PRINTF("proof time: %lu \r\n", cycles);
	resetCycleCounter();
	start= getCycleCount();
	mbedtls_ecp_point gh, result; //g^v * h^w
	mbedtls_ecp_point_init(&gh);
	mbedtls_ecp_point_init(&result);
	mbedtls_mpi helper;
	mbedtls_mpi_init(&helper);
	mbedtls_mpi_lset(&helper, 1);

	calculate_SHA256(result_P, mpiValue_n, sha256_result);
	//copyBufferToMPI(sha256_result, 22, &result_c);
	mbedtls_mpi_read_string(&result_c, 16, sha256_result);
	res = mbedtls_ecp_muladd(&grp, &gh, &result_v, &grp.G, &result_w, &H2);  //g * v + h * w

	res = mbedtls_ecp_muladd(&grp, &result, &helper, &proof, &result_c, &C ); // d * 1+ c * e
	res = mbedtls_ecp_point_cmp(&gh, &result);
	end = getCycleCount();
	cycles = end - start;
	PRINTF("verify time: %lu \r\n", cycles);
	buff[65] ="\0";
	PRINTF("%s\n", buff);
	//double t = 0.123456;
	//PRINTF("%02.6fs\r\n",t);

	//PRINTF("Starting Enrollment...\r\n");
		//start = getCycleCount();
		//start_enrolment();
	end = getCycleCount();
	cycles = end - start;

	//PRINTF("Elapsed Encycles: %lu\r\n", cycles);
	//PRINTF("Elapsed time: %6.2fs\r\n", time);
	//PRINTF("\n\nCompleted.\n\n");
	PRINTF("Enrollment time: %lu \r\n", cycles);
	resetCycleCounter();
	for (int i = 0; i < 100; i++) {
	//PRINTF("\n\nStarting Prover...\r\n");
	//for (int i = 0; i< 100; i++) {
	start = getCycleCount();
	if (start_auth_prover() != 0) {
		PRINTF("Error in Prover Process\r\n");
		return 1;
	}
	end = getCycleCount();
	cycles = end - start;
	time = (double) cycles / coreFrequency;


	PRINTF("Prover time: %lu, %6.2fs\r\n", cycles, time);
	resetCycleCounter();
	//}
	//PRINTF("Elapsed time: %6.2fs\r\n", time);
	//PRINTF("\n\nCompleted.\n\n");
	//PRINTF("\n\nStarting Verifier...\r\n\n");
	start = getCycleCount();
	if (start_auth_verifier() != 0) {
		PRINTF("Error in Verifier. \r\n");
		return 1;
	}
	end = getCycleCount();
	cycles = end - start;
	time = (double) cycles / coreFrequency;
	PRINTF("Verifier time: %lu, %6.2fs\r\n", cycles, time);
	resetCycleCounter();
	}
	/*
	PRINTF("Elapsed cycles: %lu\r\n", cycles);
	PRINTF("Elapsed time: %6.2fs\r\n", time);
	PRINTF("\n\nCompleted.\n\n");

	PRINTF("\n\nStarting Verifier...\r\n\n");
	/*
	end = getCycleCount();

	// Calculate the elapsed cycles
	cycles = end - start;
	double time = (double) cycles / coreFrequency;

	PRINTF("Elapsed cycles: %lu\r\n", cycles);
	PRINTF("Elapsed time: %6.2fs\r\n", time);
	PRINTF("\n\nCompleted.\n\n");
	*/
	return 0;
}
