

/**
 * @file    LPC55S69_PAWOS.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "LPC55S69_cm33_core0.h"
#include "fsl_debug_console.h"
#include "core_json.h"

#include <math.h>
#include <stdint.h>
#include "fsl_hashcrypt.h"
#include "mbedtls/sha256.h"

#define PADDING_KEY_GEN 4


typedef struct params {
    unsigned int n;
    unsigned int w;
    unsigned int log_w;
    unsigned int len_1;
    unsigned int len_2;
    unsigned int len;
};


static double Log2( double n )
{
    // log(n)/log(2) is log2.
    return log( n ) / log( 2 );
}


static void ull_to_bytes(unsigned char* out, unsigned int outlen,
    unsigned long long in)
{
    int i;
    for (i = outlen - 1; i >= 0; i--) {
        out[i] = in & 0xff;
        in = in >> 8;
    }
}

void hex_to_bytes_array(char* src, uint8_t* dest, size_t count) {
  size_t i;
  int value;
  for (i = 0; i < count && sscanf(src + i * 2, "%2x", &value) == 1; i++) {
        dest[i] = value;
    }
}

void puf(uint8_t challenge[32], uint8_t response[32]) {

	calc_sha_256(response, challenge, 32);
}

static void base_w(struct params* param, int* output, const int out_len, const unsigned char* input)
{
    int in = 0;
    int out = 0;
    unsigned char total;
    int bits = 0;
    int consumed;
    int log_w = param->log_w;

    for (consumed = 0; consumed < out_len; consumed++) {
        if (bits == 0) {
            total = input[in];
            in++;
            bits += 8;
        }
        bits -= log_w; // 4
        output[out] = (total >> bits) & (param->w - 1); //w = 16
        out++;
    }
}



static void wots_checksum(struct params *params, int* csum_base_w, const int* msg_base_w)
{
    int csum = 0;
    const unsigned int len = params->len_1;
    unsigned char csum_bytes[(len * params->log_w + 7) / 8];
    unsigned int i;

    // Compute checksum.
    for (i = 0; i < params->len_1; i++) {
        csum += params->w - 1 - msg_base_w[i];
    }

    //Convert checksum to base_w.
    //Make sure expected empty zero bits are the least significant bits.
    csum = csum << (8 - ((params->len_2 * params->log_w) % 8));
    ull_to_bytes(csum_bytes, sizeof(csum_bytes), csum);
    base_w(params, csum_base_w, params->len_2, csum_bytes);
}

static void chain_lengths(struct params* param, int *lengths, const unsigned char *msg)
{
    base_w(param, lengths, param->len_1, msg);
    wots_checksum(param, lengths + param->len_1, lengths);
}

void len(struct params *param) {
    int len1 = ceil(8*param->n/ param->log_w);
    param->len_1 = len1;
    int len2 = floor( Log2(len1 * (param->w - 1)) / param->log_w) + 1;
    param->len_2 = len2;
    param->len = len1 + len2;
}
void calc_sha_256(uint8_t output[], const uint8_t* input, size_t len) {
	size_t outlen = 32;
	mbedtls_sha256_context sha256_ctx;
	mbedtls_sha256_init(&sha256_ctx);
	mbedtls_sha256_starts_ret(&sha256_ctx, 0);

	mbedtls_sha256_update_ret(&sha256_ctx, input, len);
	mbedtls_sha256_finish_ret(&sha256_ctx, output);
	mbedtls_sha256_free(&sha256_ctx);

}


void PRF(struct params *param, uint8_t *out, uint8_t *input, uint8_t *key) {
    uint8_t buf[2*param->n + 32];
    //padding for hash function
    ull_to_bytes(buf, 32, 4);
    memcpy(buf + 32, key, param->n);
    memcpy(buf + 32 + param->n, input, param->n);
    calc_sha_256(out, buf, 2*param->n + 32);
}

void expand_seed(struct params *param, uint8_t *puf, uint8_t *out) {


    // out has to be len * n bytes
    uint8_t sk[param->n * param->len];
    uint8_t counter[32];
    //memcpy(counter, challenge , 32);
    for (int i = 0; i < param->len; i++) {
        ull_to_bytes(counter, 32 , i);
        //PRF(param, sk + i * param->n, counter, puf);
        PRF(param, out + i * param->n, counter, puf);
    }
    //memcpy(out, sk, param->len);
}

int verify_inclusion(char *path_json, size_t path_length, uint8_t root[32], uint8_t leaf[32]) {

	char rule[] = "rule";
    size_t ruleLength = sizeof( rule ) - 1;
    char path[] = "path";
    size_t pathLength = sizeof ( path ) - 1;
    char * rule_value;
    size_t rule_valueLength;
    char * path_value;
    size_t path_valueLength;
            // Calling JSON_Validate() is not necessary if the document is guaranteed to be valid.
   // result = JSON_Validate( buffer2, bufferLength );

    JSONStatus_t result;
    result = JSON_Validate(path_json, path_length);

    if( result == JSONSuccess )
    {
        // result = JSON_Search( buffer2, bufferLength, query, queryLength,
        //                              &value, &valueLength );
   	 result = JSON_Search(path_json, path_length, rule, ruleLength, &rule_value, &rule_valueLength);
   	 result = JSON_Search(path_json, path_length, path, pathLength, &path_value, &path_valueLength);
     }

    int rule_Length = (rule_valueLength-1) / 2;
    int path_Length = (path_valueLength-1) / 65;
    uint8_t rule_res [rule_Length];
    uint8_t path_res [path_Length][32];


    int idx = 1;
    int i;
    for(i = 0; i < rule_Length; i++) {
   	 rule_res[i] = *(rule_value + idx) - '0';
   	 idx = idx + 2;
    }
    path_value += 2;
    for(i = 0; i < path_Length - 1; i++) {

   	 hex_to_bytes_array(path_value, path_res[i], 64);
   	 path_value += 67;
    }
    hex_to_bytes_array(path_value, path_res[path_Length - 1], 64);


    if (memcmp(leaf, path_res[0], 32) != 0) {
    	return 1;
    }
    int res = verify_path(rule_res, path_res, rule_Length, root);

    return res;
}




int verify_path(uint8_t rules[], uint8_t path[][32], size_t len, uint8_t root[]) {


	uint8_t res[32];
	int index = 0;
	uint8_t bit = rules[0];
	uint8_t input[65];
	uint8_t digest[32];
	memcpy(res, path[0], 32);
	while (index < len - 1)
	{
		uint8_t next_bit = rules[index+1];
		memcpy(digest, path[index + 1], 32);

		switch (bit) {
	        case 0:
	            memset(input, 0x01, 1);
	            memcpy(input + 1 , res, 32);
	            memcpy(input + 33, digest, 32);
	            calc_sha_256(res, input, 65 );
	            break;
	        case 1:
	            memset(input, 0x01, 1);
	            memcpy(input + 1, digest, 32);
	            memcpy(input + 33, res, 32);
	            calc_sha_256(res, input, 65 );
	            break;
	        default:
	            break;
	        }
	        bit = next_bit;
	        index++;
	    }
	if(memcmp(root, res, 32) == 0) {
		return 0;
	}
	return 1;
}


void gen_chain(struct params *param, uint8_t *out, uint8_t *in, int start, int steps)
{
    uint8_t result[param->n];
    memcpy(out, in, param->n);
    for (int i = start; i < (start+steps) && i < param->w; i++) {

        calc_sha_256(out, out, param->n);
    }
}

void gen_pk(struct params *param, uint8_t *pk , uint8_t *puf) {


    expand_seed(param, puf, pk);
    for (int i = 0; i < param->len; i++ ) {

        gen_chain(param, pk + i*param->n, pk + i*param->n, 0 , param->w-1);
    }
}


void authenticate(struct params *param, uint8_t *sign, uint8_t *msg, size_t msg_len, uint8_t challenge[32]) {

    int length[param->len];
    uint8_t msg_hash[32];
    uint8_t puf_response[32];
    calc_sha_256(msg_hash, msg, msg_len);
    puf(challenge, puf_response);
    chain_lengths(param, length, msg_hash);

    expand_seed(param, puf_response, sign);

    for (int i = 0 ; i < param->len; i++ ) {

        gen_chain(param, sign + i*param->n, sign + i*param->n, 0, length[i]);

    }

}

int verify(struct params *param, uint8_t *sign, uint8_t *msg, uint8_t ID[32], char *path, size_t path_length) {

    int length[param->len];
    uint8_t out2[param->len * param->n];
    uint8_t pk_ver[32];
    chain_lengths(param, length, msg);

    for (int i = 0; i < param->len; i++) {
        gen_chain(param, out2 + i*param->n, sign + i*param->n, length[i], param->w - 1 - length[i]);
    }
    calc_sha_256(pk_ver, out2, sizeof(out2));

    uint8_t leaf[33];
    memset(leaf, 0, 33);
    memcpy(leaf+1, pk_ver, 32);
    calc_sha_256(pk_ver, leaf, sizeof(leaf));
    int res;
    res = verify_inclusion(path, path_length, ID, pk_ver);
    return res;
}

void enroll(struct params *param, uint8_t challenges[][32], int challenge_number, uint8_t enrolled_pks[][32]) {

	uint8_t puf_response[32];
	uint8_t out[param->len * param->n];
	for(int i=0;i<challenge_number;i++) {

		puf(challenges[i], puf_response);
		gen_pk(param, out, puf_response);
		calc_sha_256(enrolled_pks[i], out, sizeof(out));

	}
}




int main(void) {

    /* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();
#endif


    struct params param;
    param.n = 32;
    param.w = 16;
    param.log_w = 4;
    param.len_1 = 0;
    param.len_2 = 0;
    param.len   = 0;

    len(&param);

    uint8_t challenges[8][32] = {
        {0x1a, 0x2b, 0x3c, 0x4d, 0x5e, 0x6f, 0x70, 0x81, 0x92, 0xa3, 0xb4, 0xc5, 0xd6, 0xe7, 0xf8, 0x09,
         0x1a, 0x2b, 0x3c, 0x4d, 0x5e, 0x6f, 0x70, 0x81, 0x92, 0xa3, 0xb4, 0xc5, 0xd6, 0xe7, 0xf8, 0x09},
        {0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87, 0x78, 0x69, 0x5a, 0x4b, 0x3c, 0x2d, 0x1e, 0x0f,
         0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87, 0x78, 0x69, 0x5a, 0x4b, 0x3c, 0x2d, 0x1e, 0x0f},
        {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
         0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88},
        {0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89,
         0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89},
        {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
         0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f},
        {0x1f, 0x1e, 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10,
         0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00},
        {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99,
         0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99},
        {0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00, 0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa,
         0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00, 0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa}
    };

    uint8_t enrolled_pk[8][32];

    char msg [] = "Hello CROSSCON!";

    enroll(&param, challenges, 8, enrolled_pk);

    uint8_t sig[param.len * param.n];

    authenticate(&param, sig, msg, strlen(msg), challenges[0]);

    uint8_t msg_hash[32];
    calc_sha_256(msg_hash, msg, strlen(msg));
    uint8_t ID_bytes[32];
    char *ID = "b0396bfb767f7025112092941682916eab3afeba9d0e62e037f5147679e66f70";
    hex_to_bytes_array(ID, ID_bytes, 64);


    char *path_example_1 = "{\"rule\":[1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],\"path\":[\"f6720bf8805333f5861c6ff89834145f678e7dcdb09fcc5157d41573efdac0ee\",\"4c6bf817639562abeec7d3a2a6d4d2aaf3e1e818e0ff82cd04a43463ff84f6d6\",\"5f54a87fb83d5d3dc498468618058593313e81beaba80a0478a351d340e055fd\",\"3564219d0cae703c77ece4538e8fc21d928640f28316ec63a19551adf1169fcc\",\"4b3116d1b02d2fd73e0b3dd2d6eefdc06baa64814f46688b8bc0edf8564480c1\",\"3150f0cfdcb941ace16aa6578607aba02ff80cb146768bc5fd6d67d2feacacfa\",\"24c7046993ad5683916db4e0d140f49254ab612b9c924da95497ad7b2070208c\",\"f77400a07e1a24308a7249d67ed0df4bf2bde580aa3a14abf9248c274fa173d0\",\"da5e5524dd50811a1e22f07631dcf7e417ae611a9dc435cebae326160882f10f\",\"7aa340542e158ba69e2dba06f7088ef1594175e57e33f16467a293d4179011b4\",\"abe7990a90a20b3cefd71a8395f94a97d23e501e6bc98a24dc153adb0dea268b\",\"d8556a512bd7e78e78a1dbaf542f3021e4f7cbb654b9f00ed13aa43eebd865f2\",\"ad222a6f1da200786d2dc99e0360fa71be1e0d7db4f22357de577e78e338dd99\",\"624796462038b981b36a93640f41d1641b6caeeb9110045bd745420625ddc9cc\",\"72262c5f9994f2c377b49964069b9f73b8d767a19949d94dfdc3d89dcb8b58de\",\"322fec7ff4ea1c871e619c1ff3f63d93dc369ea9fcaec1761c37370f5feefeac\",\"9ac83739e6cd9b82cc7bc67419764eddb6498a3a6c5e4c99a1f2fba2364922d8\",\"694e48e1f35e64cb26250fef08a7936b49bbf9aa2df7ec3a3d0f637bd4966ad8\",\"0c72e43db79e2318d70d820b9b959ca20ec673a983a0e636e92bf3c8a9a2d82f\",\"1e84cfd26637f6b60cfb91611acfda2f4ba077cff0a6b3374f63b23441792bf4\",\"4676fdcedd4b14cf01e59011f9930081194a71309528a4edeeffa5729a4513aa\",\"6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d\"]}";
    /* Examplary path for other challenges (2-8)
    char *path_example_2 = "{\"rule\":[0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0],\"path\":[\"0a8c5a92df92b06b752c63bd711c064cb0fd208768bbade545a8d1c1334fddf4\",\"3655ec0611457a976552faae87693e4f8dc1443aac7f8f6f6cf0017dcc172541\",\"2508764af26d36524e57c293ea730d5f8daa29a0dabb83e2635cbfa7fbf087ab\",\"03f6a4ea2101b25108cb9ddbdfed245e1c505bc3c0a8cacda6f687bd92a93a9e\",\"b631e924b91bef2a3a3fd782600306504e53c781c34dc2d9a3817c3e74c4449f\",\"8407e15d11557a222dd683f3250df2d5a189598153cab9582eb4583e53889c3b\",\"3560fe20658206a8945fa14d5ed600649725c9218e6d75ffc74729eb8f1faac4\",\"8c2bd77f43f71320797199e0fb173c1975c4984b7b7ca0559c13088eefef467c\",\"46e03a0781b1f1a4477a9af7e2211a3e3a387dea8a2fddd3145f48097b314629\",\"581fe9559db841e1ea27f8185bc942f54b289577f34b48b670d7f16fd576b436\",\"1593a6e6091a178e1ac3f641b5302de434fb2b961dabf633dc39b655ec3747c6\",\"a179eb246997e5d449b99eaf54ef96827dbf9942e0251726719ff79eef22b92a\",\"b9651b739805bf8c9f0f528cf3c2103466cf3e9f615549cfffaf687c96412bc7\",\"46fc4019e16f59936470fd41e4a4eca66b7df997c5f77b4749f99666591f25c5\",\"7529ccbfe560b836cea362d4c0a6bad99e7629830a2f33b4275835bfecbab691\",\"64312421538f6d61c2b6e4f432b9e49c9340c40c8ef096484837b0a8b6d703b2\",\"0ba883336b77584ed502ee36280946c5be82f5226243cea9f02d9fc3192f331c\",\"9df21fb98e4bc9e1107c6d38de94b978ee6901517672fa8ecffcab14c273f637\",\"0c72e43db79e2318d70d820b9b959ca20ec673a983a0e636e92bf3c8a9a2d82f\",\"1e84cfd26637f6b60cfb91611acfda2f4ba077cff0a6b3374f63b23441792bf4\",\"4676fdcedd4b14cf01e59011f9930081194a71309528a4edeeffa5729a4513aa\",\"6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d\"]}";
    char *path_example_3 = "{\"rule\":[0,1,0,0,0,1,1,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0],\"path\":[\"6ef9158e29d10da5a6f82e73fa802d69063686a573270e10d8968f3684bacda5\",\"2c765ab9aaddfefb0f2a7ce6f5506ef1c090777e2811bf7d79d0b834f675f6fb\",\"fafe0abdf5858b994ba79ab8720f8898d19c70a1e1ded37b26167c8210893346\",\"8e0b5ed45026b56525d5650bd42f6fcb2eb3995eab962bf8251b5c05c25d91e2\",\"27c11ce4b69d6ce5ac21421c9c6a7144ba401f3f8a718fdb7ca615f584141f2c\",\"f17108c57bbf096f27ce521dc9cc366d8935aab84435f84abd5c26da17bf655b\",\"c7f79fa6773cb70f2f8c10588c0bfecf16a0fc240a913b667f6fcc1d8b0c9b63\",\"af8db37322ff2cf57491f0db3f462c58bde9ac8b6e29107f445c20bcbc98c285\",\"c499a332a82b062ad705e87bff83bc5c944ec8062f1c4f6a1e3803d22f7d5bf3\",\"53c096ee30fd011a5cf22c37e32d61ffe61d604d99754e4a86f7f3c3e7e1b6e4\",\"1593a6e6091a178e1ac3f641b5302de434fb2b961dabf633dc39b655ec3747c6\",\"a179eb246997e5d449b99eaf54ef96827dbf9942e0251726719ff79eef22b92a\",\"b9651b739805bf8c9f0f528cf3c2103466cf3e9f615549cfffaf687c96412bc7\",\"46fc4019e16f59936470fd41e4a4eca66b7df997c5f77b4749f99666591f25c5\",\"7529ccbfe560b836cea362d4c0a6bad99e7629830a2f33b4275835bfecbab691\",\"64312421538f6d61c2b6e4f432b9e49c9340c40c8ef096484837b0a8b6d703b2\",\"0ba883336b77584ed502ee36280946c5be82f5226243cea9f02d9fc3192f331c\",\"9df21fb98e4bc9e1107c6d38de94b978ee6901517672fa8ecffcab14c273f637\",\"0c72e43db79e2318d70d820b9b959ca20ec673a983a0e636e92bf3c8a9a2d82f\",\"1e84cfd26637f6b60cfb91611acfda2f4ba077cff0a6b3374f63b23441792bf4\",\"4676fdcedd4b14cf01e59011f9930081194a71309528a4edeeffa5729a4513aa\",\"6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d\"]}";
    char *path_example_4 = "{\"rule\":[1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0],\"path\":[\"b2b72d45edb6e0637dddf64bbd4d57695207677da87b148fd96d8d7fd65b7bb9\",\"53ead7a0f3f753d3e70c51d2fc82a063da3cca5220f4ccbe58ad5207f6292f54\",\"7003ed6abfa9bfe13ac53caf2bbbb48db1933461313b6458b93c1122c421236e\",\"13aae2b866534b24a31f5d2deab79761b092ee737c07e1962ded5c30995ee049\",\"71894dda04d08098c26554e030d0b14c31f14792b51c34fd94bdaef65875a01b\",\"4a7253f01e1657e6aefe7d2f6e8d3b3e689c2d71e072c2be93cce682fcef9d1e\",\"dd2e30069d6c9e4fa6a98c67257fc7a12c6707db23cab48365a481ad9cca4329\",\"f19ab92c993d3a0c16cabd79ec3f0990754b4ee17e334f23ac827d34030a4246\",\"04645dbd6eca1038f16d39c2e576bcf782d21278663afca251024e6d1c64a0ca\",\"dfbb16d5ce82f74d96f4e15ffa4a0727cffe6fd93f9303659b0e688ff4c19901\",\"59938b6220ba8882c161a9fcae2144e6a70279b1e4cfb208d5b51bb02bd5c889\",\"79e0a1b1e92642492a776d0fdd207949371ad4011eb040da3390d27cc1daf135\",\"6856a124a655b5c21a869ef151b7fb64d0515d2c8f2cb8e6e95327d83cf2f1a7\",\"06ad3cba3b5292a6772f692b4946a6a3eaeb2e0ae23d97ae43746abd38b1121b\",\"010fea2dd7155bb2846dd15743cc262f2b9e179f3caff15aa966d489ea5401ff\",\"a57eb32a0097e41ab73edd02d644094eb939a8ffbaf3f4a40ff3c60201eedfa4\",\"27067f50f5ae1a28e92fcd7bdb4020750143d837379c495fa5091d07ec3c85eb\",\"7a8ac984c3952cbeecf371ef8d644317aa2f3ca97681fa9201e9b4b7a38e63d5\",\"851238ee50336f13fc69707c86e35c48b052c8b63b14bf879e52e6d5822bf0a9\",\"fcb4d0075d93b445af447346e1e5ab221ac84f9bad3ea8adcb0aa64a3208e69e\",\"4676fdcedd4b14cf01e59011f9930081194a71309528a4edeeffa5729a4513aa\",\"6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d\"]}";
    char *path_example_5 = "{\"rule\":[0,1,0,1,1,1,1,1,1,0,0,1,0,0,1,0,0,0,0,1,0,0],\"path\":[\"ca1697c233160c827dcd0c27a6016857aca34d6f84d9a499cea772d4de19fc9d\",\"b50234a4d59e0a7fda1e3bf8d87e9fe20cff182309590e0541943ffa62a532d8\",\"bddaed74d06551553508d2554fd5d835d33c6ddf4d3e4b34892fb5a55ef38dc9\",\"3c3c3fc4d1476cddcb0f6d4247e935facf31bbc4b47825fc79d541649c23723c\",\"5da4091738811f3116b2b85b7e976235c7113229347a4951b306bf30b75378d7\",\"2086714f3e401fd451781c001b7d57e5cba4f9667ee05f4deef928b7bedf9364\",\"d6e9ce8f856075c344f859748087ff708154b3b51b589ed9fc11707d5ab92f94\",\"be2afe86d4adb6d5e59a16db254afc29dd840d2e1c96717ad632954110648c71\",\"fba972dfb6e060d1737d3555086aadb071617014467927d071d4b70283fd4044\",\"c1835ae91ae782b004fbcee8458181135315bede5f2e55ebcdedb764b33a2e3f\",\"18fca089ee487e2fa3c1de4cd7d1138d4d1e5b0ff60610740e237a34fcd0032f\",\"b89a9b00bc5595a8cb26a2310b5524bc3376f89c73f29a48f39191404f03c133\",\"ae96d64c6ae1e1e778a7c17a0c4ec3c6a152488219efc2608b0a11feb5ea5eeb\",\"fcf76fb6cab306128a9d66d892d8954f266e13bc50d963976710c377324dbf1b\",\"861a8f4e718d285dd7d9c518e4c9f8407cfaadf601b59c5a42bdfa5899d77d6e\",\"1129e8d73b34e81bb0a55563eb23aad6e13a83729e87a64f33315e91e4f6320c\",\"b582e685dae161c235332698f67a76cffc3aedcd86a8d939d9c57fef9396c1b2\",\"f3c0e7f26be1873645830f18ce082f6ec908ba35c88028773723802dd708a056\",\"ce1b5fb7e0102ae27ab3add727f3e7f7c2f679110199d9587a7fddb67010a4ed\",\"a816e9772a5bc0ae36c5b28131e8793b21454f8c1b67c82757b8f5df50791b05\",\"24ed73f44cdea1a14846de8b184894623c08a03ee4d2c49952f126c302790c84\",\"6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d\"]}";
    char *path_example_6 = "{\"rule\":[1,1,0,0,1,1,0,1,0,0,0,0,0,1,1,0,0,1,0,1,0,0],\"path\":[\"7241ddc6b3a071792e2d1356588debfec3b6dae9de19522cdb3ab078693e3249\",\"a7e0188d45c9b2c22ef5a2998634c7404c0be9d6c0fc5daacd081b6b63e8464f\",\"52efabf0e64f149a4e94a1d046377a12f362d6127e7017854f5f3f692ad413ca\",\"f3679a267421513fe8922ae3d44f5de324e4d06896c6151abafbf26a483bbb72\",\"8db31efd5a5f1cd8f0cebef416833abf05a232a90a3d04d421674f38de2ac717\",\"5002289587a7e6a1c86199e3b7c968b2fd0c6eb54c317bded74886e0611f0283\",\"7bb19010752d049e85e47df3000ada0e2a6a738c01ff22ea395931dab63a2a91\",\"50d81b48c88f8ecf7a1dee2f64a337656e95bf4770f9384e8b3b35ad77f4751b\",\"fefe0faed4a07f9e7f2569c3daa98f533bb0c973d3fa4c340268b9075b094c0e\",\"7faaaf1582f0211a49e3be188f1e751ad9cd66a0e4bf1d05259eb4d852529f03\",\"3f9e6528d0aff878d8969fa4f66bd714a1d2de0be79cbde5922a240a5e16f84a\",\"d1564852d1435414aa3121d6f5848f7e8099cddd8cb94687b2ec7649ac6ef9dd\",\"37133caedf901c8f60b5cb81b2da475b36b30b481730088c4e14983240003559\",\"b2ab1580c73e5d4f810bda5088289cc27a8d6f9647d02e6480b51aa1a5868dbf\",\"e888638bda614a0f1f21b2fd42e8b7bcf4d4018c78a3c0ba8b4df0ecf3e3c02f\",\"8255bdbf50cf7b9bf4edba1598555aecb36d1c9d2996cd7625ab873983076ebf\",\"6b0d79e38ee7c924f069562d7841ccf5ce219913bd96eb0ff5436302c5a22acd\",\"1614e55fa291d97108953d3236d458b82a56165253392a1c02e4ed1620d59d61\",\"d0cc861719f92df539cf786acf23ba43c5f44e09905f0c7b791726b2424920e9\",\"a816e9772a5bc0ae36c5b28131e8793b21454f8c1b67c82757b8f5df50791b05\",\"24ed73f44cdea1a14846de8b184894623c08a03ee4d2c49952f126c302790c84\",\"6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d\"]}";
    char *path_example_7 = "{\"rule\":[1,0,1,1,0,0,0,1,0,0,0,1,0,0,0,1,1,0,1,1,0,0],\"path\":[\"a51d7fe60d743050a49a6799abb002e927987797152018b6856848c0acc49ed7\",\"a0830ebb524270f6459f991edac1754362ad68b2e6732c8fc313634587da9f70\",\"689ab582d825597fbb691d013f50656d2ed72e61b9e0ea507b7d37ddd81de34c\",\"88170cc1b9c6f8ffecb5d093a9f4d79d9be363a49b46e78d20e9ae66e93a30fc\",\"6cf7620e185e7983de7b3a90c59eed1d255fd7a00307de40dac4c337d493ada9\",\"d33763648648ce370a0269d966695c268c7536078da4dcc605d35dc8da38e205\",\"41afa812d4c4fbc038e5a0f8452f6e6ed79fb9216b26b6a14b315b2f930c29d1\",\"2af79d2c47e89e69dcc37a40b5e94f7235370e0431a6e67fa68f97ccfb8df499\",\"4d05c69face4506b181b07ad6ffdc2591a6fc0ee489493756b394cda80b2cbd7\",\"74639ad2b27e8638c624cf52c04ebf10fc1864b55e1302c5d4b05eec8c264eae\",\"1aa82e4b8de65312ca648649cee04371ae94be4d32df13695db388a580def573\",\"d8e2db4c30326579fa59c5f89947a042cc0ca392d1511aa2b5f7c362dba9a89e\",\"85c669dcddb9527f0befe611fc6341530d37e8f99a8357993e463f8a37a16f79\",\"3b546fc3b73678929cc8bba8ff7c9db0496b14586be90aa49029a6888526476b\",\"43fd7e33b5aa809dd72b6a0f08af40d2f4d896031c30f1d141ae886a06b87a81\",\"17d3311caa7b6d3bd2d346b05bfa759a7a7dcffbc9c8890011a203396b27da7c\",\"bd3139f2af7addacca82be782711390f2dbdbe6896031435868ba8b262f35c8f\",\"6d597d5c6e8c9ed83133970ef3ff18022fe8a3be3b0366f96e957f74ad67cd77\",\"e7c4b3404812da0b4a16332fcee82dd48e27f931e4153bf0fe4b2d17ed37c55f\",\"5ac6809af1bbbcf5fd4033af2acd4c17c155d6be555f93484812acdc7d8bd5d3\",\"24ed73f44cdea1a14846de8b184894623c08a03ee4d2c49952f126c302790c84\",\"6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d\"]}";
    char *path_example_8 = "{\"rule\":[1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0],\"path\":[\"3d84d78f3c966150d9ee1619be278d8407268af93880b30d40dfce4f0fee38da\",\"346817ebc19e9fccf861a300b68e654f7f4fa2558c15c3aef1375f084387bf6b\",\"2075feac572e16c1f89c117fb6414bf453650a3317f05c4ebdf71165fb723085\",\"0e523f01233ead4a635097e5a47279968f36fb59bb74cf7c3eaf7315e7b1c68c\",\"36b3d0473f70ded487bf62e7365c7103d4a0f84d3aa17aaa3db435ff69f6dbb7\",\"c6f58075d144b376d05aceb2802002905e2c122fe0df0b1e1a7674818633bc8c\",\"64f0f04968ed88d0dfb3d5b144c0abde05016ff3dc9ddc261d137ce28a27feb4\",\"160afc35cf7a3141320e90680aa2ef1ea24a53c07a7cd3de6f2c588eb060750d\",\"2ac82615385edb4756ef741f90dfad38f34c4195365a5833889a5f07a39546a7\",\"2d5590d5f462a8f4c1a434ddd977553848ac7d7cc6ef12ff5dcbfc0ebaf09e01\",\"bc7e7b46cde55e69dc2956157003259ab804d31d2d9433ce6687d2b1e036e4b5\",\"a15ae54a684808662b312ba07f80366c970015deb131ca80dda2d27fe71667e7\",\"e0be4d2967f702966f3c08d4c34db8a1ff0cd4f9efea5a8e64a6a5fdc80a23d3\",\"d0aa55858d83a69e7d83ca3a0675170a841ac5ec35d94c6d21fd2e529ebadfa7\",\"66ac7c1249aa552b2a350007453f84fcee2bfc5d0ce2e882315c6447caa2d6b7\",\"ad4081e0ec60ccf568b0b56860a75e6617cf124447ad600885eaf0a1e25f3c54\",\"7f7fd36b24161255cd271dc3a6f5b57d38db30e047f7ef549659c9646d6f54af\",\"c64d3468cc8d5a5e059ad93e72089de2ecd6c94e90f94db7d0dc7d6ecb4ebe4e\",\"6df4984f67d11f61824dc08990fc65fa67accb47305a61fd35b3305458ede721\",\"5ac6809af1bbbcf5fd4033af2acd4c17c155d6be555f93484812acdc7d8bd5d3\",\"24ed73f44cdea1a14846de8b184894623c08a03ee4d2c49952f126c302790c84\",\"6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d\"]}";
    */
    int res = verify(&param, sig, msg_hash, ID_bytes, path_example_1, strlen(path_example_1));
    if (res != 0) {
    	printf("Verification failed!\n");
    } else {
    	printf("Verification successful\n");
    }


    return 0 ;
}
