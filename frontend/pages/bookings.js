Pages.bookings = (() => {
  let allBookings = [];
  let hotelId = 1;

  function statusFilter(val) {
    const rows = document.querySelectorAll('#bookings-tbody tr[data-status]');
    rows.forEach(r => r.style.display = (!val || r.dataset.status === val) ? '' : 'none');
  }

  async function viewBill(ref) {
    App.modal('Bill — ' + ref, '<div class="loader">Loading…</div>');
    try {
      const bill = await API.getBill(ref);
      const items = (bill.items||[]).map(i => `<div class="bill-row"><span>${i.description}</span><span>${App.inr(i.amount)}</span></div>`).join('');
      App.modal('Bill — ' + ref, `
        <div style="margin-bottom:12px"><strong>${bill.hotel?.name||''}</strong><br><small style="color:var(--text-muted)">${bill.booking?.guest_name||''} · ${bill.booking?.room_number||''}</small></div>
        <div class="bill-box">
          ${items}
          <div class="bill-row"><span>Subtotal</span><span>${App.inr(bill.subtotal)}</span></div>
          <div class="bill-row"><span>GST (${((bill.gst_rate||0)*100).toFixed(0)}%)</span><span>${App.inr(bill.gst_amount)}</span></div>
          <div class="bill-row bill-total"><span>TOTAL</span><span>${App.inr(bill.total)}</span></div>
        </div>`,
        `<button class="btn btn-secondary" onclick="Pages.bookings.previewReceipt('${ref}')">📄 View Receipt</button>
         <button class="btn btn-primary" onclick="Pages.bookings.downloadReceipt('${ref}')">⬇️ Download</button>
         <button class="btn btn-secondary" onclick="App.closeModal()">Close</button>`);
    } catch (e) { App.toast(e.message, 'error'); App.closeModal(); }
  }

  async function previewReceipt(ref) {
    App.modal('Receipt — ' + ref, '<div class="loader">Loading…</div>');
    try {
      const r = await API.getReceipt(ref);
      App.modal('Receipt — ' + ref,
        `<pre class="receipt-pre">${r.receipt_text||''}</pre>`,
        `<button class="btn btn-primary" onclick="Pages.bookings.downloadReceipt('${ref}')">⬇️ Download .txt</button>
         <button class="btn btn-secondary" onclick="App.closeModal()">Close</button>`);
    } catch (e) { App.toast(e.message, 'error'); }
  }

  async function downloadReceipt(ref) {
    try {
      const text = await API.downloadReceipt(ref);
      const blob = new Blob([text], { type: 'text/plain' });
      const a = document.createElement('a');
      a.href = URL.createObjectURL(blob);
      a.download = `RECEIPT-${ref}.txt`;
      a.click();
      App.toast('Receipt downloaded', 'success');
    } catch (e) { App.toast('Download failed: ' + e.message, 'error'); }
  }

  async function checkout(ref) {
    if (!confirm(`Check out booking ${ref}?`)) return;
    try {
      await API.checkoutBooking(ref);
      App.toast('Checked out: ' + ref, 'success');
      render(hotelId);
    } catch (e) { App.toast(e.message, 'error'); }
  }

  async function cancel(ref) {
    if (!confirm(`Cancel booking ${ref}? A 50% refund will be applied.`)) return;
    try {
      await API.cancel({ booking_id: ref });
      App.toast('Cancelled: ' + ref, 'success');
      render(hotelId);
    } catch (e) { App.toast(e.message, 'error'); }
  }

  async function render(hid) {
    hotelId = hid;
    const el = document.getElementById('page-bookings');
    el.innerHTML = `
      <div class="page-header">
        <div><div class="page-title">Bookings</div><div class="page-subtitle">All reservations</div></div>
        <button class="btn btn-primary" onclick="App.navigate('new-booking')">➕ New Booking</button>
      </div>
      <div class="filters">
        <input class="search-input" id="bk-search" placeholder="Search guest name / booking ID…"/>
        <select id="bk-status"><option value="">All Status</option><option value="active">Active</option><option value="checked_out">Checked Out</option><option value="cancelled">Cancelled</option></select>
      </div>
      <div class="card"><div class="table-wrap">
        <table><thead><tr><th>Booking ID</th><th>Guest</th><th>Room</th><th>Check-in</th><th>Check-out</th><th>Nights</th><th>Amount</th><th>Status</th><th>Actions</th></tr></thead>
        <tbody id="bookings-tbody"><tr><td colspan="9" class="loader">Loading…</td></tr></tbody></table>
      </div></div>`;

    document.getElementById('bk-search').addEventListener('input', function() {
      const q = this.value.toLowerCase();
      document.querySelectorAll('#bookings-tbody tr[data-status]').forEach(r => {
        r.style.display = (r.textContent.toLowerCase().includes(q)) ? '' : 'none';
      });
    });
    document.getElementById('bk-status').addEventListener('change', function() { statusFilter(this.value); });

    try {
      allBookings = await API.getBookings(hid);
      const tbody = document.getElementById('bookings-tbody');
      if (!allBookings.length) {
        tbody.innerHTML = `<tr><td colspan="9"><div class="empty-state"><div class="empty-icon">📋</div><p>No bookings yet</p></div></td></tr>`;
        return;
      }
      tbody.innerHTML = allBookings.map(b => `
        <tr data-status="${b.status||'active'}">
          <td><strong>${b.booking_id}</strong></td>
          <td>${b.guest_name}<br><small style="color:var(--text-muted)">${b.guest_phone||''}</small></td>
          <td>${b.room_number||'-'}</td>
          <td>${b.check_in_date}</td>
          <td>${b.check_out_date||'-'}</td>
          <td>${b.nights}</td>
          <td>${App.inr(b.total_amount||0)}</td>
          <td>${App.statusBadge(b.status||'active')}</td>
          <td>
            <button class="btn btn-sm btn-secondary" onclick="Pages.bookings.viewBill('${b.booking_id}')">💰 Bill</button>
            ${b.status==='active' ? `<button class="btn btn-sm btn-success" onclick="Pages.bookings.checkout('${b.booking_id}')">✔ Out</button>
            <button class="btn btn-sm btn-danger" onclick="Pages.bookings.cancel('${b.booking_id}')">✕</button>` : ''}
          </td>
        </tr>`).join('');
    } catch (e) {
      document.getElementById('bookings-tbody').innerHTML = `<tr><td colspan="9" style="color:var(--danger);text-align:center">${e.message}</td></tr>`;
    }
  }

  return { render, viewBill, previewReceipt, downloadReceipt, checkout, cancel };
})();
