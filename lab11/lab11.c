#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RED "\e[9;31m"
#define GRN "\e[0;32m"
#define CRESET "\e[0m"

#define handle_error(msg)                                                      \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

size_t read_all_bytes(const char *filename, void *buffer, size_t buffer_size) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    handle_error("Error opening file");
  }

  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (file_size > buffer_size) {
    handle_error("File size is too large");
  }

  if (fread(buffer, 1, file_size, file) != file_size) {
    handle_error("Error reading file");
  }

  fclose(file);
  return file_size;
}

void print_file(const char *filename, const char *color) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    handle_error("Error opening file");
  }

  printf("%s", color);
  char line[256];
  while (fgets(line, sizeof(line), file)) {
    printf("%s", line);
  }
  fclose(file);
  printf(CRESET);
}

int verify(const char *message_path, const char *sign_path, EVP_PKEY *pubkey);

int main() {
  // File paths
  const char *message_files[] = {"message1.txt", "message2.txt",
                                 "message3.txt"};
  const char *signature_files[] = {"signature1.sig", "signature2.sig",
                                   "signature3.sig"};

  // TODO: Load the public key using PEM_read_PUBKEY
  EVP_PKEY *pubkey = NULL;
  FILE *file_key = fopen("public_key.pem", "r"); // r = read mode
  pubkey = PEM_read_PUBKEY(file_key, NULL, NULL, NULL);

  // Error handling:
  if (!file_key) {
    fprintf(stderr, "Error: cannot open public key file\n");
    return 1;
  }
  if (!pubkey) {
    fprintf(stderr, "Error: failed to load the public key\n");
    return 1;
  }

  // Verify each message
  for (int i = 0; i < 3; i++) {
    printf("... Verifying message %d ...\n", i + 1);
    int result = verify(message_files[i], signature_files[i], pubkey);

    if (result < 0) {
      printf("Unknown authenticity of message %d\n", i + 1);
      print_file(message_files[i], CRESET);
    } else if (result == 0) {
      printf("Do not trust message %d!\n", i + 1);
      print_file(message_files[i], RED);
    } else {
      printf("Message %d is authentic!\n", i + 1);
      print_file(message_files[i], GRN);
    }
  }

  fclose(file_key);
  EVP_PKEY_free(pubkey);

  return 0;
}

/*
    Verify that the file `message_path` matches the signature `sign_path`
    using `pubkey`.
    Returns:
         1: Message matches signature
         0: Signature did not verify successfully
        -1: Message is does not match signature
*/
int verify(const char *message_path, const char *sign_path, EVP_PKEY *pubkey) {
#define MAX_FILE_SIZE 512
  unsigned char message[MAX_FILE_SIZE];
  unsigned char signature[MAX_FILE_SIZE];

  // TODO: Check if the message is authentic using the signature.
  EVP_MD_CTX *mdctx = NULL;
  mdctx = EVP_MD_CTX_new();

  if (mdctx == NULL) {
    fprintf(stderr, "EVP_MD_CTX failed\n");
    return -1;
  }

  // Reading exact message from file
  FILE *m_fp = fopen(message_path, "rb");
  if (!m_fp) {
    fprintf(stderr, "Error opening message\n");
    return -1;
  }
  size_t msg_len = fread(message, 1, MAX_FILE_SIZE, m_fp);

  // Reading exact signature file
  FILE *s_fp = fopen(sign_path, "rb");
  if (!s_fp) {
    fprintf(stderr, "Error opening message\n");
    return -1;
  }
  size_t sge_len = fread(signature, 1, MAX_FILE_SIZE, s_fp);
  fclose(s_fp);

  if (EVP_DigestVerifyInit(mdctx, NULL, EVP_sha256(), NULL, pubkey) != 1) {
    fprintf(stderr, "EVP_DigestVerifyInit failed\n");
    EVP_MD_CTX_free(mdctx);
    return -1;
  }

  if (EVP_DigestVerifyUpdate(mdctx, message, msg_len) != 1) {
    fprintf(stderr, "EVP_DigestVerifyUpdate failed\n");
    EVP_MD_CTX_free(mdctx);
    return -1;
  }

  int result = EVP_DigestVerifyFinal(mdctx, signature, sge_len);
  EVP_MD_CTX_free(mdctx);

  if (result == 1) {
    return 1;
  } else if (result == 0) {
    return 0;
  } else {
    return -1;
  }

  return -1;
}
