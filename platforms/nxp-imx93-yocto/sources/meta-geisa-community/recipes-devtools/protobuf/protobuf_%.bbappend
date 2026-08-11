# Build protoc for GEISA reference/development images. The compiler package is
# installed only by geisa-dev-image; application images use the runtime only.
PACKAGECONFIG:append = " compiler"
