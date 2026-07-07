#!/usr/bin/env node

import { readFileSync, writeFileSync, existsSync } from 'node:fs';
import { join, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const ROOT = join(dirname(fileURLToPath(import.meta.url)), '..');
const ENV_PATH = join(ROOT, '.env');

function parseEnv(content) {
  const env = {};
  for (const line of content.split(/\r?\n/)) {
    const trimmed = line.trim();
    if (!trimmed || trimmed.startsWith('#')) continue;
    const eq = trimmed.indexOf('=');
    if (eq === -1) continue;
    env[trimmed.slice(0, eq).trim()] = trimmed.slice(eq + 1).trim();
  }
  return env;
}

const workerUrl = existsSync(ENV_PATH)
  ? parseEnv(readFileSync(ENV_PATH, 'utf8')).WORKER_URL ?? ''
  : '';

writeFileSync(
  join(ROOT, 'preview', 'config.json'),
  JSON.stringify({ workerUrl }, null, 2) + '\n',
);

console.log(workerUrl ? `Preview config: ${workerUrl}` : 'Preview config: no WORKER_URL in .env');
