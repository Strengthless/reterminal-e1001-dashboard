#!/usr/bin/env node

import { readFileSync, writeFileSync, mkdirSync, readdirSync } from 'node:fs';
import { spawnSync } from 'node:child_process';
import { join, basename, relative, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { Resvg } from '@resvg/resvg-js';
import { PNG } from 'pngjs';

const ROOT = join(dirname(fileURLToPath(import.meta.url)), '..');
const OUT_DIR = join(ROOT, 'arduino', 'assets');
const ROUNDEL = 'elizabeth_line_roundel.svg';
const ROUNDEL_HEIGHT = 64;
const UI_ICON_SIZE = 36;
const BATTERY_ICON_SIZE = 30;
const STALE_ICON_SIZE = 24;

/** @type {Record<string, { dir: string; size: number }>} */
const ASSET_GROUPS = {
  weather: { dir: 'assets/weather', size: 64 },
  icons: { dir: 'assets/icons', size: UI_ICON_SIZE },
  tfl: { dir: 'assets/tfl', size: 30 },
};

function rasterizeSvg(svgPath, { size, fit = 'width' }) {
  const svg = readFileSync(svgPath, 'utf8');
  const fitTo = fit === 'height'
    ? { mode: 'height', value: size }
    : { mode: 'width', value: size };
  const resvg = new Resvg(svg, {
    fitTo,
    background: 'transparent',
  });
  return PNG.sync.read(resvg.render().asPng());
}

function sanitizeName(filename) {
  return basename(filename, '.svg').replace(/[^a-zA-Z0-9_]/g, '_');
}

function emitHeader(name, bitmap, mode) {
  const suffix = mode === 'gray4' ? '_gray4' : '';
  const varName = `asset_${name}${suffix}`;
  const lines = [...bitmap.data].map((b) => `0x${b.toString(16).padStart(2, '0')}`);

  return `#pragma once
#include <stdint.h>

static const uint16_t ${varName}_width = ${bitmap.width};
static const uint16_t ${varName}_height = ${bitmap.height};
static const uint16_t ${varName}_bytes_per_row = ${bitmap.bytesPerRow};
static const uint8_t ${varName}_data[] PROGMEM = {
  ${lines.join(', ')}
};
`;
}

function rgbaToBitmap1(png) {
  const { width, height, data } = png;
  const bytesPerRow = Math.ceil(width / 8);
  const out = new Uint8Array(bytesPerRow * height);

  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const i = (y * width + x) * 4;
      const lum = data[i + 3] < 128 ? 255 : data[i] * 0.299 + data[i + 1] * 0.587 + data[i + 2] * 0.114;
      if (lum < 128) {
        out[y * bytesPerRow + (x >> 3)] |= 0x80 >> (x & 7);
      }
    }
  }

  return { data: out, width, height, bytesPerRow };
}

function rgbaToBitmapGray4(png) {
  const { width, height, data } = png;
  const bytesPerRow = Math.ceil(width / 4);
  const out = new Uint8Array(bytesPerRow * height);

  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const i = (y * width + x) * 4;
      const lum = data[i + 3] < 128 ? 255 : data[i] * 0.299 + data[i + 1] * 0.587 + data[i + 2] * 0.114;
      const level = lum < 64 ? 0 : lum < 128 ? 1 : lum < 192 ? 2 : 3;
      const byteIndex = y * bytesPerRow + (x >> 2);
      const shift = 6 - (x & 3) * 2;
      out[byteIndex] |= level << shift;
    }
  }

  return { data: out, width, height, bytesPerRow };
}

function convertFile(svgPath, size, { gray4 = false, fit = 'width' } = {}) {
  const png = rasterizeSvg(svgPath, { size, fit });
  const name = sanitizeName(svgPath);
  const outputs = [{ mode: 'mono', header: emitHeader(name, rgbaToBitmap1(png), 'mono') }];

  if (gray4) {
    outputs.push({
      mode: 'gray4',
      header: emitHeader(name, rgbaToBitmapGray4(png), 'gray4'),
    });
  }

  return outputs;
}

function main() {
  mkdirSync(OUT_DIR, { recursive: true });

  const manifest = [];

  for (const { dir, size } of Object.values(ASSET_GROUPS)) {
    const absDir = join(ROOT, dir);
    const files = readdirSync(absDir).filter((f) => f.endsWith('.svg'));

    for (const file of files) {
      const svgPath = join(absDir, file);
      const isRoundel = file.toLowerCase() === ROUNDEL.toLowerCase();
      const isBattery = dir === 'assets/icons' && file.toLowerCase() === 'battery.svg';
      const isStale = file.toLowerCase() === 'stale.svg';
      const rasterSize = isRoundel ? ROUNDEL_HEIGHT
        : isBattery ? BATTERY_ICON_SIZE
        : isStale ? STALE_ICON_SIZE
        : size;
      const outputs = convertFile(
        svgPath,
        rasterSize,
        { gray4: isRoundel, fit: isRoundel ? 'height' : 'width' },
      );

      for (const { mode, header } of outputs) {
        const name = sanitizeName(file) + (mode === 'gray4' ? '_gray4' : '');
        const outPath = join(OUT_DIR, `${name}.h`);
        writeFileSync(outPath, header);
        manifest.push(relative(ROOT, outPath));
        console.log(`Wrote ${relative(ROOT, outPath)}`);
      }
    }
  }

  writeFileSync(join(OUT_DIR, 'manifest.txt'), manifest.sort().join('\n') + '\n');
  console.log(`\nGenerated ${manifest.length} headers in arduino/assets/`);

  const lookup = spawnSync(process.execPath, [join(ROOT, 'scripts', 'generate-weather-icon-lookup.mjs')], {
    cwd: ROOT,
    stdio: 'inherit',
  });
  if (lookup.status !== 0) process.exit(lookup.status ?? 1);
}

main();
