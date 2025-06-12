# Loup
A extremely tiny wayland wallpaper daemon that displays wallpaper content from an existing shared memory file.

## Build from Source

### Dependencies
- `meson` >= 1.8.0
- `gcc` >= 15 or `clang` >= 20
- `libwayland-client.so`

```sh
git clone --recursive https://github.com/00-KAMIDUKI/loup
cd loup
meson setup build
meson compile -C build
```

## Usage
`loup` expects an exsiting file at `/run/user/$uid/loup`.
User is responsible for sizing it correctly.

Use other tools like `ffmpeg` to convert your image to raw BGRA format, then start the daemon:
```sh
ffmpeg -i $1 -vf scale=1920:1080 -pix_fmt bgra -f rawvideo /run/user/$(id -u)/loup
build/loup
```

## TODO
- Inter-process synchronization (automatically toggle wallpapers).
- Interpolation animation.
