FROM debian:stretch-slim
ENV FBQN=m5stack:esp32:m5stack-m5paper
ENV PRJ=Paperweather
ENV BINARY="./arduino-cli"
ENV ESPJSON="https://dl.espressif.com/dl/package_esp32_index.json"
ENV M5JSON="https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json"
ENV ADDURLS="$ESPJSON,$M5JSON"
ENV ESPCORE="esp32:esp32"

RUN apt-get -y update && apt-get -y install curl python python-serial wget
RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh -s 0.13.0
RUN arduino-cli --additional-urls "$ADDURLS" --additional-urls "$M5JSON" core install "$ESPCORE"
RUN arduino-cli --additional-urls "$ADDURLS" lib update-index
RUN arduino-cli --additional-urls "$ADDURLS" --additional-urls "$ESPJSON" lib install M5Stack
RUN arduino-cli --additional-urls "$ADDURLS" core update-index
RUN arduino-cli --additional-urls "$ADDURLS" core install m5stack:esp32
RUN wget 'https://github.com/m5stack/M5EPD/archive/0.1.0.tar.gz'
RUN tar xvfz 0.1.0.tar.gz -C /root/Arduino/libraries

WORKDIR /ctx/
COPY . /ctx/
RUN printf %b '#!/usr/bin/env sh \n\
  arduino-cli -v --additional-urls "$ESPJSON" compile --build-path /bp --build-cache-path /bc --fqbn $FBQN $PRJ; \n\
  arduino-cli -v upload -p /dev/serial/by-id/usb-Silicon_Labs_CP2104_USB_to_UART_Bridge_Controller_* --fqbn $FBQN $PRJ \n\
' > /usr/bin/compileupload
RUN chmod +x /usr/bin/compileupload

CMD compileupload
