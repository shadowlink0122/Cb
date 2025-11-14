# Cb FFI Test Makefile
# Dockerコンテナでの多言語FFIテスト

DOCKER_IMAGE = cb-ffi-test
CONTAINER_NAME = cb-ffi-test-container

.PHONY: help build run test clean shell

help:
	@echo "Cb FFI Test Environment"
	@echo ""
	@echo "Usage:"
	@echo "  make build       - Dockerイメージをビルド"
	@echo "  make run         - コンテナを起動してテスト実行"
	@echo "  make test        - すべての言語でFFIテストを実行"
	@echo "  make test-c      - Cライブラリのテスト"
	@echo "  make test-cpp    - C++ライブラリのテスト"
	@echo "  make test-rust   - Rustライブラリのテスト"
	@echo "  make test-go     - Goライブラリのテスト"
	@echo "  make test-zig    - Zigライブラリのテスト"
	@echo "  make shell       - コンテナのシェルに入る"
	@echo "  make clean       - コンテナとイメージを削除"

# Dockerイメージのビルド
build:
	@echo "Building Docker image..."
	docker build -t $(DOCKER_IMAGE) .

# コンテナの起動とテスト実行
run: build
	@echo "Running FFI tests in Docker container..."
	docker run --rm -v $(PWD)/../..:/cb $(DOCKER_IMAGE) /bin/bash -c "cd /cb && make -C /cb && /cb/tests/ffi/run_tests.sh"

# すべての言語でテスト
test: build
	@echo "Running all FFI tests..."
	docker run --rm -v $(PWD)/../..:/cb $(DOCKER_IMAGE) /bin/bash -c "cd /cb/tests/ffi && ./run_tests.sh all"

# 言語別テスト
test-c: build
	docker run --rm -v $(PWD)/../..:/cb $(DOCKER_IMAGE) /bin/bash -c "cd /cb/tests/ffi && ./run_tests.sh c"

test-cpp: build
	docker run --rm -v $(PWD)/../..:/cb $(DOCKER_IMAGE) /bin/bash -c "cd /cb/tests/ffi && ./run_tests.sh cpp"

test-rust: build
	docker run --rm -v $(PWD)/../..:/cb $(DOCKER_IMAGE) /bin/bash -c "cd /cb/tests/ffi && ./run_tests.sh rust"

test-go: build
	docker run --rm -v $(PWD)/../..:/cb $(DOCKER_IMAGE) /bin/bash -c "cd /cb/tests/ffi && ./run_tests.sh go"

test-zig: build
	docker run --rm -v $(PWD)/../..:/cb $(DOCKER_IMAGE) /bin/bash -c "cd /cb/tests/ffi && ./run_tests.sh zig"

# デバッグ用: コンテナのシェルに入る
shell: build
	docker run --rm -it -v $(PWD)/../..:/cb $(DOCKER_IMAGE) /bin/bash

# クリーンアップ
clean:
	@echo "Cleaning up Docker resources..."
	-docker stop $(CONTAINER_NAME) 2>/dev/null || true
	-docker rm $(CONTAINER_NAME) 2>/dev/null || true
	-docker rmi $(DOCKER_IMAGE) 2>/dev/null || true
	@echo "Cleanup complete."
