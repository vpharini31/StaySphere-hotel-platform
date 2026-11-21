Pages.rooms = (() => {
  async function render(hotelId) {
    const el = document.getElementById('page-rooms');
    el.innerHTML = `
      <div class="page-header"><div><div class="page-title">Rooms</div><div class="page-subtitle">Room status overview</div></div></div>
      <div style="display:flex;gap:16px;margin-bottom:16px;flex-wrap:wrap">
        <span style="display:flex;align-items:center;gap:6px;font-size:12px"><span style="width:12px;height:12px;background:var(--success);border-radius:50%;display:inline-block"></span>Available</span>
        <span style="display:flex;align-items:center;gap:6px;font-size:12px"><span style="width:12px;height:12px;background:var(--danger);border-radius:50%;display:inline-block"></span>Occupied</span>
        <span style="display:flex;align-items:center;gap:6px;font-size:12px"><span style="width:12px;height:12px;background:var(--warning);border-radius:50%;display:inline-block"></span>Maintenance</span>
      </div>
      <div id="rooms-content"><div class="loader">Loading rooms…</div></div>`;

    try {
      const [rooms, types] = await Promise.all([API.getRooms(hotelId), API.getRoomTypes(hotelId)]);
      const typeMap = {};
      (types||[]).forEach(t => { typeMap[t.id] = t; });

      // Group rooms by type
      const grouped = {};
      (rooms||[]).forEach(r => {
        const tid = r.room_type_id;
        if (!grouped[tid]) grouped[tid] = [];
        grouped[tid].push(r);
      });

      const totalRooms = (rooms||[]).length;
      const available  = (rooms||[]).filter(r => r.status === 'available').length;
      const occupied   = (rooms||[]).filter(r => r.status === 'occupied').length;

      let html = `<div class="stats-grid" style="margin-bottom:20px">
        <div class="stat-card" style="--accent:var(--primary)"><div class="stat-label">Total Rooms</div><div class="stat-value">${totalRooms}</div></div>
        <div class="stat-card" style="--accent:var(--success)"><div class="stat-label">Available</div><div class="stat-value">${available}</div></div>
        <div class="stat-card" style="--accent:var(--danger)"><div class="stat-label">Occupied</div><div class="stat-value">${occupied}</div></div>
        <div class="stat-card" style="--accent:var(--warning)"><div class="stat-label">Maintenance</div><div class="stat-value">${totalRooms-available-occupied}</div></div>
      </div>`;

      Object.entries(grouped).forEach(([tid, roomList]) => {
        const type = typeMap[tid];
        const typeName  = type ? type.name : 'Unknown';
        const typePrice = type ? App.inr(type.current_price||type.base_price) : '';
        html += `<div class="card" style="margin-bottom:16px">
          <div class="card-header">
            <span class="card-title">${typeName}</span>
            <span style="font-size:12px;color:var(--text-muted)">${typePrice}/night · ${roomList.length} rooms</span>
          </div>
          <div class="card-body" style="padding:16px">
            <div class="room-grid">
              ${roomList.map(r => `
                <div class="room-tile ${r.status||'available'}" title="${r.status}">
                  <div class="room-number">${r.room_number}</div>
                  <div class="room-type-label">${typeName}</div>
                  <div class="room-status-dot"></div>
                </div>`).join('')}
            </div>
          </div>
        </div>`;
      });

      document.getElementById('rooms-content').innerHTML = html || `<div class="empty-state"><div class="empty-icon">🛏️</div><p>No rooms found</p></div>`;
    } catch (e) {
      document.getElementById('rooms-content').innerHTML = `<div class="alert alert-error">${e.message}</div>`;
    }
  }
  return { render };
})();
