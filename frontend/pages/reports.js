Pages.reports = (() => {
  async function render(hotelId) {
    const el = document.getElementById('page-reports');
    const today = new Date().toISOString().split('T')[0];
    const monthStart = today.slice(0,7) + '-01';

    el.innerHTML = `
      <div class="page-header"><div><div class="page-title">Reports</div><div class="page-subtitle">Revenue & analytics</div></div></div>
      <div class="card" style="margin-bottom:16px"><div class="card-body" style="padding:16px">
        <div style="display:flex;gap:10px;align-items:flex-end;flex-wrap:wrap">
          <div class="form-group" style="margin:0"><label>From</label><input id="rp-from" type="date" value="${monthStart}"/></div>
          <div class="form-group" style="margin:0"><label>To</label><input id="rp-to" type="date" value="${today}"/></div>
          <button class="btn btn-primary" id="rp-btn">Generate</button>
        </div>
      </div></div>
      <div id="rp-content"><div class="loader">Loading…</div></div>`;

    document.getElementById('rp-btn').addEventListener('click', () => loadReports(hotelId));
    loadReports(hotelId);
  }

  async function loadReports(hotelId) {
    const from = document.getElementById('rp-from').value;
    const to   = document.getElementById('rp-to').value;
    const cont = document.getElementById('rp-content');
    cont.innerHTML = '<div class="loader">Loading…</div>';
    try {
      const [rev, byType, stats, popular, acts, timeline] = await Promise.all([
        API.getRevenue(hotelId, from, to),
        API.getRevenueByType(hotelId, from, to),
        API.getStats(hotelId),
        API.getPopularRooms(hotelId),
        API.getPopularActs(hotelId),
        API.getTimeline(hotelId, 30)
      ]);

      // Revenue summary
      let html = `<div class="stats-grid">
        <div class="stat-card" style="--accent:var(--info)"><div class="stat-label">Period Revenue</div><div class="stat-value" style="font-size:20px">${App.inr(rev.total_revenue||0)}</div><div class="stat-sub">${from} → ${to}</div></div>
        <div class="stat-card" style="--accent:var(--primary)"><div class="stat-label">Room Revenue</div><div class="stat-value" style="font-size:18px">${App.inr(rev.room_revenue||0)}</div></div>
        <div class="stat-card" style="--accent:var(--success)"><div class="stat-label">Activity Revenue</div><div class="stat-value" style="font-size:18px">${App.inr(rev.activity_revenue||0)}</div></div>
        <div class="stat-card" style="--accent:var(--warning)"><div class="stat-label">Breakfast Revenue</div><div class="stat-value" style="font-size:18px">${App.inr(rev.breakfast_revenue||0)}</div></div>
        <div class="stat-card" style="--accent:var(--primary)"><div class="stat-label">Bookings (period)</div><div class="stat-value">${rev.total_bookings||0}</div></div>
        <div class="stat-card" style="--accent:var(--danger)"><div class="stat-label">Cancellations</div><div class="stat-value">${rev.cancellations||0}</div></div>
      </div>`;

      // Revenue timeline chart (30 days)
      if (timeline && timeline.length) {
        const maxRev = Math.max(...timeline.map(d => parseFloat(d.revenue)||0), 1);
        const bars = timeline.map(d => {
          const pct = Math.max(3, ((parseFloat(d.revenue)||0)/maxRev)*100);
          return `<div class="bar-wrap"><div class="bar" style="height:${pct}%" title="${App.inr(d.revenue||0)}&#10;${d.day}"></div><div class="bar-label">${(d.day||'').slice(5)}</div></div>`;
        }).join('');
        html += `<div class="card" style="margin-bottom:16px"><div class="card-header"><span class="card-title">Daily Revenue – Last 30 Days</span></div><div class="card-body" style="padding:16px"><div class="bar-chart" style="height:140px">${bars}</div></div></div>`;
      }

      // Revenue by room type
      if (byType && byType.length) {
        html += `<div class="card" style="margin-bottom:16px"><div class="card-header"><span class="card-title">Revenue by Room Type</span></div><div class="table-wrap"><table>
          <thead><tr><th>Room Type</th><th>Bookings</th><th>Revenue</th><th>Avg Nights</th></tr></thead>
          <tbody>${byType.map(r => `<tr><td>${r.room_type_name||'-'}</td><td>${r.bookings||0}</td><td>${App.inr(r.revenue||0)}</td><td>${parseFloat(r.avg_nights||0).toFixed(1)}</td></tr>`).join('')}</tbody>
        </table></div></div>`;
      }

      // Popular rooms + activities side by side
      html += `<div style="display:grid;grid-template-columns:1fr 1fr;gap:16px;margin-bottom:16px">`;
      if (popular && popular.length) {
        html += `<div class="card"><div class="card-header"><span class="card-title">Most Booked Room Types</span></div><div class="table-wrap"><table>
          <thead><tr><th>Room Type</th><th>Bookings</th></tr></thead>
          <tbody>${popular.map(r => `<tr><td>${r.room_type_name||'-'}</td><td>${r.bookings||0}</td></tr>`).join('')}</tbody>
        </table></div></div>`;
      }
      if (acts && acts.length) {
        html += `<div class="card"><div class="card-header"><span class="card-title">Popular Activities</span></div><div class="table-wrap"><table>
          <thead><tr><th>Activity</th><th>Booked</th><th>Revenue</th></tr></thead>
          <tbody>${acts.map(a => `<tr><td>${a.name||'-'}</td><td>${a.times_booked||0}</td><td>${App.inr(a.total_revenue||0)}</td></tr>`).join('')}</tbody>
        </table></div></div>`;
      }
      html += `</div>`;

      cont.innerHTML = html;
    } catch (e) {
      cont.innerHTML = `<div class="alert alert-error">${e.message}</div>`;
    }
  }

  return { render };
})();
