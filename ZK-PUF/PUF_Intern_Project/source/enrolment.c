#include "utils.h"

mbedtls_mpi result_Da;

int start_enrolment() {

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
	mbedtls_mpi_init(&result_Da);

	int ret;

	ret = update_var_for_exp_mod(&mpiValue_g, &mpiValue_R1, &mpiValue_h,
			&mpiValue_R2, &mpiValue_p, &mpiValue_n);
	if (ret != 0) {
		PRINTF("Failed to calculate exp_mod in enrollment . Error code: %d\n",
				ret);
		return 1;
	}

	ret = calculate_exp_mod(mpiValue_g, mpiValue_R1, mpiValue_h, mpiValue_R2,
			mpiValue_p, &result_Da); //calculate value of Da
	if (ret != 0) {
		PRINTF("Failed to calculate exp_mod in enrollment . Error code: %d\n",
				ret);
		return 1;
	}

	mbedtls_mpi_free(&mpiValue_g);
	mbedtls_mpi_free(&mpiValue_h);
	mbedtls_mpi_free(&mpiValue_R1);
	mbedtls_mpi_free(&mpiValue_R2);
	mbedtls_mpi_free(&mpiValue_p);
	mbedtls_mpi_free(&mpiValue_n);

	return 0;
}
mbedtls_mpi get_Da() {
	return result_Da;
}

