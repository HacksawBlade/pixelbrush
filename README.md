# PixelBrush

将图片转换为终端 ASCII 艺术画。

| 中文 | [English](README.en.md) |
| ---- | ----------------------- |

## 特性

- 多种颜色输出模式
- 多种笔刷预设
- 自动适配控制台窗口尺寸
- 支持重定向输出到文件

> 请确保终端模拟器支持 VT 序列（如 ConEmu、Windows Terminal）。仅限 Windows 平台。

## 效果

快速处理高像素照片。

![高像素照片](assets/screenshots/50mp.png)

> 此为终端字符足够小的情况下的效果图，后不再赘述。

---

使用可读字符作为笔刷。

![字符模式](assets/screenshots/letters.png)

---

通过亮度映射不同的字符，实现特别的黑白视觉效果。黑白模式仅在特定笔刷下正常绘制。

![黑白模式](assets/screenshots/blackwhite.png)

## 构建

使用 [Xmake](https://xmake.io/) 作为构建工具

首次克隆项目后，需要初始化子模块：

```sh
git submodule update --init --recursive
```

之后构建只需：

```sh
xmake
```

## 用法

```sh
pixelbrush <IMAGE-PATH> [OPTIONS]
```

直接运行 `pixelbrush` 以查看完整的参数列表。

### 笔刷

```sh
# 两种写法皆可
pixelbrush -b <BRUSH-NAME>
pixelbrush --brush <BRUSH-NAME>
```

| 名称            | 黑白模式 | 效果           |
| --------------- | :------: | -------------- |
| `block`（默认） |    —     | 纯色像素块     |
| `dot`           |    —     | 纯色圆点       |
| `shades`        |    ✓     | 亮度映射像素块 |
| `symbols`       |    ✓     | 符号           |
| `letters`       |    ✓     | 符号与字母     |

具有亮度映射功能的笔刷在真彩模式下的输出亮度略暗，但具备正常渲染纯黑白绘画的能力。

### 颜色模式

```sh
pixelbrush -c <MODE>
# 或
pixelbrush --color <MODE>
```

| 模式值       | 说明                                       | 示例图                                     |
| ------------ | ------------------------------------------ | ------------------------------------------ |
| `truecolor`  | 24-bit 真彩色（默认）                      |                                            |
| `tty16`      | 经典16色，从用户的终端颜色主题中取色       | ![TTY16](assets/screenshots/tty16.png)     |
| `tty256`     | 终端 256 色模式                            | ![TTY256](assets/screenshots/tty256.png)   |
| `grayscale`  | 灰阶模式，使用不同亮度的无彩色展现画面信息 | ![灰阶](assets/screenshots/grayscale.png)  |
| `blackwhite` | 黑白模式，具有独特的高对比度               | ![黑白](assets/screenshots/blackwhite.png) |

### 缩放算法

```sh
pixelbrush -m <MODE>
# 或
pixelbrush --scale-mode <MODE>
```

图片缩放到目标尺寸时使用的重采样算法。搭配像素画或需要保持原始锐利边缘的场景使用 `Nearest`，照片等连续色调图片用 `Fant`。

| 模式           | 说明                               |
| -------------- | ---------------------------------- |
| `Fant`（默认） | 高质量抗锯齿缩放，适合照片         |
| `Nearest`      | 最近邻插值，保留硬边缘，适合像素画 |

### 宽度缩放

终端中一个字符的长宽比是不确定的。若以“一个字符对应一个像素点”为前提进行像素画生成，实际效果往往就像图片被拉伸了一样。`PixelBrush` 默认假设当前终端“两个字符近似一个正方形”，内部设置的「宽度缩放」系数默认即为 **2.0**. 若用户的设备在这个系数下生成的绘画变形，可通过 `-w <FLOAT>` 或 `--wscale <FLOAT>` 进行系数设置，支持任意浮点数。

### 尺寸设置

在终端输出模式下，会根据用户终端的画面空间，以尽量不拉伸、不裁剪为要求自动计算需要输出的图片尺寸。若需要手动设置输出尺寸，使用 `-s <W> <H>` 或 `--size <W> <H>` 进行配置。其中，宽度的指定需要考虑到「宽度缩放」系数。如，在默认的宽度系数为 2 的情况下，绘制不变形的、常见的 4:3 比例的照片（假定需要输出 400 $\times$ 300），需要输入 `--size 800 300` (400 * 2 = 800).

### 输出到文件

```sh
pixelbrush <IMAGE-PATH> -o out.txt
```

使用 `--output` / `-o` 将输出写入文件。

`--output` 可以与 `--format` / `-f` 配合指定输出编码格式：

```sh
pixelbrush <IMAGE-PATH> -o out.txt -f UTF8
```

| 格式值            | 说明           |
| ----------------- | -------------- |
| `UTF8`            | UTF-8 编码     |
| `UTF16LE`（默认） | UTF-16 LE 编码 |

> UTF-16 LE 始终具有更好的性能，因为 Windows 使用该编码作为默认 Unicode 编码，`PixelBrush` 同样默认使用该编码，到其他编码的输出都要经过额外转换。

> `PixelBrush` 不会对文件输出进行针对性优化，文件中会包含大量 ANSI 控制序列，也会根据图片的原始尺寸生成像素画——这样的文件体积往往十分巨大。因此，推荐使用 `--color blackwhite` 模式进行作画，因为这样不会生成任何控制序列；为此需要设置支持黑白模式的笔刷；最后还建议使用 `--size` 手动指定输出尺寸，以免文件体积失控。

> 使用重定向功能，如 `pixelbrush image.jpg > output.txt` 输出的文件不再保证内容正确性，因为 PowerShell 重定向时默认的文本编码是不确定的。在传统 Command Prompt 上无该问题。

## 示例

```bash
pixelbrush photo.jpg -b block --color grayscale
pixelbrush image.png -b symbols --size 80 40
pixelbrush photo.bmp -c blackwhite -b shades > output.txt
```
