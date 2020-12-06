#!/usr/bin/env sh

compileupload() {
	docker build -t paperweather . &&
	docker run -it --rm -v /dev:/dev --privileged paperweather compileupload
}

shell() {
	docker build -t paperweather . &&
	docker run -v /dev:/dev -v $(pwd)/Paperweather:/ctx/Paperweather -it --rm --privileged paperweather bash
}

$1 || echo "Usage: ./$1 compileupload"
