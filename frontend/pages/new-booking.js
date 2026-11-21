Pages.newBooking = (() => {
  let activities = [];
  let hotelId = 1;

  async function render(hid) {
    hotelId = hid;
    const el = document.getElementById('page-new-booking');
    el.innerHTML = `
      <div class="page-header"><div><div class="page-title">New Booking</div><div class="page-subtitle">Create a reservation</div></div></div>
      <div style="display:grid;grid-template-columns:1fr 360px;gap:20px;align-items:start">
        <div class="card"><div class="card-header"><span class="card-title">Guest & Stay Details</span></div><div class="card-body" style="padding:20px">
          <div id="nb-alert"></div>
          <div class="form-row">
            <div class="form-group"><label>Guest Name *</label><input id="nb-name" placeholder="Full name"/></div>
            <div class="form-group"><label>Phone *</label><input id="nb-phone" placeholder="10-digit mobile"/></div>
          </div>
          <div class="form-row">
            <div class="form-group"><label>Email</label><input id="nb-email" type="email" placeholder="guest@email.com"/></div>
            <div class="form-group"><label>Address</label><input id="nb-address" placeholder="City / State"/></div>
          </div>
          <hr style="border-color:var(--border);margin:12px 0"/>
          <div class="form-row">
            <div class="form-group"><label>Room Type *</label><select id="nb-room-type"><option value="">Loading…</option></select></div>
            <div class="form-group"><label>Check-in Date *</label><input id="nb-checkin" type="date"/></div>
          </div>
          <div class="form-row">
            <div class="form-group"><label>Nights *</label><input id="nb-nights" type="number" value="1" min="1" max="365"/></div>
            <div class="form-group"><label>&nbsp;</label>
              <div class="form-check"><input type="checkbox" id="nb-breakfast"/><label for="nb-breakfast">🍳 Include Breakfast</label></div>
            </div>
          </div>
          <div class="form-group"><label>Activities</label><div id="nb-activities" style="display:flex;flex-wrap:wrap;gap:8px;margin-top:4px"><div class="loader">Loading…</div></div></div>
          <button class="btn btn-primary btn-full" style="margin-top:8px" id="nb-submit">Confirm Booking</button>
        </div></div>

        <div class="card" style="position:sticky;top:24px"><div class="card-header"><span class="card-title">💰 Cost Preview</span></div><div class="card-body" style="padding:16px">
          <div id="nb-preview"><div class="empty-state" style="padding:20px"><div class="empty-icon">🧮</div><p>Fill in details to see cost</p></div></div>
        </div></div>
      </div>`;

    // Set today as min date
    const today = new Date().toISOString().split('T')[0];
    document.getElementById('nb-checkin').min = today;
    document.getElementById('nb-checkin').value = today;

    try {
      const [types, acts] = await Promise.all([API.getRoomTypes(hid), API.getActivities()]);
      activities = acts || [];
      const sel = document.getElementById('nb-room-type');
      sel.innerHTML = '<option value="">Select room type…</option>' +
        (types||[]).map(t => `<option value="${t.id}" data-price="${t.current_price||t.base_price}">${t.name} — ${App.inr(t.current_price||t.base_price)}/night</option>`).join('');

      document.getElementById('nb-activities').innerHTML = activities.length
        ? activities.map(a => `<label style="display:flex;align-items:center;gap:6px;background:var(--surface2);padding:6px 10px;border-radius:8px;cursor:pointer;font-size:12px">
            <input type="checkbox" class="act-cb" value="${a.id}" style="accent-color:var(--primary)"/> ${a.name} (${App.inr(a.price)})</label>`).join('')
        : '<span style="color:var(--text-muted);font-size:13px">No activities available</span>';
    } catch (e) {
      App.toast('Failed to load room types: ' + e.message, 'error');
    }

    // Live preview on any change
    ['nb-room-type','nb-nights','nb-checkin','nb-breakfast'].forEach(id => {
      document.getElementById(id)?.addEventListener('change', updatePreview);
    });
    document.getElementById('nb-nights')?.addEventListener('input', updatePreview);
    document.getElementById('nb-activities')?.addEventListener('change', updatePreview);
    document.getElementById('nb-submit').addEventListener('click', submit);
  }

  async function updatePreview() {
    const rtId    = document.getElementById('nb-room-type').value;
    const nights  = parseInt(document.getElementById('nb-nights').value)||1;
    const checkin = document.getElementById('nb-checkin').value;
    const breakf  = document.getElementById('nb-breakfast').checked;
    const actIds  = [...document.querySelectorAll('.act-cb:checked')].map(c => parseInt(c.value));

    if (!rtId || !checkin) return;

    try {
      const data = await API.previewBooking({
        hotel_id: hotelId, room_type_id: parseInt(rtId),
        check_in_date: checkin, nights, breakfast: breakf,
        activities: JSON.stringify(actIds)
      });
      const items = (data.items||[]).map(i => `<div class="bill-row"><span>${i.description}</span><span>${App.inr(i.amount)}</span></div>`).join('');
      document.getElementById('nb-preview').innerHTML = `
        <div class="bill-box">
          ${items}
          <div class="bill-row"><span>Subtotal</span><span>${App.inr(data.subtotal)}</span></div>
          <div class="bill-row"><span>GST (5%)</span><span>${App.inr(data.gst_amount)}</span></div>
          <div class="bill-row bill-total"><span>TOTAL</span><span>${App.inr(data.total)}</span></div>
        </div>
        <div style="margin-top:10px;font-size:12px;color:var(--text-muted)">Check-out: ${data.check_out_date||'-'}</div>`;
    } catch (e) {
      document.getElementById('nb-preview').innerHTML = `<div class="alert alert-error">${e.message}</div>`;
    }
  }

  async function submit() {
    const name   = document.getElementById('nb-name').value.trim();
    const phone  = document.getElementById('nb-phone').value.trim();
    const email  = document.getElementById('nb-email').value.trim();
    const addr   = document.getElementById('nb-address').value.trim();
    const rtId   = document.getElementById('nb-room-type').value;
    const checkin= document.getElementById('nb-checkin').value;
    const nights = parseInt(document.getElementById('nb-nights').value)||1;
    const breakf = document.getElementById('nb-breakfast').checked;
    const actIds = [...document.querySelectorAll('.act-cb:checked')].map(c => parseInt(c.value));
    const alertEl= document.getElementById('nb-alert');

    alertEl.innerHTML = '';
    if (!name || !phone || !rtId || !checkin) {
      alertEl.innerHTML = '<div class="alert alert-error">Name, phone, room type and check-in date are required.</div>';
      return;
    }
    const btn = document.getElementById('nb-submit');
    btn.textContent = 'Creating…'; btn.disabled = true;
    try {
      const res = await API.createBooking({
        hotel_id: hotelId, room_type_id: parseInt(rtId),
        guest_name: name, guest_phone: phone, guest_email: email, guest_address: addr,
        check_in_date: checkin, nights, breakfast: breakf,
        activities: JSON.stringify(actIds)
      });
      App.toast('Booking created: ' + res.booking_id, 'success');
      App.navigate('bookings');
    } catch (e) {
      alertEl.innerHTML = `<div class="alert alert-error">${e.message}</div>`;
    }
    btn.textContent = 'Confirm Booking'; btn.disabled = false;
  }

  return { render };
})();
