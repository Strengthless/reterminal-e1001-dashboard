import { REGIONS } from './layout.js';

const MOCK = {
  tfl: {
    line: 'Elizabeth line',
    stop: 'Ealing Broadway',
    direction: 'Eastbound',
    status: 'Good Service',
    disruption: false,
    etas: [
      { destination: 'Shenfield', minutes: 1 },
      { destination: 'Abbey Wood', minutes: 5 },
      { destination: 'Abbey Wood', minutes: 12 },
      { destination: 'Abbey Wood', minutes: 19 },
    ],
  },
  weather: {
    location: 'Ealing, London',
    description: 'Overcast',
    icon: 'wi-cloudy',
    tempC: 28.9,
    feelsLikeC: 27.5,
    minC: 17.2,
    maxC: 29.0,
    precipMm: 0.0,
  },
  device: {
    batteryPct: 100,
    sensorTempC: 32,
    sensorHumidityPct: 34.8,
  },
};

const screen = document.getElementById('screen');
const content = document.getElementById('content');
const regionsEl = document.getElementById('regions');
const statusEl = document.getElementById('status');
const dataSource = document.getElementById('data-source');
const workerUrlWrap = document.getElementById('worker-url-wrap');
const jsonWrap = document.getElementById('json-wrap');
const workerUrlInput = document.getElementById('worker-url');
const jsonInput = document.getElementById('json-input');
const showRegions = document.getElementById('show-regions');

function setStatus(message, isError = false) {
  statusEl.textContent = message;
  statusEl.classList.toggle('error', isError);
}

function pad(n) {
  return String(n).padStart(2, '0');
}

function formatNow() {
  const now = new Date();
  return {
    date: `${now.getFullYear()}.${pad(now.getMonth() + 1)}.${pad(now.getDate())}`,
    clock: `${pad(now.getHours())}:${pad(now.getMinutes())}:${pad(now.getSeconds())}`,
    weekday: now.toLocaleDateString('en-GB', { weekday: 'long' }),
  };
}

function iconPath(name) {
  if (name.startsWith('wi-')) return `/assets/weather/${name}.svg`;
  return `/assets/icons/${name}.svg`;
}

function img(src, className, alt = '') {
  return `<img class="${className}" src="${src}" alt="${alt}" />`;
}

function renderRegions(show) {
  if (!show) {
    regionsEl.hidden = true;
    regionsEl.innerHTML = '';
    return;
  }

  regionsEl.hidden = false;
  regionsEl.innerHTML = Object.entries(REGIONS)
    .map(([name, r]) => `
      <div class="region-box" style="left:${r.x}px;top:${r.y}px;width:${r.w}px;height:${r.h}px">${name}</div>
    `)
    .join('');
}

function render(data) {
  const time = formatNow();
  const device = data.device ?? MOCK.device;
  const tfl = data.tfl ?? MOCK.tfl;
  const weather = data.weather ?? MOCK.weather;

  const tflTitleIcon = tfl.stale
    ? img('/assets/tfl/stale.svg', 'tfl-status-icon stale-icon', 'Stale data')
    : (tfl.disruption || tfl.status !== 'Good Service')
      ? img('/assets/tfl/warning.svg', 'tfl-status-icon warning-icon', 'Service alert')
      : '';
  const tflStatusInline = tflTitleIcon ? `<span class="tfl-inline-status">${tflTitleIcon}</span>` : '';

  const tflRows = (tfl.etas ?? [])
    .slice(0, 4)
    .map((eta, i) => `
      <div class="tfl-row">
        <span class="dest">${i + 1} - ${eta.destination}</span>
        <span class="mins">${eta.minutes <= 0 ? 'Arriving' : `${eta.minutes} min`}</span>
      </div>
    `)
    .join('');

  content.innerHTML = `
    <header class="header">
      <span class="header-date">${time.date}</span>
      <span class="header-sep">|</span>
      <span class="header-clock">${time.clock}</span>
      <span class="header-sep">|</span>
      <span class="header-weekday">${time.weekday}</span>
      <div class="header-battery">
        ${img('/assets/icons/battery.svg', '', 'Battery')}
        <span>${device.batteryPct}%</span>
      </div>
    </header>

    <section class="panel-tfl">
      <div class="tfl-title">
        ${img('/assets/tfl/elizabeth_line_roundel.svg', 'tfl-roundel', 'Elizabeth line')}
        <div class="tfl-title-text">
          <div class="tfl-line-name">${tfl.line}${tflStatusInline}</div>
          <div class="tfl-line-sub">From ${tfl.stop} · ${tfl.direction}</div>
        </div>
      </div>
      <div class="tfl-list">
        ${tflRows}
      </div>
    </section>

    <div class="panel-right">
      <section class="panel-sensors">
        <div class="panel-label">Bedroom</div>
        <div class="sensors-row">
          <div class="sensor-item">
            ${img('/assets/icons/temperature.svg', '', 'Temperature')}
            <span>${device.sensorTempC.toFixed(1)}°C</span>
          </div>
          <div class="sensor-item">
            ${img('/assets/icons/humidity.svg', '', 'Humidity')}
            <span>${device.sensorHumidityPct.toFixed(1)}%</span>
          </div>
        </div>
      </section>

      <section class="panel-weather">
        <div class="panel-label">${weather.location}</div>
        <div class="weather-main">
          ${img(iconPath(weather.icon), 'weather-icon', weather.description)}
          <div>
            <div class="weather-desc">${weather.description}</div>
            <div class="weather-temp">${weather.tempC.toFixed(1)}°C / feels like ${weather.feelsLikeC.toFixed(1)}°C</div>
          </div>
        </div>
        <div class="weather-stats">
          <div class="stat stat-tight">
            ${img('/assets/icons/temperature_min.svg', '', 'Min')}
            <span>${weather.minC.toFixed(1)}</span>
          </div>
          <div class="stat stat-tight">
            ${img('/assets/icons/temperature_max.svg', '', 'Max')}
            <span>${weather.maxC.toFixed(1)}</span>
          </div>
          <div class="stat">
            ${img('/assets/icons/rain.svg', '', 'Rain')}
            <span>${weather.precipMm.toFixed(1)}</span>
          </div>
        </div>
      </section>
    </div>
  `;
}

async function loadData() {
  const source = dataSource.value;

  if (source === 'mock') {
    render(MOCK);
    setStatus('Using mock data.');
    return;
  }

  if (source === 'json') {
    try {
      render(JSON.parse(jsonInput.value));
      setStatus('Rendered pasted JSON.');
    } catch (error) {
      setStatus(`Invalid JSON: ${error.message}`, true);
    }
    return;
  }

  const url = workerUrlInput.value.trim();
  if (!url) {
    setStatus('Enter a worker URL.', true);
    return;
  }

  try {
    setStatus('Fetching worker…');
    const res = await fetch(url);
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    render(await res.json());
    setStatus(`Loaded ${url}`);
  } catch (error) {
    render(MOCK);
    setStatus(`Worker fetch failed (${error.message}). Showing mock data.`, true);
  }
}

function updateSourceUi() {
  workerUrlWrap.hidden = dataSource.value !== 'worker';
  jsonWrap.hidden = dataSource.value !== 'json';
}

function loadWorkerUrlDefault() {
  fetch('/preview/config.json')
    .then((res) => (res.ok ? res.json() : null))
    .then((cfg) => { if (cfg?.workerUrl) workerUrlInput.value = cfg.workerUrl; })
    .catch(() => {});
}

function tickClock() {
  const time = formatNow();
  const clock = content.querySelector('.header-clock');
  const date = content.querySelector('.header-date');
  const weekday = content.querySelector('.header-weekday');
  if (clock) clock.textContent = time.clock;
  if (date) date.textContent = time.date;
  if (weekday) weekday.textContent = time.weekday;
}

dataSource.addEventListener('change', () => { updateSourceUi(); loadData(); });
document.getElementById('refresh-btn').addEventListener('click', loadData);
showRegions.addEventListener('change', () => renderRegions(showRegions.checked));

updateSourceUi();
loadWorkerUrlDefault();
render(MOCK);
renderRegions(false);
setInterval(tickClock, 1000);
