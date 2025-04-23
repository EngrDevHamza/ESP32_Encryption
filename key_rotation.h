#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <Arduino.h>
#include <Preferences.h>

extern String aesKey;
extern Preferences preferences;

void generateInitialKey() {
  preferences.begin("keys", false);
  if (!preferences.isKey("key0")) {
    aesKey = "01234567890123456789012345678901"; // default key
    preferences.putString("key0", aesKey); // store it
    preferences.putUInt("keyIndex", 1); // next index
  } else {
    aesKey = preferences.getString("key0"); // load existing key
  }
  preferences.end();
}

void rotateEncryptionKey() {
  String newKey = "";
  for (int i = 0; i < 32; i++) {
    newKey += char(random(33, 126)); // printable ASCII
  }

  preferences.begin("keys", false);
  uint32_t index = preferences.getUInt("keyIndex", 1);

  String keyName = "key" + String(index);
  preferences.putString(keyName.c_str(), aesKey);  // fix: use c_str()
  preferences.putUInt("keyIndex", index + 1);
  preferences.end();

  aesKey = newKey;
  Serial.println("Key rotated and previous key stored.");
}

#endif // CRYPTO_UTILS_H
