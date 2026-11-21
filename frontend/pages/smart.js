Pages.smart = (() => {
  let hotelId = 1;

  // ── Render shell with tabs ──────────────────────────────────────────────────
  async function render(hid) {
    hotelId = hid;
    const el = document.getElementById('page-smart');
    el.innerHTML = `
      <div class="page-header">
        <div><div class="page-title">🤖 AI Smart Features</div><div class="page-subtitle">Powered by intelligent heuristics — Phase 11</div></div>
      </div>

      <!-- Tab bar -->
      <div style="display:flex;gap:6px;margin-bottom:24px;border-bottom:1px solid var(--border);padding-bottom:0">
        ${[
          ['recommend','🏷️ Room Recommender'],
          ['pricing',  '💹 Dynamic Pricing'],
          ['forecast', '📅 Occupancy Forecast'],
          ['suggest',  '🎯 Activity Suggester'],
          ['guest',    '👤 Guest Intelligence'],
          ['backups',  '💾 Backups'],
        ].map(([id,label]) => `<button class="smart-tab ${id==='recommend'?'active':''}" data-tab="${id}" style="padding:10px 16px;border:none;background:none;cursor:pointer;font-size:13px;font-weight:500;color:var(--text-muted);border-bottom:2px solid transparent;margin-bottom:-1px;transition:all .2s;font-family:inherit">${label}</button>`).join('')}
      </div>

      <!-- Tab panels -->
      <div id="smart-panel-recommend">${renderRecommendPanel()}</div>
      <div id="smart-panel-pricing"   style="display:none"></div>
      <div id="smart-panel-forecast"  style="display:none"></div>
      <div id="smart-panel-suggest"   style="display:none"></div>
      <div id="smart-panel-guest"     style="display:none"></div>
      <div id="smart-panel-backups"   style="display:none"></div>
    `;

    // Tab switching
    document.querySelectorAll('.smart-tab').forEach(tab => {
      tab.addEventListener('click', () => switchTab(tab.dataset.tab));
    });

    // Bind recommend form
    bindRecommend();
  }

  function switchTab(id) {
    // Update tab styles
    document.querySelectorAll('.smart-tab').forEach(t => {
      const active = t.dataset.tab === id;
      t.style.color = active ? 'var(--primary)' : 'var(--text-muted)';
      t.style.borderBottomColor = active ? 'var(--primary)' : 'transparent';
    });
    // Show/hide panels
    ['recommend','pricing','forecast','suggest','guest','backups'].forEach(t => {
      const p = document.getElementById('smart-panel-' + t);
      if (p) p.style.display = t === id ? '' : 'none';
    });
    // Load panel content
    const loaders = {
      pricing:  loadPricing,
      forecast: loadForecast,
      suggest:  loadSuggest,
      guest:    loadGuestPanel,
      backups:  loadBackups,
    };
    if (loaders[id]) loaders[id]();
  }

  // ── 1. ROOM RECOMMENDER ─────────────────────────────────────────────────────
  function renderRecommendPanel() {
    return `
    <div class="card" style="margin-bottom:20px">
      <div class="card-header"><span class="card-title">Find Your Perfect Room</span></div>
      <div class="card-body">
        <div class="form-row">
          <div class="form-group"><label class="form-label">Budget / Night (₹)</label><input id="rec-budget" class="form-input" type="number" value="8000" min="0"/></div>
          <div class="form-group"><label class="form-label">Guests</label><input id="rec-guests" class="form-input" type="number" value="2" min="1" max="10"/></div>
          <div class="form-group"><label class="form-label">Nights</label><input id="rec-nights" class="form-input" type="number" value="3" min="1" max="30"/></div>
        </div>
        <div class="form-group"><label class="form-label">Preferred Amenities (comma-separated)</label><input id="rec-amenities" class="form-input" placeholder="WiFi, AC, Pool" value="WiFi, AC"/></div>
        <div class="form-check"><input type="checkbox" id="rec-breakfast"/><label for="rec-breakfast">Include Breakfast</label></div>
        <button class="btn btn-primary" id="rec-btn" style="margin-top:14px">🤖 Get Recommendations</button>
      </div>
    </div>
    <div id="rec-results"></div>`;
  }

  function bindRecommend() {
    const btn = document.getElementById('rec-btn');
    if (btn) btn.addEventListener('click', runRecommend);
  }

  async function runRecommend() {
    const budget    = document.getElementById('rec-budget').value || 0;
    const guests    = document.getElementById('rec-guests').value || 1;
    const nights    = document.getElementById('rec-nights').value || 1;
    const amenities = document.getElementById('rec-amenities').value.trim();
    const breakfast = document.getElementById('rec-breakfast').checked;
    const res       = document.getElementById('rec-results');

    res.innerHTML = '<div class="loader">Analysing rooms…</div>';
    try {
      const data = await API.aiRecommend(hotelId, budget, guests, nights, breakfast, amenities);
      const recs  = data.recommendations || [];
      if (!recs.length) { res.innerHTML = '<div class="empty-state"><div class="empty-icon">🔍</div><p>No rooms found matching your criteria</p></div>'; return; }

      res.innerHTML = `
        <div class="card"><div class="card-header"><span class="card-title">Recommendations (${recs.length})</span></div>
        <div class="card-body" style="display:flex;flex-direction:column;gap:14px">
          ${recs.map((r, i) => `
          <div style="background:var(--surface2);border:1px solid var(--border);border-radius:10px;padding:16px;display:flex;align-items:center;gap:16px;${i===0?'border-color:var(--primary)':''}">
            <div style="font-size:36px">${i===0?'🥇':i===1?'🥈':'🥉'}</div>
            <div style="flex:1">
              <div style="display:flex;align-items:center;gap:10px;margin-bottom:4px">
                <strong style="font-size:15px">${r.room_type_name}</strong>
                ${i===0?'<span style="background:rgba(99,102,241,0.15);color:var(--primary);font-size:11px;padding:2px 8px;border-radius:10px;font-weight:600">Best Match</span>':''}
              </div>
              <div style="font-size:12px;color:var(--text-muted);margin-bottom:8px">${r.reason}</div>
              <div style="display:flex;gap:6px;flex-wrap:wrap">
                ${(r.amenities||[]).map(a=>`<span style="font-size:11px;padding:2px 8px;background:var(--surface);border:1px solid var(--border);border-radius:4px;color:var(--text-muted)">${a}</span>`).join('')}
              </div>
            </div>
            <div style="text-align:right;flex-shrink:0">
              <div style="font-size:20px;font-weight:700;color:var(--primary)">₹${Number(r.dynamic_price).toLocaleString('en-IN')}</div>
              <div style="font-size:11px;color:var(--text-muted)">per night</div>
              <div style="margin-top:8px">
                <div style="background:var(--surface);border-radius:6px;height:6px;width:80px;margin-left:auto;overflow:hidden">
                  <div style="height:100%;background:${r.match_score>=80?'var(--success)':r.match_score>=60?'var(--primary)':'var(--warning)'};width:${r.match_score}%;border-radius:6px"></div>
                </div>
                <div style="font-size:10px;color:var(--text-muted);text-align:center;margin-top:3px">${r.match_score}% match</div>
              </div>
            </div>
          </div>`).join('')}
        </div></div>`;
    } catch(e) {
      res.innerHTML = `<div class="alert alert-error">${e.message}</div>`;
    }
  }

  // ── 2. DYNAMIC PRICING ──────────────────────────────────────────────────────
  async function loadPricing() {
    const el = document.getElementById('smart-panel-pricing');
    el.innerHTML = '<div class="loader">Loading live pricing data…</div>';
    try {
      const data = await API.aiPricing(hotelId);
      const prices = data.prices || [];
      const occ    = Math.round((data.occupancy_rate || 0) * 100);
      const tier   = data.pricing_tier || 'normal';
      const tierColor = {low:'var(--success)', normal:'var(--primary)', high:'var(--warning)', peak:'var(--danger)'}[tier] || 'var(--primary)';

      el.innerHTML = `
        <div class="stats-grid" style="margin-bottom:20px">
          <div class="stat-card" style="--accent:${tierColor}">
            <div class="stat-label">Current Occupancy</div>
            <div class="stat-value">${occ}%</div>
            <div class="stat-sub">Live figure</div>
          </div>
          <div class="stat-card" style="--accent:${tierColor}">
            <div class="stat-label">Pricing Tier</div>
            <div class="stat-value" style="font-size:22px;text-transform:capitalize">${tier}</div>
            <div class="stat-sub" style="color:${tierColor}">${{low:'−15% discount active',normal:'Base rates apply',high:'+15% surge pricing',peak:'+30–50% peak rates'}[tier]||''}</div>
          </div>
          <div class="stat-card" style="--accent:var(--info)">
            <div class="stat-label">Multiplier</div>
            <div class="stat-value">${prices.length ? prices[0].multiplier.toFixed(2) : '1.00'}×</div>
            <div class="stat-sub">Applied to all room types</div>
          </div>
        </div>
        <div class="card"><div class="card-header"><span class="card-title">Live Room Prices</span><span style="font-size:12px;color:var(--text-muted)">Updates automatically based on occupancy</span></div>
        <div class="table-wrap"><table class="data-table">
          <thead><tr><th>Room Type</th><th>Base Price</th><th>Dynamic Price</th><th>Difference</th><th>Tier</th></tr></thead>
          <tbody>${prices.map(p => {
            const diff = p.adjusted_price - p.base_price;
            const diffStr = diff > 0 ? `+₹${diff.toLocaleString('en-IN')}` : diff < 0 ? `-₹${Math.abs(diff).toLocaleString('en-IN')}` : '—';
            const diffColor = diff > 0 ? 'var(--danger)' : diff < 0 ? 'var(--success)' : 'var(--text-muted)';
            return `<tr>
              <td><strong>${p.room_type_name}</strong></td>
              <td style="color:var(--text-muted)">₹${Number(p.base_price).toLocaleString('en-IN')}</td>
              <td><strong style="color:var(--primary)">₹${Number(p.adjusted_price).toLocaleString('en-IN')}</strong></td>
              <td style="color:${diffColor}">${diffStr}</td>
              <td><span class="badge badge-${tier==='peak'?'danger':tier==='high'?'warning':tier==='low'?'success':'info'}">${tier}</span></td>
            </tr>`;
          }).join('')}</tbody>
        </table></div></div>

        <div class="card" style="margin-top:16px"><div class="card-header"><span class="card-title">Pricing Tiers Explained</span></div>
        <div class="card-body">
          ${[
            ['Low','< 30% occupied','−15%','Discounts to attract bookings','var(--success)'],
            ['Normal','30–55% occupied','Base rate','Standard pricing','var(--primary)'],
            ['High','55–75% occupied','+15%','Mild surge for rising demand','var(--warning)'],
            ['Peak','75–90% occupied','+30%','High demand surge','var(--danger)'],
          ].map(([t,occ,adj,desc,c]) => `
            <div style="display:flex;align-items:center;gap:14px;padding:10px 0;border-bottom:1px solid var(--border)">
              <span style="background:rgba(99,102,241,0.1);color:${c};font-size:11px;font-weight:600;padding:3px 10px;border-radius:10px;min-width:60px;text-align:center">${t}</span>
              <span style="color:var(--text-muted);font-size:12px;min-width:150px">${occ}</span>
              <strong style="color:${c};min-width:50px">${adj}</strong>
              <span style="font-size:12px;color:var(--text-muted)">${desc}</span>
            </div>`).join('')}
        </div></div>`;
    } catch(e) {
      el.innerHTML = `<div class="alert alert-error">${e.message}</div>`;
    }
  }

  // ── 3. OCCUPANCY FORECAST ────────────────────────────────────────────────────
  async function loadForecast() {
    const el = document.getElementById('smart-panel-forecast');
    el.innerHTML = '<div class="loader">Computing 14-day forecast…</div>';
    try {
      const data = await API.aiForecast(hotelId, 14);
      const fc   = data.forecast || [];
      if (!fc.length) { el.innerHTML = '<div class="empty-state"><p>No forecast data available</p></div>'; return; }

      const maxRate = Math.max(...fc.map(d => d.predicted_rate), 0.01);
      const labelColor = {Low:'var(--success)', Normal:'var(--primary)', High:'var(--warning)', Peak:'var(--danger)'};

      el.innerHTML = `
        <div class="card" style="margin-bottom:16px"><div class="card-header"><span class="card-title">14-Day Occupancy Forecast</span></div>
        <div class="card-body">
          <div style="display:flex;align-items:flex-end;gap:6px;height:140px;padding:8px 0">
            ${fc.map(d => {
              const pct = Math.max(4, (d.predicted_rate / maxRate) * 100);
              const c   = labelColor[d.label] || 'var(--primary)';
              const day = d.date ? d.date.slice(5) : '';
              return `<div style="flex:1;display:flex;flex-direction:column;align-items:center;justify-content:flex-end;height:100%;gap:5px">
                <div style="font-size:9px;color:var(--text-muted)">${Math.round(d.predicted_rate*100)}%</div>
                <div style="width:100%;border-radius:4px 4px 0 0;min-height:4px;height:${pct}%;background:${c};" title="${d.label}: ${Math.round(d.predicted_rate*100)}%"></div>
                <div style="font-size:9px;color:var(--text-muted);white-space:nowrap">${day}</div>
              </div>`;
            }).join('')}
          </div>
          <div style="display:flex;gap:16px;justify-content:center;margin-top:12px">
            ${Object.entries(labelColor).map(([l,c])=>`<span style="display:flex;align-items:center;gap:5px;font-size:12px;color:var(--text-muted)"><span style="width:10px;height:10px;border-radius:2px;background:${c};display:inline-block"></span>${l}</span>`).join('')}
          </div>
        </div></div>

        <div class="card"><div class="card-header"><span class="card-title">Day-by-Day Breakdown</span></div>
        <div class="table-wrap"><table class="data-table">
          <thead><tr><th>Date</th><th>Predicted Occupancy</th><th>Est. Bookings</th><th>Status</th></tr></thead>
          <tbody>${fc.map(d => `<tr>
            <td>${d.date}</td>
            <td>
              <div style="display:flex;align-items:center;gap:8px">
                <div style="flex:1;height:4px;background:var(--surface2);border-radius:2px;max-width:120px"><div style="height:100%;border-radius:2px;background:${labelColor[d.label]||'var(--primary)'};width:${Math.round(d.predicted_rate*100)}%"></div></div>
                <span>${Math.round(d.predicted_rate*100)}%</span>
              </div>
            </td>
            <td>${d.predicted_bookings}</td>
            <td><span class="badge badge-${d.label==='Peak'?'danger':d.label==='High'?'warning':d.label==='Low'?'success':'info'}">${d.label}</span></td>
          </tr>`).join('')}</tbody>
        </table></div></div>`;
    } catch(e) {
      el.innerHTML = `<div class="alert alert-error">${e.message}</div>`;
    }
  }

  // ── 4. ACTIVITY SUGGESTER ────────────────────────────────────────────────────
  function loadSuggest() {
    const el = document.getElementById('smart-panel-suggest');
    el.innerHTML = `
      <div class="card" style="margin-bottom:20px"><div class="card-header"><span class="card-title">Get Activity Suggestions</span></div>
      <div class="card-body">
        <div class="form-row">
          <div class="form-group"><label class="form-label">Nights of Stay</label><input id="sug-nights" class="form-input" type="number" value="3" min="1" max="30"/></div>
          <div class="form-group"><label class="form-label">Check-in Date</label><input id="sug-checkin" class="form-input" type="date" value="2026-05-15"/></div>
        </div>
        <button class="btn btn-primary" id="sug-btn">🎯 Suggest Activities</button>
      </div></div>
      <div id="sug-results"></div>`;

    document.getElementById('sug-btn').addEventListener('click', async () => {
      const nights  = document.getElementById('sug-nights').value || 1;
      const checkin = document.getElementById('sug-checkin').value;
      const res     = document.getElementById('sug-results');
      res.innerHTML = '<div class="loader">Calculating best activities…</div>';
      try {
        const data = await API.aiActivities(hotelId, nights, checkin);
        const sugg = data.suggestions || [];
        if (!sugg.length) { res.innerHTML = '<div class="empty-state"><p>No activities available</p></div>'; return; }
        res.innerHTML = `<div class="card"><div class="card-header"><span class="card-title">Recommended Activities</span></div>
          <div class="card-body" style="display:flex;flex-direction:column;gap:12px">
            ${sugg.map((s,i) => `
            <div style="background:var(--surface2);border:1px solid var(--border);border-radius:10px;padding:14px;display:flex;align-items:center;gap:14px">
              <div style="width:32px;height:32px;border-radius:50%;background:${i===0?'var(--primary)':i===1?'var(--warning)':'var(--border)'};display:flex;align-items:center;justify-content:center;color:#fff;font-weight:700;font-size:13px;flex-shrink:0">${s.priority}</div>
              <div style="flex:1">
                <div style="font-weight:600;margin-bottom:3px">${s.name}</div>
                <div style="font-size:12px;color:var(--text-muted)">${s.reason}</div>
              </div>
              <div style="text-align:right;flex-shrink:0">
                <div style="font-size:16px;font-weight:700;color:var(--primary)">₹${Number(s.price).toLocaleString('en-IN')}</div>
                <div style="font-size:10px;color:var(--text-muted)">per person</div>
              </div>
            </div>`).join('')}
          </div></div>`;
      } catch(e) { res.innerHTML = `<div class="alert alert-error">${e.message}</div>`; }
    });
  }

  // ── 5. GUEST INTELLIGENCE ────────────────────────────────────────────────────
  function loadGuestPanel() {
    const el = document.getElementById('smart-panel-guest');
    el.innerHTML = `
      <div class="card" style="margin-bottom:20px"><div class="card-header"><span class="card-title">Guest Intelligence Lookup</span></div>
      <div class="card-body">
        <div class="form-group"><label class="form-label">Guest Phone or Email</label>
          <div style="display:flex;gap:10px">
            <input id="gi-query" class="form-input" placeholder="9876543210 or guest@email.com" style="flex:1"/>
            <button class="btn btn-primary" id="gi-btn">🔍 Analyse</button>
          </div>
        </div>
      </div></div>
      <div id="gi-results"></div>`;

    document.getElementById('gi-btn').addEventListener('click', async () => {
      const q   = document.getElementById('gi-query').value.trim();
      const res = document.getElementById('gi-results');
      if (!q) return;
      res.innerHTML = '<div class="loader">Fetching guest history…</div>';
      try {
        const gi = await API.aiGuestIntel(hotelId, q);
        const tierColor = {New:'var(--info)', Returning:'var(--success)', Loyal:'var(--warning)', VIP:'var(--primary)'};
        const tierIcon  = {New:'🌱', Returning:'🔄', Loyal:'⭐', VIP:'👑'};
        res.innerHTML = `
          <div class="card">
            <div class="card-header">
              <span class="card-title">Guest Profile</span>
              <span class="badge badge-${gi.tier==='VIP'?'info':gi.tier==='Loyal'?'warning':'success'}">${tierIcon[gi.tier]||''} ${gi.tier}</span>
            </div>
            <div class="card-body">
              <div class="stats-grid" style="margin-bottom:20px">
                <div class="stat-card"><div class="stat-label">Total Stays</div><div class="stat-value">${gi.total_stays}</div></div>
                <div class="stat-card"><div class="stat-label">Total Spent</div><div class="stat-value" style="font-size:20px">₹${Number(gi.total_spent).toLocaleString('en-IN')}</div></div>
                <div class="stat-card"><div class="stat-label">Avg Stay</div><div class="stat-value">${gi.avg_stay_nights}</div><div class="stat-sub">nights</div></div>
                <div class="stat-card"><div class="stat-label">Preferred Room</div><div class="stat-value" style="font-size:16px">${gi.preferred_room_type||'—'}</div></div>
              </div>
              <div style="background:rgba(99,102,241,0.08);border:1px solid rgba(99,102,241,0.2);border-radius:10px;padding:16px;display:flex;align-items:center;gap:14px">
                <span style="font-size:28px">🎁</span>
                <div>
                  <div style="font-weight:600;margin-bottom:4px">Personalised Offer</div>
                  <div style="color:var(--primary);font-size:14px">${gi.personalised_offer}</div>
                </div>
              </div>
            </div>
          </div>`;
      } catch(e) {
        res.innerHTML = e.message.includes('404')
          ? `<div class="alert alert-info">No booking history found for this guest. They may be a new customer.</div>`
          : `<div class="alert alert-error">${e.message}</div>`;
      }
    });
  }

  // ── 6. BACKUPS ──────────────────────────────────────────────────────────────
  async function loadBackups() {
    const el = document.getElementById('smart-panel-backups');
    el.innerHTML = `
      <div class="page-header" style="margin-bottom:20px">
        <div><div class="page-title" style="font-size:18px">Database Backups</div></div>
        <button class="btn btn-primary" id="create-backup-btn">💾 Create Backup Now</button>
      </div>
      <div id="backup-list"><div class="loader">Loading backups…</div></div>`;

    document.getElementById('create-backup-btn').addEventListener('click', async () => {
      try {
        await API.createBackup();
        App.toast('Backup created successfully', 'success');
        await refreshBackupList();
      } catch(e) { App.toast('Backup failed: ' + e.message, 'error'); }
    });

    await refreshBackupList();
  }

  async function refreshBackupList() {
    const el = document.getElementById('backup-list');
    if (!el) return;
    try {
      const data    = await API.listBackups();
      const backups = data.backups || [];
      if (!backups.length) {
        el.innerHTML = `<div class="empty-state"><div class="empty-icon">💾</div><p>No backups yet. Click "Create Backup Now" to start.</p></div>`;
        return;
      }
      el.innerHTML = `<div class="card"><div class="card-header"><span class="card-title">Saved Backups (${backups.length})</span></div>
        <div class="table-wrap"><table class="data-table">
          <thead><tr><th>Filename</th><th>Created At</th><th>Size</th><th>Actions</th></tr></thead>
          <tbody>${backups.map(b => `<tr>
            <td style="font-family:monospace;font-size:12px">${b.filename}</td>
            <td style="color:var(--text-muted)">${b.created_at}</td>
            <td style="color:var(--text-muted)">${b.size_kb} KB</td>
            <td><div style="display:flex;gap:6px">
              <button class="btn btn-sm btn-secondary restore-btn" data-file="${b.filename}">♻️ Restore</button>
              <button class="btn btn-sm btn-danger delete-backup-btn" data-file="${b.filename}">🗑 Delete</button>
            </div></td>
          </tr>`).join('')}</tbody>
        </table></div></div>`;

      document.querySelectorAll('.delete-backup-btn').forEach(btn => {
        btn.onclick = async () => {
          if (!confirm('Delete backup ' + btn.dataset.file + '?')) return;
          try {
            await API.deleteBackup(btn.dataset.file);
            App.toast('Backup deleted', 'success');
            await refreshBackupList();
          } catch(e) { App.toast(e.message, 'error'); }
        };
      });
      document.querySelectorAll('.restore-btn').forEach(btn => {
        btn.onclick = () => App.toast('Restore: ' + btn.dataset.file + ' — restart server to apply', 'info');
      });
    } catch(e) {
      el.innerHTML = `<div class="alert alert-error">${e.message}</div>`;
    }
  }

  return { render };
})();
