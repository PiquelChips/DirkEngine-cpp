BUILD=build
RELEASE=release
RESOURCES=$(RELEASE)/resources

CMAKE_ARGS= -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ${DIRK_ENGINE_CMAKE_ARGS}

.PHONY: run
run: config
	@cmake --build $(BUILD) --config=Debug --target=run

.PHONY: build
build: config
	@cmake --build $(BUILD) --config=Debug

.PHONY: shaders
shaders: config
	@cmake --build $(BUILD) --config=Debug --target=shaders

.PHONY: release
release: config
	@cmake --build $(BUILD) --config=Release
	@echo Creating release...
	@rm -rf $(RELEASE)
	@mkdir $(RELEASE)
	@cp $(BUILD)/DirkEngine $(RELEASE)
	@cp -r $(RESOURCES) $(RELEASE)
	@cp -r $(BUILD)/shaders $(RESOURCES)
	@echo Release created!

.PHONY: config
config:
	@cmake -S . -B $(BUILD) $(CMAKE_ARGS)

.PHONY: clean
clean:
	@git clean -dfx
