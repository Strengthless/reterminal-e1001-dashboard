#!/usr/bin/env node

/**
 * Generates TFT_eSPI / Seeed_GFX VLW font headers from assets/fonts/*.ttf
 *
 * Requires Processing 4+ with the Create_font sketch from Bodmer/TFT_eSPI:
 *   <Arduino/libraries>/TFT_eSPI/Tools/Create_font/Create_font.pde
 *
 * This script copies our TTF files into that sketch's data/ folder and prints
 * the settings to use for each dashboard size.
 */

import { existsSync, copyFileSync, mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import { join, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const ROOT = join(dirname(fileURLToPath(import.meta.url)), '..');
const TTF_DIR = join(ROOT, 'assets', 'fonts');
const OUT_DIR = join(ROOT, 'arduino', 'dashboard', 'fonts');

const CHARSET = '0123456789.:|%°ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz /·-min feels likeOvercastSunnyStatusGood ServiceUnavailablestale—';

const JOBS = [
  { size: 30, ttf: 'Montserrat-Medium.ttf', header: 'Montserrat_30', charset: CHARSET },
  { size: 26, ttf: 'Montserrat-Medium.ttf', header: 'Montserrat_26', charset: CHARSET },
  { size: 24, ttf: 'Montserrat-Medium.ttf', header: 'Montserrat_24', charset: CHARSET },
  { size: 17, ttf: 'Montserrat-Medium.ttf', header: 'Montserrat_17', charset: CHARSET },
  { size: 17, ttf: 'Montserrat-Bold.ttf', header: 'Montserrat_17_Bold', charset: CHARSET },
  { size: 16, ttf: 'Montserrat-Medium.ttf', header: 'Montserrat_16', charset: CHARSET },
];

function main() {
  mkdirSync(OUT_DIR, { recursive: true });

  const missing = JOBS.map((j) => j.ttf).filter((f, i, a) => a.indexOf(f) === i)
    .filter((f) => !existsSync(join(TTF_DIR, f)));

  if (missing.length) {
    console.error('Missing TTF files. Run: npm run setup-fonts');
    console.error('Missing:', missing.join(', '));
    process.exit(1);
  }

  const instructions = `# Montserrat VLW fonts for Arduino

Generated headers go in this folder. Enable smooth fonts in Seeed_GFX \`User_Setup.h\`:

\`\`\`cpp
#define SMOOTH_FONT
\`\`\`

## Generate with Processing (TFT_eSPI Create_font)

1. Open \`TFT_eSPI/Tools/Create_font/Create_font.pde\` in Processing 4
2. Copy TTF files from \`assets/fonts/\` into the sketch \`data/\` folder
3. For each job below, set variables and run the sketch (creates \`.h\` in sketch folder)
4. Move the \`.h\` files here and uncomment \`#define USE_MONTSERRAT\` in \`display_font.h\`

| Output header | TTF | Size (pt) | Charset |
| --- | --- | --- | --- |
${JOBS.map((j) => `| \`${j.header}.h\` | ${j.ttf} | ${j.size} | \`${j.charset.slice(0, 40)}…\` |`).join('\n')}

Processing settings per job:
- \`fontName\` = TTF filename without extension
- \`fontSize\` = size column above
- \`specificCharacters\` = charset string (Create_font "Specific characters" field)
- \`createHeader\` = true

`;

  writeFileSync(join(OUT_DIR, 'README.md'), instructions);

  const manifest = JOBS.map((j) => ({
    header: j.header,
    ttf: join(TTF_DIR, j.ttf),
    size: j.size,
    charset: j.charset,
  }));

  writeFileSync(join(OUT_DIR, 'manifest.json'), JSON.stringify(manifest, null, 2) + '\n');

  console.log('Font generation manifest written to arduino/dashboard/fonts/');
  console.log('Follow arduino/dashboard/fonts/README.md using TFT_eSPI Create_font.pde');
  for (const job of JOBS) {
    console.log(`  - ${job.header}.h  (${job.ttf} @ ${job.size}pt)`);
  }
}

main();
