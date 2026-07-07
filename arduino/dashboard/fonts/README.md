# Montserrat VLW fonts for Arduino

Generated headers go in this folder. Enable smooth fonts in Seeed_GFX `User_Setup.h`:

```cpp
#define SMOOTH_FONT
```

## Generate with Processing (TFT_eSPI Create_font)

1. Open `Seeed_GFX/Tools/Create_Smooth_Font/Create_font/Create_font.pde` in Processing 4  
   (or the same path under `TFT_eSPI/Tools/Create_Smooth_Font/Create_font/` if you use that library)
2. Set `boolean openFolder = false;` near the top — Processing 4 treats `.open()` as a syntax error otherwise
3. Copy TTF files from `assets/fonts/` into the sketch `data/` folder
3. For each job below, set variables and run the sketch (creates `.h` in sketch folder)
4. Move the `.h` files here and uncomment `#define USE_MONTSERRAT` in `display_font.h`

| Output header | TTF | Size (pt) | Charset |
| --- | --- | --- | --- |
| `Montserrat_30.h` | Montserrat-Medium.ttf | 30 | `0123456789.:|%°ABCDEFGHIJKLMNOPQRSTUVWXY…` |
| `Montserrat_26.h` | Montserrat-Medium.ttf | 26 | `0123456789.:|%°ABCDEFGHIJKLMNOPQRSTUVWXY…` |
| `Montserrat_24.h` | Montserrat-Medium.ttf | 24 | `0123456789.:|%°ABCDEFGHIJKLMNOPQRSTUVWXY…` |
| `Montserrat_17.h` | Montserrat-Medium.ttf | 17 | `0123456789.:|%°ABCDEFGHIJKLMNOPQRSTUVWXY…` |
| `Montserrat_17_Bold.h` | Montserrat-Bold.ttf | 17 | `0123456789.:|%°ABCDEFGHIJKLMNOPQRSTUVWXY…` |
| `Montserrat_16.h` | Montserrat-Medium.ttf | 16 | `0123456789.:|%°ABCDEFGHIJKLMNOPQRSTUVWXY…` |

Processing settings per job:
- `fontName` = TTF filename without extension
- `fontSize` = size column above
- `specificCharacters` = charset string (Create_font "Specific characters" field)
- `createHeader` = true

