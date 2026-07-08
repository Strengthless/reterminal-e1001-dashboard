// --- Defaults (override via wrangler vars / .dev.vars) ---

const DEFAULTS = {
  TFL_LINE: 'elizabeth',
  TFL_LINE_NAME: 'Elizabeth line',
  TFL_STOP: '910GEALINGB',
  TFL_STOP_NAME: 'Ealing Broadway',
  TFL_DIRECTION: 'inbound',
  TFL_DIRECTION_LABEL: 'Eastbound',
  WEATHER_LAT: '51.5130',
  WEATHER_LON: '-0.3089',
  WEATHER_LOCATION_NAME: 'Ealing, London',
};

const MET_OFFICE_BASE = 'https://data.hub.api.metoffice.gov.uk/sitespecific/v0/point';

const WEATHER_DESCRIPTIONS = {
  [-1]: 'Trace rain',
  0: 'Clear',
  1: 'Sunny',
  2: 'Partly cloudy',
  3: 'Partly cloudy',
  5: 'Mist',
  6: 'Fog',
  7: 'Cloudy',
  8: 'Overcast',
  9: 'Light rain shower',
  10: 'Light rain shower',
  11: 'Drizzle',
  12: 'Light rain',
  13: 'Heavy rain shower',
  14: 'Heavy rain shower',
  15: 'Heavy rain',
  16: 'Sleet shower',
  17: 'Sleet shower',
  18: 'Sleet',
  19: 'Hail shower',
  20: 'Hail shower',
  21: 'Hail',
  22: 'Light snow shower',
  23: 'Light snow shower',
  24: 'Light snow',
  25: 'Heavy snow shower',
  26: 'Heavy snow shower',
  27: 'Heavy snow',
  28: 'Thunder shower',
  29: 'Thunder shower',
  30: 'Thunder',
};

const DEFAULT_WEATHER_CACHE_SECONDS = 900;

const WEATHER_CACHE_KEY = 'weather';

/** @type {{ tfl: { value: object | null, fetchedAt: number }, weather: { value: object | null, fetchedAt: number } }} */
const memoryCache = {
  tfl: { value: null, fetchedAt: 0 },
  weather: { value: null, fetchedAt: 0 },
};

/** @type {{ tfl: Promise<object> | null, weather: Promise<object> | null }} */
const inFlight = {
  tfl: null,
  weather: null,
};

function config(env) {
  return {
    tflApiKey: env.TFL_API_KEY ?? '',
    metOfficeApiKey: env.METOFFICE_API_KEY ?? '',
    tflLine: env.TFL_LINE ?? DEFAULTS.TFL_LINE,
    tflLineName: env.TFL_LINE_NAME ?? DEFAULTS.TFL_LINE_NAME,
    tflStop: env.TFL_STOP ?? DEFAULTS.TFL_STOP,
    tflStopName: env.TFL_STOP_NAME ?? DEFAULTS.TFL_STOP_NAME,
    tflDirection: env.TFL_DIRECTION ?? DEFAULTS.TFL_DIRECTION,
    tflDirectionLabel: env.TFL_DIRECTION_LABEL ?? DEFAULTS.TFL_DIRECTION_LABEL,
    weatherLat: env.WEATHER_LAT ?? DEFAULTS.WEATHER_LAT,
    weatherLon: env.WEATHER_LON ?? DEFAULTS.WEATHER_LON,
    weatherLocationName: env.WEATHER_LOCATION_NAME ?? DEFAULTS.WEATHER_LOCATION_NAME,
    weatherCacheSeconds: Number(env.WEATHER_CACHE_SECONDS) || DEFAULT_WEATHER_CACHE_SECONDS,
  };
}

function cacheFresh(fetchedAt, ttlSeconds) {
  return fetchedAt > 0 && Date.now() - fetchedAt < ttlSeconds * 1000;
}

function cacheAgeSeconds(fetchedAt) {
  return fetchedAt > 0 ? Math.floor((Date.now() - fetchedAt) / 1000) : null;
}

function weatherCacheStatus(fetchedAt, ttlSeconds) {
  return cacheFresh(fetchedAt, ttlSeconds) ? 'HIT-WEATHER' : 'STALE-WEATHER';
}

function logWeatherCache(result, cfg, { reason = null } = {}) {
  const age = cacheAgeSeconds(result.fetchedAt);
  const detail = [
    result.cacheStatus,
    result.cacheLayer ? `layer=${result.cacheLayer}` : null,
    age != null ? `age=${age}s` : null,
    `ttl=${cfg.weatherCacheSeconds}s`,
    result.fetchedAt ? `fetchedAt=${isoOrNull(result.fetchedAt)}` : null,
    reason ? `reason=${reason}` : null,
  ].filter(Boolean).join(' ');

  if (result.cacheStatus === 'MISS-WEATHER-ERROR') {
    console.error(`Weather cache ${detail}`);
  } else if (result.cacheStatus === 'STALE-WEATHER') {
    console.warn(`Weather cache ${detail}`);
  } else {
    console.info(`Weather cache ${detail}`);
  }
}

function logTflSubfetchFailure(kind, error) {
  const message = String(error);
  if (message.includes('429')) throw error;
  const log = /HTTP (401|403)/.test(message) ? console.error : console.warn;
  log(`TfL ${kind} failed:`, error);
}

function logTflCache(result, { reason = null } = {}) {
  const age = cacheAgeSeconds(result.fetchedAt);
  const detail = [
    result.cacheStatus,
    result.cacheLayer ? `layer=${result.cacheLayer}` : null,
    age != null ? `age=${age}s` : null,
    result.fetchedAt ? `fetchedAt=${isoOrNull(result.fetchedAt)}` : null,
    reason ? `reason=${reason}` : null,
  ].filter(Boolean).join(' ');

  if (result.cacheStatus === 'MISS-TFL-ERROR') {
    console.error(`TfL cache ${detail}`);
  } else if (result.cacheStatus === 'STALE-TFL') {
    console.warn(`TfL cache ${detail}`);
  }
}

function isoOrNull(ms) {
  return ms > 0 ? new Date(ms).toISOString() : null;
}

function jsonResponse(body, { cacheStatus }) {
  return Response.json(body, {
    headers: {
      'Content-Type': 'application/json',
      'Access-Control-Allow-Origin': '*',
      'Cache-Control': 'private, no-store',
      'X-Cache': cacheStatus,
    },
  });
}

function iconForMetOfficeCode(code) {
  switch (code) {
    case -1: return 'wi-sprinkle';
    case 0: return 'wi-night-clear';
    case 1: return 'wi-day-sunny';
    case 2: return 'wi-night-alt-cloudy';
    case 3: return 'wi-day-cloudy';
    case 4: return 'wi-na';
    case 5: return 'wi-fog';
    case 6: return 'wi-fog';
    case 7: return 'wi-cloudy';
    case 8: return 'wi-cloudy';
    case 9: return 'wi-night-alt-showers';
    case 10: return 'wi-day-showers';
    case 11: return 'wi-sprinkle';
    case 12: return 'wi-rain';
    case 13: return 'wi-night-alt-rain';
    case 14: return 'wi-day-rain';
    case 15: return 'wi-rain';
    case 16: return 'wi-night-alt-sleet';
    case 17: return 'wi-day-sleet';
    case 18: return 'wi-sleet';
    case 19: return 'wi-night-alt-hail';
    case 20: return 'wi-day-hail';
    case 21: return 'wi-hail';
    case 22: return 'wi-night-alt-snow';
    case 23: return 'wi-day-snow';
    case 24: return 'wi-snow';
    case 25: return 'wi-night-alt-snow';
    case 26: return 'wi-day-snow';
    case 27: return 'wi-snow';
    case 28: return 'wi-night-alt-storm-showers';
    case 29: return 'wi-day-storm-showers';
    case 30: return 'wi-thunderstorm';
    default: return 'wi-na';
  }
}

function weatherDescription(code) {
  return WEATHER_DESCRIPTIONS[code] ?? 'Unknown';
}

function beautifyList(items) {
  if (!items?.length) return 'N/A';
  if (items.length === 1) return items[0];
  return items.slice(0, -1).join(', ') + ' & ' + items.at(-1);
}

function round1(value) {
  return Math.round(value * 10) / 10;
}

function weatherFallback(cfg) {
  return {
    location: cfg.weatherLocationName,
    description: 'N/A',
    icon: 'wi-na',
    tempC: null,
    feelsLikeC: null,
    minC: null,
    maxC: null,
    precipMm: null,
  };
}

function isWeatherUsable(value) {
  return value?.tempC != null;
}

async function hydrateSlotFromKv(kv, slot, key) {
  if (!kv) return;

  try {
    const stored = await kv.get(key, 'json');
    if (!stored?.fetchedAt) return;

    if (stored.fetchedAt > slot.fetchedAt) {
      slot.value = stored.value ?? null;
      slot.fetchedAt = stored.fetchedAt;
    }
  } catch (error) {
    console.warn('KV read failed:', error);
  }
}

async function persistSlotToKv(kv, slot, key, value, fetchedAt) {
  slot.value = value;
  slot.fetchedAt = fetchedAt;
  if (!kv) return;

  try {
    await kv.put(key, JSON.stringify({ value, fetchedAt }));
  } catch (error) {
    console.warn('KV write failed:', error);
  }
}

async function getTflETA(cfg) {
  const url = `https://api.tfl.gov.uk/Line/${cfg.tflLine}/Arrivals/${cfg.tflStop}?direction=${cfg.tflDirection}&app_key=${cfg.tflApiKey}`;

  try {
    const res = await fetch(url);
    if (res.status === 429) throw new Error('TfL arrivals HTTP 429');
    if (!res.ok) throw new Error(`TfL arrivals HTTP ${res.status}`);
    const data = await res.json();

    const sorted = data.toSorted((a, b) => a.timeToStation - b.timeToStation);
    const etas = [];
    let lastVehicleId = null;

    for (const arrival of sorted) {
      if (arrival.vehicleId === lastVehicleId) continue;
      lastVehicleId = arrival.vehicleId;
      etas.push({
        destination: arrival.destinationName
          .replace('Rail Station', '')
          .replace('London Paddington', 'Paddington')
          .trim(),
        minutes: Math.floor(arrival.timeToStation / 60),
      });
      if (etas.length >= 4) break;
    }

    return etas;
  } catch (error) {
    logTflSubfetchFailure('arrivals', error);
    return [];
  }
}

async function getTflStatus(cfg) {
  const url = `https://api.tfl.gov.uk/Line/${cfg.tflLine}/Status?app_key=${cfg.tflApiKey}`;

  try {
    const res = await fetch(url);
    if (res.status === 429) throw new Error('TfL status HTTP 429');
    if (!res.ok) throw new Error(`TfL status HTTP ${res.status}`);
    const data = await res.json();

    if (data.length !== 1) {
      console.warn('TfL Status API returned unexpected line count:', data.length);
      return 'N/A';
    }

    const statuses = data[0].lineStatuses.map((s) => s.statusSeverityDescription);
    return beautifyList(statuses);
  } catch (error) {
    logTflSubfetchFailure('status', error);
    return 'N/A';
  }
}

async function getTflDisruption(cfg) {
  const url = `https://api.tfl.gov.uk/Line/${cfg.tflLine}/Disruption?app_key=${cfg.tflApiKey}`;

  try {
    const res = await fetch(url);
    if (res.status === 429) throw new Error('TfL disruption HTTP 429');
    if (!res.ok) throw new Error(`TfL disruption HTTP ${res.status}`);
    const data = await res.json();
    return data.length > 0;
  } catch (error) {
    logTflSubfetchFailure('disruption', error);
    return false;
  }
}

async function fetchMetOffice(endpoint, cfg) {
  const params = new URLSearchParams({
    latitude: cfg.weatherLat,
    longitude: cfg.weatherLon,
    includeLocationName: 'true',
    excludeParameterMetadata: 'true',
  });

  const res = await fetch(`${MET_OFFICE_BASE}/${endpoint}?${params}`, {
    headers: { apikey: cfg.metOfficeApiKey },
  });

  if (res.status === 429) throw new Error(`Met Office ${endpoint} HTTP 429`);
  if (!res.ok) throw new Error(`Met Office ${endpoint} HTTP ${res.status}`);
  return res.json();
}

function pickCurrentHourlyEntry(timeSeries) {
  const now = Date.now();
  let best = timeSeries[0];
  let bestDelta = Infinity;

  for (const entry of timeSeries) {
    const delta = Math.abs(new Date(entry.time).getTime() - now);
    if (delta < bestDelta) {
      bestDelta = delta;
      best = entry;
    }
  }

  return best;
}

function pickTodayDailyEntry(timeSeries) {
  const today = new Date().toISOString().slice(0, 10);

  return timeSeries.find((entry) => entry.time.startsWith(today)) ?? timeSeries[0];
}

async function getWeather(cfg) {
  const fallback = weatherFallback(cfg);

  if (!cfg.metOfficeApiKey) {
    console.warn('METOFFICE_API_KEY not configured');
    return fallback;
  }

  try {
    const [hourly, daily] = await Promise.all([
      fetchMetOffice('hourly', cfg),
      fetchMetOffice('daily', cfg),
    ]);

    const hourlySeries = hourly.features?.[0]?.properties?.timeSeries ?? [];
    const dailySeries = daily.features?.[0]?.properties?.timeSeries ?? [];

    if (!hourlySeries.length) return fallback;

    const current = pickCurrentHourlyEntry(hourlySeries);
    const today = pickTodayDailyEntry(dailySeries);
    const code = current.significantWeatherCode;

    return {
      location: cfg.weatherLocationName,
      description: weatherDescription(code),
      icon: iconForMetOfficeCode(code),
      tempC: round1(current.screenTemperature),
      feelsLikeC: round1(current.feelsLikeTemperature),
      minC: today ? round1(today.nightMinScreenTemperature) : null,
      maxC: today ? round1(today.dayMaxScreenTemperature) : null,
      precipMm: today ? round1(today.totalPrecipAmount ?? 0) : null,
    };
  } catch (error) {
    if (String(error).includes('429')) throw error;
    console.warn('Error fetching weather:', error);
    return fallback;
  }
}

async function fetchTflBundle(cfg) {
  const [etas, status, disruption] = await Promise.all([
    getTflETA(cfg),
    getTflStatus(cfg),
    getTflDisruption(cfg),
  ]);

  return {
    line: cfg.tflLineName,
    stop: cfg.tflStopName,
    direction: cfg.tflDirectionLabel,
    status,
    disruption,
    etas,
  };
}

async function getCachedTfl(cfg) {
  const slot = memoryCache.tfl;

  if (!inFlight.tfl) {
    inFlight.tfl = (async () => {
      try {
        const value = await fetchTflBundle(cfg);
        const fetchedAt = Date.now();
        slot.value = value;
        slot.fetchedAt = fetchedAt;
        return { value, cacheStatus: 'MISS-TFL', fetchedAt };
      } catch (error) {
        if (slot.value) {
          const result = {
            value: slot.value,
            cacheStatus: 'STALE-TFL',
            fetchedAt: slot.fetchedAt,
            cacheLayer: 'memory',
          };
          logTflCache(result, { reason: String(error) });
          return result;
        }
        const result = {
          value: {
            line: cfg.tflLineName,
            stop: cfg.tflStopName,
            direction: cfg.tflDirectionLabel,
            status: 'N/A',
            disruption: false,
            etas: [],
          },
          cacheStatus: 'MISS-TFL-ERROR',
          fetchedAt: 0,
        };
        logTflCache(result, { reason: String(error) });
        return result;
      } finally {
        inFlight.tfl = null;
      }
    })();
  }

  return inFlight.tfl;
}

async function getCachedWeather(cfg, kv) {
  const slot = memoryCache.weather;
  const memoryFetchedAt = slot.fetchedAt;
  const hadMemory = isWeatherUsable(slot.value);
  await hydrateSlotFromKv(kv, slot, WEATHER_CACHE_KEY);
  const cacheLayer = hadMemory && slot.fetchedAt === memoryFetchedAt ? 'memory' : 'kv';

  if (isWeatherUsable(slot.value) && cacheFresh(slot.fetchedAt, cfg.weatherCacheSeconds)) {
    const result = {
      value: slot.value,
      cacheStatus: 'HIT-WEATHER',
      fetchedAt: slot.fetchedAt,
      cacheLayer,
    };
    logWeatherCache(result, cfg);
    return result;
  }

  if (!inFlight.weather) {
    inFlight.weather = (async () => {
      try {
        const value = await getWeather(cfg);
        if (isWeatherUsable(value)) {
          const fetchedAt = Date.now();
          await persistSlotToKv(kv, slot, WEATHER_CACHE_KEY, value, fetchedAt);
          const result = {
            value,
            cacheStatus: 'MISS-WEATHER',
            fetchedAt,
            cacheLayer: 'upstream',
          };
          logWeatherCache(result, cfg);
          return result;
        }

        console.warn('Weather fetch returned no usable data');
        await hydrateSlotFromKv(kv, slot, WEATHER_CACHE_KEY);
        if (isWeatherUsable(slot.value)) {
          const result = {
            value: slot.value,
            cacheStatus: weatherCacheStatus(slot.fetchedAt, cfg.weatherCacheSeconds),
            fetchedAt: slot.fetchedAt,
            cacheLayer: 'kv',
          };
          logWeatherCache(result, cfg, { reason: 'upstream-unusable' });
          return result;
        }
        const result = {
          value,
          cacheStatus: 'MISS-WEATHER-ERROR',
          fetchedAt: 0,
        };
        logWeatherCache(result, cfg, { reason: 'upstream-unusable-no-cache' });
        return result;
      } catch (error) {
        console.warn('Weather fetch failed:', error);
        await hydrateSlotFromKv(kv, slot, WEATHER_CACHE_KEY);
        if (isWeatherUsable(slot.value)) {
          const result = {
            value: slot.value,
            cacheStatus: weatherCacheStatus(slot.fetchedAt, cfg.weatherCacheSeconds),
            fetchedAt: slot.fetchedAt,
            cacheLayer: 'kv',
          };
          logWeatherCache(result, cfg, { reason: String(error) });
          return result;
        }
        const result = {
          value: weatherFallback(cfg),
          cacheStatus: 'MISS-WEATHER-ERROR',
          fetchedAt: 0,
        };
        logWeatherCache(result, cfg, { reason: String(error) });
        return result;
      } finally {
        inFlight.weather = null;
      }
    })();
  }

  return inFlight.weather;
}

export default {
  async fetch(request, env) {
    const cfg = config(env);
    const kv = env.CACHE;
    const { pathname } = new URL(request.url);

    if (pathname === '/weather') {
      const weatherResult = await getCachedWeather(cfg, kv);

      return jsonResponse(
        {
          weather: weatherResult.value,
          meta: {
            cache: weatherResult.cacheStatus,
            fetchedAt: isoOrNull(weatherResult.fetchedAt),
          },
        },
        { cacheStatus: weatherResult.cacheStatus },
      );
    }

    if (pathname === '/tfl') {
      const tflResult = await getCachedTfl(cfg);

      return jsonResponse(
        {
          tfl: tflResult.value,
          meta: {
            cache: tflResult.cacheStatus,
            fetchedAt: isoOrNull(tflResult.fetchedAt),
          },
        },
        { cacheStatus: tflResult.cacheStatus },
      );
    }

    if (pathname !== '/') {
      return new Response('Not Found', { status: 404 });
    }

    const [tflResult, weatherResult] = await Promise.all([
      getCachedTfl(cfg),
      getCachedWeather(cfg, kv),
    ]);

    const cacheStatus = [tflResult.cacheStatus, weatherResult.cacheStatus].join(',');

    return jsonResponse(
      {
        tfl: tflResult.value,
        weather: weatherResult.value,
        meta: {
          lastFetched: {
            tfl: isoOrNull(tflResult.fetchedAt),
            metOffice: isoOrNull(weatherResult.fetchedAt),
          },
        },
      },
      { cacheStatus },
    );
  },
};
