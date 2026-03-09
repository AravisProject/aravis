ARG DISTRO_NAME=trixie
ARG BASE_IMAGE=debian:${DISTRO_NAME}

FROM ${BASE_IMAGE} AS zsh-builder

# Install only what is required to prepare oh-my-zsh assets.
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    curl \
    git \
    zsh \
 && apt-get clean && rm -rf /var/lib/apt/lists/*

# Prepare oh-my-zsh and plugins in non-interactive mode.
RUN set -eux; \
    export RUNZSH=no CHSH=no KEEP_ZSHRC=yes; \
    sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"; \
    mkdir -p /root/.ssh; \
    chmod 700 /root/.ssh; \
    git clone --depth=1 https://github.com/zsh-users/zsh-autosuggestions.git /root/.oh-my-zsh/custom/plugins/zsh-autosuggestions; \
    git clone --depth=1 https://github.com/zsh-users/zsh-syntax-highlighting.git /root/.oh-my-zsh/custom/plugins/zsh-syntax-highlighting; \
    sed -i 's/^plugins=(.*)$/plugins=(git ssh-agent zsh-autosuggestions zsh-syntax-highlighting)/' /root/.zshrc

FROM ${BASE_IMAGE} AS aravis-workspace

# Keep a copy of the current workspace so the builder can compile local sources.
COPY . /opt/aravis-workspace

FROM ${BASE_IMAGE} AS aravis-builder

ARG ARAVIS_VERSION=0.9.1
ARG ARAVIS_SRC_DIR=/opt/aravis
ARG ARAVIS_SOURCE_MODE=release

# Install Aravis build dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    wget \
    build-essential \
    meson \
    ninja-build \
    pkg-config \
    libxml2-dev \
    libglib2.0-dev \
    libusb-1.0-0-dev \
    gobject-introspection \
    libgirepository1.0-dev \
    libgtk-3-dev \
    gtk-doc-tools \
    xsltproc \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    gettext \
    cmake \
 && apt-get clean && rm -rf /var/lib/apt/lists/*

COPY --from=aravis-workspace /opt/aravis-workspace /opt/aravis-workspace

# Build Aravis either from a release tarball or from the local workspace source.
RUN set -eux; \
    mkdir -p "${ARAVIS_SRC_DIR}"; \
    if [ "${ARAVIS_SOURCE_MODE}" = "workspace" ]; then \
        cp -a /opt/aravis-workspace/. "${ARAVIS_SRC_DIR}"; \
    else \
        wget -O /tmp/aravis.tar.gz "https://github.com/AravisProject/aravis/archive/refs/tags/${ARAVIS_VERSION}.tar.gz"; \
        tar -xzf /tmp/aravis.tar.gz --strip-components=1 -C "${ARAVIS_SRC_DIR}"; \
        rm -f /tmp/aravis.tar.gz; \
    fi; \
    cd "${ARAVIS_SRC_DIR}"; \
    meson setup build; \
    meson compile -C build

FROM ${BASE_IMAGE} AS development

ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=${USER_UID}
ARG ARAVIS_SRC_DIR=/opt/aravis

# Install runtime tools and Aravis runtime dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    sudo \
    zsh \
    openssh-client \
    ca-certificates \
    curl \
    git \
    wget \
    unzip \
    xclip \
    micro \
    btop \
    python3 \
    python3-pip \
    python3-venv \
    x11-apps \
    libxml2 \
    libglib2.0-0 \
    libusb-1.0-0 \
    libgtk-3-0 \
    libgstreamer1.0-0 \
    libgstreamer-plugins-base1.0-0 \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-gtk3 \
 && apt-get clean && rm -rf /var/lib/apt/lists/*

# Copy built Aravis from builder stage
COPY --from=aravis-builder /opt/aravis /opt/aravis

# Create or reuse user/group matching host UID/GID and grant sudo.
RUN set -eux; \
    if [ -z "${USERNAME}" ]; then \
        USERNAME=vscode; \
    fi; \
    existing_user="$(getent passwd "${USER_UID}" | cut -d: -f1 || true)"; \
    if [ -n "${existing_user}" ] && [ "${existing_user}" != "${USERNAME}" ]; then \
        usermod -l "${USERNAME}" "${existing_user}"; \
        usermod -d "/home/${USERNAME}" -m "${USERNAME}" || true; \
    fi; \
    if ! getent group "${USER_GID}" >/dev/null 2>&1; then \
        groupadd --gid "${USER_GID}" "${USERNAME}"; \
    else \
        group_name="$(getent group "${USER_GID}" | cut -d: -f1)"; \
        if [ "${group_name}" != "${USERNAME}" ]; then \
            groupmod -n "${USERNAME}" "${group_name}" || true; \
        fi; \
    fi; \
    if ! id -u "${USERNAME}" >/dev/null 2>&1; then \
        useradd --uid "${USER_UID}" --gid "${USER_GID}" -m "${USERNAME}"; \
    else \
        usermod -u "${USER_UID}" -g "${USER_GID}" "${USERNAME}" || true; \
    fi; \
    echo "${USERNAME} ALL=(root) NOPASSWD:ALL" > "/etc/sudoers.d/${USERNAME}"; \
    chmod 0440 "/etc/sudoers.d/${USERNAME}"

# Copy prepared zsh configuration from the builder stage for root and user.
COPY --from=zsh-builder /root/.oh-my-zsh /root/.oh-my-zsh
COPY --from=zsh-builder /root/.zshrc /root/.zshrc

RUN set -eux; \
    mkdir -p /root/.ssh; \
    chmod 700 /root/.ssh; \
    cp -a /root/.oh-my-zsh "/home/${USERNAME}/.oh-my-zsh"; \
    cp /root/.zshrc "/home/${USERNAME}/.zshrc"; \
    mkdir -p "/home/${USERNAME}/.ssh"; \
    chmod 700 "/home/${USERNAME}/.ssh"; \
    chown -R "${USERNAME}:${USER_GID}" "/home/${USERNAME}"; \
    chsh -s /bin/zsh root; \
    chsh -s /bin/zsh "${USERNAME}"

CMD ["/bin/zsh"]

# Viewer-only stage - minimal runtime for just running arv-viewer
FROM ${BASE_IMAGE} AS viewer

ARG ARAVIS_SRC_DIR=/opt/aravis

# Install only runtime dependencies needed for the viewer
RUN apt-get update && apt-get install -y --no-install-recommends \
    libxml2 \
    libglib2.0-0 \
    libusb-1.0-0 \
    libgtk-3-0 \
    libgstreamer1.0-0 \
    libgstreamer-plugins-base1.0-0 \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-gtk3 \
 && apt-get clean && rm -rf /var/lib/apt/lists/*

# Copy only the built viewer and necessary libraries from builder
COPY --from=aravis-builder /opt/aravis/build/viewer/arv-viewer-0.10 /usr/local/bin/arv-viewer
COPY --from=aravis-builder /opt/aravis/build/src/libaravis-0.10.so.* /usr/local/lib/
COPY --from=aravis-builder /opt/aravis/build/gst/libgstaravis.*.so /usr/local/lib/gstreamer-1.0/

# Update library cache
RUN ldconfig

CMD ["/usr/local/bin/arv-viewer"]

