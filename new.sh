arduino-cli --additional-urls "$ADDURLS" --additional-urls "$M5JSON" core install "$ESPCORE"
arduino-cli --additional-urls "$ADDURLS" lib update-index
arduino-cli --additional-urls "$ADDURLS" --additional-urls "$ESPJSON" lib install M5Stack
arduino-cli --additional-urls "$ADDURLS" core update-index
arduino-cli --additional-urls "$ADDURLS" core install m5stack:esp32
