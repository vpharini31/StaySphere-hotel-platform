Pages.hotel = (() => {
  async function render(hotelId) {
    const el = document.getElementById('page-hotel');
    el.innerHTML = `<div class="page-header"><div><div class="page-title">Hotel Info</div></div></div><div class="loader">Loading…</div>`;
    try {
      const hotel = await API.getHotel(hotelId);
      el.innerHTML = `
        <div class="page-header"><div><div class="page-title">Hotel Info</div><div class="page-subtitle">View and update hotel details</div></div></div>
        <div class="card"><div class="card-header"><span class="card-title">Details</span></div><div class="card-body" style="padding:20px">
          <div id="hotel-alert"></div>
          <div class="form-row">
            <div class="form-group"><label>Hotel Name</label><input id="h-name" value="${hotel.name||''}"/></div>
            <div class="form-group"><label>Location / City</label><input id="h-loc" value="${hotel.location||''}"/></div>
          </div>
          <div class="form-group"><label>Address</label><input id="h-addr" value="${hotel.address||''}"/></div>
          <div class="form-row">
            <div class="form-group"><label>Phone</label><input id="h-phone" value="${hotel.phone||''}"/></div>
            <div class="form-group"><label>Email</label><input id="h-email" value="${hotel.email||''}"/></div>
          </div>
          <div class="form-group"><label>Website</label><input id="h-web" value="${hotel.website||''}"/></div>
          <button class="btn btn-primary" id="h-save">Save Changes</button>
        </div></div>`;

      document.getElementById('h-save').addEventListener('click', async () => {
        const btn = document.getElementById('h-save');
        btn.textContent = 'Saving…'; btn.disabled = true;
        try {
          await API.updateHotel(hotelId, {
            name:     document.getElementById('h-name').value.trim(),
            location: document.getElementById('h-loc').value.trim(),
            address:  document.getElementById('h-addr').value.trim(),
            phone:    document.getElementById('h-phone').value.trim(),
            email:    document.getElementById('h-email').value.trim(),
            website:  document.getElementById('h-web').value.trim(),
          });
          App.toast('Hotel details saved', 'success');
        } catch (e) {
          document.getElementById('hotel-alert').innerHTML = `<div class="alert alert-error">${e.message}</div>`;
        }
        btn.textContent = 'Save Changes'; btn.disabled = false;
      });
    } catch (e) {
      el.innerHTML = `<div class="alert alert-error">${e.message}</div>`;
    }
  }
  return { render };
})();
