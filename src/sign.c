// NurOS Ruzen42 2026 apg/sign.c
// Last change: Feb 2 

#include <soduim.h>
#include <stdbool.h>
#include <stdio.h>

const char *key_path = "/etc/apg/keys/";

bool 
sign_file_by_key(const char *path, const char *sig_path, const unsigned char *public_key[crypto_sign_PUBLICKEYBYTES])
{
  if (sodium_init() < 0) return false;

  unsigned char signature[crypto_sign_BYTES];

  FILE *sig_f = fopen(sig_path, "rb"); // open file

  if (!sig_f) return false;

  int sig_read = fread(signature, 1, crypto_sign_BYTES, sig_f);

  fclose(sig_f); 
  
  crypto_sign_state state; // Sodium signature state now 
  crypto_sign_init(&state);

  FILE *pkg_f = fopen(path, "rb");

  if (!pkg_f) return false;

  unsigned char buffer[4096]; // 4KB magic number
  int bytes_read;

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
     

  return sign_file_by_key(pkg_path
  
}
