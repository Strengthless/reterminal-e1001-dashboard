#!/usr/bin/env node

import { mkdirSync, createWriteStream, existsSync } from 'node:fs';
import { join, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { pipeline } from 'node:stream/promises';
import { get } from 'node:https';

const ROOT = join(dirname(fileURLToPath(import.meta.url)), '..');

const TTF_SOURCES = [
  ['Montserrat-Regular.ttf', 'https://raw.githubusercontent.com/JulietaUla/Montserrat/master/fonts/ttf/Montserrat-Regular.ttf'],
  ['Montserrat-Medium.ttf', 'https://raw.githubusercontent.com/JulietaUla/Montserrat/master/fonts/ttf/Montserrat-Medium.ttf'],
  ['Montserrat-SemiBold.ttf', 'https://raw.githubusercontent.com/JulietaUla/Montserrat/master/fonts/ttf/Montserrat-SemiBold.ttf'],
  ['Montserrat-Bold.ttf', 'https://raw.githubusercontent.com/JulietaUla/Montserrat/master/fonts/ttf/Montserrat-Bold.ttf'],
];

const WOFF2_SOURCES = [
  ['montserrat-latin-400-normal.woff2', 'https://fonts.gstatic.com/s/montserrat/v30/JTUSjIg1_i6t8kCHKm459WlhyyTh89Y.woff2'],
  ['montserrat-latin-500-normal.woff2', 'https://fonts.gstatic.com/s/montserrat/v30/JTUSjIg1_i6t8kCHKm459WRhyyTh89Y.woff2'],
  ['montserrat-latin-600-normal.woff2', 'https://fonts.gstatic.com/s/montserrat/v30/JTUSjIg1_i6t8kCHKm459W1hyyTh89Y.woff2'],
  ['montserrat-latin-700-normal.woff2', 'https://fonts.gstatic.com/s/montserrat/v30/JTUSjIg1_i6t8kCHKm459WdhyyTh89Y.woff2'],
];

async function download(url, dest) {
  if (existsSync(dest)) {
    console.log(`Skip (exists): ${dest}`);
    return;
  }

  await new Promise((resolve, reject) => {
    get(url, (res) => {
      if (res.statusCode === 301 || res.statusCode === 302) {
        download(res.headers.location, dest).then(resolve, reject);
        return;
      }
      if (res.statusCode !== 200) {
        reject(new Error(`HTTP ${res.statusCode} for ${url}`));
        return;
      }
      pipeline(res, createWriteStream(dest)).then(resolve, reject);
    }).on('error', reject);
  });

  console.log(`Wrote ${dest}`);
}

async function main() {
  const ttfDir = join(ROOT, 'assets', 'fonts');
  const woffDir = join(ROOT, 'preview', 'fonts', 'files');
  mkdirSync(ttfDir, { recursive: true });
  mkdirSync(woffDir, { recursive: true });

  for (const [name, url] of TTF_SOURCES) {
    await download(url, join(ttfDir, name));
  }

  for (const [name, url] of WOFF2_SOURCES) {
    await download(url, join(woffDir, name));
  }

  console.log('\nIf bold (700) looked wrong, delete preview/fonts/files/montserrat-latin-700-normal.woff2 and re-run.');

  console.log('\nFonts ready for preview (woff2) and Arduino VLW generation (ttf).');
  console.log('Generate device fonts: npm run generate-fonts');
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
