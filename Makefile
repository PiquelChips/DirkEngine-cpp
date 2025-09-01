BUILD_TOOL=Engine/Build/Binaries/DirkBuildTool

.PHONY: all
all:
	@echo "Setting up repository for development..."
	@$(MAKE) setup
	@echo "Generating build files..."
	@$(MAKE) generate

.PHONY: setup
setup:
	@echo "Making sure build tool is up to date"
	@$(MAKE) -C Engine/Build/BuildTool
	@echo "Dirk Build Tool is up to date!"
	@echo "Running build tool setup..."
	@$(BUILD_TOOL) setup

.PHONY: generate
generate:
	@$(BUILD_TOOL) generate
