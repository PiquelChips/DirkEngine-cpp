BUILD=build
RELEASE=release
RESOURCES=$(RELEASE)/resources
NUM_JOBS=8

CMAKE_ARGS= -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ${DIRK_ENGINE_CMAKE_ARGS}

.PHONY: run
run: config
	@cmake --build $(BUILD) --config=Debug --target=run -j $(NUM_JOBS)

.PHONY: build
build: config
	@cmake --build $(BUILD) --config=Debug -j $(NUM_JOBS)

.PHONY: shaders
shaders: config
	@cmake --build $(BUILD) --config=Debug --target=shaders -j $(NUM_JOBS)

.PHONY: release
release: config
	@cmake --build $(BUILD) --config=Release -j $(NUM_JOBS)
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
