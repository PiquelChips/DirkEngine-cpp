wayland-scanner client-header < protocols/wayland.xml > include/wayland-client-protocol.h
wayland-scanner client-header < protocols/xdg-shell.xml > include/xdg-shell-client-protocol.h
wayland-scanner public-code < protocols/xdg-shell.xml > src/xdg-shell-protocol.cpp
