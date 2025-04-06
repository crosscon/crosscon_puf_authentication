/*
 * enrollment.h
 *
 *  Created on: 13 Jun 2023
 *      Author: muhammad
 */

#ifndef INCLUDE_ENROLMENT_H_

#include "mbedtls/bignum.h"
#define INCLUDE_ENROLMENT_H_

#define IS_WRITE_TO_FLASH_ENABLED 1
#define IS_ENROLMENT 1

int start_enrolment();
mbedtls_mpi get_Da();

#endif /* INCLUDE_ENROLMENT_H_ */
