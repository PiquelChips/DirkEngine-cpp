BUILD_TOOL=Engine/Build/Binaries/DirkBuildTool
BUILD_DIR=Engine/Build
TARGETS=Editor Runtime

.PHONY: run $(TARGETS)
run:
	@$(MAKE) -C $(BUILD_DIR) run

$(TARGETS):
	@$(MAKE) -C $(BUILD_DIR) $@


.PHONY: setup
setup:
	@echo "Making sure build tool is up to date"
	@$(MAKE) -C Engine/Build/BuildTool
	@echo "Dirk Build Tool is up to date!"
	@echo "Running build tool setup..."
	@$(BUILD_TOOL) setup
