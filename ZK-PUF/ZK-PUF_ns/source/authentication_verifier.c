/*
 * authentication_verifier.c
 *
 *  Created on: 14 Jun 2023
 *      Author: muhammad
 */
#include "authentication_verifier.h"

int start_auth_verifier() {

	mbedtls_mpi mpiValue_R1;
	mbedtls_mpi mpiValue_R2;
	mbedtls_mpi mpiValue_p;
	mbedtls_mpi mpiValue_g;
	mbedtls_mpi mpiValue_h;
	mbedtls_mpi mpiValue_n;
	mbedtls_mpi result_c;
	mbedtls_mpi result_COMP_01;
	mbedtls_mpi result_COMP_02;

	mbedtls_mpi Da;
	mbedtls_mpi P;

	mbedtls_mpi_init(&mpiValue_R1);
	mbedtls_mpi_init(&mpiValue_R2);
	mbedtls_mpi_init(&mpiValue_p);
	mbedtls_mpi_init(&mpiValue_g);
	mbedtls_mpi_init(&mpiValue_h);
	mbedtls_mpi_init(&mpiValue_n);
	mbedtls_mpi_init(&result_c);
	mbedtls_mpi_init(&result_COMP_01);
	mbedtls_mpi_init(&result_COMP_02);
	mbedtls_mpi_init(&Da);
	mbedtls_mpi_init(&P);

	if (update_var_for_exp_mod(&mpiValue_g, &mpiValue_R1, &mpiValue_h,
			&mpiValue_R2, &mpiValue_p, &mpiValue_n) != 0) {
		PRINTF("Error updating values \r\n");
		return 1;
	}

	// calculate value of c
	unsigned char sha256_result[32];
	if (calculate_SHA256(get_Pover_Result_P(), mpiValue_n,
			sha256_result) != 0) {
		PRINTF("Error Calculating c \r\n");
		return 1;
	}

	if (copyBufferToMPI(sha256_result, 32, &result_c) != 0) {
		PRINTF("Error in Copy Buffer\r\n");
		return 1;
	}

	if (calculate_exp_mod(mpiValue_g, get_Pover_Result_v(), mpiValue_h,
			get_Pover_Result_w(), mpiValue_p, &result_COMP_01) != 0) {
		PRINTF("Error Calculating mod \r\n");
		return 1;
	}

	char buffer[1024];
	size_t t, t1;
	mbedtls_mpi_write_string(&result_COMP_01, 10, buffer, sizeof(buffer), &t);
	//PRINTF("Result 01: %s\r\n\n", buffer);

	Da = get_Da();
	P = get_Pover_Result_P();

	if (mbedtls_mpi_exp_mod(&result_COMP_02, &Da, &result_c, &mpiValue_p,
	NULL) != 0) {
		PRINTF("Error Calculating mod \r\n");
		return 1;
	}


	/*
	if (mbedtls_mpi_mul_mpi(&result_COMP_02, &result_COMP_02, &P) != 0) {   //P is Prover P
		PRINTF("Error multiplication \r\n");
		return 1;
	}
	if (mbedtls_mpi_mod_mpi(&result_COMP_02, &result_COMP_02, &mpiValue_p) //mpiValue_p mod p
			!= 0) {
		PRINTF("Error Calculating mod2 \r\n\n");
		return 1;
	}
	*/
	char buffer2[1024];
	mbedtls_mpi_write_string(&result_COMP_02, 10, buffer2, sizeof(buffer2),
			&t1);
//	PRINTF("Result 02: %s\r\n\n\n", buffer2);

	mbedtls_mpi_free(&mpiValue_R1);
	mbedtls_mpi_free(&mpiValue_R2);
	mbedtls_mpi_free(&mpiValue_p);
	mbedtls_mpi_free(&mpiValue_g);
	mbedtls_mpi_free(&mpiValue_h);
	mbedtls_mpi_free(&mpiValue_n);
	mbedtls_mpi_free(&result_c);
	mbedtls_mpi_free(&result_COMP_01);
	mbedtls_mpi_free(&result_COMP_02);
	//mbedtls_mpi_free(&Da);
	mbedtls_mpi_free(&P);


	return 0;
}
int verify_ECC(mbedtls_ecp_group *grp, mbedtls_ecp_point *g, mbedtls_ecp_point *h, mbedtls_ecp_point *proof, mbedtls_ecp_point *C, mbedtls_mpi *result_v, mbedtls_mpi *result_w, mbedtls_mpi *nonce) {

	unsigned char sha256_result[32];
	mbedtls_ecp_point gh, result; //g^v * h^w
	mbedtls_ecp_point_init(&gh);
	mbedtls_ecp_point_init(&result);
	mbedtls_mpi helper, result_c;
	mbedtls_mpi_init(&helper);
	mbedtls_mpi_init(&result_c);
	mbedtls_mpi_lset(&helper, 1);

	//calculate_SHA256(result_P, mpiValue_n, sha256_result);
		//copyBufferToMPI(sha256_result, 22, &result_c);
	//mbedtls_mpi_read_string(&result_c, 16, sha256_result);
	int res;
	size_t olen;
	unsigned char buff[100];
	mbedtls_ecp_point_write_binary(grp, C, MBEDTLS_ECP_PF_UNCOMPRESSED, &olen, buff, sizeof(buff));
	unsigned char buff2[olen + mbedtls_mpi_size(nonce)];
	memcpy(buff2, buff, olen);
	mbedtls_mpi_write_binary(nonce, buff2 + olen, sizeof(buff2) - olen);
	sha256Hash(buff2, sizeof(buff2), sha256_result);
	mbedtls_mpi_read_string(&result_c, 16, sha256_result);

	res = mbedtls_ecp_muladd(grp, &gh, result_v, g, result_w, h);  //  g * v + h * w

	res = mbedtls_ecp_muladd(grp, &result, &helper, proof, &result_c, C); // d * 1 + c * e
	res = mbedtls_ecp_point_cmp(&gh, &result);
	if (res != 0) {
		return 1;
	}
	return 0;
}

