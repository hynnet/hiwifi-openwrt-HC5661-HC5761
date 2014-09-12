#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <unistd.h>

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>

#if OPENSSL_VERSION_NUMBER >= 0x10000000L
#define HAVE_ERR_REMOVE_THREAD_STATE
#endif

struct rsa_public_key {
	uint32_t len;		/* Length of modulus[] in number of uint32_t */
	uint32_t n0inv;		/* -1 / modulus[0] mod 2^32 */
	uint32_t modulus[64];	/* modulus as little endian array */
	uint32_t rr[64];		/* R^2 as little endian array */
}__packed;

extern int optind;
extern int opterr;
extern char *optarg;

static int rsa_err(const char *msg)
{
	unsigned long sslErr = ERR_get_error();

	fprintf(stderr, "%s", msg);
	fprintf(stderr, ": %s\n",
	ERR_error_string(sslErr, 0));

	return -1;
}

static int rsa_get_pub_key(const char *path, RSA **rsap)
{
	EVP_PKEY *key;
	X509 *cert;
	RSA *rsa;
	FILE *f;
	int ret;

	*rsap = NULL;
	f = fopen(path, "r");
	if (!f) {
		fprintf(stderr, "Couldn't open RSA certificate: '%s': %s\n",
			path, strerror(errno));
		return -EACCES;
	}

	/* Read the certificate */
	cert = NULL;
	if (!PEM_read_X509(f, &cert, NULL, NULL)) {
		rsa_err("Couldn't read certificate");
		ret = -EINVAL;
		goto err_cert;
	}

	/* Get the public key from the certificate. */
	key = X509_get_pubkey(cert);
	if (!key) {
		rsa_err("Couldn't read public key\n");
		ret = -EINVAL;
		goto err_pubkey;
	}

	/* Convert to a RSA_style key. */
	rsa = EVP_PKEY_get1_RSA(key);
	if (!rsa) {
		rsa_err("Couldn't convert to a RSA style key");
		goto err_rsa;
	}
	fclose(f);
	EVP_PKEY_free(key);
	X509_free(cert);
	*rsap = rsa;

	return 0;

err_rsa:
	EVP_PKEY_free(key);
err_pubkey:
	X509_free(cert);
err_cert:
	fclose(f);
	return ret;
}

static int rsa_get_priv_key(const char *key_path, RSA **rsap)
{
	RSA *rsa;
	FILE *f;

	*rsap = NULL;
	f = fopen(key_path, "r");
	if (!f) {
		fprintf(stderr, "Couldn't open RSA private key: '%s': %s\n",
			key_path, strerror(errno));
		return -ENOENT;
	}

	rsa = PEM_read_RSAPrivateKey(f, 0, NULL, (void *)key_path);
	if (!rsa) {
		rsa_err("Failure reading private key");
		fclose(f);
		return -EPROTO;
	}
	fclose(f);
	*rsap = rsa;

	return 0;
}

static int rsa_init(void)
{
	int ret;

	ret = SSL_library_init();
	if (!ret) {
		fprintf(stderr, "Failure to init SSL library\n");
		return -1;
	}
	SSL_load_error_strings();

	OpenSSL_add_all_algorithms();
	OpenSSL_add_all_digests();
	OpenSSL_add_all_ciphers();

	return 0;
}

static void rsa_remove(void)
{
	CRYPTO_cleanup_all_ex_data();
	ERR_free_strings();
#ifdef HAVE_ERR_REMOVE_THREAD_STATE
	ERR_remove_thread_state(NULL);
#else
	ERR_remove_state(0);
#endif
	EVP_cleanup();
}

/*
 * rsa_get_params(): - Get the important parameters of an RSA public key
 */
static int rsa_get_params(RSA *key, uint32_t *n0_invp, BIGNUM **modulusp,
		   BIGNUM **r_squaredp)
{
	BIGNUM *big1, *big2, *big32, *big2_32;
	BIGNUM *n, *r, *r_squared, *tmp;
	BN_CTX *bn_ctx = BN_CTX_new();
	int ret = 0;

	/* Initialize BIGNUMs */
	big1 = BN_new();
	big2 = BN_new();
	big32 = BN_new();
	r = BN_new();
	r_squared = BN_new();
	tmp = BN_new();
	big2_32 = BN_new();
	n = BN_new();
	if (!big1 || !big2 || !big32 || !r || !r_squared || !tmp || !big2_32 ||
	    !n) {
		fprintf(stderr, "Out of memory (bignum)\n");
		return -ENOMEM;
	}

	if (!BN_copy(n, key->n) || !BN_set_word(big1, 1L) ||
	    !BN_set_word(big2, 2L) || !BN_set_word(big32, 32L))
		ret = -1;

	/* big2_32 = 2^32 */
	if (!BN_exp(big2_32, big2, big32, bn_ctx))
		ret = -1;

	/* Calculate n0_inv = -1 / n[0] mod 2^32 */
	if (!BN_mod_inverse(tmp, n, big2_32, bn_ctx) ||
	    !BN_sub(tmp, big2_32, tmp))
		ret = -1;
	*n0_invp = BN_get_word(tmp);

	/* Calculate R = 2^(# of key bits) */
	if (!BN_set_word(tmp, BN_num_bits(n)) ||
	    !BN_exp(r, big2, tmp, bn_ctx))
		ret = -1;

	/* Calculate r_squared = R^2 mod n */
	if (!BN_copy(r_squared, r) ||
	    !BN_mul(tmp, r_squared, r, bn_ctx) ||
	    !BN_mod(r_squared, tmp, n, bn_ctx))
		ret = -1;

	*modulusp = n;
	*r_squaredp = r_squared;

	BN_free(big1);
	BN_free(big2);
	BN_free(big32);
	BN_free(r);
	BN_free(tmp);
	BN_free(big2_32);
	if (ret) {
		fprintf(stderr, "Bignum operations failed\n");
		return -ENOMEM;
	}

	return ret;
}

static int get_key_bignum(BIGNUM *num, int num_bits, uint32_t *key_mod)
{
	BIGNUM *tmp, *big2, *big32, *big2_32;
	BN_CTX *ctx;
	int ret;

	tmp = BN_new();
	big2 = BN_new();
	big32 = BN_new();
	big2_32 = BN_new();
	if (!tmp || !big2 || !big32 || !big2_32) {
		fprintf(stderr, "Out of memory (bignum)\n");
		return -1;
	}
	ctx = BN_CTX_new();
	if (!tmp) {
		fprintf(stderr, "Out of memory (bignum context)\n");
		return -1;
	}
	BN_set_word(big2, 2L);
	BN_set_word(big32, 32L);
	BN_exp(big2_32, big2, big32, ctx); /* B = 2^32 */

	for (ret = 0; ret <= 63; ret++) {
		BN_mod(tmp, num, big2_32, ctx); /* n = N mod B */
		key_mod[ret] = htonl(BN_get_word(tmp));
		BN_rshift(num, num, 32); /*  N = N/B */
	}

	BN_free(tmp);
	BN_free(big2);
	BN_free(big32);
	BN_free(big2_32);

	return 0;
}

static int rsa_sign_with_key(RSA *rsa, const void *data,
		const int data_size, uint8_t **sigp, uint *sig_size)
{
	EVP_PKEY *key;
	EVP_MD_CTX *context;
	int size, ret = 0;
	uint8_t *sig;

	key = EVP_PKEY_new();
	if (!key)
		return rsa_err("EVP_PKEY object creation failed");

	if (!EVP_PKEY_set1_RSA(key, rsa)) {
		ret = rsa_err("EVP key setup failed");
		goto err_set;
	}

	size = EVP_PKEY_size(key);
	sig = malloc(size);
	if (!sig) {
		fprintf(stderr, "Out of memory for signature (%d bytes)\n",
			size);
		ret = -ENOMEM;
		goto err_alloc;
	}

	context = EVP_MD_CTX_create();
	if (!context) {
		ret = rsa_err("EVP context creation failed");
		goto err_create;
	}
	EVP_MD_CTX_init(context);
	if (!EVP_SignInit(context, EVP_sha1())) {
		ret = rsa_err("Signer setup failed");
		goto err_sign;
	}

	if (!EVP_SignUpdate(context, data, data_size)) {
		ret = rsa_err("Signing data failed");
		goto err_sign;
	}

	if (!EVP_SignFinal(context, sig, sig_size, key)) {
		ret = rsa_err("Could not obtain signature");
		goto err_sign;
	}
	EVP_MD_CTX_cleanup(context);
	EVP_MD_CTX_destroy(context);
	EVP_PKEY_free(key);

	printf("Got signature: %d bytes, expected %d\n", *sig_size, size);
	*sigp = sig;
	*sig_size = size;

	return 0;

err_sign:
	EVP_MD_CTX_destroy(context);
err_create:
	free(sig);
err_alloc:
err_set:
	EVP_PKEY_free(key);
	return ret;
}

int rsa_sign(const char *private_path, const void *data, const int size,
	     uint8_t **sigp, uint *sig_len)
{
	RSA *rsa;
	int ret;

	ret = rsa_init();

	if (ret)
		return ret;

	ret = rsa_get_priv_key(private_path, &rsa);
	if (ret)
		goto err_priv;

	ret = rsa_sign_with_key(rsa, data, size, sigp, sig_len);
	if (ret)
		goto err_sign;

	RSA_free(rsa);
	rsa_remove();

	return ret;

err_sign:
	RSA_free(rsa);
err_priv:
	rsa_remove();
	return ret;	
}

static int sign_file(const char *k_path, const char *i_path, const char *o_path,
					 const struct rsa_public_key *key, int add_flag)
{
	int input_fd;
	FILE *output_fp;
	uint8_t *value;
	uint value_len;
	struct stat stat_buf;
	void *i_map;
	int ret;

	if (stat(i_path, &stat_buf) < 0) {
		fprintf(stderr, "file %s not exist.\n", i_path);
		return -1;	
	}

	if (0 == S_ISREG(stat_buf.st_mode)) {
		fprintf(stderr, "file %s not regular file.\n", i_path);
		return -1;		
	}

	input_fd = open(i_path, O_RDONLY);
	if (input_fd < 0) {
		fprintf(stderr, "can't open %s file with read mode.\n", i_path);
		return -1;
	}

	i_map = mmap(NULL, stat_buf.st_size, PROT_READ, MAP_PRIVATE, input_fd, 0);

	if (i_map == MAP_FAILED) {
		fprintf(stderr, "mmap file failed.\n");
		close(input_fd);
		return -1;
	}

	ret = rsa_sign(k_path, i_map, stat_buf.st_size, &value, &value_len);

	if (ret) {
		printf("Failed to sign signature node: %d\n", ret);
		munmap(i_map, stat_buf.st_size);
		close(input_fd);
		return -1;
	}
	ret = -1;

	output_fp = fopen(o_path, "w+");

	if (!output_fp) {
		fprintf(stderr, "open %s file failed.\n", o_path);
		goto err_out;	
	}

	if (fwrite(key, sizeof(struct rsa_public_key), 1, output_fp) < 0) {
		fprintf(stderr, "write public key to %s file failed.\n", o_path);
		goto err_out_out;
	}

	if (fwrite(value, 1, value_len, output_fp) < 0) {
		fprintf(stderr, "write signature value to %s file failed.\n", o_path);
		goto err_out_out;
	}

	if (add_flag) {
		if (fwrite((unsigned char *)i_map, 1, stat_buf.st_size, output_fp) < 0) {
			fprintf(stderr, "write %s file failed.\n", o_path);
			goto err_out_out;
		}
	}

	ret = 0;

err_out_out:
	fclose(output_fp);

err_out:
	munmap(i_map, stat_buf.st_size);
	close(input_fd);
	free(value);

	return ret;
}

static int get_rsa_pub_key(const char *path, struct rsa_public_key *key)
{
	BIGNUM *modulus, *r_squared;
	uint32_t n0_inv;
	int ret;
	int bits;
	RSA *rsa;

	ret = rsa_get_pub_key(path, &rsa);
	if (ret)
		return ret;
	ret = rsa_get_params(rsa, &n0_inv, &modulus, &r_squared);
	if (ret)
		return ret;

	bits = BN_num_bits(modulus);
	get_key_bignum(modulus, bits, key->modulus);
	get_key_bignum(r_squared, bits, key->rr);

	key->len = htonl(bits);
	key->n0inv = htonl(n0_inv);

	BN_free(modulus);
	BN_free(r_squared);

	return 0;
}

static void usage(void)
{
	fprintf(stderr, "Usage: hiwifi_sign_img -i input_file -o output_file -k key_file -c cert_file [-a]\n");
	fprintf(stderr, "                       -i: set intput file\n");
	fprintf(stderr, "                       -o: set output file\n");
	fprintf(stderr, "                       -k: set the openssl rsa key file\n");
	fprintf(stderr, "                       -c: set the openssl rsa cert file\n");
	fprintf(stderr, "                       -a: add signature and input file to output file\n");

	exit(1);
}

int main(int argc, char *argv[])
{
	int op;
	char *input_file = NULL;
	char *output_file = NULL;
	struct rsa_public_key pub_key;
	char *key_path = NULL;
	char *cert_path = NULL;
	int add_flag = 0;

	while ((op = getopt(argc, argv, "i:o:k:c:a")) != -1) {
		switch (op) {
		case 'i':
			if (optarg[0] == '0' && optarg[1] == 0) {
				fprintf(stderr, "input file format error.\n");
				usage();
			}

			input_file = optarg;
			break;

		case 'o':
			if (optarg[0] == '0' && optarg[1] == 0) {
				fprintf(stderr, "input file format error.\n");
				usage();				
			}

			output_file = optarg;
			break;

		case 'k':
			if (optarg[0] == '0' && optarg[1] == 0) {
				fprintf(stderr, "input file format error.\n");
				usage();
			}

			key_path = optarg;
			break;

		case 'c':
			if (optarg[0] == '0' && optarg[1] == 0) {
				fprintf(stderr, "input file format error.\n");
				usage();				
			}

			cert_path = optarg;
			break;

		case 'a':
			add_flag = 1;
			break;

		default:
			usage();				
		}		
	}

	if (!input_file || !output_file)
		usage();	

	if (get_rsa_pub_key(cert_path, &pub_key) != 0) {
		fprintf(stderr, "get public key from %s file failed.\n", cert_path);
		usage();
	}

	if (sign_file(key_path, input_file, output_file, &pub_key, add_flag) != 0) {
		fprintf(stderr, "signature file failed\n");
		usage();
	}

	return 0;
}
