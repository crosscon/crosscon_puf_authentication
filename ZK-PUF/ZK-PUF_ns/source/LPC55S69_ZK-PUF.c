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
#include "veneer_table.h"


#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/sha256.h"
#include "fsl_puf.h"



#define mbedtls_printf PRINTF


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


int main(void) {

	status_t status;
	 __attribute__((aligned(16))) uint8_t activation_code[PUF_ACTIVATION_CODE_SIZE];

	 puf_config_t pufConfig; // PUF configuration structure
	 pufConfig.dischargeTimeMsec = 400; // Corrected field name for discharge time
	 pufConfig.coreClockFrequencyHz = CLOCK_GetFreq(kCLOCK_CoreSysClk); // Corrected field name for core clock frequency

	/* Init board hardware. */
	BOARD_InitBootPins();
	BOARD_InitBootClocks();
	BOARD_InitBootPeripherals();
	BOARD_InitDebugConsole();
	#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
		/* Init FSL debug console. */

	#endif

	BOARD_InitDebugConsole();
	// Enable and reset the cycle counter
	enableCycleCounter();
	resetCycleCounter();
	// Start the timer
	int res;

	PRINTF("Starting PUF Key Generation Example...\r\n");

	// Initialize the PUF
	status = PUF_Init(PUF, &pufConfig);
	if (status != kStatus_Success) {
		PRINTF("Error: PUF initialization failed!\r\n");
		return -1;
	}
	PRINTF("PUF Initialized Successfully.\r\n");

	// Enroll the PUF
	memset(activation_code, 0, sizeof(activation_code));
	status = PUF_Enroll(PUF, activation_code, sizeof(activation_code));
	if (status != kStatus_Success) {
		PRINTF("Error: PUF enrollment failed!\r\n");
		return -1;
	}
	PRINTF("PUF Enroll successful. Activation Code created.\r\n");

	// Start the PUF
	PUF_Deinit(PUF, &pufConfig); // Proper deinitialization
	status = PUF_Init(PUF, &pufConfig);
	status = PUF_Start(PUF, activation_code, sizeof(activation_code));
	if (status != kStatus_Success) {
		PRINTF("Error: PUF start failed!\r\n");
		return -1;
	}
	PRINTF("PUF started successfully.\r\n");


	mbedtls_ecp_group grp;
	mbedtls_ecp_point h, C;
	init_ECC(&grp, &h, &C);
	res = enroll_ECC(&grp, &h, &C);

	if (res != 0) {
			PRINTF("enrollment failed\n");
		} else {
			PRINTF("enrollment successful\n");
	}


	mbedtls_ecp_point proof;
	mbedtls_ecp_point_init(&proof);
	mbedtls_mpi result_v, result_w, nonce;
	mbedtls_mpi_init(&result_v);
	mbedtls_mpi_init(&result_w);
	mbedtls_mpi_init(&nonce);
	res = authenticate_ECC(&grp, &grp.G, &h, &proof, &C, &result_v, &result_w, &nonce );

	if (res != 0) {
		PRINTF("authentication failed\n");
	} else {
		PRINTF("authentication successful\n");
	}

	res = verify_ECC(&grp, &grp.G, &h, &proof, &C, &result_v, &result_w, &nonce );

	if (res != 0) {
		PRINTF("verification failed\n");
	} else {
		PRINTF("verification successful\n");
	}

	return 0;
}
