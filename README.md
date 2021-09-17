## Setup cmake
```bash
mkdir bin
cd bin
cmake ..
```

## Build and run Docker Image
```bash
docker build -t ubuntu_vs .
docker run -p 5000:22 -i -t ubuntu-vs /bin/bash
./tmp/start_service.sh
```

## Buid
```bash
make
```

## Run
1. Add your API keys to the clients
2. Run the examples:
```bash
./src/example/rest_test
./src/example/ws_test
./src/example/bitstamp_ws_test
```
