BUILD_TOOL=Engine/Build/Binaries/DirkBuildTool

.PHONY: setup
setup:
	@echo "Making sure build tool is up to date"
	@$(MAKE) -C Engine/Build/BuildTool
	@echo "Dirk Build Tool is up to date!"
	@echo "Running build tool setup..."
	@$(BUILD_TOOL) setup
