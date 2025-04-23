#include <mbedtls/aes.h>

extern String aesKey;   // 32 bytes for AES-256 = "01234567890123456789012345678901";

String encryptData(String plainText) {
  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  uint8_t iv[16] = {0}; // IV can be randomized

  size_t inputLen = plainText.length();
  size_t paddedLen = ((inputLen + 15) / 16) * 16;
  uint8_t input[paddedLen];
  memset(input, 0, paddedLen);
  memcpy(input, plainText.c_str(), inputLen);

  uint8_t output[paddedLen];
  mbedtls_aes_setkey_enc(&aes, (const unsigned char*)aesKey.c_str(), 256);
  mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, paddedLen, iv, input, output);

  mbedtls_aes_free(&aes);

  String encrypted = "";
  for (int i = 0; i < paddedLen; i++) {
    encrypted += String(output[i], HEX);
  }
  return encrypted;
}
