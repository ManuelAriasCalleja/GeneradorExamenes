# Entorno de compilación del paquete de Linux: glibc 2.35 y el mismo Qt que el paquete de Windows.
FROM ubuntu:22.04
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build patchelf file python3-pip ca-certificates \
    libgl1-mesa-dev libegl1-mesa-dev libxkbcommon-dev \
    libxcb-cursor0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-randr0 libxcb-render-util0 libxcb-render0 \
    libxcb-shape0 libxcb-shm0 libxcb-sync1 libxcb-util1 libxcb-xfixes0 libxcb-xinerama0 libxcb-xkb1 \
    libxkbcommon0 libxkbcommon-x11-0 libdbus-1-3 libfontconfig1 libfreetype6 libglib2.0-0 libcups2 \
    libwayland-client0 libwayland-cursor0 libwayland-egl1 libnss3 libnspr4 fonts-dejavu-core fonts-liberation
RUN pip3 install --no-cache-dir aqtinstall \
    && aqt install-qt linux desktop 6.8.3 linux_gcc_64 -m qtpdf -O /opt/Qt \
    && rm -f aqtinstall.log
