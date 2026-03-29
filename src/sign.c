// NurOS Ruzen42 2026 apg/sign.c
// Last change: Feb 2 

#include <sodium.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/util.h"

const char *key_path = "/etc/apg/keys/";

bool
sign_file_by_key(const char *path, const char *sig_path, const unsigned char *public_key)
{
  if (sodium_init() < 0) return false;

  unsigned char signature[crypto_sign_BYTES];

  FILE *sig_f = fopen(sig_path, "rb");
  if (!sig_f) return false;

  size_t sig_read = fread(signature, 1, crypto_sign_BYTES, sig_f);
  fclose(sig_f);
  if (sig_read != crypto_sign_BYTES) return false;

  crypto_sign_state state;
  crypto_sign_init(&state);

  FILE *pkg_f = fopen(path, "rb");
  if (!pkg_f) return false;

  unsigned char buffer[4096];
  size_t bytes_read;

  while ((bytes_read = fread(buffer, 1, sizeof(buffer), pkg_f)) > 0) {
    crypto_sign_update(&state, buffer, bytes_read);
  }
  fclose(pkg_f);

  if (crypto_sign_final_verify(&state, signature, public_key) != 0) {
    return false;
  }
  return true;
}


bool
sign_file(const char *pkg_path)
{
  char *sig_path = concat(pkg_path, ".sig");
  if (!sig_path) return false;

  char *key_file = concat(key_path, "trusted.pub");
  if (!key_file) {
    free(sig_path);
    return false;
  }

  unsigned char public_key[crypto_sign_PUBLICKEYBYTES];
  FILE *key_f = fopen(key_file, "rb");
  free(key_file);
  if (!key_f) {
    free(sig_path);
    return false;
  }

  size_t key_read = fread(public_key, 1, crypto_sign_PUBLICKEYBYTES, key_f);
  fclose(key_f);
  if (key_read != crypto_sign_PUBLICKEYBYTES) {
    free(sig_path);
    return false;
  }

  bool result = sign_file_by_key(pkg_path, sig_path, public_key);
  free(sig_path);
  return result;
}
