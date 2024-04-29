
/**
 * @file    LPC55S69_PAVOC.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "LPC55S69_cm33_core0.h"
#include "fsl_debug_console.h"
#include "mbedtls/sha256.h"
#include "mbedtls/md.h"
#include "mbedtls/md_internal.h"
#include "core_json.h"



void hex_to_bytes_array(char* src, uint8_t* dest, size_t count) {
  size_t i;
  int value;
  for (i = 0; i < count && sscanf(src + i * 2, "%2x", &value) == 1; i++) {
        dest[i] = value;
    }
}

void calc_sha_256(uint8_t output[], const uint8_t* input, size_t len) {
	size_t outlen = 32;
	//HASHCRYPT_SHA(HASHCRYPT, kHASHCRYPT_Sha256, input, len, output, &outlen);

	mbedtls_sha256_context sha256_ctx;
	mbedtls_sha256_init(&sha256_ctx);
	mbedtls_sha256_starts_ret(&sha256_ctx, 0);

	mbedtls_sha256_update_ret(&sha256_ctx, input, len);
	mbedtls_sha256_finish_ret(&sha256_ctx, output);

	mbedtls_sha256_free(&sha256_ctx);

}

void puf(uint8_t challenge[32], uint8_t response[32]) {

	calc_sha_256(response, challenge, 32);
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

    JSONStatus_t result;
    result = JSON_Validate(path_json, path_length);

    if( result == JSONSuccess )
    {
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
		//hex_to_bytes_array(path1[index+1], digest, strlen(path1[index+1]));
	        //memcpy(digest, path1[index+1], strlen(digest));
	        //memcpy(input, res, 32);
	        //memcpy(input[32], path[index], 32);

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

void generateHashChain(uint8_t start[32], int len, uint8_t res[32]) {
	uint8_t buffer[32];
	uint8_t* input = (uint8_t *)start;
	for(int i = 0; i < len; i++) {
		calc_sha_256(buffer, input, 32);
		input = buffer;
	}
	memcpy(res, buffer, 32);
}

void generateMAC(unsigned char *message, size_t message_length, unsigned char output[], unsigned char *key) {

	 mbedtls_md_context_t ctx;
	 const mbedtls_md_info_t *info;
	 info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
	 mbedtls_md_init(&ctx);
	 int res = mbedtls_md_setup(&ctx, info, 1);
	 res = mbedtls_md_hmac_starts(&ctx, key, 32);
	 res = mbedtls_md_hmac_update(&ctx, message, message_length);
	 res = mbedtls_md_hmac_finish(&ctx, output);
	 mbedtls_md_free(&ctx);

}

void authenticate(uint8_t challenge[32], int time_step, uint8_t *msg, int message_length, uint8_t mac[32]){

	uint8_t response[32];
	uint8_t chain_element[32];
	puf(challenge, response);
	generateHashChain(response, time_step, chain_element);
	generateMAC(msg, message_length, mac, chain_element);
}

int verify(uint8_t chain_element[32], int time_step, uint8_t *msg, int message_length, uint8_t mac[32], uint8_t ID[32], char *path, size_t path_length) {

	uint8_t mac_check[32];
	generateMAC(msg, message_length, mac_check, chain_element);
	if(memcmp(mac, mac_check, 32) != 0) {
		return 1;
	}
	uint8_t chain_end[33];
	uint8_t leaf[32];
	memset(chain_end, 0, 33);
	generateHashChain(chain_element, time_step, chain_end + 1);
	calc_sha_256(leaf, chain_end, 33);
	int res;
	res = verify_inclusion(path, path_length, ID, leaf);
	return res;
}

void enroll(int responses_to_enroll, uint8_t result[][32], uint8_t challenges[][32], int chain_length[] ) {

	for (int i=0; i<responses_to_enroll;i++){
		uint8_t puf_response[32];
		puf(challenges[i], puf_response);
		generateHashChain(puf_response, chain_length[i], result[i]);
	}
}


void revealChainElement(uint8_t challenge[32], uint8_t chain_element[32], int time_step) {

	uint8_t puf_response[32];
	puf(challenge, puf_response);
	generateHashChain(puf_response, time_step, chain_element);
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
    HASHCRYPT_Init(HASHCRYPT);

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

    uint8_t responses[8][32];
    int chain_length[8] = {10, 20, 30, 40, 50, 80, 90, 100};
    // Examplary message to be authenticated
    uint8_t msg[] = "Hello CROSSCON!";
    uint8_t mac[32];
    enroll(8, responses, challenges, chain_length);


    authenticate(challenges[0], 2, msg, strlen(msg), mac);

    uint8_t chain_element[32];
    revealChainElement(challenges[0], chain_element, 2);
    char *root = "3d4100223842a586b0c526dcd74cd57447f22149cdd7e9de029adf1cd1d96045";
    uint8_t root_bytes[32];
	hex_to_bytes_array(root, root_bytes, 64);



    char *path_example_1 = "{\"rule\":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],\"path\":[\"a5bc8d412aaf98a5ddda3c6d8c5a776f6808c607913fb11ced54112cf6678a19\",\"58705e7af8dbab9f2f5b6449ba18d22cce7eedf245fca8dcfd93cf0f906ccf95\",\"94d8e890a83c68081af355575ee41d8738f4eab7150e5b41862787b5996682ef\",\"dda8915079e27037f07130ff899b9ce7e6d619fec11809b6afa5b3794a239dc8\",\"4b3116d1b02d2fd73e0b3dd2d6eefdc06baa64814f46688b8bc0edf8564480c1\",\"3150f0cfdcb941ace16aa6578607aba02ff80cb146768bc5fd6d67d2feacacfa\",\"24c7046993ad5683916db4e0d140f49254ab612b9c924da95497ad7b2070208c\",\"f77400a07e1a24308a7249d67ed0df4bf2bde580aa3a14abf9248c274fa173d0\",\"da5e5524dd50811a1e22f07631dcf7e417ae611a9dc435cebae326160882f10f\",\"7aa340542e158ba69e2dba06f7088ef1594175e57e33f16467a293d4179011b4\",\"abe7990a90a20b3cefd71a8395f94a97d23e501e6bc98a24dc153adb0dea268b\",\"d8556a512bd7e78e78a1dbaf542f3021e4f7cbb654b9f00ed13aa43eebd865f2\",\"ad222a6f1da200786d2dc99e0360fa71be1e0d7db4f22357de577e78e338dd99\",\"624796462038b981b36a93640f41d1641b6caeeb9110045bd745420625ddc9cc\",\"72262c5f9994f2c377b49964069b9f73b8d767a19949d94dfdc3d89dcb8b58de\",\"322fec7ff4ea1c871e619c1ff3f63d93dc369ea9fcaec1761c37370f5feefeac\",\"9ac83739e6cd9b82cc7bc67419764eddb6498a3a6c5e4c99a1f2fba2364922d8\",\"ecf5982017ddbd668bf506c95ee0ca78b87bcb9652503119112947b06205025b\",\"0c72e43db79e2318d70d820b9b959ca20ec673a983a0e636e92bf3c8a9a2d82f\",\"226b8218eb3106a54f6cef6a5e16daa8197cf82b1c6760b52164f94108975639\",\"2c15ec880a73572365895de6426fbafb87ed4459c167d137ac9c084e949e7eb3\",\"6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d\"]}";

    /* Examplary Path data for other challenges (2-8)
    char *path_example_2 = "{\"rule\":[0,1,1,0,0,0,1,0,1,1,1,1,0,0,1,0,0,0,1,0,0,0],\"path\":[\"78b8a8044fea31fd947caa6880c910d5d8a5df6c32630c6bc0232f95c1cd16b1\",\"3bf16ea9782cee42247d27b8b81007998556f6dde9d0eb8402cf8d9071cb1be9\",\"e7759561edb7a2dbaaaa628a2af10850e5c993ea2430102e460434e1cd721bc3\",\"d646b3f84f0296bcab1532f1caf80a241580ffde97737ab5eb7fcec3abc3afe2\",\"e5e02c05c91f002c9ca10ca990ab9befc9ecb1307041fcb26b19a8e177d114b2\",\"f3f658f1dd64c600cec1bf39bb4703071eb545f7dd7e82b1479a5a429c664bb4\",\"22b0a48879d0d0497424bbb51c75f88097e0b431ee5a589ac1a616ebcd401867\",\"5e9cece8bf358e4655bacda461281bc2e07f8b6e9f493d101133d02fbc611f9d\",\"675049a6927430a9d12cc11b618901a1f54512570f3417f99c33602b607f4822\",\"e7c2b40143b4bc3c89459f56fdacc3e618ceb58deb8e32b2fd14cdcbfae5ee30\",\"8b7160f647985773b7c7ae1ec771f1e8dec87fa8774dd446d1cda6006c3a44e7\",\"a9f4ac17471fcc8dc5699bc66dbc75fe753d40ae571ac23009ee2d998819ed37\",\"a9aea31b7f5c6efbf1cd5c65d0c87e33228c804c5551ea792a519c69e928dffc\",\"34f93148b4bb9ece0a275f1d41ec3f2c8b350210774fe603893483587c48c5b8\",\"d8efff6abe37835a18cd46586ac2aac79c0d697af02f9c0aedb6b3540d8daca8\",\"7bb1db63c1fc92123f1348cf2cc64924950984f20db18415283748713e34c040\",\"0a08369f5eb10712313c82779503efdd7ee166c84224f31ba29650c9d066e39b\",\"38b04337ecb4f923bc96102254b2cf747b82b21b8286c95da923a367672045bf\",\"6704f5ee848a99fd5f9850984f266da14d18b2f8b37e2e3bfd5ea4de1e4686b8\",\"0b872866cad017f96eb8f174f2b1e40f13b58a34e1d5d7f7f015cf97309e9353\",\"2c15ec880a73572365895de6426fbafb87ed4459c167d137ac9c084e949e7eb3\",\"6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d\"]}";
    char *path_example_3 = "{\"rule\":[1,1,0,1,1,1,0,1,0,1,1,1,1,0,1,1,1,0,1,0,0,0],\"path\":[\"0a98d557bf229289d3f38513e3037f01a5fffb91a7cfada2db9793b0f638e326\",\"78d0f99048921506ef227164cd5854cd9934c5176653c8fb8467248b6aa657e4\",\"044b5d776c2b5a5a6286c81271cec21b43e2b09f556aecd06fbdb771e1e0eedd\",\"687b01851279a71f7f91aea8948ce69850e591a29e89868127d842ae6f2d1bde\",\"28b7f8a892d9d504592933201ec48774a084042beb4f2c379eadaba6fc7a63a8\",\"4045bfd5eb6543aafe467669951d39944794e0b951f7b049fa56c1ab0daa13f4\",\"c5ebc2924673479ef156b41f82c4c641510af5a75cd4aaa12b687da0c03c2694\",\"abf4cdb470378a45bb7aea918c3030ea7ca6f149d32cc267363ece10e90699b6\",\"fc3368d5d586639b33fd02aa9a7defa76aeb5260f1ad935dd0e0f6ce6714c4e3\",\"329fa9bdfcc9837e01cdbbcaf39fac22d618bf8729eef588047df9d8536887ab\",\"d81887b983cb04a0ec13143f43a2b8bda7437708f881b11539b5f96b77c5dea5\",\"db2892c4aa5ba59722b7c62fed6dcc6cf62444bc434d85fdeffece5b9c9c197d\",\"2245c8f9892c48c05e79dd315f56393470c9a1d27e6ab2c5303d379c78807dd6\",\"8be64bc07e4b9be3e943ac231ac0fa472db734adab50efb78b2dafb21882d3fb\",\"a3b7a3c43c94a017a3052fed1475beb7813cf4b697c25c4cf879c0d90c71fe27\",\"70458d27027875a057b400efe3ac0abb875cf6bb14a7b901d97947a2a92a43e4\",\"9186cebb2f52cd689ca84125e9682e43c719565c6f9e781b4fc40a1b8ceae473\",\"e16b3efc3bbaabada1c6cbc73b972c433bd4a990561ec01b8a5e960c7a41bc31\",\"6704f5ee848a99fd5f9850984f266da14d18b2f8b37e2e3bfd5ea4de1e4686b8\",\"0b872866cad017f96eb8f174f2b1e40f13b58a34e1d5d7f7f015cf97309e9353\",\"2c15ec880a73572365895de6426fbafb87ed4459c167d137ac9c084e949e7eb3\",\"6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d\"]}";
    char *path_example_4 = "{\"rule\":[1,1,0,0,0,1,1,1,0,1,0,1,1,1,1,0,1,1,1,0,0,0],\"path\":[\"248f4268cd0e59079ce047b7bbda153b210e143809188867dd76c57376b29ffe\",\"ae5aa4f58dac3b2f52076666b0267704671b52f8b19c7042326915d1952ecadd\",\"ab99ca1c2213b9d765969b4ed78410cdd17047726397f403b27ea0b23ef82104\",\"ed91f344cb0a5f2a663d594988d768104d5fddfbf3f71af10104883e850ed47b\",\"420954b534b5aaa988596949ae717031b3d61041c31cf619841d80eacbda7f08\",\"6e7f615377edd7d023dccb4c9bbad018e6bca745293f4631c10fa0c5fb9c4bdf\",\"7cd308265e7f66191e77e5a05d11c48d368063aeaf17ab766564039a70861f64\",\"220eadb84f290d702dc1a6568846109eb195f6924aefc492942da16fcfc1101a\",\"cebeb96d791c79246ad8d0e8369dacea1cee91374ed95a581fcca7822aa12c00\",\"2be4f94e797f3069cfb59248cca65d770669d41e5269194f3709caac6bd7995b\",\"547669f0b32a35a319a556918a80e9083930cef030f0cace99512a2ccb604f5d\",\"6679addafee5b40e69c2622d2f69205191dd9434084cf7a290c252a89ede786a\",\"0125e6e9fc034d8ddd5c6a75561bb3cb00f359423e88223cbf05b088f38c5e0b\",\"6c291ac731996da0d21489a907808df07f67ff1673d9eb3966e162ee9fcf061b\",\"4258702488707cc3062229e4b709360ed8cf601aeaa479055aaad4d07c18e570\",\"083d9d39b59e18e5403ab0eb2b2dca919faf70132f28732fadaf8e505dfbcbb6\",\"3dc6c9fbf2bbec37d0b9e3d5620fb9b935d201e4c6dd90ee271d200af4609a32\",\"557293d59b6d5cfe0779716776ed026296f9993be8b03a27d4a78eb307258dbc\",\"851238ee50336f13fc69707c86e35c48b052c8b63b14bf879e52e6d5822bf0a9\",\"0b872866cad017f96eb8f174f2b1e40f13b58a34e1d5d7f7f015cf97309e9353\",\"2c15ec880a73572365895de6426fbafb87ed4459c167d137ac9c084e949e7eb3\",\"6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d\"]}";
    char *path_example_5 = "{\"rule\":[0,1,1,1,1,0,1,1,1,1,0,1,1,1,0,1,1,0,0,1,0,0],\"path\":[\"8bf1156190c24c64a0e106816e3b6e3de9697dcac779f520f9948155d5d99589\",\"44474b085dbec2dfaa554b3a2067935acfc64910cf59a897efc217d51fd5fc64\",\"8564cb18174c5a3e2802bbc980740e537a3278bd37cafdb60931f510a74434ee\",\"2a5a9fffc2f64193664ea8c0309b49e7ad8ceee3e795ded8d996dd246ceece68\",\"ab50ce47a486cd8478a7cc662e6bbd6d1ba61bd6081014968ddddfb6ab92ec91\",\"c63b096ebcfd068e7f95e61ebfd54ef1570acc820ff22e1600f21d141193e918\",\"cc54111b641839e8c2c6706747ff229a7fc90fe85c34373a7873d86ae03901e9\",\"9af77ace6811d7daf034eb7220d1d1319b1a7cb37ed225753a86999379c57c03\",\"9f0d65df4fb4f17fc45a5626b194af21cd8dad57d1a336744afc306dfa873f80\",\"445d069a4c3072645b1866cd45512b04daa84cbd4638a8f04e8ab29c3dd9ba78\",\"fd4f7a5de0cf1fa20d4c68dbac3eccc217118097a67aa4bb44ea38024f9ad66c\",\"24ce1059b1a02d15291c0d0143c1a969eac3a4a8d8006177c6ac66817f5aef99\",\"7e528068af319751d9b1d64e4582050edf33e7b07941dbd07e38402bfdcdbdb3\",\"32ba1b5bdcfbaea1de94136bab74cb0ff922d633ec9e81b9c8b5a1922d3ffe8c\",\"12225677650b802d7694b9de8733d82d9fbb401d5ab4bbff8ad5b66129aa6bb0\",\"4e209b3ad1060f4eff2f30b3a2c64f962212ad0f46ade7ba56605860e17a2598\",\"a88028ef8e1eed8b590f526582ef25e8e9b591ad82f0f5e0a0d48a05ebde8845\",\"39f64400865de971988840e0dc7c7b688a86a7f766771412e9e9266798dbe6bb\",\"8753b11b0ee32a6054da3628537a2d4849723380acaa0b23400c6dfb193c7d78\",\"7c51b0f8b2d92db2b95b0ab502e3b76edf0abe57e1d0231619beafeb06a42fb1\",\"15137d6843c5ba55380a7e307435f6da9dc32f8d5228437e67897907256dc037\",\"6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d\"]}";
    char *path_example_6 = "{\"rule\":[1,1,1,1,1,0,1,1,0,0,0,1,1,1,1,1,0,1,0,1,0,0],\"path\":[\"150b9e1a9d6a73ade575191cf84da4b03b2bd016208d79b17500f200c7cc45bb\",\"077684ff72070a83aac85c691dfc3439464fea124e538422ebc68ad68c63855f\",\"036d75b85d622e93346e8f95b841e588e85cc17e24d365b2c66df0cc0674ae41\",\"395155d016548e95e37cf653a2eb5170e1e3987f56403c7bd8bc3687b31d5865\",\"613cc4748d4623d76d502f888ba870056222f666b19bb6143298363c41c22776\",\"37eed19be98883c63bbae2a6539f84920fd5d359a78887deaff6f680935a152d\",\"6492f978ec7539bd8ec2f97fb7ee948c4add092668934c28f78ed3e244b927f8\",\"9b5fadb9d54b35e1353788051854a4dfe8f8f3af00803c77718754698390ab33\",\"9fe247755d01e107588e9959bc3f35c292086f4e5c5595f8baa8a796ce1246cf\",\"dff9866306e61aa7a21da93ddd219a62b3c1bc438aae81cff0025a5766853535\",\"88e1bb5ba238fd3bf267cbf3baff01c6af4060c7ea97bd9cf21deaf1c03f5e97\",\"41fb3c83f981099b2ca1a58f47a924fc32239258eaff5e5f6dcffbb50d707c73\",\"1cdc40f32b56840afb06ebe232d6e7413c82310f44819f62e330dfc810956f46\",\"d0edfa1b45e3fb5129ea48ac6f49ac198b69657481b0deb3047809859273af41\",\"30ea822fb52b8ecc1e4b9828608f01d6b12051b171e43edbf3c344d137a36509\",\"ce14a23d15152c2713e2967c60a1a48a92ac0bf7fcef6c9de23ef730dffc0798\",\"5275a5a956133e343d47fb49dd3ea13e2731fde1be27765997cdfb1aae36cac8\",\"1614e55fa291d97108953d3236d458b82a56165253392a1c02e4ed1620d59d61\",\"d6af65b2808210dd1a369f7ea06fb29b12eda03df912e2b784c96ef9a41a2704\",\"7c51b0f8b2d92db2b95b0ab502e3b76edf0abe57e1d0231619beafeb06a42fb1\",\"15137d6843c5ba55380a7e307435f6da9dc32f8d5228437e67897907256dc037\",\"6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d\"]}";
    char *path_example_7 = "{\"rule\":[1,0,0,0,0,1,0,1,0,0,1,1,1,1,0,1,0,1,1,1,0,0],\"path\":[\"b7ecfc34ef42373291930743f73020016c50f99cbd08ef088cdd060b5d225778\",\"4fb6ac6502b0daff47ea8e1ffce12a50432ab43d4a69089a605a445a671a5f5d\",\"ce268e775af38db8ba3333601707abfea18c292728444d39af1a1dd48c7c28bc\",\"95d10c8443339f4b71e50a0e157eb0e7febcc3360bc11b2352f82d2eefe42260\",\"a2003640e946dcfecaf733b92311ab47af2bb5c27fdee0ff137682526fcbd0bb\",\"d9fd35410cacfa0fedf51cb5213c3c4f41bb14555257229a52b74aa164760e07\",\"ebbabce1d99e79f8532022789c2e28619b8b4dc528f3618c70957b4d355335ba\",\"119e339e7d9956801aec786a36ad8ede331883082b76c259ff297c9259784dd4\",\"0b74da77b88ca0d60b3204c2e8b35201937aefc5c8941d4c37352de6773b3ca8\",\"fb93c08206fa6321bc94e9e085c7e30c0add721c9e396b1a95ff78d889f7797e\",\"18d7adf3627dec5f3fe33e9ab00c39212ddd8fb040edacaffe866d37569ec321\",\"ee934cf2dabcd91e3cb4a9b88dbcfbc6d4f4fcd037aaad42140182af8310905c\",\"9541b0ae3e56093185d55e9ef86b764d42e905a19eb8ca0579b3346840c3f991\",\"aca8b50a54a8808d42d41fad5638d8f17574750cc63cf49fa5827aee4c74839a\",\"8edb61be3ca7de5735953ab43ec657837a9361ed6d7c00c8aed78609e4db2a6a\",\"99b725f25bcade4a11505f1a3a101038ecd66930e42d4aa11cf1d177b0212e4c\",\"bd9a14e59037ab50a034da8818af2c6d061fec05dc3f4d0fb4ac443394f80e9b\",\"49675d6a9967ad7ee8fb10a19d1a0a6320e42e7d103d45ed2af835e7375784d1\",\"ad0739515e5bc469104b75dc33340d4a8f194ca5ec6ebf8b7b78eeb21be952a0\",\"e860d1739c1ba74f2652ecaf4f2576e892387c0cf063cdbe4632fdae82528e51\",\"15137d6843c5ba55380a7e307435f6da9dc32f8d5228437e67897907256dc037\",\"6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d\"]}";
    char *path_example_8 = "{\"rule\":[1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0],\"path\":[\"1faaaf17ec1b1313a6641e24c22c634eb02b44f4c0c96721ac3e69f47f58b9c6\",\"8f892fc6cf48303e5546af068a89f47666045aa82fe57fd8b0944cc7e3060709\",\"1f086f812d79211f3698b1eeb548d3cb7f957e23696e2dc3bbfd6ae19b220533\",\"acef80620d3dc44ff62739201a93d76bf523bf6b1eb24841861c9c2b2eda2b63\",\"36b3d0473f70ded487bf62e7365c7103d4a0f84d3aa17aaa3db435ff69f6dbb7\",\"c6f58075d144b376d05aceb2802002905e2c122fe0df0b1e1a7674818633bc8c\",\"64f0f04968ed88d0dfb3d5b144c0abde05016ff3dc9ddc261d137ce28a27feb4\",\"160afc35cf7a3141320e90680aa2ef1ea24a53c07a7cd3de6f2c588eb060750d\",\"2ac82615385edb4756ef741f90dfad38f34c4195365a5833889a5f07a39546a7\",\"2d5590d5f462a8f4c1a434ddd977553848ac7d7cc6ef12ff5dcbfc0ebaf09e01\",\"bc7e7b46cde55e69dc2956157003259ab804d31d2d9433ce6687d2b1e036e4b5\",\"a15ae54a684808662b312ba07f80366c970015deb131ca80dda2d27fe71667e7\",\"e0be4d2967f702966f3c08d4c34db8a1ff0cd4f9efea5a8e64a6a5fdc80a23d3\",\"d0aa55858d83a69e7d83ca3a0675170a841ac5ec35d94c6d21fd2e529ebadfa7\",\"66ac7c1249aa552b2a350007453f84fcee2bfc5d0ce2e882315c6447caa2d6b7\",\"ad4081e0ec60ccf568b0b56860a75e6617cf124447ad600885eaf0a1e25f3c54\",\"7f7fd36b24161255cd271dc3a6f5b57d38db30e047f7ef549659c9646d6f54af\",\"c64d3468cc8d5a5e059ad93e72089de2ecd6c94e90f94db7d0dc7d6ecb4ebe4e\",\"ad0739515e5bc469104b75dc33340d4a8f194ca5ec6ebf8b7b78eeb21be952a0\",\"e860d1739c1ba74f2652ecaf4f2576e892387c0cf063cdbe4632fdae82528e51\",\"15137d6843c5ba55380a7e307435f6da9dc32f8d5228437e67897907256dc037\",\"6e340b9cffb37a989ca544e6bb780a2c78901d3fb33738768511a30617afa01d\"]}";
	*/
    int res = verify(chain_element, (chain_length[0] - 2), msg, strlen(msg), mac, root_bytes, path_example_1, strlen(path_example_1));
    if(res != 0) {
    	printf("Verification failed\n");
    } else {
    	printf("Verification successful");
    }




    return 0 ;
}
