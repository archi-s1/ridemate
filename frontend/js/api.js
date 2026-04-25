'use strict';

const API_BASE = 'http://localhost:3001/api';

const LOCATIONS = [
  'MG Road','Brigade Road','Electronic City','Banashankari',
  'Whitefield','Indiranagar','Ghaziabad','JP Nagar',
  'Hebbal','Yelahanka','Koramangala','Malleshwaram',
  'Rajajinagar','Basavanagudi','BTM Layout','Silk Board',
  'Marathahalli','Domlur','Jayanagar','Bannerghatta'
];

const VEHICLE_META = {
  moto:  { label:'Moto',  icon:'🛵', mult:0.7, desc:'Fastest & cheapest' },
  auto:  { label:'Auto',  icon:'🛺', mult:1.0, desc:'Standard 3-wheeler' },
  mini:  { label:'Mini',  icon:'🚗', mult:1.3, desc:'Comfortable AC cab' },
  sedan: { label:'Sedan', icon:'🚙', mult:2.0, desc:'Premium ride' },
};

// ── Auth ─────────────────────────────────────────────────────────────────
function getToken()  { return localStorage.getItem('rm_token'); }
function getUser()   { return JSON.parse(localStorage.getItem('rm_user') || 'null'); }
function saveAuth(token, userId, role, name) {
  localStorage.setItem('rm_token', token);
  localStorage.setItem('rm_user', JSON.stringify({ userId, role, name }));
}
function clearAuth() {
  localStorage.removeItem('rm_token');
  localStorage.removeItem('rm_user');
}
function isLoggedIn()   { return !!getToken(); }
function requireAuth()  { if (!isLoggedIn()) window.location.href = 'index.html'; }
function requireRole(r) {
  requireAuth();
  const u = getUser();
  if (u && u.role !== r) window.location.href = u.role === 'driver' ? 'driver.html' : 'dashboard.html';
}

// ── Core fetch ────────────────────────────────────────────────────────────
async function api(method, path, body = null, auth = true) {
  const headers = { 'Content-Type': 'application/json' };
  if (auth) headers['Authorization'] = `Bearer ${getToken()}`;
  const opts = { method, headers };
  if (body !== null) opts.body = JSON.stringify(body);
  try {
    const res = await fetch(`${API_BASE}${path}`, opts);
    return res.json();
  } catch (e) {
    return { ok: false, msg: 'Cannot reach server.' };
  }
}

// ── Domain APIs ───────────────────────────────────────────────────────────
const Auth    = {
  register: (name,email,pw,role,loc,vt) => api('POST','/auth/register',{name,email,password:pw,role,location:loc,vehicleType:vt},false),
  login:    (email,pw) => api('POST','/auth/login',{email,password:pw},false),
  logout:   ()         => api('POST','/auth/logout'),
};
const Rides   = {
  preview:  (pickup,drop,type) => api('POST','/rides/preview',{pickup,drop,rideType:type}),
  request:  (pid,pickup,drop,type) => api('POST','/rides/request',{passengerId:pid,pickup,drop,rideType:type}),
  assign:   () => api('POST','/rides/assign',{}),
  cancel:   (id) => api('POST',`/rides/${id}/cancel`,{}),
  complete: (id,rating) => api('POST',`/rides/${id}/complete`,{rating}),
  ongoing:  () => api('GET','/rides/ongoing'),
  recent:   () => api('GET','/rides/recent'),
};
const Drivers = {
  all:      () => api('GET','/drivers'),
  avail:    () => api('GET','/drivers/available'),
  add:      (name,loc,vt) => api('POST','/drivers',{name,location:loc,vehicleType:vt}),
  remove:   (id) => api('DELETE',`/drivers/${id}`,{}),
  toggle:   (id) => api('PATCH',`/drivers/${id}/toggle`,{}),
  history:  (id) => api('GET',`/drivers/${id}/history`),
};
const Passengers = {
  all:     () => api('GET','/passengers'),
  history: (id) => api('GET',`/passengers/${id}/history`),
};
const MapApi = {
  get:    () => api('GET','/map'),
  search: (q) => api('GET',`/map/search?prefix=${encodeURIComponent(q)}`),
};
const Meta = {
  algorithm: () => api('GET','/meta/algorithm'),
  health:    () => api('GET','/meta/health',null,false),
};

// ── UI Utilities ──────────────────────────────────────────────────────────
function locationName(i) { return LOCATIONS[i] ?? `Node ${i}`; }
function formatFare(f)   { return `₹${Number(f).toFixed(0)}`; }
function stars(r)        { const n=Math.round(r||0); return '★'.repeat(n)+'☆'.repeat(5-n); }

function toast(msg, type = 'success') {
  const t = Object.assign(document.createElement('div'), { className: `toast toast-${type}`, textContent: msg });
  document.body.appendChild(t);
  requestAnimationFrame(() => t.classList.add('show'));
  setTimeout(() => { t.classList.remove('show'); setTimeout(() => t.remove(), 400); }, 3200);
}

function setLoading(btn, loading) {
  if (!btn) return;
  btn.disabled = loading;
  btn.dataset.orig = btn.dataset.orig || btn.textContent;
  btn.textContent = loading ? '...' : btn.dataset.orig;
}

function locationSelect(name, selectedIdx = 0) {
  return `<select name="${name}" class="input">
    ${LOCATIONS.map((l,i) => `<option value="${i}"${i===selectedIdx?' selected':''}>${i} — ${l}</option>`).join('')}
  </select>`;
}
