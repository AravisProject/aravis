# Using containers for Aravis development and testing

This document provides instructions for using Docker to set up a development environment for Aravis, as well as a viewer-only mode for testing. The Docker setup includes:

- Multi-stage build that automatically compiles Aravis
- Two build targets:
  - **development**: Full dev environment with zsh, tools, and pre-built Aravis
  - **viewer**: Minimal runtime image that launches arv-viewer directly

## Requirements

- X11 server (for GUI applications)
- USB access for physical cameras (configured via privileged mode)

## Quick Start

### Development Environment

For development work with access to all tools and shell:

```bash
docker compose up -d dev
docker compose exec dev zsh
```

Inside the container, Aravis is pre-built at `/opt/aravis/build/`:
- Viewer: `/opt/aravis/build/viewer/arv-viewer-0.10`
- Tools: `/opt/aravis/build/src/arv-tool-0.10`
- Tests: `/opt/aravis/build/tests/`

### Viewer-Only Mode

To run just the Aravis viewer:

```bash
docker compose up viewer
```

This launches a minimal container that automatically opens arv-viewer.

## Building

The Dockerfile uses multi-stage builds:

```bash
# Build development image (default)
docker build -t aravis-dev .

# Build viewer-only image
docker build --target viewer -t aravis-viewer .

# Build from the local workspace source instead of a release tarball
docker build --build-arg ARAVIS_SOURCE_MODE=workspace -t aravis-dev .
```

## Configuration

Create a `.env` file to customize build parameters:

```env
USERNAME=your-username
DISTRO_NAME=trixie
ARAVIS_VERSION=0.9.1
ARAVIS_SOURCE_MODE=release
CONTAINER_DISPLAY=:10
```

`ARAVIS_SOURCE_MODE` accepts:
- `release` (default): downloads and builds from `ARAVIS_VERSION`
- `workspace`: builds from the current local repository source copied into the image

## Testing with Fake Camera

To test without physical hardware:

```bash
# In one terminal, start the fake camera
docker compose exec dev /opt/aravis/build/src/arv-fake-gv-camera-0.10

# In another terminal, run the viewer Inside the dev container
/opt/aravis/build/viewer/arv-viewer-0.10
```

## X11 Display

The container is configured for X11 forwarding. Ensure your host allows X11 connections:

```bash
xhost +local:docker
```