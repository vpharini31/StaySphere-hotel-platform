window.Pages.search = {
  render: async function() {
    const isValidImage = (value) => typeof value === "string" && value.trim() && !["null", "undefined", "none", "n/a", "na", "#"].includes(value.trim().toLowerCase());
    const hotelImage = (h) => {
      const gallery = Array.isArray(h.images) ? h.images : [];
      return [h.image_path, h.image, h.image_url, gallery[0]?.image_path, gallery[0]?.image_url].find(isValidImage) || "";
    };
    const imageLoads = (src) => new Promise((resolve) => {
      if (!isValidImage(src)) return resolve(false);
      const img = new Image();
      const timer = setTimeout(() => resolve(false), 6000);
      img.onload = () => {
        clearTimeout(timer);
        resolve(img.naturalWidth > 0 && img.naturalHeight > 0);
      };
      img.onerror = () => {
        clearTimeout(timer);
        resolve(false);
      };
      img.src = src;
    };
    const container = document.getElementById('page-search');
    container.innerHTML = `
      <div style="padding: 30px; max-width: 1200px; margin: 0 auto;">
        <div style="margin-bottom: 30px; text-align: center; animation: formEnter 0.5s ease;">
          <h2 style="font-family: 'Poppins', sans-serif; font-size: 32px; color: #0f172a; margin-bottom: 8px;">Find Your Perfect Stay</h2>
          <p style="color: #64748b; font-size: 16px; margin: 0;">Explore luxury hotels and beautiful resorts around the world.</p>
        </div>
        <div id="hotel-cards" style="display: grid; grid-template-columns: repeat(auto-fill, minmax(320px, 1fr)); gap: 24px;">
          <div style="color: #64748b; text-align: center; grid-column: 1 / -1; padding: 40px;">Loading amazing properties...</div>
        </div>
      </div>
    `;

    try {
      const hotels = await API.getHotels();
      const cardsContainer = document.getElementById('hotel-cards');
      cardsContainer.innerHTML = '';
      
      if (!hotels || hotels.length === 0) {
        cardsContainer.innerHTML = `<div style="color: #64748b; text-align: center; grid-column: 1 / -1; padding: 40px;">No hotels available right now. Check back later!</div>`;
        return;
      }

      const validHotels = [];
      for (const hotel of hotels) {
        const imageUrl = hotelImage(hotel);
        if (await imageLoads(imageUrl)) validHotels.push({ ...hotel, imageUrl });
      }

      if (validHotels.length === 0) {
        cardsContainer.innerHTML = `<div style="color: #64748b; text-align: center; grid-column: 1 / -1; padding: 40px;">No hotels with verified images are available right now. Check back later!</div>`;
        return;
      }

      validHotels.forEach((h) => {
        const bg = '#f1f5f9';
        const imageUrl = h.imageUrl;
        
        const imgHTML = `
          <div style="height: 200px; background: ${bg}; border-radius: 16px 16px 0 0; overflow: hidden;">
            <img src="${imageUrl}" alt="${h.name}" loading="lazy" onerror="this.closest('[data-hotel-card]')?.remove()" style="width: 100%; height: 100%; object-fit: cover;">
          </div>
        `;
        
        cardsContainer.innerHTML += `
          <div data-hotel-card style="background: #fff; border-radius: 16px; box-shadow: 0 4px 6px -1px rgba(0,0,0,0.05); border: 1px solid #e2e8f0; transition: transform 0.2s, box-shadow 0.2s; display: flex; flex-direction: column;" 
               onmouseover="this.style.transform='translateY(-6px)'; this.style.boxShadow='0 12px 20px -3px rgba(0,0,0,0.1)';" 
               onmouseout="this.style.transform='none'; this.style.boxShadow='0 4px 6px -1px rgba(0,0,0,0.05)';">
            ${imgHTML}
            <div style="padding: 24px; flex: 1; display: flex; flex-direction: column;">
              <div style="display: flex; justify-content: space-between; align-items: flex-start; margin-bottom: 8px;">
                <h3 style="margin: 0; font-family: 'Poppins', sans-serif; font-size: 20px; color: #0f172a;">${h.name}</h3>
                <span style="background: #dcfce7; color: #15803d; padding: 4px 10px; border-radius: 20px; font-size: 12px; font-weight: 600;">⭐ ${h.review_score || 'New'}</span>
              </div>
              <p style="margin: 0 0 8px 0; color: #475569; font-size: 14px; display: flex; align-items: center; gap: 4px;">
                <span style="font-size: 16px;">📍</span> ${h.location}
              </p>
              <p style="margin: 0; color: #0f172a; font-weight: 600; font-size: 18px; margin-top: 8px;">
                Rs. ${Number(h.price_from).toLocaleString("en-IN", { maximumFractionDigits: 0 })} <span style="font-size: 12px; font-weight: 400; color: #64748b;">/ night</span>
              </p>
              <div style="margin-top: auto; padding-top: 20px;">
                <button class="btn btn-primary" style="width: 100%; padding: 12px; font-size: 14px;" data-details="${h.id}">View Rooms</button>
              </div>
            </div>
          </div>
        `;
      });
    } catch (e) {
      App.toast(e.message, 'error');
    }
  }
};
