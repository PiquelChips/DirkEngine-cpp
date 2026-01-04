.PHONY: protocols
protocols:
	@mkdir -p include src
	@for protocol in protocols/*.xml; do \
		basename=$$(basename $$protocol .xml); \
		echo "Generating $$basename..."; \
		wayland-scanner client-header < $$protocol > include/$$basename-client-protocol.h; \
		wayland-scanner public-code < $$protocol > src/$$basename-protocol.cpp; \
	done
