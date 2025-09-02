BUILD_TOOL=Engine/Build/Binaries/DirkBuildTool
BUILD_DIR=Engine/Build
TARGETS=Editor Runtime

BIN_DIR=Binaries
INT_DIR=Intermediate

.PHONY: run $(TARGETS) clean
run:
	@$(MAKE) -C $(BUILD_DIR) run

$(TARGETS):
	@$(MAKE) -C $(BUILD_DIR) $@

clean:
	@echo Cleaning...
	@rm -rf $(BIN_DIR) $(INT_DIR)


.PHONY: setup
setup:
	@echo "Making sure build tool is up to date"
	@$(MAKE) -C Engine/Build/BuildTool
	@echo "Dirk Build Tool is up to date!"
	@echo "Running build tool setup..."
	@$(BUILD_TOOL) setup
