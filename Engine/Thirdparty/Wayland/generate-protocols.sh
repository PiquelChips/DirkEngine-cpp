wayland-scanner client-header < wayland.xml > wayland-client-protocol.h
wayland-scanner client-header < xdg-shell.xml > xdg-shell-client-protocol.h
wayland-scanner private-code < xdg-shell.xml > xdg-shell-client-protocol.c
