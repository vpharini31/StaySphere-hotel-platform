Pages.dashboard = (() => {
  async function render(hotelId) {
    const el = document.getElementById('page-dashboard');
    el.innerHTML = `<div class="page-header"><div><div class="page-title">Dashboard</div><div class="page-subtitle">Live overview</div></div></div>
<div class="stats-grid" id="dash-stats"><div class="loader">Loading stats…</div></div>
<div style="display:grid;grid-template-columns:1fr 1fr;gap:16px">
  <div class="card"><div class="card-header"><span class="card-title">Today's Arrivals</span></div><div class="card-body" id="dash-arrivals"><div class="loader">Loading…</div></div></div>
  <div class="card"><div class="card-header"><span class="card-title">Today's Departures</span></div><div class="card-body" id="dash-departures"><div class="loader">Loading…</div></div></div>
</div>
<div class="card" style="margin-top:16px"><div class="card-header"><span class="card-title">Revenue – Last 14 Days</span></div><div class="card-body" style="padding:16px"><div id="dash-chart"></div></div></div>`;

    try {
      const [stats, today, timeline] = await Promise.all([
        API.getStats(hotelId),
        API.searchToday(hotelId),
        API.getTimeline(hotelId, 14)
      ]);

      document.getElementById('dash-stats').innerHTML = `
        <div class="stat-card" style="--accent:var(--primary)"><span class="stat-icon">📋</span><div class="stat-label">Total Bookings</div><div class="stat-value">${stats.total||0}</div><div class="stat-sub">${stats.active||0} active now</div></div>
        <div class="stat-card" style="--accent:var(--success)"><span class="stat-icon">✅</span><div class="stat-label">Checked Out</div><div class="stat-value">${stats.checked_out||0}</div><div class="stat-sub">completed stays</div></div>
        <div class="stat-card" style="--accent:var(--danger)"><span class="stat-icon">❌</span><div class="stat-label">Cancelled</div><div class="stat-value">${stats.cancelled||0}</div></div>
        <div class="stat-card" style="--accent:var(--warning)"><span class="stat-icon">🏨</span><div class="stat-label">Occupancy</div><div class="stat-value">${(stats.occupancy_rate||0).toFixed(1)}%</div></div>
        <div class="stat-card" style="--accent:var(--info)"><span class="stat-icon">💰</span><div class="stat-label">Total Revenue</div><div class="stat-value" style="font-size:20px">${App.inr(stats.total_revenue||0)}</div></div>
        <div class="stat-card" style="--accent:var(--primary)"><span class="stat-icon">🌙</span><div class="stat-label">Avg Stay</div><div class="stat-value">${(stats.avg_stay_nights||0).toFixed(1)}</div><div class="stat-sub">nights</div></div>`;

      const guestRows = (arr) => arr.length ? arr.map(b => `<tr><td>${b.booking_id||'-'}</td><td>${b.guest_name}</td><td>${b.room_number||'-'}</td></tr>`).join('') : `<tr><td colspan="3" style="text-align:center;color:var(--text-muted)">None today</td></tr>`;
      document.getElementById('dash-arrivals').innerHTML   = `<table><thead><tr><th>Ref</th><th>Guest</th><th>Room</th></tr></thead><tbody>${guestRows(today.arrivals||[])}</tbody></table>`;
      document.getElementById('dash-departures').innerHTML = `<table><thead><tr><th>Ref</th><th>Guest</th><th>Room</th></tr></thead><tbody>${guestRows(today.departures||[])}</tbody></table>`;

      // Bar chart
      const days = timeline || [];
      if (days.length) {
        const maxRev = Math.max(...days.map(d => parseFloat(d.revenue)||0), 1);
        const bars = days.map(d => {
          const pct = Math.max(4, ((parseFloat(d.revenue)||0) / maxRev) * 100);
          const label = (d.day||'').slice(5); // MM-DD
          return `<div class="bar-wrap"><div class="bar" style="height:${pct}%" title="${App.inr(d.revenue||0)}"></div><div class="bar-label">${label}</div></div>`;
        }).join('');
        document.getElementById('dash-chart').innerHTML = `<div class="bar-chart">${bars}</div>`;
      } else {
        document.getElementById('dash-chart').innerHTML = `<div class="empty-state" style="padding:20px"><p>No revenue data yet</p></div>`;
      }
    } catch (e) {
      App.toast('Dashboard error: ' + e.message, 'error');
    }
  }
  return { render };
})();
