# SkikoWayland

> Warning: this is slop code generated with AI. It is intended to be used by
> Echo and people who want to try BetterWindow on direct Wayland rendering.

This fork is based on JetBrains Skiko and prototypes direct Compose Desktop
rendering on JBR's Wayland AWT toolkit (`sun.awt.wl.WLToolkit`).

The goal is to let Skiko render through Wayland EGL into a JBR-owned
`wl_surface` instead of using the old X11 JAWT path or falling back to
Swing/AWT graphics.

## Changes In This Fork

- Adds a Linux Wayland EGL redrawer for Compose Desktop/Skiko AWT.
- Detects `sun.awt.wl.WLToolkit` and selects the Wayland redrawer on Linux.
- Reads JBR's `JAWT_WaylandDrawingSurfaceInfo` from `JAWT_DrawingSurface`.
- Uses the JBR-provided `wl_display` and render `wl_surface`.
- Creates a `wl_egl_window` and EGL OpenGL context for that Wayland surface.
- Resizes the existing `wl_egl_window` when the component/window size changes.
- Uses `wl_surface_set_buffer_scale(surface, 1)` and explicit buffer sizing.
- Supports fractional/effective scale from patched JBR, for example `1.5x`.
- Keeps the old GLX/X11 redrawer path for non-Wayland Linux toolkits.
- Keeps the old software X11 redrawer from accidentally using Wayland surface
  info.
- Adds EGL/Wayland native linker inputs:
  - `-lEGL`
  - `-lwayland-client`
  - `-lwayland-egl`

This Skiko fork expects a matching patched JBR fork that exposes Wayland JAWT
surface info. Without that JBR-side API, direct Wayland EGL rendering cannot
work.

## Matching JBR Fork

Use this with:

```text
https://github.com/brahmkshatriya/JBRWayland
```

At runtime the JBR must provide:

- working `JAWT_DrawingSurface.Lock` under `WLToolkit`
- `JAWT_WaylandDrawingSurfaceInfo`
- JBR-owned direct render `wl_surface`
- integer Wayland buffer scale
- fractional/effective UI scale

## Build Skiko

Install normal Skiko build dependencies plus Linux Wayland/EGL development
packages:

- `wayland-client`
- `wayland-egl`
- `EGL`
- OpenGL development headers
- X11 development headers
- fontconfig development headers

Build the AWT Kotlin and Linux x64 native bindings:

```bash
./gradlew :skiko:compileKotlinAwt \
  :skiko:linkJvmBindingsLinuxX64 \
  --no-daemon
```

## Publish To Maven Local

For local Compose Desktop testing, publish this fork with the same version
Compose is already requesting:

```bash
./gradlew :skiko:publishToMavenLocal \
  -Pdeploy.version=0.148.1 \
  --no-daemon
```

This publishes `0.148.1-SNAPSHOT` artifacts to Maven local, including:

- `org.jetbrains.skiko:skiko`
- `org.jetbrains.skiko:skiko-awt`
- `org.jetbrains.skiko:skiko-awt-runtime-linux-x64`

## Use In Compose Desktop

Make sure `mavenLocal()` is first in repositories:

```kotlin
repositories {
    mavenLocal()
    google()
    mavenCentral()
}
```

Then substitute Compose's Skiko dependencies to the locally published fork:

```kotlin
subprojects {
    configurations.configureEach {
        resolutionStrategy.dependencySubstitution {
            substitute(module("org.jetbrains.skiko:skiko"))
                .using(module("org.jetbrains.skiko:skiko:0.148.1-SNAPSHOT"))
            substitute(module("org.jetbrains.skiko:skiko-awt"))
                .using(module("org.jetbrains.skiko:skiko-awt:0.148.1-SNAPSHOT"))
            substitute(module("org.jetbrains.skiko:skiko-awt-runtime-linux-x64"))
                .using(module("org.jetbrains.skiko:skiko-awt-runtime-linux-x64:0.148.1-SNAPSHOT"))
        }
    }
}
```

Run the app on the patched JBRWayland runtime and enable WLToolkit:

```text
-Dawt.toolkit.name=WLToolkit
-Dcompose.application.configure.swing.globals=false
-Dskiko.hardwareInfo.enabled=true
--enable-native-access=ALL-UNNAMED
--add-exports=java.desktop/sun.awt=ALL-UNNAMED
--add-opens=java.desktop/sun.awt.wl=ALL-UNNAMED
```

`compose.application.configure.swing.globals=false` matters for fractional
Wayland scale. Without it, Compose can hit a Swing setup path that rounds
scale and breaks fullscreen sizing.

## Expected Runtime Logs

With `-Dskiko.hardwareInfo.enabled=true`, a `1.5x` Wayland setup should show:

```text
LinuxWaylandEGLRedrawer initialized ... scale=2, effectiveScale=1.5
LinuxWaylandEGLRedrawer surface update ... contentScale=1.5 ... buffer=1200x900
```

Here:

- `scale=2` is the integer Wayland buffer scale.
- `effectiveScale=1.5` is the fractional UI/render scale from JBR.
- `buffer=1200x900` for an `800x600` window confirms fractional sizing.

## Current Status

This is a prototype. It was tested with BetterWindow on Wayland using the
matching JBRWayland fork and fractional `1.5x` OS scale.

Known rough edges:

- The JBR-side Wayland JAWT API is not upstream.
- The Skiko-side Wayland EGL path is not upstream.
- The native API shape is experimental and may change.
- This is Linux/Wayland-focused; X11 keeps using the old Skiko path.
