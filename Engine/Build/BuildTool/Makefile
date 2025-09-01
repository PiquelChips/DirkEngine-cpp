OUT=../Binaries/DirkBuildTool
SRC=$(shell find . -type f -name '*.go')

.PHONY: all
all: $(OUT)
	@echo Building...

$(OUT): $(SRC)
	@go build -o $(OUT) main.go

