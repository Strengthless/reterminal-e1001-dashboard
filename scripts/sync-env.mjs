#!/usr/bin/env node

import { readFileSync, writeFileSync, existsSync } from 'node:fs';
import { join, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const ROOT = join(dirname(fileURLToPath(import.meta.url)), '..');
const ENV_PATH = join(ROOT, '.env');

const CLOUDFLARE_KEYS = [
  'METOFFICE_API_KEY',
  'TFL_API_KEY',
  'TFL_LINE',
  'TFL_LINE_NAME',
  'TFL_STOP',
  'TFL_STOP_NAME',
  'TFL_DIRECTION',
  'TFL_DIRECTION_LABEL',
  'WEATHER_LAT',
  'WEATHER_LON',
  'WEATHER_LOCATION_NAME',
];

const WRANGLER_VAR_KEYS = CLOUDFLARE_KEYS.filter((k) => !k.endsWith('_API_KEY'));

const ARDUINO_KEYS = ['WORKER_URL', 'WIFI_SSID', 'WIFI_PASS'];

const GENERATED_BANNER = '# Generated from .env by scripts/sync-env.mjs — do not edit manually.\n';

function parseEnv(content) {
  /** @type {Record<string, string>} */
  const env = {};

  for (const line of content.split(/\r?\n/)) {
    const trimmed = line.trim();
    if (!trimmed || trimmed.startsWith('#')) continue;

    const eq = trimmed.indexOf('=');
    if (eq === -1) continue;

    const key = trimmed.slice(0, eq).trim();
    let value = trimmed.slice(eq + 1).trim();

    if (
      (value.startsWith('"') && value.endsWith('"')) ||
      (value.startsWith("'") && value.endsWith("'"))
    ) {
      value = value.slice(1, -1);
    }

    env[key] = value;
  }

  return env;
}

function escapeC(value) {
  return value
    .replace(/\\/g, '\\\\')
    .replace(/"/g, '\\"');
}

function pick(env, keys) {
  /** @type {Record<string, string>} */
  const out = {};
  for (const key of keys) {
    if (env[key] !== undefined && env[key] !== '') {
      out[key] = env[key];
    }
  }
  return out;
}

function writeDevVars(env) {
  const values = pick(env, CLOUDFLARE_KEYS);
  const lines = Object.entries(values).map(([k, v]) => `${k}=${v}`);
  const path = join(ROOT, 'cloudflare', '.dev.vars');
  writeFileSync(path, GENERATED_BANNER + lines.join('\n') + '\n');
  return path;
}

function writeWranglerVars(env) {
  const path = join(ROOT, 'cloudflare', 'wrangler.jsonc');
  const content = readFileSync(path, 'utf8');
  const values = pick(env, WRANGLER_VAR_KEYS);

  const entries = Object.entries(values)
    .map(([k, v]) => `    "${k}": ${JSON.stringify(v)}`)
    .join(',\n');

  const varsBlock = `{\n${entries}\n  }`;

  const match = content.match(/"vars"\s*:\s*\{[\s\S]*?\r?\n  \}/);
  if (!match) {
    throw new Error('Could not find vars block in cloudflare/wrangler.jsonc');
  }

  const updated = content.replace(match[0], `"vars": ${varsBlock}`);

  if (updated !== content) {
    writeFileSync(path, updated);
  }
  return path;
}

function writeArduinoConfig(env) {
  const workerUrl = env.WORKER_URL ?? 'https://reterminal.<your-subdomain>.workers.dev/';
  const wifiSsid = env.WIFI_SSID ?? '';
  const wifiPass = env.WIFI_PASS ?? '';

  const content = `#pragma once

// Generated from .env by scripts/sync-env.mjs — do not edit manually.

static const char* WORKER_URL = "${escapeC(workerUrl)}";

static const char* WIFI_SSID = "${escapeC(wifiSsid)}";
static const char* WIFI_PASS = "${escapeC(wifiPass)}";

// Refresh intervals (milliseconds)
// Full-screen GC refresh interval (epaper.update — clears ghosting, visible flash).
static const uint32_t INTERVAL_CLOCK_MS = 20 * 1000;
static const uint32_t INTERVAL_TFL_MS = 20 * 1000;
static const uint32_t INTERVAL_SENSORS_MS = 5 * 60 * 1000;
static const uint32_t INTERVAL_WEATHER_MS = 60 * 60 * 1000;
`;

  const path = join(ROOT, 'arduino', 'dashboard', 'config.h');
  writeFileSync(path, content);
  return path;
}

function main() {
  if (!existsSync(ENV_PATH)) {
    console.error('Missing .env — copy .env.example to .env and fill in values.');
    process.exit(1);
  }

  const env = parseEnv(readFileSync(ENV_PATH, 'utf8'));

  const outputs = [
    writeDevVars(env),
    writeWranglerVars(env),
    writeArduinoConfig(env),
  ];

  console.log('Synced .env →');
  for (const path of outputs) {
    console.log(`  ${path.replace(/\\/g, '/')}`);
  }
}

main();
