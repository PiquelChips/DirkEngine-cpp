TARGET=Editor
DEPS=Runtime
SOURCE_DIR=../Source
BIN_DIR=../../Binaries

.PHONY: run $(TARGET) $(DEPS)
run: $(TARGET)
	@echo Running $(BIN_DIR)/$(TARGET)

$(TARGET): $(DEPS)
	@$(MAKE) -C $(SOURCE_DIR)/$@

$(DEPS):
	@$(MAKE) -C $(SOURCE_DIR)/$@
