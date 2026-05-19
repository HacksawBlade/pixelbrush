# PixelBrush

Convert images to terminal ASCII art.

| [中文](README.md) | English |
| ----------------- | ------- |

## Features

- Multiple color output modes
- Multiple brush presets
- Auto-fit to console window size
- Supports redirection to file output

> Make sure your terminal emulator supports VT sequences (e.g., ConEmu, Windows Terminal). Windows only.

## Gallery

High-resolution photo rendered.

![High-resolution photo](assets/screenshots/50mp.png)

> Rendered at a sufficiently small character size, truncated for display.

---

Readable characters used as brush strokes.

![Character mode](assets/screenshots/letters.png)

---

Brightness-mapped characters for a distinctive black & white look. Black & white mode only works correctly with certain brushes.

![Black & white mode](assets/screenshots/blackwhite.png)

## Build

Built with [Xmake](https://xmake.io/).

After cloning the project for the first time, initialize the submodule:

```sh
git submodule update --init --recursive
```

Then build simply with:

```sh
xmake
```

## Usage

```sh
pixelbrush <IMAGE-PATH> [OPTIONS]
```

Run `pixelbrush` without arguments to view the full parameter list.

### Brushes

```sh
# Both forms work
pixelbrush -b <BRUSH-NAME>
pixelbrush --brush <BRUSH-NAME>
```

| Name              | B&W Mode | Description                    |
| ----------------- | :------: | ------------------------------ |
| `block` (default) |    —     | Solid color pixel blocks       |
| `dot`             |    —     | Solid color dots               |
| `shades`          |    ✓     | Brightness-mapped pixel blocks |
| `symbols`         |    ✓     | Symbols                        |
| `letters`         |    ✓     | Symbols and letters            |

Brushes with brightness mapping produce slightly dimmer colors in true color mode but support proper black & white rendering.

### Color Mode

```sh
pixelbrush -c <MODE>
# or
pixelbrush --color <MODE>
```

| Mode         | Description                                                           | Example                                        |
| ------------ | --------------------------------------------------------------------- | ---------------------------------------------- |
| `truecolor`  | 24-bit true color (default)                                           |                                                |
| `tty16`      | Classic 16 colors, sourced from your terminal's color theme           | ![TTY16](assets/screenshots/tty16.png)         |
| `tty256`     | Terminal 256-color mode                                               | ![TTY256](assets/screenshots/tty256.png)       |
| `grayscale`  | Grayscale mode, displays image info using different brightness levels | ![Grayscale](assets/screenshots/grayscale.png) |
| `blackwhite` | Black & white mode, unique high contrast effect                       | ![B&W](assets/screenshots/blackwhite.png)      |

### Width Scale

Terminal character aspect ratios vary across platforms and devices. If each character maps to one pixel, the output may appear stretched. PixelBrush defaults to assuming "two characters ≈ a square" for modern terminals, setting the width scale to **2.0** by default. If the output appears distorted on your device, adjust it with `-w <FLOAT>` or `--wscale <FLOAT>` — any floating-point value is accepted.

### Output Size

In terminal mode, PixelBrush automatically computes the best output size to avoid stretching or cropping based on the available console space. To manually set the output size, use `-s <W> <H>` or `--size <W> <H>`. The width value should account for the width scale factor. For example, at the default scale of 2, to render a common 4:3 image with 400 effective columns (effectively 300 rows), use `--size 800 300` (400 × 2 = 800).

### Output to File

```sh
pixelbrush <IMAGE-PATH> > out.txt
```

Redirect output to a file using `>`. PixelBrush does not optimize for file output — the file will contain ANSI escape sequences and the output size follows the original image dimensions, which can lead to very large files. It is therefore recommended to:

- Use `--color blackwhite` mode to avoid color control sequences.
- Use a brush that supports black & white mode.
- Manually specify output size with `--size` to prevent uncontrolled file growth.

## Examples

```bash
pixelbrush photo.jpg -b block --color grayscale
pixelbrush image.png -b symbols --size 80 40
pixelbrush photo.bmp --color blackwhite > output.txt
```
