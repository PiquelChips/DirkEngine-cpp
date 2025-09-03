BUILD_DIR=Engine/Build

BIN_DIR=Binaries
INT_DIR=Intermediate

.PHONY: run build clean
run: build
	@echo Running Editor...
	@Binaries/Editor

build:
	@$(MAKE) -C $(BUILD_DIR) Editor

clean:
	@echo Cleaning...
	@rm -rf $(BIN_DIR) $(INT_DIR)
