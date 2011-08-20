#define LIBSSH_STATIC

#include "torture.h"
#include "pki.c"
#include <sys/stat.h>
#include <fcntl.h>

#define LIBSSH_RSA_TESTKEY "libssh_testkey.id_rsa"
#define LIBSSH_DSA_TESTKEY "libssh_testkey.id_dsa"
#define LIBSSH_PASSPHRASE "libssh-rocks"

static void setup_rsa_key(void **state) {
    int rc;

    (void) state; /* unused */

    unlink(LIBSSH_RSA_TESTKEY);
    unlink(LIBSSH_RSA_TESTKEY ".pub");

    rc = system("ssh-keygen -t rsa -q -N \"\" -f " LIBSSH_RSA_TESTKEY);
    assert_true(rc == 0);
}

static void setup_dsa_key(void **state) {
    int rc;

    (void) state; /* unused */

    unlink(LIBSSH_DSA_TESTKEY);
    unlink(LIBSSH_DSA_TESTKEY ".pub");

    rc = system("ssh-keygen -t dsa -q -N \"\" -f " LIBSSH_DSA_TESTKEY);
    assert_true(rc == 0);
}

static void setup_both_keys(void **state) {
    (void) state; /* unused */

    setup_rsa_key(state);
    setup_dsa_key(state);
}

static void setup_both_keys_passphrase(void **state) {
    int rc;

    (void) state; /* unused */

    rc = system("ssh-keygen -t rsa -q -N " LIBSSH_PASSPHRASE " -f " LIBSSH_RSA_TESTKEY);
    assert_true(rc == 0);

    rc = system("ssh-keygen -t dsa -q -N " LIBSSH_PASSPHRASE " -f " LIBSSH_DSA_TESTKEY);
    assert_true(rc == 0);
}

static void teardown(void **state) {
    (void) state; /* unused */

    unlink(LIBSSH_DSA_TESTKEY);
    unlink(LIBSSH_DSA_TESTKEY ".pub");

    unlink(LIBSSH_RSA_TESTKEY);
    unlink(LIBSSH_RSA_TESTKEY ".pub");
}

static char *read_file(const char *filename) {
    char *key;
    int fd;
    int size;
    struct stat buf;

    assert_true(filename != NULL);
    assert_true(*filename != '\0');

    stat(filename, &buf);

    key = malloc(buf.st_size + 1);
    assert_true(key != NULL);

    fd = open(filename, O_RDONLY);
    assert_true(fd >= 0);

    size = read(fd, key, buf.st_size);
    assert_true(size == buf.st_size);

    close(fd);

    key[size] = '\0';
    return key;
}

static int torture_read_one_line(const char *filename, char *buffer, size_t len) {
  FILE *fp;
  size_t rc;

  fp = fopen(filename, "r");
  if (fp == NULL) {
    return -1;
  }

  rc = fread(buffer, len, 1, fp);
  if (rc != 0 || ferror(fp)) {
    fclose(fp);
    return -1;
  }

  fclose(fp);

  return 0;
}

static void torture_pki_import_privkey_base64_RSA(void **state) {
    int rc;
    char *key_str;
    ssh_key key;
    const char *passphrase = LIBSSH_PASSPHRASE;
    enum ssh_keytypes_e type;

    (void) state; /* unused */

    key_str = read_file(LIBSSH_RSA_TESTKEY);
    assert_true(key_str != NULL);

    rc = ssh_pki_import_privkey_base64(key_str, passphrase, NULL, NULL, &key);
    assert_true(rc == 0);

    type = ssh_key_type(key);
    assert_true(type == SSH_KEYTYPE_RSA);

    rc = ssh_key_is_public(key);
    assert_true(rc == 1);

    free(key_str);
    ssh_key_free(key);
}

static void torture_pki_import_privkey_base64_NULL_key(void **state) {
    int rc;
    char *key_str;
    ssh_key key;
    const char *passphrase = LIBSSH_PASSPHRASE;

    (void) state; /* unused */

    key_str = read_file(LIBSSH_RSA_TESTKEY);
    assert_true(key_str != NULL);

    key = ssh_key_new();
    assert_true(key != NULL);

    /* test if it returns -1 if key is NULL */
    rc = ssh_pki_import_privkey_base64(key_str, passphrase, NULL, NULL, NULL);
    assert_true(rc == -1);

    free(key_str);
    ssh_key_free(key);
}

static void torture_pki_import_privkey_base64_NULL_str(void **state) {
    int rc;
    char *key_str;
    ssh_key key = NULL;
    const char *passphrase = LIBSSH_PASSPHRASE;

    (void) state; /* unused */

    key_str = read_file(LIBSSH_RSA_TESTKEY);
    assert_true(key_str != NULL);

    /* test if it returns -1 if key_str is NULL */
    rc = ssh_pki_import_privkey_base64(NULL, passphrase, NULL, NULL, &key);
    assert_true(rc == -1);

    free(key_str);
    ssh_key_free(key);
}

static void torture_pki_import_privkey_base64_DSA(void **state) {
    int rc;
    char *key_str;
    ssh_key key;
    const char *passphrase = LIBSSH_PASSPHRASE;

    (void) state; /* unused */

    key_str = read_file(LIBSSH_DSA_TESTKEY);
    assert_true(key_str != NULL);

    rc = ssh_pki_import_privkey_base64(key_str, passphrase, NULL, NULL, &key);
    assert_true(rc == 0);

    free(key_str);
    ssh_key_free(key);
}

static void torture_pki_import_privkey_base64_passphrase(void **state) {
    int rc;
    char *key_str;
    ssh_key key = NULL;
    const char *passphrase = LIBSSH_PASSPHRASE;

    (void) state; /* unused */

    key_str = read_file(LIBSSH_RSA_TESTKEY);
    assert_true(key_str != NULL);

    rc = ssh_pki_import_privkey_base64(key_str, passphrase, NULL, NULL, &key);
    assert_true(rc == 0);
    ssh_key_free(key);

    /* test if it returns -1 if passphrase is wrong */
    rc = ssh_pki_import_privkey_base64(key_str, "wrong passphrase !!", NULL,
            NULL, &key);
    assert_true(rc == -1);

#ifndef HAVE_LIBCRYPTO
    /* test if it returns -1 if passphrase is NULL */
    /* libcrypto asks for a passphrase, so skip this test */
    rc = ssh_pki_import_privkey_base64(key_str, NULL, NULL, NULL, &key);
    assert_true(rc == -1);
#endif

    free(key_str);

    /* same for DSA */
    key_str = read_file(LIBSSH_DSA_TESTKEY);
    assert_true(key_str != NULL);

    rc = ssh_pki_import_privkey_base64(key_str, passphrase, NULL, NULL, &key);
    assert_true(rc == 0);
    ssh_key_free(key);

    /* test if it returns -1 if passphrase is wrong */
    rc = ssh_pki_import_privkey_base64(key_str, "wrong passphrase !!", NULL, NULL, &key);
    assert_true(rc == -1);

#ifndef HAVE_LIBCRYPTO
    /* test if it returns -1 if passphrase is NULL */
    /* libcrypto asks for a passphrase, so skip this test */
    rc = ssh_pki_import_privkey_base64(key_str, NULL, NULL, NULL, &key);
    assert_true(rc == -1);
#endif

    free(key_str);
}

static void torture_pki_pki_publickey_from_privatekey_RSA(void **state) {
    int rc;
    char *key_str;
    ssh_key key;
    ssh_key pubkey;
    const char *passphrase = NULL;

    (void) state; /* unused */

    key_str = read_file(LIBSSH_RSA_TESTKEY);
    assert_true(key_str != NULL);

    rc = ssh_pki_import_privkey_base64(key_str, passphrase, NULL, NULL, &key);
    assert_true(rc == 0);

    pubkey = ssh_pki_publickey_from_privatekey(key);
    assert_true(pubkey != NULL);

    free(key_str);
    ssh_key_free(key);
    ssh_key_free(pubkey);
}

static void torture_pki_pki_publickey_from_privatekey_DSA(void **state) {
    int rc;
    char *key_str;
    ssh_key key;
    ssh_key pubkey;
    const char *passphrase = NULL;

    (void) state; /* unused */

    key_str = read_file(LIBSSH_DSA_TESTKEY);
    assert_true(key_str != NULL);

    rc = ssh_pki_import_privkey_base64(key_str, passphrase, NULL, NULL, &key);
    assert_true(rc == 0);

    pubkey = ssh_pki_publickey_from_privatekey(key);
    assert_true(pubkey != NULL);

    free(key_str);
    ssh_key_free(key);
    ssh_key_free(pubkey);
}

static void torture_pki_publickey_dsa_base64(void **state)
{
    enum ssh_keytypes_e type;
    char *b64_key, *key_buf, *p;
    const char *q;
    ssh_key key;
    int rc;

    (void) state; /* unused */

    key_buf = read_file(LIBSSH_DSA_TESTKEY ".pub");
    assert_true(key_buf != NULL);

    q = p = key_buf;
    while (*p != ' ') p++;
    *p = '\0';

    type = ssh_key_type_from_name(q);
    assert_true(type == SSH_KEYTYPE_DSS);

    q = ++p;
    while (*p != ' ') p++;
    *p = '\0';

    rc = ssh_pki_import_pubkey_base64(q, type, &key);
    assert_true(rc == 0);

    rc = ssh_pki_export_pubkey_base64(key, &b64_key);
    assert_true(rc == 0);

    assert_string_equal(q, b64_key);

    free(b64_key);
    free(key_buf);
    ssh_key_free(key);
}

static void torture_pki_publickey_rsa_base64(void **state)
{
    enum ssh_keytypes_e type;
    char *b64_key, *key_buf, *p;
    const char *q;
    ssh_key key;
    int rc;

    (void) state; /* unused */

    key_buf = read_file(LIBSSH_RSA_TESTKEY ".pub");
    assert_true(key_buf != NULL);

    q = p = key_buf;
    while (*p != ' ') p++;
    *p = '\0';

    type = ssh_key_type_from_name(q);
    assert_true(((type == SSH_KEYTYPE_RSA) ||
                 (type == SSH_KEYTYPE_RSA1)));

    q = ++p;
    while (*p != ' ') p++;
    *p = '\0';

    rc = ssh_pki_import_pubkey_base64(q, type, &key);
    assert_true(rc == 0);

    rc = ssh_pki_export_pubkey_base64(key, &b64_key);
    assert_true(rc == 0);

    assert_string_equal(q, b64_key);

    free(b64_key);
    free(key_buf);
    ssh_key_free(key);
}

static void torture_generate_pubkey_from_privkey(void **state) {
    char pubkey_original[4096] = {0};
    char pubkey_generated[4096] = {0};
    ssh_key privkey;
    ssh_key pubkey;
    int rc;

    (void) state; /* unused */

    rc = torture_read_one_line(LIBSSH_DSA_TESTKEY ".pub",
                               pubkey_original,
                               sizeof(pubkey_original));
    assert_true(rc == 0);

    /* remove the public key, generate it from the private key and write it. */
    unlink(LIBSSH_RSA_TESTKEY ".pub");

    rc = ssh_pki_import_privkey_file(LIBSSH_DSA_TESTKEY,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &privkey);
    assert_true(rc == 0);

    pubkey = ssh_pki_publickey_from_privatekey(privkey);
    assert_true(pubkey != NULL);

    rc = ssh_pki_export_pubkey_file(pubkey, LIBSSH_DSA_TESTKEY ".pub");
    assert_true(rc == 0);

    rc = torture_read_one_line(LIBSSH_DSA_TESTKEY ".pub",
                               pubkey_generated,
                               sizeof(pubkey_generated));
    assert_true(rc == 0);

    assert_string_equal(pubkey_original, pubkey_generated);

    ssh_key_free(privkey);
    ssh_key_free(pubkey);
}

int torture_run_tests(void) {
    int rc;
    const UnitTest tests[] = {
        /* ssh_pki_import_privkey_base64 */
        unit_test_setup_teardown(torture_pki_import_privkey_base64_NULL_key,
                                 setup_rsa_key,
                                 teardown),
        unit_test_setup_teardown(torture_pki_import_privkey_base64_NULL_str,
                                 setup_rsa_key,
                                 teardown),
        unit_test_setup_teardown(torture_pki_import_privkey_base64_RSA,
                                 setup_rsa_key,
                                 teardown),
        unit_test_setup_teardown(torture_pki_import_privkey_base64_DSA,
                                 setup_dsa_key,
                                 teardown),
        unit_test_setup_teardown(torture_pki_import_privkey_base64_passphrase,
                                 setup_both_keys_passphrase,
                                 teardown),
        /* ssh_pki_publickey_from_privatekey */
        unit_test_setup_teardown(torture_pki_pki_publickey_from_privatekey_RSA,
                                 setup_rsa_key,
                                 teardown),
        unit_test_setup_teardown(torture_pki_pki_publickey_from_privatekey_DSA,
                                 setup_dsa_key,
                                 teardown),
        /* public key */
        unit_test_setup_teardown(torture_pki_publickey_dsa_base64,
                                 setup_dsa_key,
                                 teardown),
        unit_test_setup_teardown(torture_pki_publickey_rsa_base64,
                                 setup_rsa_key,
                                 teardown),

        unit_test_setup_teardown(torture_generate_pubkey_from_privkey,
                                 setup_dsa_key,
                                 teardown),


    };

    (void)setup_both_keys;

    ssh_init();
    rc=run_tests(tests);
    ssh_finalize();
    return rc;
}
