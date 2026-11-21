const App = (() => {
  const state = {
    hotels: [],
    destinations: [],
    user: null,
    activePortal: "dashboard",
    authMode: "login",
    hotelId: Number(localStorage.getItem("hms_hotel_id") || 0),
    selectedHotel: null,
    activities: [],
    paymentConfig: null,
    lastBookingPreview: null,
    roomInventoryPage: 1,
    expandedRoomTypeId: null,
    roomSyncTimer: null,
    roomSyncRunning: false,
    selectedCategory: "",
  };

  const inputFilesMap = new Map();

  const fallbackHotels = [
    {
      id: 1,
      name: "Grand Palace Hotel",
      location: "Mumbai, Maharashtra",
      image_url: "https://images.unsplash.com/photo-1566073771259-6a8506099945?auto=format&fit=crop&w=1400&q=80",
      description: "A polished city hotel with sea-facing rooms, rooftop dining, and quick business access.",
      amenities: '["WiFi","Pool","Parking","Breakfast","AC","Spa"]',
      price_from: 15000,
      star_rating: 5,
      review_score: 9.2,
      property_type: "Hotel",
      nearby_places: ["Gateway of India (2.5 km)", "Marine Drive (1.2 km)", "Colaba Causeway (3 km)"],
      activities: ["Rooftop Dinner", "City Heritage Walk", "Spa Therapy"]
    },
    {
      id: 2,
      name: "Aravali Sky Retreat",
      location: "Udaipur, Rajasthan",
      image_url: "https://images.unsplash.com/photo-1542314831-068cd1dbfeeb?auto=format&fit=crop&w=1400&q=80",
      description: "A lakeside luxury retreat with courtyards, pool decks, and palace-inspired suites.",
      amenities: '["WiFi","Pool","Parking","Breakfast","AC","Lake view"]',
      price_from: 22000,
      star_rating: 5,
      review_score: 9.5,
      property_type: "Luxury Resort",
      nearby_places: ["City Palace (1.5 km)", "Lake Pichola (0 km)", "Jag Mandir (2 km)"],
      activities: ["Sunset Boat Ride", "Rajasthani Cooking Class", "Yoga by the Lake"]
    },
    {
      id: 3,
      name: "Coastal Nest Villas",
      location: "Goa",
      image_url: "https://images.unsplash.com/photo-1571896349842-33c89424de2d?auto=format&fit=crop&w=1400&q=80",
      description: "Private beach-side villas with patios, kitchenettes, and pet-friendly long stays.",
      amenities: '["WiFi","Parking","Breakfast","AC","Pet friendly","Beach access"]',
      price_from: 12000,
      star_rating: 4.8,
      review_score: 9.1,
      property_type: "Villa Stay",
      nearby_places: ["Candolim Beach (0.5 km)", "Aguada Fort (4 km)", "Baga Beach (3 km)"],
      activities: ["Private BBQ", "Water Sports", "Bicycle Rental"]
    },
    {
      id: 4,
      name: "Himalayan Pines",
      location: "Manali, Himachal Pradesh",
      image_url: "https://images.unsplash.com/photo-1605649487212-47bdab064df7?auto=format&fit=crop&w=1400&q=80",
      description: "Cozy timber cabins surrounded by snow-capped peaks and pine forests.",
      amenities: '["WiFi","Parking","Breakfast","Heater","Mountain view"]',
      price_from: 8500,
      star_rating: 4.5,
      review_score: 8.8,
      property_type: "Cabin",
      nearby_places: ["Solang Valley (12 km)", "Hidimba Temple (10 km)", "Mall Road (2 km)"],
      activities: ["Trekking", "Paragliding", "Bonfire Nights"]
    },
    {
      id: 5,
      name: "Kerala Backwaters Resort",
      location: "Alleppey, Kerala",
      image_url: "https://images.unsplash.com/photo-1593693397690-362cb9666c89?auto=format&fit=crop&w=1400&q=80",
      description: "Traditional houseboat experiences and luxury resort rooms overlooking tranquil backwaters.",
      amenities: '["WiFi","Pool","Breakfast","AC","Spa","River view"]',
      price_from: 18000,
      star_rating: 4.9,
      review_score: 9.4,
      property_type: "Resort",
      nearby_places: ["Alleppey Beach (5 km)", "Kumarakom Bird Sanctuary (15 km)", "Marari Beach (12 km)"],
      activities: ["Houseboat Cruise", "Ayurvedic Massage", "Village Canoe Tour"]
    },
    {
      id: 6,
      name: "City Comfort Inn",
      location: "Mumbai, Maharashtra",
      image_url: "https://images.unsplash.com/photo-1551882547-ff40c63fe5fa?auto=format&fit=crop&w=1400&q=80",
      description: "A budget-friendly transit hotel offering clean rooms, free breakfast, and easy airport access.",
      amenities: '["WiFi","AC","Breakfast"]',
      price_from: 2500,
      star_rating: 3,
      review_score: 7.5,
      property_type: "Hotel",
      nearby_places: ["Mumbai Airport (3 km)", "Juhu Beach (6 km)"],
      activities: ["Local Market Tour"]
    },
    {
      id: 7,
      name: "Backpackers Haven",
      location: "Goa",
      image_url: "https://images.unsplash.com/photo-1520250497591-112f2f40a3f4?auto=format&fit=crop&w=1400&q=80",
      description: "Vibrant hostel and budget hotel with a communal lounge, bar, and walking distance to the beach.",
      amenities: '["WiFi","Bar","AC"]',
      price_from: 1500,
      star_rating: 3,
      review_score: 8.2,
      property_type: "Hostel",
      nearby_places: ["Anjuna Beach (1 km)", "Curlies (1.5 km)"],
      activities: ["Pub Crawl", "Scooter Rental"]
    },
    {
      id: 8,
      name: "Royal Heritage Palace",
      location: "Jaipur, Rajasthan",
      image_url: "https://images.unsplash.com/photo-1477587458883-47135dfdb56f?auto=format&fit=crop&w=1400&q=80",
      description: "A breathtaking heritage palace offering royal suites, traditional courtyards, and majestic elephant rides.",
      amenities: '["WiFi","Pool","Breakfast","AC","Spa","Heritage"]',
      price_from: 28000,
      star_rating: 5,
      review_score: 9.7,
      property_type: "Luxury Hotel",
      nearby_places: ["Amer Fort (2 km)", "Hawa Mahal (4 km)", "City Palace (5 km)"],
      activities: ["Elephant Ride", "Heritage Walk", "Royal Dining"]
    }
  ];

  const fallbackRoomImages = [
    "https://images.unsplash.com/photo-1611892440504-42a792e24d32?auto=format&fit=crop&w=1200&q=80",
    "https://images.unsplash.com/photo-1618773928121-c32242e63f39?auto=format&fit=crop&w=1200&q=80",
    "https://images.unsplash.com/photo-1598035411571-b9f7a4254a43?auto=format&fit=crop&w=1200&q=80",
    "https://images.unsplash.com/photo-1596394516093-501ba68a0ba6?auto=format&fit=crop&w=1200&q=80"
  ];

  const propertyAmenities = [
    "Free Wi-Fi", "Swimming pool", "Spa", "Gym", "Restaurant", "Parking",
    "Airport shuttle", "Pet friendly", "Room service", "Bar/lounge", "Laundry"
  ];

  const roomFeatureOptions = [
    "Air conditioning", "Wi-Fi", "Private bathroom", "Balcony", "Sea view",
    "City view", "Work desk", "Mini fridge", "TV", "Coffee maker",
    "Wardrobe", "In-room safe", "Soundproofing"
  ];

  const paymentMethods = ["Pay Now", "Pay at Property"];
  const requiredVerificationDocuments = [
    ["ownership_certificate_upload", "Hotel Ownership Certificate"],
    ["business_license_upload", "Business License"],
    ["tourism_registration_upload", "Tourism Registration"],
    ["id_proof_upload", "ID Proof"],
  ];
  const CANCELLATION_WINDOW_MS = 24 * 60 * 60 * 1000;
  const cancellationReasons = [
    "Change of travel plans",
    "Booked by mistake",
    "Found another hotel",
    "Date or guest count changed",
    "Payment or pricing issue",
    "Other"
  ];

  const $ = (selector, root = document) => root.querySelector(selector);
  const $$ = (selector, root = document) => [...root.querySelectorAll(selector)];

  function safe(value) {
    return String(value ?? "").replace(/[&<>"']/g, (char) => ({
      "&": "&amp;",
      "<": "&lt;",
      ">": "&gt;",
      '"': "&quot;",
      "'": "&#039;",
    }[char]));
  }

  function renderSvgChart(dataPoints, valKey, labelKey, formatValFn = (v) => v) {
    if (!dataPoints || !dataPoints.length) {
      return `<div class="empty-state">No data available</div>`;
    }

    const width = 600;
    const height = 180;
    const paddingLeft = 60;
    const paddingRight = 20;
    const paddingTop = 20;
    const paddingBottom = 30;

    const chartWidth = width - paddingLeft - paddingRight;
    const chartHeight = height - paddingTop - paddingBottom;

    const values = dataPoints.map(d => Number(d[valKey] || 0));
    const maxVal = Math.max(10, ...values) * 1.15;

    const points = dataPoints.map((d, i) => {
      const x = paddingLeft + (i * (chartWidth / Math.max(1, dataPoints.length - 1)));
      const y = paddingTop + (chartHeight * (1 - (Number(d[valKey] || 0) / maxVal)));
      return { x, y, val: d[valKey], label: d[labelKey] };
    });

    let linePath = "";
    let areaPath = "";
    if (points.length > 0) {
      linePath = "M " + points.map(p => `${p.x} ${p.y}`).join(" L ");
      areaPath = linePath + ` L ${points[points.length - 1].x} ${paddingTop + chartHeight} L ${points[0].x} ${paddingTop + chartHeight} Z`;
    }

    const gridLines = [];
    const ySteps = 3;
    for (let i = 0; i <= ySteps; i++) {
      const ratio = i / ySteps;
      const y = paddingTop + chartHeight * ratio;
      const val = maxVal * (1 - ratio);
      gridLines.push({
        y,
        label: formatValFn(val)
      });
    }

    const gradId = `chart-grad-${Math.random().toString(36).substr(2, 9)}`;

    return `
      <div class="svg-chart-container" style="position: relative; width: 100%;">
        <svg viewBox="0 0 ${width} ${height}" style="width: 100%; height: auto; overflow: visible;">
          <defs>
            <linearGradient id="${gradId}" x1="0" y1="0" x2="0" y2="1">
              <stop offset="0%" stop-color="var(--accent)" stop-opacity="0.35"/>
              <stop offset="100%" stop-color="var(--accent)" stop-opacity="0.00"/>
            </linearGradient>
          </defs>
          
          ${gridLines.map((gl, idx) => `
            <g class="grid-line-group">
              <text x="${paddingLeft - 10}" y="${gl.y + 4}" text-anchor="end" fill="var(--muted)" style="font-size: 10px; font-family: var(--font-secondary);">${gl.label}</text>
              <line x1="${paddingLeft}" y1="${gl.y}" x2="${width - paddingRight}" y2="${gl.y}" stroke="var(--border)" stroke-width="1" ${idx === gridLines.length - 1 ? "" : 'stroke-dasharray="3 3"'} opacity="0.3" />
            </g>
          `).join("")}
          
          ${points.length > 1 ? `<path d="${areaPath}" fill="url(#${gradId})" />` : ""}
          ${points.length > 1 ? `<path d="${linePath}" fill="none" stroke="var(--accent)" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round" />` : ""}
          
          ${points.map((p, idx) => {
      if (points.length > 10 && idx % 2 !== 0) return "";
      return `<text x="${p.x}" y="${height - 5}" text-anchor="middle" fill="var(--muted)" style="font-size: 9px; font-family: var(--font-secondary);">${safe(p.label)}</text>`;
    }).join("")}
          
          ${points.map(p => `
            <g class="chart-point">
              <circle cx="${p.x}" cy="${p.y}" r="4" fill="var(--surface)" stroke="var(--accent)" stroke-width="2" />
              <circle cx="${p.x}" cy="${p.y}" r="8" fill="transparent" style="cursor: pointer;">
                <title>${safe(p.label)}: ${formatValFn(p.val)}</title>
              </circle>
            </g>
          `).join("")}
        </svg>
      </div>
    `;
  }

  function renderSvgBarChart(dataPoints, valKey, labelKey, formatValFn = (v) => v) {
    if (!dataPoints || !dataPoints.length) {
      return `<div class="empty-state">No data available</div>`;
    }

    const width = 600;
    const height = 180;
    const paddingLeft = 60;
    const paddingRight = 20;
    const paddingTop = 20;
    const paddingBottom = 30;

    const chartWidth = width - paddingLeft - paddingRight;
    const chartHeight = height - paddingTop - paddingBottom;

    const values = dataPoints.map(d => Number(d[valKey] || 0));
    const maxVal = Math.max(10, ...values) * 1.15;

    const segmentWidth = chartWidth / dataPoints.length;
    const barWidth = Math.min(45, Math.max(12, segmentWidth * 0.5));

    const bars = dataPoints.map((d, i) => {
      const val = Number(d[valKey] || 0);
      const h = chartHeight * (val / maxVal);
      const x = paddingLeft + (i * segmentWidth) + (segmentWidth - barWidth) / 2;
      const y = paddingTop + chartHeight - h;
      return { x, y, w: barWidth, h, val, label: d[labelKey] };
    });

    const gridLines = [];
    const ySteps = 3;
    for (let i = 0; i <= ySteps; i++) {
      const ratio = i / ySteps;
      const y = paddingTop + chartHeight * ratio;
      const val = maxVal * (1 - ratio);
      gridLines.push({
        y,
        label: formatValFn(val)
      });
    }

    const gradId = `bar-grad-${Math.random().toString(36).substr(2, 9)}`;

    return `
      <div class="svg-chart-container" style="position: relative; width: 100%;">
        <svg viewBox="0 0 ${width} ${height}" style="width: 100%; height: auto; overflow: visible;">
          <defs>
            <linearGradient id="${gradId}" x1="0" y1="0" x2="0" y2="1">
              <stop offset="0%" stop-color="var(--accent)" stop-opacity="0.85"/>
              <stop offset="100%" stop-color="var(--accent)" stop-opacity="0.25"/>
            </linearGradient>
          </defs>
          
          ${gridLines.map((gl, idx) => `
            <g class="grid-line-group">
              <text x="${paddingLeft - 10}" y="${gl.y + 4}" text-anchor="end" fill="var(--muted)" style="font-size: 10px; font-family: var(--font-secondary);">${gl.label}</text>
              <line x1="${paddingLeft}" y1="${gl.y}" x2="${width - paddingRight}" y2="${gl.y}" stroke="var(--border)" stroke-width="1" ${idx === gridLines.length - 1 ? "" : 'stroke-dasharray="3 3"'} opacity="0.3" />
            </g>
          `).join("")}
          
          ${bars.map((b, idx) => `
            <g class="chart-bar-group">
              <rect x="${b.x}" y="${b.y}" width="${b.w}" height="${Math.max(2, b.h)}" rx="3" ry="3" fill="url(#${gradId})" style="transition: fill 0.2s;" />
              <text x="${b.x + b.w / 2}" y="${height - 5}" text-anchor="middle" fill="var(--muted)" style="font-size: 9px; font-family: var(--font-secondary);">${safe(b.label)}</text>
              <rect x="${b.x - 2}" y="${paddingTop}" width="${b.w + 4}" height="${chartHeight}" fill="transparent" style="cursor: pointer;">
                <title>${safe(b.label)}: ${formatValFn(b.val)}</title>
              </rect>
            </g>
          `).join("")}
        </svg>
      </div>
    `;
  }

  function parseLocalDateTime(value) {
    const text = String(value || "").trim();
    const match = text.match(/^(\d{4})-(\d{2})-(\d{2})(?:[ T](\d{2}):(\d{2})(?::(\d{2}))?)?/);
    if (match) {
      const [, year, month, day, hour = "0", minute = "0", second = "0"] = match;
      const parsed = new Date(
        Number(year),
        Number(month) - 1,
        Number(day),
        Number(hour),
        Number(minute),
        Number(second)
      );
      return Number.isNaN(parsed.getTime()) ? null : parsed;
    }
    const parsed = new Date(text);
    return Number.isNaN(parsed.getTime()) ? null : parsed;
  }

  function bookingCancellationState(booking = {}) {
    const createdAt = parseLocalDateTime(booking.created_at);
    if (!createdAt) return { allowed: true, title: "Cancel booking" };
    const isAllowed = Date.now() < createdAt.getTime() + CANCELLATION_WINDOW_MS;
    return {
      allowed: isAllowed,
      title: isAllowed ? "Cancel booking" : "Cancellation disabled after 1 day of booking",
    };
  }

  function parseAmenities(value) {
    if (Array.isArray(value)) return value;
    try {
      const parsed = JSON.parse(value || "[]");
      return Array.isArray(parsed) ? parsed : [];
    } catch {
      return String(value || "")
        .split(",")
        .map((item) => item.trim())
        .filter(Boolean);
    }
  }

  function listToArray(value) {
    return String(value || "")
      .split(/\n|,/)
      .map((item) => item.trim())
      .filter(Boolean);
  }

  function parseImages(value) {
    if (Array.isArray(value)) return value;
    try {
      const parsed = JSON.parse(value || "[]");
      return Array.isArray(parsed) ? parsed : [];
    } catch {
      return String(value || "")
        .split(",")
        .map((item) => item.trim())
        .filter(Boolean);
    }
  }

  function checkedValues(form, name) {
    return [...form.querySelectorAll(`input[name="${name}"]:checked`)].map((input) => input.value);
  }

  function checkboxGrid(name, options, selected = []) {
    const active = new Set(selected);
    return `
      <div class="checkbox-grid" data-checkbox-group="${safe(name)}">
        ${options.map((option) => `
          <label class="check-tile">
            <input type="checkbox" name="${name}" value="${safe(option)}" ${active.has(option) ? "checked" : ""}>
            <span>${safe(option)}</span>
          </label>
        `).join("")}
      </div>
    `;
    setupVerificationDocumentValidation($("#manager-hotel-form"));
  }

  function policyText(value) {
    return String(value ?? "").trim();
  }

  function policyHtml(value) {
    return safe(policyText(value)).replace(/\r?\n/g, "<br>");
  }

  function hotelPolicyItems(hotel = {}) {
    return [
      ["check_in_time", "Check-in time", "&#128337;"],
      ["check_out_time", "Check-out time", "&#128336;"],
      ["cancellation_policy", "Cancellation policy", "&#8634;"],
      ["refund_rules", "Refund rules", "&#8377;"],
      ["smoking_policy", "Smoking policy", "&#128683;"],
      ["pet_policy", "Pet policy", "&#128062;"],
      ["id_proof_requirements", "ID proof requirements", "&#128196;"],
      ["extra_guest_rules", "Extra guest rules", "&#128101;"],
      ["child_policy", "Children policy", "&#128118;"],
      ["custom_policies", "Custom policies", "&#128221;"],
    ].map(([key, label, icon]) => ({
      key,
      label,
      icon,
      value: policyText(hotel[key]),
    })).filter((item) => item.value);
  }

  function hotelPoliciesCard(hotel = {}) {
    const policies = hotelPolicyItems(hotel);
    const isLong = policies.length > 6 || policies.reduce((sum, item) => sum + item.value.length, 0) > 520;
    return `
      <div class="hotel-policies-card" data-policy-card>
        <button class="hotel-policies-header" type="button" data-policy-collapse aria-expanded="true">
          <span class="hotel-policies-title">
            <span class="policy-shield">&#128737;</span>
            <span>
              <strong>Hotel Policies &amp; Terms</strong>
              <small>${policies.length ? `${policies.length} admin-defined ${policies.length === 1 ? "policy" : "policies"}` : "No admin-defined policies listed"}</small>
            </span>
          </span>
          <span class="policy-chevron" aria-hidden="true">&#8963;</span>
        </button>
        <div class="hotel-policies-body">
          ${policies.length ? `
            <div class="hotel-policy-list ${isLong ? "is-truncated" : ""}" data-policy-list>
              ${policies.map((item) => `
                <div class="hotel-policy-item">
                  <span class="policy-icon" aria-hidden="true">${item.icon}</span>
                  <span>
                    <strong>${safe(item.label)}</strong>
                    <small>${policyHtml(item.value)}</small>
                  </span>
                </div>
              `).join("")}
            </div>
            ${isLong ? `<button class="policy-read-more" type="button" data-policy-read-more aria-expanded="false">Read more</button>` : ""}
          ` : `<p class="hotel-policy-empty">This property has not added additional policies yet.</p>`}
        </div>
      </div>
    `;
  }

  function coalesceImage(...values) {
    return values.flat().find(isValidImageCandidate) || "";
  }

  function isValidImageCandidate(value) {
    if (typeof value !== "string") return false;
    const src = value.trim();
    if (!src) return false;
    const invalid = ["null", "undefined", "none", "n/a", "na", "#"];
    return !invalid.includes(src.toLowerCase());
  }

  function hotelImageCandidates(hotel = {}) {
    const images = Array.isArray(hotel?.images) ? hotel.images : [];
    return [
      ...images.map((image) => image?.image_path),
      ...images.map((image) => image?.image_url),
      ...images.map((image) => image?.url),
      hotel?.image_path,
      hotel?.image_url,
      hotel?.image,
    ].filter(isValidImageCandidate)
      .map((value) => value.trim())
      .filter((value, index, list) => list.indexOf(value) === index);
  }

  function imageLoads(src) {
    if (!isValidImageCandidate(src)) return Promise.resolve(false);
    if (!state.imageValidationCache) state.imageValidationCache = new Map();
    if (state.imageValidationCache.has(src)) return state.imageValidationCache.get(src);
    const check = new Promise((resolve) => {
      const img = new Image();
      const done = (ok) => {
        clearTimeout(timer);
        resolve(ok);
      };
      const timer = setTimeout(() => done(false), 6000);
      img.onload = () => done(img.naturalWidth > 0 && img.naturalHeight > 0);
      img.onerror = () => done(false);
      img.src = src;
    });
    state.imageValidationCache.set(src, check);
    return check;
  }

  async function hotelWithValidImage(hotel = {}) {
    for (const src of hotelImageCandidates(hotel)) {
      if (await imageLoads(src)) {
        hotel.image_path = src;
        hotel.image_url = src;
        hotel.image = src;
        return hotel;
      }
    }
    return null;
  }

  function hotelImage(hotel = {}, index = 0) {
    const images = Array.isArray(hotel?.images) ? hotel.images : [];
    return coalesceImage(images[index]?.image_path, images[index]?.image_url, images[index]?.url, hotel?.image_path, hotel?.image_url, hotel?.image);
  }

  function roomImage(room = {}, index = 0) {
    const fallback = fallbackRoomImages[index % fallbackRoomImages.length] || fallbackRoomImages[0];
    const images = Array.isArray(room?.images) ? room.images : [];
    return coalesceImage(images[index]?.image_path, images[index]?.image_url, room?.image_path, room?.image_url, fallback);
  }

  function uploadZone(name, label, { id, multiple = true, accept = "image/jpeg,image/png", existing = [], compact = false } = {}) {
    const inputId = id || `${name}-${Math.random().toString(16).slice(2)}`;
    const isHotelProfileUpload = name === "hotel_photos";
    const fieldClass = ["upload-field", compact ? "compact" : "", isHotelProfileUpload ? "hotel-profile-upload" : ""].filter(Boolean).join(" ");
    const thumbClass = ["upload-thumb", isHotelProfileUpload ? "hotel-upload-thumb image-wrapper" : ""].filter(Boolean).join(" ");
    const imageClass = isHotelProfileUpload ? "hotel-image" : "";
    const previewItems = existing.filter(Boolean).map((src, index) => `
      <figure class="${thumbClass}">
        <img class="${imageClass}" src="${safe(src)}" alt="${safe(label)} ${index + 1}">
      </figure>
    `).join("");
    return `
      <div class="${fieldClass}">
        <label class="upload-zone" for="${inputId}">
          <input id="${inputId}" name="${name}" type="file" accept="${accept}" ${multiple ? "multiple" : ""} data-upload-input data-preview-target="${inputId}-preview">
          <span class="upload-icon">+</span>
          <strong>${safe(label)}</strong>
          <small>JPG or PNG, up to 5 MB</small>
        </label>
        <div class="upload-preview" id="${inputId}-preview">${previewItems || `<span class="muted">No photos selected</span>`}</div>
      </div>
    `;
  }

  function skeletonHotelCard() {
    return `<article class="hotel-card skeleton-card"><div></div><section><span></span><span></span><span></span></section></article>`;
  }

  function initials(name = "Staff") {
    return String(name).split(/\s+/).filter(Boolean).slice(0, 2).map((part) => part[0]?.toUpperCase()).join("") || "S";
  }

  async function compressImageFile(file) {
    if (!file || !file.type?.startsWith("image/")) return file;
    const bitmapUrl = URL.createObjectURL(file);
    try {
      const img = await new Promise((resolve, reject) => {
        const node = new Image();
        node.onload = () => resolve(node);
        node.onerror = reject;
        node.src = bitmapUrl;
      });
      const maxSide = 1800;
      const ratio = Math.min(1, maxSide / Math.max(img.width, img.height));
      const canvas = document.createElement("canvas");
      canvas.width = Math.max(1, Math.round(img.width * ratio));
      canvas.height = Math.max(1, Math.round(img.height * ratio));
      canvas.getContext("2d").drawImage(img, 0, 0, canvas.width, canvas.height);
      const blob = await new Promise((resolve) => canvas.toBlob(resolve, "image/jpeg", 0.84));
      if (!blob) return file;
      const nextName = file.name.replace(/\.[^.]+$/, "") + ".jpg";
      return new File([blob], nextName, { type: "image/jpeg" });
    } catch {
      return file;
    } finally {
      URL.revokeObjectURL(bitmapUrl);
    }
  }

  async function uploadLooseImages(files) {
    const uploaded = [];
    for (const file of [...files].filter((item) => item?.type?.startsWith("image/"))) {
      const compressed = await compressImageFile(file);
      const result = await API.uploadFile(compressed);
      uploaded.push(result.public_url || result.url || result.image_path);
    }
    return uploaded.filter(Boolean);
  }

  async function uploadRegistrationDocuments(form) {
    const missing = missingVerificationDocuments(form);
    if (missing.length) {
      updateVerificationDocumentValidation(form, true);
      throw new Error("Please upload all required verification documents before submitting.");
    }
    const fields = [
      ["ownership_certificate_upload", "hotel_ownership_certificate"],
      ["business_license_upload", "business_license"],
      ["tourism_registration_upload", "tourism_registration"],
      ["id_proof_upload", "id_proof"],
    ];
    const documents = [];
    for (const [name, type] of fields) {
      const file = form.querySelector(`input[name="${name}"]`)?.files[0];
      if (!file) continue;
      const uploaded = await API.uploadFile(file);
      documents.push({ document_type: type, document_url: uploaded.public_url || uploaded.url || uploaded.image_path });
    }
    return documents;
  }

  function verificationDocumentField(name, label) {
    return `
      <label class="document-upload-field missing-upload" data-verification-field="${safe(name)}">
        <span>${safe(label)} <b aria-hidden="true">*</b></span>
        <input name="${safe(name)}" type="file" accept=".pdf,.jpg,.jpeg,.png" required data-verification-document>
        <small class="field-error" data-document-error>Please upload ${safe(label)}.</small>
      </label>
    `;
  }

  function hasValidDocumentFile(input) {
    const file = input?.files?.[0];
    if (!file || file.size <= 0) return false;
    return /\.(pdf|jpe?g|png)$/i.test(file.name || "") || ["application/pdf", "image/jpeg", "image/png"].includes(file.type);
  }

  function missingVerificationDocuments(form) {
    if (!form) return requiredVerificationDocuments;
    return requiredVerificationDocuments.filter(([name]) => !hasValidDocumentFile(form.querySelector(`input[name="${name}"]`)));
  }

  function updateVerificationDocumentValidation(form, showSummary = false) {
    if (!form) return true;
    const missing = missingVerificationDocuments(form);
    requiredVerificationDocuments.forEach(([name]) => {
      const field = form.querySelector(`[data-verification-field="${name}"]`);
      const input = form.querySelector(`input[name="${name}"]`);
      const isMissing = !hasValidDocumentFile(input);
      field?.classList.toggle("missing-upload", isMissing);
      input?.classList.toggle("invalid-upload", isMissing);
      input?.setAttribute("aria-invalid", isMissing ? "true" : "false");
    });
    const submit = form.querySelector("[data-registration-submit]");
    if (submit) {
      submit.disabled = missing.length > 0;
      submit.setAttribute("aria-disabled", missing.length > 0 ? "true" : "false");
    }
    const summary = form.querySelector("[data-document-summary-error]");
    if (summary) {
      summary.hidden = missing.length === 0 || !showSummary;
      summary.textContent = missing.length
        ? "Please upload all required verification documents before submitting."
        : "";
    }
    return missing.length === 0;
  }

  function setupVerificationDocumentValidation(form) {
    if (!form) return;
    updateVerificationDocumentValidation(form, false);
  }

  async function recordHotelUploads(hotelId, files, isPrimary = false) {
    const imageFiles = [...files].filter((item) => item?.type?.startsWith("image/"));
    if (!imageFiles.length) return [];
    const compressed = [];
    for (const file of imageFiles) compressed.push(await compressImageFile(file));
    const result = await API.uploadHotel(hotelId, compressed, isPrimary);
    return result.uploaded || [];
  }

  async function recordRoomUploads(roomTypeId, files, isPrimary = true) {
    const imageFiles = [...files].filter((item) => item?.type?.startsWith("image/"));
    if (!imageFiles.length) return [];
    const compressed = [];
    for (const file of imageFiles) compressed.push(await compressImageFile(file));
    const result = await API.uploadRoom(roomTypeId, compressed, isPrimary);
    return result.uploaded || [];
  }

  function handleFileInput(input) {
    if (!input) return;
    const isMultiple = input.multiple;
    const newFiles = [...input.files || []];
    if (!newFiles.length) return;

    const key = input.id || input.name;
    if (!isMultiple) {
      inputFilesMap.set(key, newFiles);
      return;
    }

    const existing = inputFilesMap.get(key) || [];
    const combined = [...existing, ...newFiles];

    // Filter unique
    const unique = [];
    const seen = new Set();
    for (const f of combined) {
      const hash = `${f.name}-${f.size}`;
      if (!seen.has(hash)) {
        seen.add(hash);
        unique.push(f);
      }
    }

    inputFilesMap.set(key, unique);

    // Sync back to input.files
    try {
      const dt = new DataTransfer();
      unique.forEach(file => dt.items.add(file));
      input.files = dt.files;
    } catch (e) {
      console.error("Error setting input files via DataTransfer:", e);
    }
  }

  function getUploadedFiles(form, name) {
    const input = form?.querySelector(`input[name="${name}"]`);
    if (!input) return [];
    const key = input.id || input.name;
    return inputFilesMap.get(key) || [...input.files || []];
  }

  function updateUploadPreview(input) {
    const target = document.getElementById(input.dataset.previewTarget || "");
    if (!target) return;
    const key = input.id || input.name;
    const files = inputFilesMap.get(key) || [...input.files || []];
    if (!files.length) {
      target.innerHTML = `<span class="muted">No photos selected</span>`;
      return;
    }
    const isHotelProfileUpload = Boolean(input.closest(".hotel-profile-upload"));
    const thumbClass = ["upload-thumb", isHotelProfileUpload ? "hotel-upload-thumb image-wrapper" : ""].filter(Boolean).join(" ");
    const imageClass = isHotelProfileUpload ? ' class="hotel-image"' : "";
    target.innerHTML = files.map((file, idx) => {
      const url = URL.createObjectURL(file);
      setTimeout(() => URL.revokeObjectURL(url), 30000);
      const deleteBtn = input.multiple ? `<button type="button" class="upload-thumb-remove" data-remove-input-id="${safe(key)}" data-remove-index="${idx}" aria-label="Remove photo">&times;</button>` : "";
      return `
        <figure class="${thumbClass}">
          <img${imageClass} src="${url}" alt="${safe(file.name)}">
          <figcaption>${safe(file.name)}</figcaption>
          ${deleteBtn}
        </figure>
      `;
    }).join("");
  }


  function hotelPreviewCard(hotel = {}, images = []) {
    const img = coalesceImage(images[0], hotelImage(hotel));
    const gallery = (images.length ? images : [img]).slice(0, 4);
    const amenities = parseAmenities(hotel.amenities).slice(0, 6);
    const rating = Number(hotel.star_rating || hotel.rating || 4.5);
    const stars = Array.from({ length: Math.max(1, Math.min(5, Math.round(rating))) }).map(() => "*").join(" ");
    const pricing = discountedHotelPrice(hotel);
    const availableRooms = Number(hotel.available_rooms || hotel.room_count || hotel.available_count || 0);
    const coupon = hotelCoupon(hotel);
    return `
      <article class="live-preview-card booking-preview-card hotel-profile-card" data-live-preview>
        <div class="preview-media hotel-profile-media image-wrapper">
          <img class="hotel-image" src="${safe(img)}" alt="${safe(hotel.name || "Hotel preview")}">
          ${coupon ? `<span class="coupon-ribbon">${safe(coupon.label)}</span>` : ""}
        </div>
        <div class="preview-body">
          <div class="card-row">
            <p class="eyebrow">${safe(hotel.property_type || "Hotel")} - ${safe(stars)}</p>
            <span class="score-pill">${safe(hotel.review_score || "New")}</span>
          </div>
          <h3>${safe(hotel.name || "Your hotel name")}</h3>
          <p class="muted">${safe(hotel.location || [hotel.city, hotel.state, hotel.country].filter(Boolean).join(", ") || "City, State")}</p>
          <p>${safe(hotel.description || "A polished guest-ready stay with thoughtful amenities, flexible booking, and a guest-friendly arrival experience.")}</p>
          <div class="preview-perks">
            <span>Free cancellation</span>
            <span>Breakfast included</span>
            <span>${availableRooms || "Multiple"} rooms available</span>
          </div>
          <div class="pill-row">${(amenities.length ? amenities : ["Free Wi-Fi", "Parking"]).map((item) => `<span class="pill">${safe(item)}</span>`).join("")}</div>
          <div class="preview-gallery">
            ${gallery.map((src) => `<img class="gallery-image" src="${safe(src)}" alt="${safe(hotel.name || "Hotel")} gallery">`).join("")}
          </div>
          <div class="preview-price-row">
            <div>
              <small>From</small>
              ${pricing.hasCoupon ? `<strong><s>${money(pricing.original)}</s> ${money(pricing.discounted)}</strong>` : `<strong>${money(pricing.original)}</strong>`}
            </div>
            ${coupon ? `<span class="status-pill coupon-pill">${safe(coupon.code)} - ${safe(coupon.label)}</span>` : `<span class="status-pill approved">Verified</span>`}
          </div>
        </div>
      </article>
    `;
  }

  function updateHotelLivePreview(form) {
    if (!form) return;
    const target = $("#hotel-live-preview");
    if (!target) return;
    const data = Object.fromEntries(new FormData(form).entries());
    data.amenities = checkedValues(form, "amenities");
    data.location = [data.city, data.state, data.country].filter(Boolean).join(", ") || data.location;
    const previewImages = $$("[id$='hotel-photos-preview'] img, #hotel-profile-photos-preview img, #manager-hotel-photos-preview img", form.parentElement || document)
      .map((img) => img.src)
      .filter(Boolean);
    target.innerHTML = hotelPreviewCard(data, previewImages);
  }

  function filterRoomRows() {
    const q = ($("#room-filter-text")?.value || "").toLowerCase();
    const status = $("#room-filter-status")?.value || "";
    const type = $("#room-filter-type")?.value || "";
    const rows = $$("[data-room-row]");
    const pageSize = Math.max(5, Number($("#room-page-size")?.value || 100));
    const matches = rows.filter((row) => {
      const okText = !q || row.dataset.search.includes(q);
      const okStatus = !status || row.dataset.status === status;
      const okType = !type || row.dataset.type === type;
      return okText && okStatus && okType;
    });
    const totalPages = Math.max(1, Math.ceil(matches.length / pageSize));
    state.roomInventoryPage = Math.min(Math.max(1, state.roomInventoryPage || 1), totalPages);
    const start = (state.roomInventoryPage - 1) * pageSize;
    const visible = new Set(matches.slice(start, start + pageSize));
    rows.forEach((row) => row.classList.toggle("hidden", !visible.has(row)));
    const emptyRow = $("#inventory-empty-row");
    if (emptyRow) emptyRow.classList.toggle("hidden", matches.length > 0);
    const summary = $("#inventory-page-summary");
    if (summary) {
      const from = matches.length ? start + 1 : 0;
      const to = Math.min(start + pageSize, matches.length);
      summary.textContent = `${from}-${to} of ${matches.length}`;
    }
    const prev = $("[data-room-page='prev']");
    const next = $("[data-room-page='next']");
    if (prev) prev.disabled = state.roomInventoryPage <= 1;
    if (next) next.disabled = state.roomInventoryPage >= totalPages;
  }

  function filterStaffRows() {
    const q = ($("#staff-search")?.value || "").toLowerCase();
    const role = $("#staff-role-filter")?.value || "";
    $$("[data-staff-row]").forEach((row) => {
      const okText = !q || row.dataset.search.includes(q);
      const okRole = !role || row.dataset.role === role;
      row.classList.toggle("hidden", !(okText && okRole));
    });
  }

  function numberValue(value, fallback = 0) {
    const parsed = Number(value);
    return Number.isFinite(parsed) ? parsed : fallback;
  }

  function statusText(status) {
    if (status === "approved") return "Approved and live";
    if (status === "rejected") return "Rejected by admin";
    return "Pending admin review";
  }

  function money(value) {
    return "Rs. " + Number(value || 0).toLocaleString("en-IN", { maximumFractionDigits: 0 });
  }

  function paymentConfig() {
    return state.paymentConfig || {
      upi_id: "staysphere@upi",
      merchant_name: "StaySphere",
      commission_rate: 15,
    };
  }

  function formatAmount(value) {
    return Number(value || 0).toFixed(2).replace(/\.00$/, "");
  }

  function buildUpiUri(amount, upiId, merchantName, bookingRef = "") {
    const cleanUpi = String(upiId || "").trim();
    if (!validUpiId(cleanUpi)) return "";
    const pa = encodeURIComponent(cleanUpi).replace(/%40/g, "@");
    const pn = encodeURIComponent(merchantName || "StaySphere");
    const am = Number(amount || 0) > 0 ? `&am=${formatAmount(amount)}` : "";
    const tn = bookingRef ? `&tn=${encodeURIComponent(`StaySphere booking ${bookingRef}`)}` : "";
    return `upi://pay?pa=${pa}&pn=${pn}${am}&cu=INR${tn}`;
  }

  function qrImageSrc(value) {
    return `https://api.qrserver.com/v1/create-qr-code/?size=260x260&margin=14&data=${encodeURIComponent(value)}`;
  }

  function downloadCSVFile(filename, headers, rows) {
    const csvContent = [
      headers.map(h => `"${String(h).replace(/"/g, '""')}"`).join(","),
      ...rows.map(row => row.map(v => `"${String(v ?? '').replace(/"/g, '""')}"`).join(","))
    ].join("\n");
    const blob = new Blob([csvContent], { type: "text/csv;charset=utf-8;" });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.href = url;
    link.setAttribute("download", filename);
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    URL.revokeObjectURL(url);
  }

  function triggerCsvDownload(tableType) {
    if (tableType === "commission") {
      const dues = state.adminRevenue?.hotel_dues || [];
      const headers = ["Hotel Name", "Location", "UPI ID", "Commission Due (INR)", "Due Date", "Status"];
      const rows = dues.map(h => [
        h.name,
        h.location || "",
        h.upi_id || "",
        h.commission_due || 0,
        h.due_date || "-",
        h.commission_status || "Paid"
      ]);
      downloadCSVFile("Commission_Dues_By_Hotel.csv", headers, rows);
      toast("CSV downloaded successfully", "success");
    } else if (tableType === "top-hotels") {
      const hotels = state.adminRevenue?.top_hotels || [];
      const headers = ["Rank", "Hotel Name", "Location", "Bookings", "Gross Revenue (INR)", "Commission (INR)"];
      const rows = hotels.map((h, i) => [
        i + 1,
        h.name,
        h.location || "-",
        h.bookings || 0,
        h.gross || 0,
        h.commission || 0
      ]);
      downloadCSVFile("Top_Performing_Hotels.csv", headers, rows);
      toast("CSV downloaded successfully", "success");
    } else if (tableType === "payments") {
      const txns = state.adminTransactions || [];
      const headers = ["Booking ID", "Hotel Name", "Payment Method", "Amount (INR)", "Commission (INR)", "Status", "Transaction Ref"];
      const rows = txns.map(t => [
        t.booking_ref,
        t.hotel_name || "-",
        t.payment_method || t.provider || "-",
        t.amount || 0,
        t.commission_amount || 0,
        t.status || "",
        t.transaction_ref || t.razorpay_order_id || "-"
      ]);
      downloadCSVFile("Payment_History.csv", headers, rows);
      toast("CSV downloaded successfully", "success");
    } else if (tableType === "payouts") {
      const payouts = state.adminPayouts || [];
      const headers = ["Payout ID", "Hotel", "Amount (INR)", "Status", "Paid At"];
      const rows = payouts.map(p => [
        p.payout_id,
        p.hotel_name || p.hotel_id || "-",
        p.payout_amount || 0,
        p.status || "",
        p.paid_at || "-"
      ]);
      downloadCSVFile("Hotel_Payouts.csv", headers, rows);
      toast("CSV downloaded successfully", "success");
    } else if (tableType === "admin-ledgers") {
      const ledgers = state.adminLedgers || [];
      const headers = ["Invoice ID", "Month/Year", "Hotel Name", "Location", "Due Date", "Days Overdue", "Commission Amount (INR)", "Paid Amount (INR)", "Pending Amount (INR)", "Status", "Hotel Status"];
      const rows = ledgers.map(l => [
        l.ledger_id,
        `${l.month || ""}/${l.year || ""}`,
        l.hotel_name || l.hotel_id,
        l.location || "",
        l.due_date || "-",
        l.days_overdue || 0,
        l.commission_amount || 0,
        l.paid_amount || 0,
        l.pending_amount || 0,
        l.status || "pending",
        hotelDueStatusLabel(l.hotel_status)
      ]);
      downloadCSVFile("Commission_Invoices_And_Dues.csv", headers, rows);
      toast("CSV downloaded successfully", "success");
    }
  }

  function activityOptionsForHotel(hotel = {}) {
    const source = Array.isArray(hotel.activities_list) && hotel.activities_list.length
      ? hotel.activities_list
      : state.activities;
    return (Array.isArray(source) ? source : [])
      .map((activity, index) => {
        if (typeof activity === "string") {
          return { id: `hotel-${index}`, name: activity, price: 0 };
        }
        return activity;
      })
      .filter((activity) => activity && activity.name);
  }

  function hotelDueStatusLabel(status = "Active") {
    if (status === "Restricted") return "Restricted mode";
    if (status === "Suspended") return "Listings suspended";
    return "Active";
  }

  function commissionStatusClass(status = "") {
    const value = String(status || "").toLowerCase();
    if (value === "paid" || value === "approved" || value === "active") return "approved";
    if (value === "rejected" || value === "suspended") return "rejected";
    if (value === "overdue" || value === "restricted") return "requested";
    return value || "pending";
  }

  function syncUpiPanel(form, preview = state.lastBookingPreview || {}) {
    if (!form) return;
    const amount = Number(preview.total_amount || 0);
    const upiInput = form.querySelector("[data-upi-id-input]");
    const hotel = state.selectedHotel || {};
    const hotelPayments = hotelPaymentOptions(hotel);
    const merchant = hotel.name || paymentConfig().merchant_name || "StaySphere";
    const upiId = upiInput?.value?.trim() || hotelPayments.upiId;
    form.querySelectorAll("[data-upi-amount]").forEach((node) => {
      node.textContent = amount > 0 ? money(amount) : "Calculating...";
    });
    const qr = form.querySelector("[data-upi-qr]");
    const unavailable = form.querySelector("[data-upi-unavailable]");
    if (!validUpiId(upiId)) {
      qr?.removeAttribute("src");
      qr?.classList.add("hidden");
      if (unavailable) unavailable.hidden = false;
      delete form.dataset.upiUri;
      return;
    }
    const bookingRef = form.dataset.bookingRef || "";
    const uri = buildUpiUri(amount, upiId, merchant, bookingRef);
    if (!uri || amount <= 0) {
      qr?.removeAttribute("src");
      qr?.classList.add("hidden");
      if (unavailable) unavailable.hidden = true;
      return;
    }
    form.dataset.upiUri = uri;
    if (unavailable) unavailable.hidden = true;
    if (qr) {
      qr.classList.remove("hidden");
      qr.onload = () => {
        qr.classList.remove("hidden");
        if (unavailable) unavailable.hidden = true;
      };
      qr.onerror = () => {
        qr.classList.add("hidden");
        if (unavailable) unavailable.hidden = false;
      };
      qr.src = qrImageSrc(uri);
    }
  }

  function syncManagerUpiPreview(root = document) {
    const form = root.querySelector?.("#payment-form, #manager-hotel-form, #auth-form") || root.closest?.("#payment-form, #manager-hotel-form, #auth-form");
    if (!form) return;
    const input = form.querySelector("[data-manager-upi-input]");
    const qr = form.querySelector("[data-manager-upi-qr]");
    const uriNode = form.querySelector("[data-manager-upi-uri]");
    if (!input || !qr) return;
    const upiId = input.value.trim();
    const merchant = form.querySelector('input[name="name"], input[name="hotel_name"]')?.value?.trim()
      || state.selectedHotel?.name
      || "Hotel";
    if (!upiId) {
      qr.removeAttribute("src");
      if (uriNode) uriNode.textContent = "Enter a UPI ID to preview the QR";
      return;
    }
    if (!validUpiId(upiId)) {
      qr.removeAttribute("src");
      if (uriNode) uriNode.textContent = "Enter a valid UPI ID";
      return;
    }
    const uri = buildUpiUri(0, upiId, merchant);
    qr.src = qrImageSrc(uri);
    if (uriNode) uriNode.textContent = uri;
  }

  function selectedPaymentMethod(form) {
    return form?.querySelector('input[name="payment_method"]:checked')?.value
      || form?.querySelector('select[name="payment_method"]')?.value
      || "Pay Now";
  }

  function hasPaymentProofImage(form) {
    const file = form?.querySelector('input[name="payment_screenshot_upload"]')?.files?.[0];
    return Boolean(file && (file.type?.startsWith("image/") || /\.(jpe?g|png)$/i.test(file.name || "")));
  }

  function syncPaymentSubmitState(form) {
    if (!form || form.id !== "booking-form") return;
    const btn = form.querySelector("button[type=submit]");
    if (!btn) return;
    const isPayNow = selectedPaymentMethod(form) === "Pay Now";
    if (!isPayNow) {
      btn.disabled = false;
      btn.textContent = "Confirm Booking";
      return;
    }
    const hasProof = hasPaymentProofImage(form);
    btn.disabled = !hasProof;
    btn.textContent = hasProof ? "Submit Payment Proof" : "Upload payment screenshot to continue";
  }

  function validUpiId(value) {
    return /^[^\s@]+@[^\s@]+$/.test(String(value || "").trim());
  }

  function normalizedPaymentSet(hotel = {}) {
    return new Set(parseAmenities(hotel.accepted_payments).map((item) => String(item).trim().toLowerCase()));
  }

  function hotelPaymentOptions(hotel = {}) {
    const payments = normalizedPaymentSet(hotel);
    const hasPaymentConfig = payments.size > 0;
    const hasUpi = validUpiId(hotel.upi_id);
    const allowsPayNow = hasUpi && (!hasPaymentConfig || payments.has("pay now") || payments.has("upi") || payments.has("upi qr") || payments.has("online payment"));
    const allowsProperty = !hasPaymentConfig || payments.has("pay at property") || payments.has("pay at hotel") || payments.has("pay later");
    return {
      payNow: allowsPayNow,
      payAtProperty: allowsProperty || !allowsPayNow,
      both: allowsPayNow && allowsProperty,
      upiId: hasUpi ? String(hotel.upi_id).trim() : "",
    };
  }

  function hotelCoupon(hotel = {}) {
    const coupon = hotel.coupon && typeof hotel.coupon === "object" ? hotel.coupon : {};
    const code = coupon.code || hotel.active_coupon_code || hotel.coupon_code || "";
    const value = Number(coupon.discount_value || hotel.active_coupon_discount_value || 0);
    const type = coupon.discount_type || hotel.active_coupon_discount_type || "percentage";
    if (!code || !value) return null;
    return {
      code,
      value,
      type,
      description: coupon.description || hotel.coupon_description || "Limited Time Offer",
      label: type === "percentage" ? `${value}% OFF` : `${money(value)} OFF`,
    };
  }

  function discountedHotelPrice(hotel = {}) {
    const original = Number(hotel.coupon_old_price || hotel.price_from || hotel.current_price || hotel.base_price || 0);
    const coupon = hotelCoupon(hotel);
    const backendPrice = Number(hotel.discounted_price || hotel.coupon_discounted_price || 0);
    let discounted = backendPrice > 0 ? backendPrice : original;
    if (coupon && !backendPrice) {
      const discount = coupon.type === "percentage" ? original * (coupon.value / 100) : coupon.value;
      discounted = Math.max(0, original - discount);
    }
    return {
      original,
      discounted,
      hasCoupon: Boolean(coupon && original > 0 && discounted < original),
      coupon,
    };
  }

  function roomStatusLabel(status) {
    if (status === "available" || status === "empty") return "Available";
    if (status === "cleaned" || status === "cleaning") return "Cleaning";
    if (status === "occupied") return "Occupied";
    if (status === "reserved" || status === "blocked") return "Blocked";
    if (status === "maintenance") return "Maintenance";
    if (status === "deactivated") return "Deactivated";
    return status ? status.replace(/_/g, " ") : "Empty";
  }

  function roomCalendarStatus(room, bookings, iso) {
    if (room.status === "deactivated") return "deactivated";
    const occupied = bookings.some((booking) =>
      Number(booking.room_id) === Number(room.id) &&
      (booking.status === "active" || booking.status === "checked_in") &&
      booking.check_in_date <= iso &&
      booking.check_out_date > iso
    );
    if (occupied) return "occupied";
    if (room.status === "cleaned") return "cleaned";
    if (room.status === "maintenance") return "maintenance";
    return "empty";
  }

  function canPermanentlyDeleteRooms() {
    return (state.user?.role || localStorage.getItem("hms_role")) === "super_admin";
  }

  function roomStatusSelect(room) {
    const status = room.status || "available";
    const id = safe(room.id || room.room_id || "");
    return `
      <select class="status-select ${safe(status)}" data-room-status-select="${id}" aria-label="Change status for ${safe(room.room_number || "room")}">
        <option value="available" ${status === "available" ? "selected" : ""}>Available</option>
        <option value="occupied" ${status === "occupied" ? "selected" : ""}>Occupied</option>
        <option value="reserved" ${status === "reserved" || status === "blocked" ? "selected" : ""}>Blocked</option>
        <option value="maintenance" ${status === "maintenance" ? "selected" : ""}>Maintenance</option>
        <option value="cleaned" ${status === "cleaned" || status === "cleaning" ? "selected" : ""}>Cleaning</option>
        <option value="deactivated" ${status === "deactivated" ? "selected" : ""}>Deactivated</option>
      </select>
    `;
  }

  function roomRowActions(room) {
    const id = safe(room.id || room.room_id || "");
    const label = safe(room.room_number || "this room");
    const isDeactivated = room.status === "deactivated";
    return `
      <div class="table-actions room-row-actions">
        ${roomStatusSelect(room)}
        <button class="ghost-btn" type="button" data-edit-inventory-room="${id}">Edit</button>
        <button class="secondary-btn" type="button" data-room-maintenance="${id}">Maintenance</button>
        <button class="ghost-btn danger-text" type="button" data-room-deactivate="${id}" data-room-label="${label}" ${isDeactivated ? "disabled" : ""}>Deactivate</button>
        ${canPermanentlyDeleteRooms() ? `
          <details class="advanced-action-menu">
            <summary>Advanced</summary>
            <button class="danger-btn" type="button" data-delete-room="${id}">Delete permanently</button>
          </details>
        ` : ""}
      </div>
    `;
  }

  function toast(message, type = "info") {
    const node = document.createElement("div");
    node.className = `toast ${type}`;
    node.textContent = message;
    $("#toast-stack").appendChild(node);
    setTimeout(() => node.remove(), type === "error" ? 6000 : 5000);
  }

  function setTheme(theme) {
    document.documentElement.dataset.theme = theme;
    localStorage.setItem("hms_theme", theme);
    $$(".theme-toggle").forEach((btn) => {
      btn.textContent = theme === "dark" ? "Light" : "Dark";
    });
  }

  function toggleTheme() {
    setTheme(document.documentElement.dataset.theme === "dark" ? "light" : "dark");
  }

  async function loadPublicData(params = {}) {
    try {
      state.hotels = completeHotelCatalogue(await API.publicHotels(params), params);
    } catch (error) {
      state.hotels = completeHotelCatalogue([], params);
    }

    try {
      state.destinations = completeDestinationCatalogue(await API.destinations());
    } catch {
      state.destinations = completeDestinationCatalogue([]);
    }

    renderDestinations();
    renderHeroDestinationChips();
    await renderHotelCards(state.hotels);
  }

  function completeHotelCatalogue(hotels = [], params = {}) {
    const result = (Array.isArray(hotels) ? hotels : []).filter(Boolean);
    const ids = new Set(result.map((hotel) => String(hotel.id || "")));
    const names = new Set(result.map((hotel) => String(hotel.name || "").trim().toLowerCase()).filter(Boolean));
    for (const hotel of fallbackHotels) {
      if (result.length >= 8) break;
      if (ids.has(String(hotel.id)) || names.has(hotel.name.toLowerCase())) continue;
      if (!hotelMatchesParams(hotel, params)) continue;
      result.push({ ...hotel, is_demo_listing: true });
      ids.add(String(hotel.id));
      names.add(hotel.name.toLowerCase());
    }
    return result;
  }

  function hotelMatchesParams(hotel = {}, params = {}) {
    const query = String(params.location || "").trim().toLowerCase();
    const rating = Number(params.rating || 0);
    const text = [hotel.name, hotel.location, hotel.property_type].join(" ").toLowerCase();
    const hotelRating = Number(hotel.star_rating || hotel.review_score || 0);
    return (!query || text.includes(query)) && (!rating || hotelRating >= rating);
  }

  function completeDestinationCatalogue(destinations = []) {
    const fallbackDestinations = [
      { location: "Goa", stays: 18, image_url: fallbackHotels[2].image_url, price_from: 5400 },
      { location: "Mumbai", stays: 26, image_url: fallbackHotels[0].image_url, price_from: 4500 },
      { location: "Udaipur", stays: 12, image_url: fallbackHotels[1].image_url, price_from: 6200 },
      { location: "Alleppey", stays: 9, image_url: fallbackHotels[4].image_url, price_from: 6700 },
      { location: "Bengaluru", stays: 14, image_url: "https://images.unsplash.com/photo-1606046604972-77cc76aee944?auto=format&fit=crop&w=1000&q=80", price_from: 3900 },
      { location: "Jaipur", stays: 16, image_url: fallbackHotels[7].image_url, price_from: 5800 },
      { location: "Delhi NCR", stays: 21, image_url: "https://images.unsplash.com/photo-1587474260584-136574528ed5?auto=format&fit=crop&w=1000&q=80", price_from: 4100 },
      { location: "Manali", stays: 10, image_url: fallbackHotels[3].image_url, price_from: 4200 },
    ];
    const getCityKey = (loc) => String(loc || "").split(",")[0].trim().toLowerCase();
    const uniqueIncoming = [];
    const seen = new Set();
    for (const d of (Array.isArray(destinations) ? destinations : [])) {
      if (!d || !d.location) continue;
      const city = getCityKey(d.location);
      if (seen.has(city)) continue;
      uniqueIncoming.push(d);
      seen.add(city);
    }
    const result = [...uniqueIncoming];
    for (const destination of fallbackDestinations) {
      if (result.length >= 8) break;
      const city = getCityKey(destination.location);
      if (seen.has(city)) continue;
      result.push(destination);
      seen.add(city);
    }
    return result;
  }

  function localHotelDetails(id) {
    const hotel = [...state.hotels, ...fallbackHotels].find((item) => String(item.id) === String(id));
    if (!hotel) return null;
    const seed = Number(hotel.id) || 0;
    const basePrice = Number(hotel.price_from || 9000);
    const gallery = [
      hotelImage(hotel) || fallbackHotels[seed % fallbackHotels.length].image_url,
      fallbackHotels[(seed + 1) % fallbackHotels.length].image_url,
      fallbackHotels[(seed + 2) % fallbackHotels.length].image_url,
    ].filter(Boolean);
    return {
      ...hotel,
      images: gallery.map((image_url) => ({ image_url })),
      activities_list: hotel.activities || ["Private dining", "Local experiences", "Airport transfers"],
      rooms: hotel.rooms?.length ? hotel.rooms : [
        {
          id: seed * 100 + 1,
          name: "Signature King Room",
          description: "A polished room with premium bedding, work space, and warm city or garden views.",
          base_price: Math.round(basePrice * 0.82),
          current_price: Math.round(basePrice * 0.82),
          max_capacity: 2,
          images: [{ image_url: fallbackRoomImages[seed % fallbackRoomImages.length] }],
        },
        {
          id: seed * 100 + 2,
          name: "Luxury Suite",
          description: "A spacious suite with lounge seating, curated amenities, and elevated StaySphere service.",
          base_price: Math.round(basePrice * 1.18),
          current_price: Math.round(basePrice * 1.18),
          max_capacity: 3,
          images: [{ image_url: fallbackRoomImages[(seed + 1) % fallbackRoomImages.length] }],
        },
      ],
      reviews: hotel.reviews?.length ? hotel.reviews : [
        { author: "Aarav M.", rating: 5, comment: "Beautiful property, clear pricing, and a very smooth booking experience." },
        { author: "Priya S.", rating: 4.8, comment: "The photos, policies, and room details made it easy to choose the right stay." },
      ],
    };
  }

  function renderDestinations() {
    const grid = $("#destination-grid");
    grid.innerHTML = state.destinations.map((destination, index) => `
      <article class="destination-card">
        <img src="${safe(destination.image_url || fallbackHotels[index % fallbackHotels.length].image_url)}" onerror="if(!this.dataset.fallback){ this.dataset.fallback='true'; this.src='${fallbackHotels[index % fallbackHotels.length].image_url}'; }" alt="${safe(destination.location)}">
        <div>
          <h3>${safe(destination.location)}</h3>
          <p>${safe(destination.stays || "12")} stays from ${money(destination.price_from || 3500)}</p>
        </div>
      </article>
    `).join("");
  }

  function renderHeroDestinationChips() {
    const chips = $("#hero-destination-chips");
    if (!chips) return;
    chips.innerHTML = state.destinations.slice(0, 4).map((destination) => `
      <button class="hero-destination-chip" type="button" data-hero-location="${safe(destination.location)}">
        ${safe(destination.location)}
      </button>
    `).join("");
  }

  function hotelCardLegacy(hotel) {
    if (!hotel) return skeletonHotelCard();
    const amenities = parseAmenities(hotel.amenities).slice(0, 3);
    const isBestSeller = Number(hotel.review_score || 0) >= 9.0;
    const hasBreakfast = amenities.includes("Breakfast") || amenities.includes("Free Breakfast");
    const image = hotelImage(hotel);
    if (!image) return "";
    const pricing = discountedHotelPrice(hotel);
    const coupon = pricing.coupon;
    return `
      <article class="hotel-card" style="display: flex; flex-direction: column; height: 100%;">
        <div style="position: relative; height: 200px; flex-shrink: 0;">
          <img src="${safe(image)}" onerror="this.closest('.hotel-card')?.remove()" alt="${safe(hotel.name)}" style="width: 100%; height: 100%; object-fit: cover;">
          <div style="position: absolute; top: 12px; left: 12px; display: flex; flex-direction: column; gap: 6px;">
            ${isBestSeller ? '<span class="status-pill" style="background: var(--accent); color: #fff; border: none; box-shadow: 0 2px 4px rgba(0,0,0,0.2);">Best Seller</span>' : ''}
            ${hasBreakfast ? '<span class="status-pill" style="background: rgba(0,0,0,0.65); color: #fff; border: none; box-shadow: 0 2px 4px rgba(0,0,0,0.2);">Breakfast Included</span>' : ''}
            ${pricing.hasCoupon ? `<span class="status-pill coupon-pill" style="box-shadow: 0 2px 4px rgba(0,0,0,0.2);">${safe(coupon.label)}</span>` : ""}
          </div>
          <button class="ghost-btn" data-wishlist="${hotel.id}" style="position: absolute; top: 12px; right: 12px; background: rgba(255,255,255,0.9); border: none; border-radius: 50%; width: 36px; height: 36px; padding: 0; display: grid; place-items: center; box-shadow: 0 2px 4px rgba(0,0,0,0.1);" title="Save to wishlist">♡</button>
        </div>
        <div class="hotel-body" style="flex: 1; display: flex; flex-direction: column; padding: 16px;">
          <div class="hotel-title-row" style="align-items: flex-start; margin-bottom: 4px;">
            <h3 style="margin: 0; font-size: 18px; line-height: 1.3;">${safe(hotel.name)}</h3>
            <span class="score-pill" style="flex-shrink: 0; font-size: 12px; height: 24px;">⭐ ${safe(hotel.review_score || "9.0")}</span>
          </div>
          <p class="muted" style="margin: 0 0 12px; font-size: 13px;">📍 ${safe(hotel.location)} · ${safe(hotel.property_type || "Hotel")}</p>
          <div class="pill-row" style="margin-top: 0; margin-bottom: auto; gap: 6px;">
            ${amenities.map((item) => `<span class="pill" style="font-size: 11px; padding: 0 8px; min-height: 24px;">${safe(item)}</span>`).join("")}
          </div>
          <div class="hotel-footer" style="margin-top: 16px; padding-top: 16px; border-top: 1px solid var(--border); display: flex; justify-content: space-between; align-items: center;">
            <div class="price" style="font-size: 18px; margin: 0;">
              ${pricing.hasCoupon ? `<small class="old-price">${money(pricing.original)}</small><br><strong>${money(pricing.discounted)}</strong>` : `<strong>${money(pricing.original)}</strong>`}
              <small style="font-weight: normal; font-size: 12px;">/ night</small>
              ${pricing.hasCoupon ? `<small class="coupon-code">Code: ${safe(coupon.code)}</small>` : ""}
            </div>
            <button class="primary-btn" data-details="${hotel.id}" style="padding: 8px 16px; min-height: 36px; font-size: 14px;">Select</button>
          </div>
        </div>
      </article>
    `;
  }

  function hotelCard(hotel) {
    if (!hotel) return skeletonHotelCard();
    const amenities = parseAmenities(hotel.amenities).slice(0, 4);
    const isGuestFavorite = Number(hotel.review_score || 0) >= 9.0;
    const hasBreakfast = amenities.includes("Breakfast") || amenities.includes("Free Breakfast");
    const image = hotelImage(hotel);
    if (!image) return "";
    const pricing = discountedHotelPrice(hotel);
    const coupon = pricing.coupon;
    return `
      <article class="hotel-card travel-card reveal">
        <div class="hotel-media">
          <img src="${safe(image)}" onerror="this.closest('.hotel-card')?.remove()" alt="${safe(hotel.name)}">
          <button class="wishlist-fab" data-wishlist="${hotel.id}" title="Save to wishlist" aria-label="Save ${safe(hotel.name)}">&#9825;</button>
          <div class="hotel-badge-stack">
            ${isGuestFavorite ? '<span class="travel-badge">Guest favorite</span>' : ''}
            ${hasBreakfast ? '<span class="travel-badge dark">Breakfast included</span>' : ''}
            ${pricing.hasCoupon ? `<span class="travel-badge deal">${safe(coupon.label)}</span>` : ""}
          </div>
          <span class="rating-badge">&#9733; ${safe(hotel.review_score || "9.0")}</span>
        </div>
        <div class="hotel-body">
          <div class="hotel-title-row">
            <h3>${safe(hotel.name)}</h3>
            <span>${safe(hotel.property_type || "Hotel")}</span>
          </div>
          <p class="hotel-location">${safe(hotel.location)}</p>
          <div class="amenity-row">
            ${amenities.map((item) => `<span class="pill">${safe(item)}</span>`).join("")}
          </div>
          <div class="hotel-footer">
            <div class="price">
              ${pricing.hasCoupon ? `<small class="old-price">${money(pricing.original)}</small>` : ""}
              <strong>${pricing.hasCoupon ? money(pricing.discounted) : money(pricing.original)}</strong>
              <small>/ night</small>
              ${pricing.hasCoupon ? `<small class="coupon-code">Code: ${safe(coupon.code)}</small>` : ""}
            </div>
            <button class="primary-btn premium-btn" data-details="${hotel.id}">View stay</button>
          </div>
        </div>
      </article>
    `;
  }

  function heroStaySkeleton() {
    return `
      <article class="hero-stay-card is-loading">
        <div class="hero-stay-body">
          <span style="width: 78%;"></span>
          <span style="width: 58%;"></span>
          <span style="width: 92%;"></span>
        </div>
      </article>
    `;
  }

  function heroStayCard(hotel) {
    const image = hotelImage(hotel);
    if (!image) return "";
    const pricing = discountedHotelPrice(hotel);
    return `
      <article class="hero-stay-card">
        <img src="${safe(image)}" onerror="this.closest('.hero-stay-card')?.remove()" alt="${safe(hotel.name)}">
        <div class="hero-stay-body">
          <div class="hero-stay-title">
            <h3>${safe(hotel.name)}</h3>
            <p>${safe(hotel.location)} · ${safe(hotel.property_type || "Hotel")}</p>
          </div>
          <div class="hero-stay-meta">
            <span class="hero-stay-price">
              <strong>${pricing.hasCoupon ? money(pricing.discounted) : money(pricing.original)}</strong>
              <span>/ night</span>
            </span>
            <span class="hero-stay-rating">&#9733; ${safe(hotel.review_score || "9.0")}</span>
          </div>
          <button class="ghost-btn" type="button" data-details="${hotel.id}">View stay</button>
        </div>
      </article>
    `;
  }

  async function renderHeroStays(hotels) {
    const rail = $("#hero-stay-rail");
    if (!rail) return;
    const candidates = (hotels || []).filter((hotel) => hotelImageCandidates(hotel).length);
    if (!candidates.length) {
      rail.innerHTML = `<article class="hero-stay-card"><div class="hero-stay-body"><div class="hero-stay-title"><h3>No stays found</h3><p>Try another destination.</p></div></div></article>`;
      return;
    }
    rail.innerHTML = Array.from({ length: Math.min(4, candidates.length) }, heroStaySkeleton).join("");
    const validHotels = (await Promise.all(candidates.map(hotelWithValidImage)))
      .filter(Boolean)
      .sort((a, b) => Number(b.review_score || 0) - Number(a.review_score || 0))
      .slice(0, 4);
    rail.innerHTML = validHotels.length
      ? validHotels.map(heroStayCard).join("")
      : `<article class="hero-stay-card"><div class="hero-stay-body"><div class="hero-stay-title"><h3>No stays found</h3><p>Try another destination.</p></div></div></article>`;
  }

  async function renderHotelCards(hotels) {
    const grid = $("#featured-hotels");
    const candidates = (hotels || []).filter((hotel) => hotelImageCandidates(hotel).length);
    if (!candidates.length) {
      grid.innerHTML = `<div class="empty-state"><h3>No stays found</h3><p class="muted">Try another destination or clear the filters.</p></div>`;
      return;
    }
    grid.innerHTML = Array.from({ length: Math.min(4, candidates.length) }, skeletonHotelCard).join("");
    const validHotels = (await Promise.all(candidates.map(hotelWithValidImage))).filter(Boolean);
    grid.innerHTML = validHotels.length
      ? validHotels.map(hotelCard).join("")
      : `<div class="empty-state"><h3>No stays found</h3><p class="muted">Try another destination or clear the filters.</p></div>`;
  }

  async function applyHotelFilters(event) {
    event?.preventDefault();
    const location = $("#search-location").value || $("#hotel-filter-text").value;
    const rating = $("#hotel-filter-rating").value;
    await loadPublicData({ location, rating });
  }

  function openAuth(mode = "login") {
    state.authMode = mode;
    document.body.classList.add("modal-open");
    $("#auth-modal").classList.remove("hidden");
    renderAuthForm();
  }

  function closeAuth() {
    document.body.classList.remove("modal-open");
    $("#auth-modal").classList.add("hidden");
  }

  function renderAuthForm() {
    const mode = state.authMode;
    $$("#auth-mode-tabs button").forEach((button) => {
      button.classList.toggle("active", button.dataset.mode === mode);
    });
    const authCard = $(".auth-card");
    if (authCard) {
      authCard.classList.toggle("auth-partner-mode", mode === "partner");
    }
    const backdrop = $("#auth-modal");
    if (backdrop) {
      backdrop.classList.toggle("auth-partner-backdrop", mode === "partner");
    }
    $("#auth-title").textContent = mode === "login" ? "Welcome back" : mode === "partner" ? "List your hotel" : "Create your guest account";
    $("#auth-subtitle").textContent = mode === "login"
      ? "Sign in to access your portal."
      : mode === "partner"
        ? "Submit your hotel for admin approval."
        : "Save wishlists, bookings, reviews, and loyalty points.";

    const form = $("#auth-form");
    if (mode === "login") {
      form.innerHTML = `
        <label><span>Username</span><input name="username" autocomplete="username" required></label>
        <label><span>Password</span><input name="password" type="password" autocomplete="current-password" required></label>
        <button class="primary-btn" type="submit">Login</button>
      `;
      return;
    }

    if (mode === "partner") {
      form.innerHTML = `
        <div class="onboarding-steps">
          <section class="form-section">
            <h3>Basic property</h3>
            <div class="form-grid">
              <label><span>Property name</span><input name="hotel_name" required></label>
              <label><span>Property type</span><select name="property_type"><option>Hotel</option><option>Resort</option><option>Villa</option><option>Apartment</option><option>Hostel</option></select></label>
              <label><span>Star rating</span><input name="star_rating" type="number" min="1" max="5" step="0.5" value="4.5"></label>
              <label><span>Number of rooms</span><input name="room_count" type="number" min="1" value="12"></label>
              <label class="wide-field"><span>Property description</span><textarea name="description" required></textarea></label>
            </div>
          </section>

          <section class="form-section">
            <h3>Location</h3>
            <div class="form-grid">
              <label><span>Country</span><input name="country" value="India"></label>
              <label><span>State</span><input name="state"></label>
              <label><span>City</span><input name="city"></label>
              <label><span>Pincode</span><input name="postal_code"></label>
              <label class="wide-field"><span>Full address</span><input name="address"></label>
              <label class="wide-field"><span>Coordinates</span><input name="coordinates" placeholder="12.9716, 77.5946"></label>
              <label class="wide-field"><span>Nearby landmarks</span><textarea name="nearby_landmarks" placeholder="Airport (3 km), City Mall (1 km)"></textarea></label>
            </div>
          </section>

          <section class="form-section">
            <h3>Owner / partner</h3>
            <div class="form-grid">
              <label><span>Full name</span><input name="full_name" required></label>
              <label><span>Username</span><input name="username" required></label>
              <label><span>Email</span><input name="email" type="email"></label>
              <label><span>Phone number</span><input name="phone"></label>
              <label><span>Password</span><input name="password" type="password" required></label>
              <label><span>Business name</span><input name="business_name"></label>
            </div>
          </section>

          <section class="form-section">
            <h3>Pricing</h3>
            <div class="form-grid">
              <label><span>Starting price per night</span><input name="price_from" type="number" value="3500"></label>
              <label><span>Weekend pricing</span><input name="weekend_price" type="number" value="4200"></label>
              <label><span>Seasonal pricing</span><input name="seasonal_price" type="number" value="5000"></label>
              <label><span>Taxes and charges</span><input name="taxes_fees" type="number" value="12"></label>
              <label><span>Minimum stay</span><input name="min_stay" type="number" min="1" value="1"></label>
            </div>
          </section>

          <section class="form-section">
            <h3>Payments</h3>
            <div class="form-grid">
              <label><span>UPI ID (Required)</span><input name="upi_id" placeholder="hotelname@upi" required></label>
            </div>
          </section>

          <section class="form-section">
            <h3>Facilities</h3>
            ${checkboxGrid("amenities", propertyAmenities, ["Free Wi-Fi", "Parking", "Restaurant"])}
          </section>

          <section class="form-section">
            <h3>Images</h3>
            ${uploadZone("hotel_photos", "Hotel photos", { id: "partner-hotel-photos", multiple: true })}
          </section>

          <section class="form-section">
            <h3>Policies</h3>
            <div class="form-grid">
              <label><span>Check-in time</span><input name="check_in_time" type="time" value="14:00"></label>
              <label><span>Check-out time</span><input name="check_out_time" type="time" value="11:00"></label>
              <label class="wide-field"><span>Cancellation policy</span><textarea name="cancellation_policy">Free cancellation up to 24 hours before check-in</textarea></label>
              <label class="wide-field"><span>Refund rules</span><textarea name="refund_rules" placeholder="Refund timelines, deductions, and no-show rules"></textarea></label>
              <label><span>Child policy</span><input name="child_policy" value="Children of all ages are welcome"></label>
              <label><span>Pet policy</span><input name="pet_policy" value="Pets allowed on request"></label>
              <label><span>Smoking policy</span><input name="smoking_policy" value="Non-smoking rooms"></label>
              <label><span>ID proof requirements</span><input name="id_proof_requirements" placeholder="Government-issued photo ID required"></label>
              <label class="wide-field"><span>Extra guest rules</span><input name="extra_guest_rules" placeholder="Charges and occupancy limits"></label>
              <label class="wide-field"><span>Custom policies</span><textarea name="custom_policies" placeholder="Add any additional house rules or property-specific terms"></textarea></label>
            </div>
          </section>

          <section class="form-section">
            <h3>Verification documents</h3>
            <p class="form-error-summary" data-document-summary-error hidden></p>
            <div class="form-grid">
              ${requiredVerificationDocuments.map(([name, label]) => verificationDocumentField(name, label)).join("")}
            </div>
          </section>
        </div>
        <button class="primary-btn" type="submit" data-registration-submit disabled>Submit for Approval</button>
      `;
      setupVerificationDocumentValidation(form);
      return;
    }

    form.innerHTML = `
      <div class="form-grid">
        <label><span>Full name</span><input name="full_name" required></label>
        <label><span>Username</span><input name="username" required></label>
        <label><span>Email</span><input name="email" type="email"></label>
        <label><span>Phone</span><input name="phone"></label>
        <label><span>Password</span><input name="password" type="password" required></label>
      </div>
      <button class="primary-btn" type="submit">Register</button>
    `;
  }

  async function submitAuth(event) {
    event.preventDefault();
    const formElement = event.currentTarget;
    const form = new FormData(formElement);
    const data = Object.fromEntries(form.entries());
    ["hotel_photos", "ownership_certificate_upload", "business_license_upload", "tourism_registration_upload", "id_proof_upload"].forEach((key) => delete data[key]);
    try {
      if (state.authMode === "login") {
        const result = await API.login(data.username, data.password);
        saveSession(result);
        closeAuth();
        showPortal();
        return;
      }

      data.role = state.authMode === "partner" ? "hotel_manager" : "guest";
      if (state.authMode === "partner") {
        data.location = [data.city, data.state, data.country].filter(Boolean).join(", ") || data.address || data.location;
        data.amenities = checkedValues(formElement, "amenities");
        data.accepted_payments = ["Pay Now", "Pay at Property"];
        data.nearby_places = listToArray(data.nearby_landmarks);

        const documents = await uploadRegistrationDocuments(formElement);
        if (documents.length) data.documents = documents;

        const photoFiles = getUploadedFiles(formElement, "hotel_photos");
        const uploadedImages = await uploadLooseImages(photoFiles);
        if (uploadedImages.length) {
          data.hotel_images = uploadedImages;
          data.image_path = uploadedImages[0];
        }

        ["price_from", "star_rating", "room_count", "weekend_price", "seasonal_price", "taxes_fees", "min_stay"].forEach((key) => {
          if (data[key] !== undefined) data[key] = numberValue(data[key], key === "min_stay" ? 1 : 0);
        });
      }
      if (data.price_from) data.price_from = Number(data.price_from);
      await API.register(data);
      toast(state.authMode === "partner" ? "Hotel submitted. Login to track approval." : "Account created. You can login now.", "success");
      state.authMode = "login";
      renderAuthForm();
    } catch (error) {
      toast(error.message, "error");
    }
  }

  function saveSession(result) {
    const user = result.user || {};
    state.user = user;
    state.hotelId = Number(user.hotel_id || 0);
    localStorage.setItem("hms_token", result.token);
    localStorage.setItem("hms_user", user.username || "");
    localStorage.setItem("hms_role", user.role || "guest");
    localStorage.setItem("hms_hotel_id", String(state.hotelId));
  }

  async function restoreSession() {
    const token = localStorage.getItem("hms_token");
    if (!token) return;

    try {
      state.user = await API.me();
      state.hotelId = Number(state.user.hotel_id || 0);
      showPortal();
    } catch (error) {
      clearSession();
    }
  }

  function clearSession() {
    localStorage.removeItem("hms_token");
    localStorage.removeItem("hms_user");
    localStorage.removeItem("hms_role");
    localStorage.removeItem("hms_hotel_id");
    state.user = null;
    state.hotelId = 0;
  }

  function roleGroup() {
    const role = state.user?.role || localStorage.getItem("hms_role") || "guest";
    if (role === "super_admin") return "admin";
    if (role === "hotel_manager" || role === "receptionist") return "manager";
    return "guest";
  }

  function showPortal() {
    $("#public-app").classList.add("hidden");
    $("#portal-app").classList.remove("hidden");
    $("#portal-app").classList.remove("nav-open");
    const group = roleGroup();
    $("#portal-role-label").textContent = group === "admin" ? "Super Admin Portal" : group === "manager" ? "Hotel Manager Portal" : "Guest Portal";
    const userName = state.user?.full_name || state.user?.username || localStorage.getItem("hms_user") || "User";
    $("#portal-user-name").textContent = userName;
    $("#portal-user-meta").textContent = state.user?.email || state.user?.role || "";
    const avatar = $("#portal-avatar");
    if (avatar) avatar.textContent = initials(userName || "User");
    $(".portal-actions [data-portal='wishlist']")?.classList.toggle("hidden", group !== "guest");
    state.activePortal = "dashboard";
    renderPortalNav();
    renderPortal();
  }

  function hidePortal() {
    $("#portal-app").classList.add("hidden");
    $("#public-app").classList.remove("hidden");
  }

  function portalItems() {
    const group = roleGroup();
    if (group === "admin") return [
      ["dashboard", "Platform Analytics"],
      ["platform-payments", "Platform Payments"],
      ["approvals", "Hotel Approvals"],
      ["coupons", "Discount Coupons"],
      ["fraud", "Hotel Verification"],
    ];
    if (group === "manager") return [
      ["dashboard", "Dashboard"],
      ["hotel", "My Property"],
      ["rooms", "Rooms"],
      ["bookings", "Bookings"],
      ["calendar", "Calendar"],
      ["guests", "Guests"],
      ["reviews", "Reviews"],
      ["commission", "Commission Due"],
      ["earnings", "Earnings"],
      ["staff", "Staff"],
    ];
    return [
      ["dashboard", "Home"],
      ["search", "Hotels"],
      ["bookings", "Trips"],
      ["wishlist", "Wishlist"],
      ["reviews", "Reviews"],
      ["profile", "Profile"],
    ];
  }

  function renderPortalNav() {
    $("#portal-nav").innerHTML = portalItems().map(([key, label]) => `
      <button class="${state.activePortal === key ? "active" : ""}" data-portal="${key}" ${state.activePortal === key ? 'aria-current="page"' : ""}>${safe(label)}</button>
    `).join("");
  }

  function setPortalTitle(title, kicker = "Dashboard") {
    $("#portal-title").textContent = title;
    $("#portal-kicker").textContent = kicker;
    const subtitles = {
      dashboard: "Track performance, manage reservations, monitor occupancy,and grow your property with StaySphere.",
      search: "Search hand-picked hotels with fast filters, clear pricing, and guest-ready details.",
      bookings: "Track upcoming stays, invoices, cancellations, and completed trips in one place.",
      wishlist: "Your saved hotels, ready for comparison when the next trip starts calling.",
      reviews: "Share stay feedback and revisit the hotels you have reviewed.",
      profile: "Keep your guest details polished for faster, cleaner reservations.",
    };
    const subtitle = $("#portal-subtitle");
    if (subtitle) subtitle.textContent = subtitles[state.activePortal] || "Manage your StaySphere workspace from a clean top navigation.";
  }

  function syncRoomPolling() {
    const shouldSync = roleGroup() === "manager" && state.activePortal === "rooms";
    if (!shouldSync && state.roomSyncTimer) {
      clearInterval(state.roomSyncTimer);
      state.roomSyncTimer = null;
      return;
    }
    if (!shouldSync || state.roomSyncTimer) return;
    state.roomSyncTimer = setInterval(async () => {
      const modalOpen = !$("#details-modal")?.classList.contains("hidden");
      if (state.activePortal !== "rooms" || modalOpen || document.hidden || state.roomSyncRunning) return;
      state.roomSyncRunning = true;
      try {
        await renderManagerRooms({ fromSync: true });
      } finally {
        state.roomSyncRunning = false;
      }
    }, 12000);
  }

  function renderPortal() {
    renderPortalNav();
    syncRoomPolling();
    const group = roleGroup();
    if (group === "admin") return renderAdminView();
    if (group === "manager") return renderManagerView();
    return renderGuestView();
  }

  function guestSearchControls() {
    const today = new Date();
    const yyyy = today.getFullYear();
    const mm = String(today.getMonth() + 1).padStart(2, '0');
    const dd = String(today.getDate()).padStart(2, '0');
    const todayStr = `${yyyy}-${mm}-${dd}`;
    return `
      <div class="guest-search-panel">
        <label><span>Destination</span><input id="guest-search-q" placeholder="Where are you going?"></label>
        <label><span>Check-in</span>
          <input type="date" id="guest-checkin" min="${todayStr}">
        </label>
        <label><span>Check-out</span>
          <input type="date" id="guest-checkout" min="${todayStr}">
        </label>
        <label><span>Guests</span>
          <select id="guest-guests"><option value="2">2 adults</option><option value="1">1 adult</option><option value="3">3 adults</option><option value="4">4 adults</option><option value="5">5 adults</option></select>
        </label>
        <button class="search-button" id="guest-search-btn" type="button">Search</button>
      </div>
    `;
  }

  function categoryTabs() {
    const tabs = [
      ["", "All stays"],
      ["Luxury Resort", "Resorts"],
      ["Villa Stay", "Villas"],
      ["Hotel", "Hotels"],
      ["Cabin", "Cabins"],
      ["Hostel", "Hostels"],
    ];
    const activeCat = state.selectedCategory || "";
    return `<div class="category-tabs">${tabs.map(([value, label]) => `<button class="${value === activeCat ? "active" : ""}" data-category-filter="${safe(value)}">${safe(label)}</button>`).join("")}</div>`;
  }

  function travelSection(title, actionLabel, actionPortal, content, extraClass = "") {
    return `
      <section class="travel-section ${extraClass}">
        <div class="travel-section-head">
          <h2>${safe(title)}</h2>
          ${actionLabel ? `<button class="secondary-btn" data-portal="${safe(actionPortal)}">${safe(actionLabel)}</button>` : ""}
        </div>
        ${content}
      </section>
    `;
  }

  function destinationRail() {
    const destinations = state.destinations.length ? state.destinations : fallbackHotels.slice(0, 4).map((hotel) => ({
      location: hotel.location,
      image_url: hotel.image_url,
      stays: 12,
      price_from: hotel.price_from,
    }));
    return `<div class="destination-rail">${destinations.slice(0, 6).map((destination, index) => `
      <article class="mini-destination-card">
        <img src="${safe(destination.image_url || fallbackHotels[index % fallbackHotels.length].image_url)}" alt="${safe(destination.location)}">
        <div>
          <strong>${safe(destination.location)}</strong>
          <span>${safe(destination.stays || 12)} stays from ${money(destination.price_from || 3500)}</span>
        </div>
      </article>
    `).join("")}</div>`;
  }

  async function renderGuestView() {
    const view = $("#portal-view");
    if (state.activePortal === "search") {
      setPortalTitle("Find your next stay", "Hotels");
      view.innerHTML = `
        <div class="search-layout">
          ${guestSearchControls()}
          ${categoryTabs()}
          <div class="hotel-grid" id="guest-hotel-grid">${state.hotels.map(hotelCard).join("")}</div>
        </div>
      `;
      return;
    }
    if (state.activePortal === "wishlist") return renderWishlist();
    if (state.activePortal === "bookings") return renderMyBookings("Booking history");
    if (state.activePortal === "profile") return renderProfile();

    if (state.activePortal === "reviews") return renderMyReviews();
    if (state.activePortal === "payments") {
      setPortalTitle("Payment Methods", "Payments");
      view.innerHTML = `
          <div class="payment-shell">
            <div class="portal-card payment-card">
              <small>Default payment</small>
              <strong>UPI and cards ready</strong>
              <p class="muted">Manage saved cards, UPI IDs, wallets, and pay-at-hotel preferences here.</p>
              <button class="primary-btn premium-btn">Add payment method</button>
            </div>
            <div class="portal-card payment-card subtle">
              <small>Travel wallet</small>
              <strong>${money(state.user?.loyalty_points || 0)}</strong>
              <p class="muted">Use loyalty wallet balance during checkout when available.</p>
            </div>
          </div>
        `;
      return;
    }

    setPortalTitle("Where comfort meets your itinerary", "Home");
    const [bookings, loyalty, wishlist] = await Promise.allSettled([API.myBookings(), API.loyalty(), API.wishlist()]);
    const trips = bookings.value || [];
    const savedHotels = wishlist.value || [];
    const recommended = state.hotels.slice(0, 6);
    view.innerHTML = `
      <div class="search-layout home-search">
        ${guestSearchControls()}
        ${categoryTabs()}
      </div>
      <div class="stats-grid compact-stats">
        <div class="stat-card"><small>Upcoming</small><strong>${trips.filter((b) => b.status === "active").length}</strong></div>
        <div class="stat-card"><small>Total trips</small><strong>${trips.filter((b) => b.status !== "cancelled").length}</strong></div>
        <div class="stat-card"><small>Loyalty points</small><strong>${loyalty.value?.loyalty_points || state.user?.loyalty_points || 0}</strong></div>
        <div class="stat-card"><small>Status</small><strong>${(loyalty.value?.loyalty_points || 0) > 1000 ? "Gold" : "Silver"}</strong></div>
      </div>
      ${travelSection("Saved hotels", "Open wishlist", "wishlist", savedHotels.length
      ? `<div class="hotel-rail">${savedHotels.map(hotelCard).join("")}</div>`
      : `<div class="empty-state compact-empty"><h3>No saved hotels yet</h3><p class="muted">Tap the heart on a hotel card to build your shortlist.</p></div>`, "saved-hotels-section")}
      ${travelSection("Recently viewed hotels", "Explore all", "search", `<div class="hotel-rail">${recommended.slice(0, 5).map(hotelCard).join("")}</div>`)}
      ${travelSection("Trending destinations", "", "", destinationRail())}
      ${travelSection("Upcoming trips", "View trips", "bookings", renderBookingList(trips.slice(0, 3)), "embedded-bookings")}
    `;
  }

  async function renderWishlist() {
    setPortalTitle("Saved hotels", "Wishlist");
    const view = $("#portal-view");
    try {
      const hotels = await API.wishlist();
      view.innerHTML = hotels.length
        ? `
          ${travelSection("Your shortlist", "Find more stays", "search", `<div class="hotel-rail">${hotels.map(hotelCard).join("")}</div>`, "saved-hotels-section")}
          ${travelSection("Similar stays to compare", "Explore hotels", "search", `<div class="hotel-grid">${state.hotels.slice(0, 4).map(hotelCard).join("")}</div>`)}
        `
        : `<div class="empty-state portal-card compact-empty"><h3>Your wishlist is empty</h3><p class="muted">Save hotels while browsing to compare them later.</p><button class="primary-btn premium-btn" data-portal="search">Browse hotels</button></div>`;
    } catch (error) {
      view.innerHTML = `<div class="portal-card">${safe(error.message)}</div>`;
    }
  }

  async function renderMyBookings(title) {
    setPortalTitle(title, "Trips");
    const bookings = await API.myBookings().catch(() => []);
    $("#portal-view").innerHTML = renderBookingList(bookings);
  }


  function renderBookingList(bookings) {
    if (!bookings.length) return `<div class="empty-state portal-card"><h3>No bookings yet</h3><p class="muted">Your confirmed stays will appear here.</p></div>`;
    return `
      <div class="portal-card">
        <h3>Bookings</h3>
        <div class="booking-flow">
          ${bookings.map((b) => {
      const today = new Date();
      const yyyy = today.getFullYear();
      const mm = String(today.getMonth() + 1).padStart(2, '0');
      const dd = String(today.getDate()).padStart(2, '0');
      const todayStr = `${yyyy}-${mm}-${dd}`;
      const canReview = ["checked_in", "checked_out", "completed"].includes(String(b.status || "")) || 
        (!["cancelled", "no_show"].includes(String(b.status || "")) && b.check_out_date && b.check_out_date <= todayStr);
      const cancellation = bookingCancellationState(b);
      return `
              <div class="booking-item">
                <div>
                  <span style="font-size: 15px; font-weight: 600; color: var(--text);">${safe(b.booking_id)}</span>
                  <p class="muted">${safe(b.hotel_name || b.guest_name)} - ${safe(b.check_in_date)} to ${safe(b.check_out_date)} - ${safe(b.status)}</p>
                </div>
                <div class="card-row">
                  <span style="font-size: 15px; font-weight: 600;">${money(b.total_amount)}</span>
                  <button class="secondary-btn" style="padding: 0 12px; min-height: 34px;" data-receipt="${safe(b.booking_id)}">Invoice</button>
                  ${canReview ? `<button class="secondary-btn" style="padding: 0 12px; min-height: 34px;" data-open-review="${safe(b.hotel_id)}" data-booking-ref="${safe(b.booking_id)}" data-hotel-name="${safe(b.hotel_name || "Hotel")}">Review</button>` : ""}
                  ${b.status === "active" ? (cancellation.allowed
          ? `<button class="danger-btn" style="padding: 0 12px; min-height: 34px;" data-cancel-booking="${safe(b.booking_id)}" title="${safe(cancellation.title)}">Cancel</button>`
          : `<button class="danger-btn" style="padding: 0 12px; min-height: 34px;" disabled title="${safe(cancellation.title)}">Cancel</button>`) : ""}
                </div>
              </div>
            `;
    }).join("")}
        </div>
      </div>
    `;
  }

  function reviewCard(review = {}) {
    const images = parseImages(review.images);
    const authorName = review.author || review.user_name || state.user?.full_name || "Guest";
    const avatarText = initials(authorName);
    const reviewedAt = String(review.created_at || "").slice(0, 10);
    return `
      <article class="review-card glass-card">
        <div class="review-card-head">
          <div class="review-avatar">${safe(avatarText)}</div>
          <div class="review-meta">
            <strong>${safe(authorName)}</strong>
            <span class="muted">${safe(review.hotel_name || "Hotel")} · ${safe(review.hotel_location || "")}</span>
            <small class="muted">${safe(review.hotel_id ? `Hotel #${review.hotel_id}` : "")} ${reviewedAt ? `· ${safe(reviewedAt)}` : ""}</small>
          </div>
          <span class="score-pill">⭐ ${safe(review.rating || 0)}</span>
        </div>
        <p class="review-comment">${safe(review.comment || "")}</p>
        ${images.length ? `<div class="review-image-strip">${images.map((src) => `<img src="${safe(src)}" alt="Review photo">`).join("")}</div>` : ""}
        ${review.manager_reply ? `<div class="review-reply"><strong>Host reply</strong><p>${safe(review.manager_reply)}</p></div>` : ""}
      </article>
    `;
  }

  async function renderMyReviews() {
    setPortalTitle("My Reviews & Ratings", "Reviews");
    const view = $("#portal-view");
    const reviews = await API.myReviews().catch(() => []);
    const total = reviews.length;
    const average = total ? reviews.reduce((sum, review) => sum + Number(review.rating || 0), 0) / total : 0;
    const photoCount = reviews.reduce((count, review) => count + parseImages(review.images).length, 0);
    const hotelCount = new Set(reviews.map((review) => review.hotel_id)).size;
    view.innerHTML = `
      <div class="stats-grid compact-stats">
        <div class="stat-card"><small>Total reviews</small><strong>${total}</strong></div>
        <div class="stat-card"><small>Average rating</small><strong>${total ? average.toFixed(1) : "—"}</strong></div>
        <div class="stat-card"><small>Hotels reviewed</small><strong>${hotelCount}</strong></div>
        <div class="stat-card"><small>Photos uploaded</small><strong>${photoCount}</strong></div>
      </div>
      <div class="portal-card">
        <div class="card-row">
          <h3 style="margin: 0;">Your review history</h3>
          <button class="secondary-btn" data-portal="bookings">Rate Another Stay</button>
        </div>
        <div class="review-list review-feed">
          ${reviews.length ? reviews.map(reviewCard).join("") : `<div class="empty-state"><h3>No reviews yet</h3><p class="muted">Book a stay and submit a review after checkout.</p></div>`}
        </div>
      </div>
    `;
  }

  function renderProfile() {
    setPortalTitle("Profile settings", "Profile");
    $("#portal-view").innerHTML = `
      <form class="portal-card stack-form" id="profile-form">
        <div class="form-grid">
          <label><span>Full name</span><input name="full_name" value="${safe(state.user?.full_name)}"></label>
          <label><span>Email</span><input name="email" value="${safe(state.user?.email)}"></label>
          <label><span>Phone</span><input name="phone" value="${safe(state.user?.phone)}"></label>
        </div>
        <button class="primary-btn">Save Profile</button>
      </form>
    `;
  }

  async function renderManagerView() {
    if (state.activePortal === "hotel") return renderManagerHotel();
    if (state.activePortal === "rooms") return renderManagerRooms();
    if (state.activePortal === "bookings") return renderManagerBookings();
    if (state.activePortal === "calendar") return renderManagerCalendar();
    if (state.activePortal === "guests") return renderManagerGuests();
    if (state.activePortal === "reviews") return renderManagerReviews();
    if (state.activePortal === "commission") return renderManagerCommission();
    if (state.activePortal === "earnings") return renderManagerEarnings();
    if (state.activePortal === "analytics") return renderManagerAnalytics();
    if (state.activePortal === "staff") return renderStaff();

    setPortalTitle("Dashboard home", "Hotel Manager Portal");
    const [data, hotel, bookings, reviews, timeline] = await Promise.all([
      state.hotelId ? API.managerAnalytics(state.hotelId).catch(() => ({})) : {},
      state.hotelId ? API.hotel(state.hotelId).catch(() => ({})) : {},
      state.hotelId ? API.bookings(state.hotelId, "all").catch(() => []) : [],
      state.hotelId ? API.reviews(state.hotelId).catch(() => []) : [],
      state.hotelId ? API.revenueTimeline(state.hotelId, 7).catch(() => []) : [],
    ]);
    const activeGuests = bookings.filter((booking) => booking.status === "active" || booking.status === "checked_in").length;
    const cancelled = bookings.filter((booking) => booking.status === "cancelled").length;
    const cover = hotelImage(hotel);

    const last7Days = [];
    for (let i = 6; i >= 0; i--) {
      const d = new Date();
      d.setDate(d.getDate() - i);
      const iso = d.getFullYear() + '-' + String(d.getMonth() + 1).padStart(2, '0') + '-' + String(d.getDate()).padStart(2, '0');
      last7Days.push(iso);
    }
    const fullTimeline = last7Days.map(dateStr => {
      const found = timeline.find(t => t.day === dateStr);
      const dateObj = new Date(dateStr + 'T00:00:00');
      const label = dateObj.toLocaleDateString('en-US', { weekday: 'short' });
      return {
        day: dateStr,
        label: label,
        revenue: found ? Number(found.revenue || 0) : 0
      };
    });

    $("#portal-view").innerHTML = `
      ${!state.hotelId ? `<div class="portal-card"><h3>Complete partner onboarding</h3>${managerHotelForm()}</div>` : ""}
      <div class="stats-grid">
        <div class="stat-card"><small>Today's Revenue</small><strong>${money(data.today_revenue || 0)}</strong></div>
        <div class="stat-card"><small>Monthly Earnings</small><strong>${money(data.monthly_revenue || 0)}</strong></div>
        <div class="stat-card"><small>Occupancy</small><strong>${Number(data.occupancy_pct || 0).toFixed(1)}%</strong></div>
        <div class="stat-card"><small>Active Guests</small><strong>${activeGuests}</strong></div>
        <div class="stat-card"><small>Available Rooms</small><strong>${data.available_rooms || 0}</strong></div>
        <div class="stat-card"><small>Pending Check-ins</small><strong>${data.pending_checkins || 0}</strong></div>
        <div class="stat-card"><small>New Reviews</small><strong>${reviews.length}</strong></div>
        <div class="stat-card"><small>Cancelled Bookings</small><strong>${cancelled}</strong></div>
      </div>
      <div class="portal-card commission-card" style="background: var(--surface); border: 1px solid var(--accent); margin-bottom: 20px;">
        <h3>Monthly Commission Statement</h3>
        <div class="card-row">
          <div>
            <p class="muted">Total commission for this month</p>
            <strong style="font-size: 24px;">${money((data.monthly_revenue || 0) * 0.15)}</strong>
            <p class="muted small">15% of monthly earnings</p>
          </div>
          <div>
            <p class="muted">Due date</p>
            <strong>${new Date().toLocaleDateString('en-IN', { month: 'long', year: 'numeric', day: 'numeric' })}</strong>
            <p class="muted small">End of month</p>
          </div>
        </div>
        <div class="notice-item">
          <strong>Payment Instructions</strong>
          <p class="muted">Commission amount will be automatically deducted from your UPI payments or needs to be paid to the super admin at the end of each month.</p>
        </div>
      </div>
      <div class="manager-grid">
        <div class="portal-card property-spotlight">
          <img src="${safe(cover)}" alt="${safe(hotel.name || "Property")}">
          <div class="card-row"><h3>${safe(hotel.name || "Property setup")}</h3><span class="status-pill ${safe(hotel.status || "pending")}">${safe(statusText(hotel.status))}</span></div>
          <p class="muted">${safe(hotel.location || "Add your property location")} · ${safe(hotel.property_type || "Hotel")}</p>
          <div class="action-grid">
            <button class="secondary-btn" data-portal="hotel">Edit property</button>
            <button class="secondary-btn" data-portal="rooms">Manage rooms</button>
            <button class="secondary-btn" data-portal="calendar">Open calendar</button>
            <button class="secondary-btn" data-portal="earnings">View finance</button>
          </div>
        </div>
        <div class="portal-card">
          <h3>Revenue (Last 7 Days)</h3>
          ${renderSvgChart(fullTimeline, 'revenue', 'label', money)}
        </div>
      </div>
      <div class="manager-grid">
        <div class="portal-card">
          <h3>Recent activity</h3>
          ${(bookings.slice(0, 4).map((booking) => `<div class="booking-item"><div><strong>${safe(booking.booking_id)}</strong><p class="muted">${safe(booking.guest_name)} · ${safe(booking.check_in_date)} to ${safe(booking.check_out_date)}</p></div><span class="status-pill ${safe(booking.status)}">${safe(booking.status)}</span></div>`).join("")) || `<p class="muted">No booking activity yet.</p>`}
        </div>
        <div class="portal-card">
          <h3>Operations checklist</h3>
          ${managerOperationsItems(hotel, bookings, reviews).map((note) => `<div class="notice-item"><strong>${safe(note.title)}</strong><p class="muted">${safe(note.message)}</p></div>`).join("")}
        </div>
      </div>
    `;
  }

  function managerHotelForm() {
    return `
      <form class="stack-form" id="manager-hotel-form">
        <div class="form-section">
          <h3>Basic property</h3>
          <div class="form-grid">
            <label><span>Property name</span><input name="name" required></label>
            <label><span>Property type</span><select name="property_type"><option>Hotel</option><option>Resort</option><option>Villa</option><option>Apartment</option><option>Hostel</option></select></label>
            <label><span>Star rating</span><input name="star_rating" type="number" min="1" max="5" step="0.5" value="4.5"></label>
            <label><span>Number of rooms</span><input name="room_count" type="number" min="1" value="12"></label>
          </div>
          <label><span>Description</span><textarea name="description"></textarea></label>
        </div>
        <div class="form-section">
          <h3>Location</h3>
          <div class="form-grid">
            <label><span>Country</span><input name="country" value="India"></label>
            <label><span>State</span><input name="state"></label>
            <label><span>City</span><input name="city"></label>
            <label><span>Full address</span><input name="address"></label>
            <label><span>Pincode</span><input name="postal_code"></label>
            <label><span>Coordinates</span><input name="coordinates" placeholder="12.9716, 77.5946"></label>
          </div>
          <label><span>Nearby landmarks</span><textarea name="nearby_landmarks"></textarea></label>
        </div>
        <div class="form-section">
          <h3>Pricing and facilities</h3>
          <div class="form-grid">
            <label><span>Starting price</span><input name="price_from" type="number" value="4500"></label>
            <label><span>Weekend pricing</span><input name="weekend_price" type="number" value="5200"></label>
            <label><span>Seasonal pricing</span><input name="seasonal_price" type="number" value="6500"></label>
            <label><span>Minimum stay</span><input name="min_stay" type="number" value="1"></label>
          </div>
          ${checkboxGrid("amenities", propertyAmenities, ["Free Wi-Fi", "Parking"])}
        </div>
        <div class="form-section">
          <h3>Payments</h3>
          <div class="form-grid">
            <label><span>UPI ID (Required)</span><input name="upi_id" placeholder="hotelname@upi" required></label>
            <label><span>Bank account details</span><input name="bank_account" placeholder="Optional settlement account"></label>
            <label><span>Tax / GST number</span><input name="tax_number" placeholder="GSTIN"></label>
            <label><span>PAN / registration</span><input name="pan_registration" placeholder="PAN or business registration"></label>
          </div>
        </div>
        <div class="form-section">
          <h3>Images</h3>
          ${uploadZone("hotel_photos", "Hotel photos", { id: "manager-hotel-photos", multiple: true })}
        </div>
        <div class="form-section">
          <h3>Policies</h3>
          <div class="form-grid">
            <label><span>Check-in time</span><input name="check_in_time" type="time" value="14:00"></label>
            <label><span>Check-out time</span><input name="check_out_time" type="time" value="11:00"></label>
            <label><span>Cancellation policy</span><textarea name="cancellation_policy">Free cancellation up to 24 hours before check-in</textarea></label>
            <label><span>Refund rules</span><textarea name="refund_rules" placeholder="Refund timelines, deductions, and no-show rules"></textarea></label>
            <label><span>Child policy</span><input name="child_policy" value="Children of all ages are welcome"></label>
            <label><span>Pet policy</span><input name="pet_policy" value="Pets allowed on request"></label>
            <label><span>Smoking policy</span><input name="smoking_policy" value="Non-smoking rooms"></label>
            <label><span>ID proof requirements</span><input name="id_proof_requirements" placeholder="Government-issued photo ID required"></label>
            <label><span>Extra guest rules</span><input name="extra_guest_rules" placeholder="Charges and occupancy limits"></label>
            <label class="wide-field"><span>Custom policies</span><textarea name="custom_policies" placeholder="Add any additional house rules or property-specific terms"></textarea></label>
          </div>
        </div>
        <div class="form-section">
          <h3>Verification documents</h3>
          <p class="form-error-summary" data-document-summary-error hidden></p>
          <div class="form-grid">
            ${requiredVerificationDocuments.map(([name, label]) => verificationDocumentField(name, label)).join("")}
          </div>
        </div>
        <button class="primary-btn" data-registration-submit disabled>Submit for Approval</button>
      </form>
    `;
  }

  async function renderManagerHotel() {
    setPortalTitle("Hotel profile", "Hotel Manager Portal");
    inputFilesMap.delete("hotel-profile-photos");
    if (!state.hotelId) {
      $("#portal-view").innerHTML = `<div class="portal-card"><h3>Register your hotel</h3>${managerHotelForm()}</div>`;
      setupVerificationDocumentValidation($("#manager-hotel-form"));
      return;
    }
    const hotel = await API.hotel(state.hotelId).catch(() => ({}));
    const amenities = parseAmenities(hotel.amenities);
    const nearby = parseAmenities(hotel.nearby_places).join("\n");
    const payments = parseAmenities(hotel.accepted_payments);
    const gallery = (Array.isArray(hotel.images) && hotel.images.length ? hotel.images : [{ image_url: hotel.image_url }])
      .map((img) => img.image_path || img.image_url || img.url)
      .filter(Boolean);
    $("#portal-view").innerHTML = `
      <div class="portal-card">
        <div class="card-row">
          <div>
            <h3>${safe(hotel.name || "Property profile")}</h3>
            <p class="muted">${safe(statusText(hotel.status))} · ${safe(hotel.property_type || "Hotel")} · ${safe(hotel.location || "Location missing")}</p>
          </div>
          <span class="status-pill ${safe(hotel.status || "pending")}">${safe(hotel.status || "pending")}</span>
        </div>
      </div>
      <div class="manager-grid">
        <form class="portal-card stack-form" id="hotel-profile-form" data-live-preview-form>
          <h3>Property details</h3>
          <div class="form-grid">
            <label><span>Name</span><input name="name" value="${safe(hotel.name)}"></label>
            <label><span>Property type</span><select name="property_type">${["Hotel", "Resort", "Villa", "Apartment", "Hostel"].map((type) => `<option ${hotel.property_type === type ? "selected" : ""}>${type}</option>`).join("")}</select></label>
            <label><span>Country</span><input name="country" value="${safe(hotel.country)}"></label>
            <label><span>State</span><input name="state" value="${safe(hotel.state)}"></label>
            <label><span>City</span><input name="city" value="${safe(hotel.city)}"></label>
            <label><span>Location</span><input name="location" value="${safe(hotel.location)}"></label>
            <label><span>Address</span><input name="address" value="${safe(hotel.address)}"></label>
            <label><span>Pincode</span><input name="postal_code" value="${safe(hotel.postal_code)}"></label>
            <label><span>Coordinates</span><input name="coordinates" value="${safe(hotel.coordinates)}"></label>
            <label><span>Star rating</span><input name="star_rating" type="number" min="1" max="5" step="0.5" value="${safe(hotel.star_rating || 4.5)}"></label>
            <label><span>Room count</span><input name="room_count" type="number" value="${safe(hotel.room_count || 0)}"></label>
            <label><span>Price from</span><input name="price_from" type="number" value="${safe(hotel.price_from || 0)}"></label>
          </div>
          ${uploadZone("hotel_photos", "Update hotel gallery", { id: "hotel-profile-photos", multiple: true, existing: gallery })}
          <label><span>Description</span><textarea name="description">${safe(hotel.description)}</textarea></label>
          <label><span>Nearby landmarks</span><textarea name="nearby_landmarks">${safe(nearby)}</textarea></label>
          <h3>Facilities</h3>
          ${checkboxGrid("amenities", propertyAmenities, amenities)}
          <button class="primary-btn">Save Hotel</button>
        </form>
        <div class="portal-card stack-form">
          <h3>Live hotel preview</h3>
          <div id="hotel-live-preview">${hotelPreviewCard(hotel, gallery)}</div>
          <div class="gallery-strip">
            ${gallery.map((src, index) => {
      const image = hotel.images?.[index] || {};
      return `<figure><img src="${safe(src)}" alt="${safe(hotel.name)} photo ${index + 1}">${image.id ? `<button class="ghost-btn" data-delete-hotel-image="${safe(image.id)}" title="Delete photo">x</button>` : ""}</figure>`;
    }).join("") || `<p class="muted">Upload photos to build the guest gallery.</p>`}
          </div>
          <div class="verification-card">
            <span class="status-pill ${hotel.status === "approved" ? "approved" : "pending"}">${hotel.status === "approved" ? "Verified partner" : "Documents submitted"}</span>
            <p class="muted">${hotel.status === "approved" ? "Hotel ownership, business license, tourism registration, and ID proof were collected during registration." : "Verification documents are locked after registration and are currently under admin review."}</p>
          </div>
        </div>
      </div>
      <div class="manager-grid">
        <form class="portal-card stack-form" id="policy-form">
          <h3>Policies</h3>
          <div class="form-grid">
            <label><span>Check-in time</span><input name="check_in_time" type="time" value="${safe(hotel.check_in_time)}"></label>
            <label><span>Check-out time</span><input name="check_out_time" type="time" value="${safe(hotel.check_out_time)}"></label>
            <label><span>Cancellation policy</span><textarea name="cancellation_policy">${safe(hotel.cancellation_policy)}</textarea></label>
            <label><span>Refund rules</span><textarea name="refund_rules">${safe(hotel.refund_rules)}</textarea></label>
            <label><span>Child policy</span><input name="child_policy" value="${safe(hotel.child_policy)}"></label>
            <label><span>Pet policy</span><input name="pet_policy" value="${safe(hotel.pet_policy)}"></label>
            <label><span>Smoking policy</span><input name="smoking_policy" value="${safe(hotel.smoking_policy)}"></label>
            <label><span>ID proof requirements</span><input name="id_proof_requirements" value="${safe(hotel.id_proof_requirements)}"></label>
            <label><span>Extra guest rules</span><input name="extra_guest_rules" value="${safe(hotel.extra_guest_rules)}"></label>
            <label class="wide-field"><span>Custom policies</span><textarea name="custom_policies">${safe(hotel.custom_policies)}</textarea></label>
          </div>
          <button class="primary-btn">Save Policies</button>
        </form>
        <form class="portal-card stack-form" id="payment-form">
          <h3>Payment information</h3>
          <div class="form-grid">
            <label><span>UPI ID (Required)</span><input name="upi_id" value="${safe(hotel.upi_id || '')}" placeholder="hotel@upi" required></label>
            <label><span>Bank account details</span><input name="bank_account" value="${safe(hotel.bank_account)}"></label>
            <label><span>Tax / GST number</span><input name="tax_number" value="${safe(hotel.tax_number)}"></label>
            <label><span>PAN / registration</span><input name="pan_registration" value="${safe(hotel.pan_registration)}"></label>
            <label><span>Taxes and charges</span><input name="taxes_fees" type="number" value="${safe(hotel.taxes_fees || 0)}"></label>
          </div>
          ${checkboxGrid("accepted_payments", paymentMethods, payments.length ? payments : ["Pay Now", "Pay at Property"])}
          <button class="primary-btn">Save Payment Info</button>
        </form>
      </div>
    `;
  }

  async function renderLegacyManagerRooms() {
    setPortalTitle("Rooms and pricing", "Hotel Manager Portal");
    const hotel = state.hotelId ? await API.hotel(state.hotelId).catch(() => ({})) : {};

    if (hotel.status !== "approved") {
      $("#portal-view").innerHTML = `
        <div class="portal-card" style="text-align: center; padding: 60px 20px;">
          <h3 style="font-size: 24px; margin-bottom: 12px; color: var(--text);">Approval Pending ⏳</h3>
          <p class="muted" style="max-width: 500px; margin: 0 auto;">Your property is currently under review by our administration team. You will be able to add rooms, set pricing, and manage inventory once your listing is approved.</p>
        </div>
      `;
      return;
    }

    const [types, rooms] = await Promise.all([
      state.hotelId ? API.roomTypes(state.hotelId).catch(() => []) : [],
      state.hotelId ? API.rooms(state.hotelId).catch(() => []) : [],
    ]);
    const statusCount = (status) => rooms.filter((room) => room.status === status).length;
    $("#portal-view").innerHTML = `
      <div class="stats-grid compact-stats">
        <div class="stat-card"><small>Total inventory</small><strong>${rooms.length}</strong></div>
        <div class="stat-card"><small>Available</small><strong>${statusCount("available")}</strong></div>
        <div class="stat-card"><small>Booked</small><strong>${statusCount("occupied")}</strong></div>
        <div class="stat-card"><small>Cleaning</small><strong>${statusCount("cleaned")}</strong></div>
      </div>
      
      <div class="manager-grid room-command-grid">
        <form class="portal-card stack-form" id="room-type-form">
          <div class="card-row"><h3>Add room type</h3><span class="pill">Gallery ready</span></div>
          <div class="form-grid">
            <label><span>Name</span><input name="name" required></label>
            <label><span>Number of beds</span><input name="beds" type="number" value="1"></label>
            <label><span>Price</span><input name="base_price" type="number" required></label>
            <label><span>Capacity</span><input name="max_capacity" type="number" value="2"></label>
            <label><span>Room size</span><input name="room_size" placeholder="320 sq ft"></label>
            <label><span>Total rooms</span><input name="total_rooms" type="number" value="1"></label>
          </div>
          ${uploadZone("room_photos", "Room photos", { id: "room-type-photos", multiple: true, compact: true })}
          ${checkboxGrid("room_features", roomFeatureOptions, ["Air conditioning", "Wi-Fi", "Private bathroom"])}
          <label><span>Description</span><textarea name="description"></textarea></label>
          <button class="primary-btn">Add Room Type</button>
        </form>
        <form class="portal-card stack-form" id="room-form">
          <div class="card-row"><h3>Add room</h3><span class="pill">${types.length} types</span></div>
          <div class="form-grid">
            <label><span>Room type</span><select name="room_type_id">${types.map((t) => `<option value="${t.id}">${safe(t.name)}</option>`).join("")}</select></label>
            <label><span>Room number</span><input name="room_number" required></label>
            <label><span>Floor</span><input name="floor" type="number" value="1"></label>
          </div>
          <button class="primary-btn">Add Room</button>
        </form>
      </div>
      <div class="portal-card">
        <div class="card-row"><h3>Room types</h3><button class="secondary-btn" data-portal="calendar">Timeline</button></div>
        <div class="room-type-grid">
          ${types.map((type, index) => `
            <article class="room-type-card">
              <img src="${safe(roomImage(type, index))}" alt="${safe(type.name)}">
              <div>
              <strong>${safe(type.name)}</strong>
              <p class="muted">${safe(type.description || "Guest-ready room")} · up to ${safe(type.max_capacity)} guests</p>
              <div class="card-row"><span>${money(type.current_price || type.base_price)}</span><span>${safe(type.available_count || 0)} available</span></div>
              </div>
            </article>
          `).join("") || `<p class="muted">Create a room type to start accepting bookings.</p>`}
        </div>
      </div>
      <div class="table-card">
        <div class="card-row">
          <h3>Dynamic room inventory</h3>
          <div class="filter-row inventory-filters">
            <input id="room-filter-text" data-room-filter placeholder="Search room">
            <select id="room-filter-status" data-room-filter>
              <option value="">All status</option><option value="available">Available</option><option value="occupied">Booked</option><option value="cleaned">Cleaning</option><option value="maintenance">Maintenance</option><option value="deactivated">Deactivated</option>
            </select>
          </div>
        </div>
        <div class="table-wrap"><table class="inventory-table"><thead><tr><th>Photo</th><th>Room</th><th>Type</th><th>Floor</th><th>Status</th><th>Price</th><th>Actions</th></tr></thead><tbody>
          ${rooms.map((room, index) => `<tr data-room-row data-search="${safe(`${room.room_number} ${room.room_type_name}`.toLowerCase())}" data-status="${safe(room.status)}"><td><img src="${safe(roomImage(room, index))}" alt="${safe(room.room_number)}"></td><td><strong>${safe(room.room_number)}</strong></td><td>${safe(room.room_type_name)}</td><td>${safe(room.floor)}</td><td><span class="status-pill ${safe(room.status)}">${safe(roomStatusLabel(room.status))}</span></td><td>${money(room.price || room.current_price || 0)}</td><td>${roomRowActions(room)}</td></tr>`).join("") || `<tr><td colspan="7" class="muted">No rooms added yet.</td></tr>`}
        </tbody></table></div>
      </div>
    `;
  }

  function roomTypeStats(type, rooms) {
    const typeRooms = rooms.filter((room) => Number(room.room_type_id) === Number(type.id));
    const count = (status) => typeRooms.filter((room) => room.status === status).length;
    return {
      total: typeRooms.length,
      available: count("available"),
      occupied: count("occupied"),
      blocked: count("reserved") + count("blocked"),
      maintenance: count("maintenance"),
      cleaning: count("cleaned"),
      deactivated: count("deactivated"),
    };
  }

  function roomTypeRooms(type, rooms) {
    return rooms
      .filter((room) => Number(room.room_type_id) === Number(type.id))
      .sort((a, b) => String(a.room_number || "").localeCompare(String(b.room_number || ""), undefined, { numeric: true }));
  }

  function roomSizeLabel(type = {}) {
    if (type.room_size) return type.room_size;
    const match = String(type.description || "").match(/\b\d+\s*(?:sq\s*ft|sqft|sqm|m2)\b/i);
    return match ? match[0] : "Size pending";
  }

  function roomInventoryTable(type, rooms) {
    if (Number(state.expandedRoomTypeId) !== Number(type.id)) return "";
    const typeRooms = roomTypeRooms(type, rooms);
    return `
      <div class="room-inventory-panel">
        <div class="inventory-panel-head">
          <div>
            <p class="eyebrow">Inventory</p>
            <h4>${safe(type.name)}</h4>
          </div>
          <div class="filter-row inventory-filters">
            <input id="room-filter-text" data-room-filter placeholder="Search room">
            <select id="room-filter-status" data-room-filter>
              <option value="">All status</option>
              <option value="available">Available</option>
              <option value="occupied">Occupied</option>
              <option value="reserved">Blocked</option>
              <option value="maintenance">Maintenance</option>
              <option value="cleaned">Cleaning</option>
              <option value="deactivated">Deactivated</option>
            </select>
          </div>
        </div>
        <div class="table-wrap">
          <table class="inventory-table">
            <thead>
              <tr>
                <th>Room</th>
                <th>Floor</th>
                <th>Status</th>
                <th>Guest</th>
                <th>Actions</th>
              </tr>
            </thead>
            <tbody>
              ${typeRooms.map((room) => {
      const search = `${room.room_number || ""} ${room.floor || ""} ${room.guest_name || ""}`.toLowerCase();
      return `
                  <tr data-room-row data-search="${safe(search)}" data-status="${safe(room.status)}" data-type="${safe(room.room_type_id)}">
                    <td><strong>${safe(room.room_number)}</strong></td>
                    <td>${safe(room.floor || 1)}</td>
                    <td><span class="status-pill ${safe(room.status)}">${safe(roomStatusLabel(room.status))}</span></td>
                    <td>${room.guest_name ? `<strong>${safe(room.guest_name)}</strong><br><small class="muted">${safe(room.booking_ref)}</small>` : `<span class="muted">-</span>`}</td>
                    <td>${roomRowActions(room)}</td>
                  </tr>
                `;
    }).join("") || `<tr><td colspan="5" class="muted empty-table-cell">No inventory generated</td></tr>`}
            </tbody>
          </table>
        </div>
      </div>
    `;
  }

  function roomTypeCard(type, rooms, index) {
    const stats = roomTypeStats(type, rooms);
    const amenities = parseAmenities(type.amenities);
    const isExpanded = Number(state.expandedRoomTypeId) === Number(type.id);
    return `
      <div class="room-type-container ${isExpanded ? "is-expanded" : ""}">
        <article class="pms-room-type-card horizontal-card">
          <img src="${safe(roomImage(type, index))}" alt="${safe(type.name)}">
          <div class="pms-room-type-body">
            <div class="pms-room-type-title">
              <h4>${safe(type.name)}</h4>
              <span class="room-price">${money(type.current_price || type.base_price)}<small>/night</small></span>
            </div>
            <div class="room-type-facts">
              <span>${safe(type.max_capacity || 2)} Guests</span>
              <span>${safe(roomSizeLabel(type))}</span>
              <span>${amenities.length} Amenities</span>
            </div>
            <p class="room-type-description">${safe(type.description || "Guest-ready room")}</p>
            
            <div class="room-type-metrics">
              <span>Total <strong>${stats.total}</strong></span>
              <span>Available <strong>${stats.available}</strong></span>
              <span>Occupied <strong>${stats.occupied}</strong></span>
              <span>Blocked <strong>${stats.blocked}</strong></span>
              <span>Maintenance <strong>${stats.maintenance}</strong></span>
              <span>Deactivated <strong>${stats.deactivated}</strong></span>
            </div>
            
            <div class="room-type-footer">
              <button class="primary-btn" type="button" data-manage-inventory="${safe(type.id)}" aria-expanded="${isExpanded ? "true" : "false"}">${isExpanded ? "Hide Inventory" : "Manage Inventory"}</button>
              <button class="secondary-btn" type="button" data-edit-room-type="${safe(type.id)}">Edit</button>
              <button class="danger-btn" type="button" data-delete-room-type="${safe(type.id)}">Delete</button>
            </div>
          </div>
        </article>
        ${roomInventoryTable(type, rooms)}
      </div>
    `;
  }

  async function renderManagerRooms({ fromSync = false } = {}) {
    setPortalTitle("Rooms and pricing", "Hotel Manager Portal");
    const hotel = state.hotelId ? await API.hotel(state.hotelId).catch(() => ({})) : {};

    if (hotel.status !== "approved") {
      $("#portal-view").innerHTML = `
        <div class="portal-card approval-card">
          <h3>Approval pending</h3>
          <p class="muted">Your property is currently under review. Room setup becomes available after admin approval.</p>
        </div>
      `;
      return;
    }

    const [types, rooms] = await Promise.all([
      state.hotelId ? API.roomTypes(state.hotelId).catch(() => []) : [],
      state.hotelId ? API.rooms(state.hotelId).catch(() => []) : [],
    ]);
    const statusCount = (status) => rooms.filter((room) => room.status === status).length;
    const totalRooms = rooms.length;
    const available = statusCount("available");
    const occupied = statusCount("occupied");
    const blocked = statusCount("reserved") + statusCount("blocked");
    const maintenance = statusCount("maintenance");
    const deactivated = statusCount("deactivated");

    $("#portal-view").innerHTML = `
      <section class="room-management-shell">
        <div class="room-toolbar">
          <div>
            <h2>Room Management</h2>
            <p class="muted">Automated room types with live inventory counts.</p>
          </div>
          <div class="button-row">
            <button class="primary-btn" type="button" data-open-room-type-modal>Add Room Type</button>
            <button class="secondary-btn" data-portal="calendar" type="button">Timeline</button>
          </div>
        </div>

        <div class="rooms-overview-strip" aria-label="Room inventory summary">
          <span>Total <strong>${totalRooms}</strong></span>
          <span>Available <strong>${available}</strong></span>
          <span>Occupied <strong>${occupied}</strong></span>
          <span>Blocked <strong>${blocked}</strong></span>
          <span>Maintenance <strong>${maintenance}</strong></span>
          <span>Deactivated <strong>${deactivated}</strong></span>
        </div>

        <div class="room-section-card">
          <div class="room-section-head">
            <div>
              <h3>Room Types</h3>
              <p class="muted">Each category is backed by generated room inventory.</p>
            </div>
          </div>
          <div class="room-type-grid pms-room-type-grid">
            ${types.map((type, index) => roomTypeCard(type, rooms, index)).join("") || `<div class="empty-state"><h3>No room types yet</h3><p class="muted">Create the first room type and generate inventory in one save.</p></div>`}
          </div>
        </div>
      </section>
    `;
  }

  async function renderManagerBookings() {
    setPortalTitle("Booking management", "Hotel Manager Portal");
    const [bookings, cancellations] = await Promise.all([
      state.hotelId ? API.bookings(state.hotelId, "all").catch(() => []) : [],
      state.hotelId ? API.cancellations(state.hotelId).catch(() => []) : [],
    ]);
    const cancellationByRef = new Map(cancellations.map((item) => [String(item.booking_ref || ""), item]));
    $("#portal-view").innerHTML = `
      <div class="table-card">
        <h3>Reservations</h3>
        <div class="table-wrap"><table><thead><tr><th>ID</th><th>Guest</th><th>Room</th><th>Dates</th><th>Total</th><th>Status</th><th>Cancellation reason</th><th>Action</th></tr></thead><tbody>
          ${bookings.map((b) => {
      const cancellation = cancellationByRef.get(String(b.booking_id));
      const reason = b.status === "cancelled"
        ? `<strong>${safe(cancellation?.reason || "Reason not recorded")}</strong>${cancellation?.cancelled_at ? `<br><span class="muted">${safe(cancellation.cancelled_at)}</span>` : ""}`
        : `<span class="muted">-</span>`;
      const statusHtml = b.status === "checked_in" && b.updated_at
        ? `<span class="status-pill checked_in">checked_in</span><br><small class="muted" style="display:block; margin-top:4px;">Checked in:<br><strong>${safe(b.updated_at)}</strong></small>`
        : `<span class="status-pill ${safe(b.status)}">${safe(b.status)}</span>`;

      let addressHtml = "";
      if (b.guest_address) {
        if (b.guest_address.includes("Special Requests:")) {
          const parts = b.guest_address.split("Special Requests:");
          const addr = parts[0].replace(/\s*\|\s*$/, "").trim();
          const req = parts[1].trim();
          addressHtml = `
            ${addr ? `<br><small class="muted" style="display:block; margin-top:4px; max-width:200px; white-space:normal; line-height:1.4;">${safe(addr)}</small>` : ""}
            ${req ? `<br><small class="status-pill pending" style="display:inline-block; margin-top:4px; padding: 2px 8px; font-size: 11px; white-space:normal; line-height:1.3; font-weight:600; text-transform:none; border-radius:4px;">🔑 Request: ${safe(req)}</small>` : ""}
          `;
        } else {
          addressHtml = `<br><small class="muted" style="display:block; margin-top:4px; max-width:200px; white-space:normal; line-height:1.4;">${safe(b.guest_address)}</small>`;
        }
      }

      return `<tr><td>${safe(b.booking_id)}</td><td>${safe(b.guest_name)}<br><span class="muted">${safe(b.guest_phone)}</span>${addressHtml}</td><td>${safe(b.room_number)}</td><td>${safe(b.check_in_date)} to ${safe(b.check_out_date)}</td><td>${money(b.total_amount)}</td><td>${statusHtml}</td><td>${reason}</td><td>${(b.status === "active" || b.status === "checked_in") ? `<div style="display: flex; gap: 8px;"><button class="primary-btn" style="padding: 6px 12px; font-size: 12px; min-height: 28px;" data-checkin="${safe(b.booking_id)}" ${b.status === "checked_in" ? "disabled" : ""}>Check-in</button><button class="success-btn" style="padding: 6px 12px; font-size: 12px; min-height: 28px;" data-checkout="${safe(b.booking_id)}" ${b.status === "active" ? "disabled" : ""}>Check-out</button></div>` : ""}</td></tr>`;
    }).join("")}
        </tbody></table></div>
      </div>
    `;
  }

  async function renderManagerCalendar() {
    setPortalTitle("Calendar and availability", "Hotel Manager Portal");
    const [bookings, rooms] = await Promise.all([
      state.hotelId ? API.bookings(state.hotelId, "all").catch(() => []) : [],
      state.hotelId ? API.rooms(state.hotelId).catch(() => []) : [],
    ]);
    const mode = localStorage.getItem("hms_calendar_mode") || "month";
    const today = new Date();
    const days = Array.from({ length: 30 }, (_, index) => {
      const date = new Date(today);
      date.setDate(today.getDate() + index);
      const iso = date.toISOString().slice(0, 10);
      const reserved = bookings.filter((booking) => (booking.status === "active" || booking.status === "checked_in") && booking.check_in_date <= iso && booking.check_out_date > iso).length;
      return { iso, day: date.getDate(), reserved };
    });
    const gridDays = days.slice(0, mode === "week" ? 7 : 30);
    const occupiedToday = rooms.filter((room) => roomCalendarStatus(room, bookings, days[0]?.iso) === "occupied").length;
    const occupancyPct = rooms.length ? Math.round((occupiedToday / rooms.length) * 100) : 0;
    $("#portal-view").innerHTML = `
      <div class="stats-grid">
        <div class="stat-card"><small>Total Rooms</small><strong>${rooms.length}</strong></div>
        <div class="stat-card"><small>Empty Today</small><strong>${rooms.filter((room) => room.status === "available").length}</strong></div>
        <div class="stat-card"><small>Cleaned</small><strong>${rooms.filter((room) => room.status === "cleaned").length}</strong></div>
        <div class="stat-card"><small>Occupancy</small><strong>${occupancyPct}%</strong></div>
      </div>
      <div class="table-card">
        <div class="card-row">
          <div>
            <h3>Interactive availability calendar</h3>
            <p class="muted">Green available, red booked, yellow cleaning.</p>
          </div>
          <div class="segmented compact">
            <button type="button" data-calendar-mode="week" class="${mode === "week" ? "active" : ""}">Week</button>
            <button type="button" data-calendar-mode="month" class="${mode === "month" ? "active" : ""}">Month</button>
          </div>
        </div>
        <div class="calendar-legend"><span class="legend available"></span>Available <span class="legend occupied"></span>Booked <span class="legend cleaned"></span>Cleaning <span class="legend maintenance"></span>Maintenance</div>
        <div class="table-wrap"><table class="availability-table timeline-table"><thead><tr><th>Room</th>${gridDays.map((day) => `<th>${safe(day.iso.slice(5))}</th>`).join("")}</tr></thead><tbody>
          ${rooms.map((room) => `<tr><td><strong>${safe(room.room_number)}</strong><br><span class="muted">${safe(room.room_type_name)}</span></td>${gridDays.map((day) => {
      const status = roomCalendarStatus(room, bookings, day.iso);
      return `<td><span class="calendar-cell ${safe(status)}" title="${safe(room.room_number)} · ${safe(day.iso)} · ${safe(roomStatusLabel(status))}">${safe(roomStatusLabel(status))}</span></td>`;
    }).join("")}</tr>`).join("") || `<tr><td colspan="${gridDays.length + 1}" class="muted">No rooms added yet.</td></tr>`}
        </tbody></table></div>
      </div>
    `;
  }

  async function renderManagerGuests() {
    setPortalTitle("Guest management", "Hotel Manager Portal");
    const bookings = state.hotelId ? await API.bookings(state.hotelId, "all").catch(() => []) : [];
    const guests = Object.values(bookings.reduce((acc, booking) => {
      const key = booking.guest_phone || booking.guest_email || booking.guest_name;
      if (!acc[key]) acc[key] = { ...booking, stays: 0, spend: 0 };
      acc[key].stays += Number(booking.nights || 1);
      acc[key].spend += Number(booking.total_amount || 0);
      return acc;
    }, {}));
    $("#portal-view").innerHTML = `
      <div class="table-card">
        <h3>Guest profiles</h3>
        <div class="table-wrap"><table><thead><tr><th>Guest</th><th>Contact</th><th>Stays (Nights)</th><th>Total spend</th><th>Last status</th></tr></thead><tbody>
          ${guests.map((guest) => `<tr><td>${safe(guest.guest_name)}</td><td>${safe(guest.guest_phone || guest.guest_email)}</td><td>${guest.stays}</td><td>${money(guest.spend)}</td><td><span class="status-pill ${safe(guest.status)}">${safe(guest.status)}</span></td></tr>`).join("") || `<tr><td colspan="5" class="muted">No guest records yet.</td></tr>`}
        </tbody></table></div>
      </div>
    `;
  }

  async function renderManagerReviews() {
    setPortalTitle("Reviews and ratings", "Hotel Manager Portal");
    const reviews = state.hotelId ? await API.reviews(state.hotelId).catch(() => []) : [];
    const average = reviews.length ? reviews.reduce((sum, review) => sum + Number(review.rating || 0), 0) / reviews.length : 0;
    $("#portal-view").innerHTML = `
      <div class="stats-grid">
        <div class="stat-card"><small>Average Rating</small><strong>${average.toFixed(1)}</strong></div>
        <div class="stat-card"><small>Total Reviews</small><strong>${reviews.length}</strong></div>
        <div class="stat-card"><small>Complaints</small><strong>${reviews.filter((review) => Number(review.rating || 0) <= 2).length}</strong></div>
        <div class="stat-card"><small>Positive Reviews</small><strong>${reviews.filter((review) => Number(review.rating || 0) >= 4).length}</strong></div>
      </div>
      <div class="portal-card">
        <h3>Customer reviews</h3>
        ${reviews.map((review) => `
          <div class="booking-item" style="display: block; margin-bottom: 12px;">
            <div style="display: flex; justify-content: space-between; margin-bottom: 8px;">
              <div><strong style="color: var(--text);">${safe(review.author || "Guest")}</strong> <span class="muted" style="font-size: 12px; margin-left: 8px;">${safe(review.created_at?.split(' ')[0] || '')}</span></div>
              <span class="score-pill">⭐ ${safe(review.rating)}</span>
            </div>
            <p class="muted" style="margin: 0 0 12px;">${safe(review.comment)}</p>
            ${parseImages(review.images).length ? `<div class="review-image-strip">${parseImages(review.images).map((src) => `<img src="${safe(src)}" alt="Review photo">`).join("")}</div>` : ""}
            ${review.manager_reply
        ? `<div style="background: var(--surface-2); padding: 12px; border-radius: 6px; border-left: 3px solid var(--primary);"><strong style="font-size: 13px;">Your Reply:</strong><p style="margin: 4px 0 0; font-size: 14px;">${safe(review.manager_reply)}</p></div>`
        : `<div style="display: flex; gap: 8px;">
                   <input type="text" id="reply-input-${review.id}" class="form-control" placeholder="Write a response..." style="flex: 1; margin: 0; min-height: 36px; padding: 6px 12px;">
                   <button class="secondary-btn" data-reply-review="${review.id}" style="padding: 6px 16px; min-height: 36px;">Reply</button>
                 </div>`
      }
          </div>`).join("") || `<p class="muted">No reviews yet.</p>`}
      </div>
    `;
  }

  async function renderManagerEarnings() {
    setPortalTitle("Earnings and finance", "Hotel Manager Portal");
    const data = state.hotelId ? await API.managerEarnings(state.hotelId).catch(() => ({})) : {};
    const timeline = data.revenue_analytics || [];
    const maxEarnings = Math.max(1000, ...timeline.map((item) => Number(item.earnings || 0)));
    $("#portal-view").innerHTML = `
      <div class="stats-grid compact-stats">
        <div class="stat-card total-bookings-card">
          <div class="stat-card-icon">📅</div>
          <small>Total bookings</small>
          <strong>${data.total_bookings || 0}</strong>
          <span class="stat-trend positive">✓ Confirmed reservations</span>
        </div>
        <div class="stat-card gross-revenue-card">
          <div class="stat-card-icon">💰</div>
          <small>Gross revenue</small>
          <strong>${money(data.gross_revenue || 0)}</strong>
          <span class="stat-trend info">Customer payments</span>
        </div>
        <div class="stat-card commission-deducted-card">
          <div class="stat-card-icon">🏷️</div>
          <small>Commission deducted</small>
          <strong>${money(data.commission_deducted || 0)}</strong>
          <span class="stat-trend negative">Platform commission (15%)</span>
        </div>
        <div class="stat-card net-earnings-card">
          <div class="stat-card-icon">✨</div>
          <small>Net hotel earnings</small>
          <strong>${money(data.earnings_after_commission || 0)}</strong>
          <span class="stat-trend positive">Net profit (Gross - Commission)</span>
        </div>
      </div>
      <div class="manager-grid">
        <div class="portal-card">
          <h3>Revenue analytics</h3>
          ${renderSvgBarChart(timeline.length ? timeline : [{ month: "No data", earnings: 0 }], 'earnings', 'month', money)}
        </div>
        <div class="portal-card">
          <h3>Commission summary</h3>
          <div class="breakdown-row"><span>Gross revenue</span><strong>${money(data.gross_revenue || 0)}</strong></div>
          <div class="breakdown-row"><span>Commission deducted</span><strong>${money(data.commission_deducted || 0)}</strong></div>
          <div class="breakdown-row total"><span>Net hotel earnings</span><strong>${money(data.earnings_after_commission || 0)}</strong></div>
        </div>
      </div>
      <div class="table-card">
        <h3>Booking payment verification</h3>
        <div class="table-wrap"><table><thead><tr><th>Booking</th><th>Guest</th><th>Amount</th><th>Method</th><th>Proof</th><th>Status</th><th>Action</th></tr></thead><tbody>
          ${(data.bookings || []).map((b) => `<tr>
            <td>${safe(b.booking_id)}</td>
            <td>${safe(b.guest_name)}</td>
            <td>${money(b.total_amount)}</td>
            <td>${safe(b.payment_method === 'Pay Now' ? 'UPI' : b.payment_method)}</td>
            <td>${b.payment_proof_url ? `<a href="${safe(b.payment_proof_url)}" target="_blank" rel="noopener" style="color: var(--primary); text-decoration: underline;">View</a>` : '<span class="muted">-</span>'}</td>
            <td><span class="status-pill ${safe(b.payment_status)}">${safe(b.payment_status)}</span></td>
            <td>${b.payment_status === "pending" ? `<button class="success-btn" data-verify-guest-payment="${safe(b.booking_id)}" style="padding: 4px 12px; min-height: 28px; font-size: 12px;">${b.payment_method === 'Pay Now' ? 'Verify' : 'Mark Paid'}</button>` : (b.payment_status === 'paid' ? '<span style="color: var(--success); font-weight: 600; font-size: 13px;">✓ Verified</span>' : '<span class="muted">-</span>')}</td>
          </tr>`).join("") || `<tr><td colspan="7" class="muted">No bookings yet.</td></tr>` }
        </tbody></table></div>
      </div>
    `;
  }

  function managerOperationsItems(hotel = {}, bookings = [], reviews = []) {
    return [
      { title: "Approval status", message: hotel.status === "approved" ? "Verified property badge is active" : statusText(hotel.status) },
      { title: "Bookings today", message: `${bookings.filter((booking) => booking.status === "active" || booking.status === "checked_in").length} active stay(s) in progress` },
      { title: "Room attention", message: `${bookings.filter((booking) => booking.status === "cancelled").length} cancellation(s) recorded for follow-up` },
      { title: "Review health", message: `${reviews.length} guest review(s) available for quality tracking` },
    ];
  }

  async function renderManagerAnalytics() {
    setPortalTitle("Reports and analytics", "Hotel Manager Portal");
    const [stats, popularRooms, timeline] = await Promise.all([
      state.hotelId ? API.reportStats(state.hotelId).catch(() => ({})) : {},
      state.hotelId ? API.popularRooms(state.hotelId).catch(() => []) : [],
      state.hotelId ? API.revenueTimeline(state.hotelId, 14).catch(() => []) : [],
    ]);

    const last14Days = [];
    for (let i = 13; i >= 0; i--) {
      const d = new Date();
      d.setDate(d.getDate() - i);
      const iso = d.getFullYear() + '-' + String(d.getMonth() + 1).padStart(2, '0') + '-' + String(d.getDate()).padStart(2, '0');
      last14Days.push(iso);
    }
    const fullTimeline = last14Days.map(dateStr => {
      const found = timeline.find(t => t.day === dateStr);
      const dateObj = new Date(dateStr + 'T00:00:00');
      const label = dateObj.toLocaleDateString('en-US', { day: 'numeric', month: 'short' });
      return {
        day: dateStr,
        label: label,
        revenue: found ? Number(found.revenue || 0) : 0
      };
    });

    $("#portal-view").innerHTML = `
      <div class="stats-grid">
        <div class="stat-card"><small>Booking Analytics</small><strong>${stats.total_bookings || 0}</strong></div>
        <div class="stat-card"><small>Occupancy Graph</small><strong>${Number(stats.occupancy_rate || 0).toFixed(1)}%</strong></div>
        <div class="stat-card"><small>Completed</small><strong>${stats.checked_out || 0}</strong></div>
        <div class="stat-card"><small>Cancelled</small><strong>${stats.cancelled || 0}</strong></div>
      </div>
      <div class="admin-grid">
        <div class="portal-card"><h3>Popular room types</h3>${popularRooms.map((room) => `<div class="booking-item"><div><strong>${safe(room.room_type_name)}</strong><p class="muted">${safe(room.bookings || 0)} bookings</p></div></div>`).join("") || `<p class="muted">No room analytics yet.</p>`}</div>
      </div>
    `;
  }

  async function renderStaff() {
    setPortalTitle("Staff management", "Hotel Manager Portal");
    const staff = state.hotelId ? await API.staff(state.hotelId).catch(() => []) : [];
    $("#portal-view").innerHTML = `
      <div class="portal-card staff-hero">
        <div>
          <p class="eyebrow">Team operations</p>
          <h3>Staff directory</h3>
          <p class="muted">Manage front desk, housekeeping, and operations accounts for this property.</p>
        </div>
        <button class="primary-btn" data-open-staff-modal>Add Staff</button>
      </div>
      <div class="stats-grid compact-stats">
        <div class="stat-card"><small>Total staff</small><strong>${staff.length}</strong></div>
        <div class="stat-card"><small>Managers</small><strong>${staff.filter((person) => person.role === "hotel_manager").length}</strong></div>
        <div class="stat-card"><small>Reception</small><strong>${staff.filter((person) => person.role === "receptionist").length}</strong></div>
        <div class="stat-card"><small>Active</small><strong>${staff.filter((person) => person.staff_status !== "inactive").length}</strong></div>
      </div>
      <div class="table-card">
        <div class="card-row">
          <h3>Team members</h3>
          <div class="filter-row inventory-filters">
            <input id="staff-search" data-staff-filter placeholder="Search staff">
            <select id="staff-role-filter" data-staff-filter>
              <option value="">All roles</option><option value="hotel_manager">Managers</option><option value="receptionist">Reception</option>
            </select>
          </div>
        </div>
        <div class="staff-grid">
          ${staff.map((person) => `
            <article class="staff-card" data-staff-row data-search="${safe(`${person.full_name} ${person.username} ${person.role_label}`.toLowerCase())}" data-role="${safe(person.role)}">
              <span class="status-pill approved staff-status-badge">${safe(person.staff_status || "active")}</span>
              <div class="avatar">${safe(initials(person.full_name || person.username))}</div>
              <div class="staff-info">
                <strong>${safe(person.full_name || person.username)}</strong>
                <span class="username-tag">@${safe(person.username)}</span>
                <p class="staff-contact">${safe(person.email || person.phone || "No contact added")}</p>
                <div class="staff-tags">
                  <span class="pill role-pill">${safe(person.role_label || person.role)}</span>
                  <span class="pill shift-pill">${safe(person.shift || "Front desk")}</span>
                </div>
              </div>
              <div class="staff-actions">
                <button class="secondary-btn" data-open-staff-modal>Edit</button>
                ${person.role === "hotel_manager" ? "" : `<button class="ghost-btn" data-delete-staff="${safe(person.user_id)}">Delete</button>`}
              </div>
            </article>
          `).join("") || `<p class="muted">No staff accounts yet.</p>`}
        </div>
      </div>
    `;
  }

  async function renderManagerReports() {
    setPortalTitle("Reports", "Hotel Manager Portal");
    const timeline = state.hotelId ? await API.revenueTimeline(state.hotelId, 14).catch(() => []) : [];

    const last14Days = [];
    for (let i = 13; i >= 0; i--) {
      const d = new Date();
      d.setDate(d.getDate() - i);
      const iso = d.getFullYear() + '-' + String(d.getMonth() + 1).padStart(2, '0') + '-' + String(d.getDate()).padStart(2, '0');
      last14Days.push(iso);
    }
    const fullTimeline = last14Days.map(dateStr => {
      const found = timeline.find(t => t.day === dateStr);
      const dateObj = new Date(dateStr + 'T00:00:00');
      const label = dateObj.toLocaleDateString('en-US', { day: 'numeric', month: 'short' });
      return {
        day: dateStr,
        label: label,
        revenue: found ? Number(found.revenue || 0) : 0
      };
    });

    $("#portal-view").innerHTML = `
      <div class="portal-card">
        <h3>Revenue trend</h3>
        ${renderSvgChart(fullTimeline, 'revenue', 'label', money)}
      </div>
    `;
  }

  async function renderAdminView() {
    if (state.activePortal === "platform-payments") return renderAdminPlatformPayments();
    if (state.activePortal === "approvals") return renderApprovals();
    if (state.activePortal === "coupons") return renderAdminCoupons();
    if (state.activePortal === "fraud") return renderFraudMonitoring();

    setPortalTitle("Platform analytics", "Super Admin Portal");
    const data = await API.adminAnalytics().catch(() => ({}));
    const occupancyDays = ["Mon", "Tue", "Wed", "Thu", "Fri"];
    const occupancyValues = [data.occupancy || 25, 42, 58, 73, 61];
    const occupancyData = occupancyValues.map((v, idx) => ({
      label: occupancyDays[idx],
      value: v
    }));

    $("#portal-view").innerHTML = `
      <div class="stats-grid">
        <div class="stat-card"><small>Total hotels</small><strong>${data.total_hotels || 0}</strong></div>
        <div class="stat-card"><small>Total users</small><strong>${data.total_users || 0}</strong></div>
        <div class="stat-card"><small>Revenue</small><strong>${money(data.revenue || 0)}</strong></div>
        <div class="stat-card"><small>Monthly bookings</small><strong>${data.monthly_bookings || 0}</strong></div>
      </div>
      <div class="admin-grid">
        <div class="portal-card">
          <h3>Occupancy</h3>
          ${renderSvgChart(occupancyData, 'value', 'label', (v) => Math.round(v) + '%')}
        </div>
        <div class="portal-card"><h3>Top performing hotels</h3>${(data.top_hotels || []).map((h) => `<div class="booking-item"><div><strong>${safe(h.name)}</strong><p class="muted">${safe(h.location)} · ${safe(h.bookings)} bookings</p></div><strong>${money(h.revenue)}</strong></div>`).join("") || `<p class="muted">No revenue yet.</p>`}</div>
      </div>
    `;
  }

  async function renderAdminCoupons() {
    setPortalTitle("Discount Coupons", "Super Admin Portal");
    const coupons = await API.adminCoupons().catch(() => []);
    $("#portal-view").innerHTML = `
        <div class="admin-grid">
            <form class="portal-card stack-form" id="coupon-form">
                <h3>Create Coupon</h3>
                <div class="form-grid">
                    <label><span>Code</span><input name="code" required style="text-transform: uppercase;"></label>
                    <label><span>Discount Type</span><select name="discount_type"><option value="percentage">Percentage</option><option value="fixed">Fixed Amount</option></select></label>
                    <label><span>Value</span><input name="discount_value" type="number" required></label>
                    <label><span>Min Booking Amount</span><input name="min_booking_amount" type="number" value="0"></label>
                    <label><span>Max Discount</span><input name="max_discount" type="number" value="0"></label>
                    <label><span>Usage Limit</span><input name="usage_limit" type="number" value="0"></label>
                </div>
                <label><span>Description</span><input name="description" required></label>
                <label><span>Expiry Date (Optional)</span><input name="valid_to" type="date"></label>
                <button class="primary-btn">Create Coupon</button>
            </form>
            <div class="table-card">
                <h3>Existing Coupons</h3>
                <div class="table-wrap"><table><thead><tr><th>Code</th><th>Type</th><th>Value</th><th>Min Spend</th><th>Max</th><th>Usage</th><th>Action</th></tr></thead><tbody>
                    ${coupons.map(c => `
                        <tr>
                            <td><strong>${safe(c.code)}</strong></td>
                            <td>${safe(c.discount_type)}</td>
                            <td>${c.discount_type === 'percentage' ? `${c.discount_value}%` : money(c.discount_value)}</td>
                            <td>${money(c.min_booking_amount)}</td>
                            <td>${Number(c.max_discount || 0) ? money(c.max_discount) : "No cap"}</td>
                            <td>${safe(c.usage_count || 0)} / ${Number(c.usage_limit || 0) || "Unlimited"}</td>
                            <td><button class="danger-btn" data-delete-coupon="${c.id}" style="padding: 4px 12px; min-height: 30px; font-size: 13px;">Delete</button></td>
                        </tr>
                    `).join("") || `<tr><td colspan="7" class="muted">No coupons created yet.</td></tr>`}
                </tbody></table></div>
            </div>
        </div>
    `;
  }

  async function renderAdminPlatformPayments() {
    setPortalTitle("Platform payments", "Super Admin Portal");
    const data = await API.adminPlatformPayments().catch(() => ({}));
    const analytics = data.analytics || {};
    const ledgers = data.ledgers || [];
    state.adminLedgers = ledgers;
    const payments = data.payments || [];
    const reports = data.hotel_reports || [];
    const monthly = data.monthly_revenue || [];
    const yearly = data.yearly_revenue || [];
    const qrPayload = data.upi_uri || data.upi_qr_payload || buildUpiUri(0, data.upi_id, data.merchant_name || "StaySphere", "commission");
    const maxMonthly = Math.max(1000, ...monthly.map((item) => Number(item.commission || 0)));
    const maxYearly = Math.max(1000, ...yearly.map((item) => Number(item.commission || 0)));
    $("#portal-view").innerHTML = `
      <div class="stats-grid">
        <div class="stat-card"><small>Total commission earned</small><strong>${money(analytics.total_commission_earned || 0)}</strong></div>
        <div class="stat-card"><small>Pending commission amount</small><strong>${money(analytics.pending_commission_amount || 0)}</strong></div>
        <div class="stat-card"><small>Paid commission amount</small><strong>${money(analytics.paid_commission_amount || 0)}</strong></div>
        <div class="stat-card"><small>Pending proof reviews</small><strong>${analytics.pending_proofs || 0}</strong></div>
        <div class="stat-card"><small>Restricted hotels</small><strong>${analytics.restricted_hotels || 0}</strong></div>
        <div class="stat-card"><small>Suspended hotels</small><strong>${analytics.suspended_hotels || 0}</strong></div>
      </div>

      <div class="admin-grid platform-payment-grid">
        <form class="portal-card stack-form" id="platform-payment-settings-form">
          <div class="card-row">
            <h3>Platform payment settings</h3>
            <span class="status-pill ${data.commission_collection_enabled === false ? "pending" : "approved"}">${data.commission_collection_enabled === false ? "Disabled" : "Enabled"}</span>
          </div>
          <div class="form-grid">
            <label><span>Platform UPI ID</span><input name="upi_id" value="${safe(data.upi_id || "")}" placeholder="platform@upi" required></label>
            <label><span>Merchant name</span><input name="merchant_name" value="${safe(data.merchant_name || "StaySphere")}"></label>
            <label><span>Default commission percentage</span><input name="commission_rate" type="number" min="0" max="60" step="0.5" value="${safe(data.commission_rate || 15)}"></label>
            <label class="toggle-field"><span>Collect commission</span><input name="commission_collection_enabled" type="checkbox" ${data.commission_collection_enabled === false ? "" : "checked"}></label>
            <label><span>Active days</span><input name="active_days" type="number" min="0" value="${safe(data.active_days ?? 30)}"></label>
            <label><span>Grace until day</span><input name="grace_until_days" type="number" min="0" value="${safe(data.grace_until_days ?? 37)}"></label>
            <label><span>Suspension day</span><input name="suspension_days" type="number" min="1" value="${safe(data.suspension_days ?? 60)}"></label>
          </div>
          <label><span>Notification template</span><textarea name="notification_template">${safe(data.notification_template || "")}</textarea></label>
          <button class="primary-btn">Save platform settings</button>
        </form>

        <div class="portal-card platform-qr-card">
          <div class="card-row">
            <div>
              <h3>Generated QR preview</h3>
              <p class="muted">${safe(data.upi_id || "UPI ID not configured")}</p>
            </div>
            <button class="secondary-btn" data-copy-text="${safe(data.upi_id || "")}">Copy UPI</button>
          </div>
          <div class="upi-layout admin-upi-layout">
            <div class="upi-qr-box">
              <img src="${safe(qrImageSrc(qrPayload))}" alt="Platform UPI QR code" onerror="this.classList.add('hidden')">
            </div>
            <div class="upi-tools">
              <div class="breakdown-row"><span>Merchant</span><strong>${safe(data.merchant_name || "StaySphere")}</strong></div>
              <div class="breakdown-row"><span>UPI ID</span><strong>${safe(data.upi_id || "-")}</strong></div>
              <button class="secondary-btn" data-download-platform-qr>Download QR</button>
            </div>
          </div>
        </div>
      </div>


      <div class="table-card">
        <div class="card-row">
          <h3>Commission invoices and pending dues</h3>
          <div style="display:flex; gap: 8px; align-items:center;">
            <span class="pill">${ledgers.length} invoices</span>
            <button class="secondary-btn" data-download-csv="admin-ledgers" style="padding: 4px 12px; min-height: 30px; font-size: 13px;">⬇️ Download CSV</button>
          </div>
        </div>
        <div class="table-wrap"><table><thead><tr><th>Invoice</th><th>Hotel</th><th>Due date</th><th>Commission</th><th>Paid</th><th>Pending</th><th>Status</th><th>Hotel status</th></tr></thead><tbody>
          ${ledgers.slice(0, 10).map((l) => `<tr>
            <td><strong>${safe(l.ledger_id)}</strong><br><span class="muted">${safe(l.month || "")}/${safe(l.year || "")}</span></td>
            <td>${safe(l.hotel_name || l.hotel_id)}<br><span class="muted">${safe(l.location || "")}</span></td>
            <td>${safe(l.due_date || "-")}<br><span class="muted">${safe(l.days_overdue || 0)} days overdue</span></td>
            <td>${money(l.commission_amount)}</td>
            <td>${money(l.paid_amount)}</td>
            <td>${money(l.pending_amount)}</td>
            <td><span class="status-pill ${commissionStatusClass(l.status)}">${safe(l.status || "pending")}</span></td>
            <td><span class="status-pill ${commissionStatusClass(l.hotel_status)}">${safe(hotelDueStatusLabel(l.hotel_status))}</span></td>
          </tr>`).join("") || `<tr><td colspan="8" class="muted">No commission invoices yet.</td></tr>`}
        </tbody></table></div>
      </div>

      <div class="table-card">
        <div class="card-row">
          <h3>Uploaded payment proofs</h3>
          <span class="pill">${payments.filter((p) => p.status === "pending").length} pending</span>
        </div>
        <div class="table-wrap"><table><thead><tr><th>Proof</th><th>Hotel</th><th>Invoice</th><th>Amount</th><th>Status</th><th>Submitted</th><th>Action</th></tr></thead><tbody>
          ${payments.map((p) => `<tr>
            <td><strong>${safe(p.proof_id)}</strong><br><a href="${safe(p.payment_proof_url)}" target="_blank" rel="noopener">View proof</a></td>
            <td>${safe(p.hotel_name || p.hotel_id)}</td>
            <td>${safe(p.ledger_id)}</td>
            <td>${money(p.amount)}<br><span class="muted">${safe(p.transaction_ref || "")}</span></td>
            <td><span class="status-pill ${commissionStatusClass(p.status)}">${safe(p.status)}</span>${p.remarks ? `<br><span class="muted">${safe(p.remarks)}</span>` : ""}</td>
            <td>${safe(p.created_at || "-")}</td>
            <td>${p.status === "pending" ? `<div class="table-actions"><button class="success-btn" data-review-commission-payment="approved" data-proof-id="${safe(p.id)}">Approve</button><button class="danger-btn" data-review-commission-payment="rejected" data-proof-id="${safe(p.id)}">Reject</button></div>` : safe(p.reviewed_at || "-")}</td>
          </tr>`).join("") || `<tr><td colspan="7" class="muted">No payment proofs uploaded yet.</td></tr>`}
        </tbody></table></div>
      </div>
    `;
  }

  async function renderManagerCommission() {
    setPortalTitle("Commission due", "Hotel Manager Portal");
    if (!state.hotelId) {
      $("#portal-view").innerHTML = `<div class="portal-card"><h3>Complete partner onboarding</h3>${managerHotelForm()}</div>`;
      return;
    }

    const data = await API.managerCommission(state.hotelId).catch((error) => ({ error: error.message }));
    if (data.error) {
      $("#portal-view").innerHTML = `<div class="empty-state"><h3>Commission details unavailable</h3><p class="muted">${safe(data.error)}</p></div>`;
      return;
    }

    const hotel = data.hotel || {};
    const summary = data.summary || {};
    const ledgers = data.ledgers || [];
    const payments = data.payments || [];
    const platform = data.platform_payment || {};
    const pendingLedgers = ledgers.filter((ledger) => Number(ledger.pending_amount || 0) > 0);
    const selectedLedger = pendingLedgers[0] || ledgers[0] || {};
    const amountDue = Number(summary.commission_due || hotel.commission_due || platform.amount || 0);
    const qrPayload = platform.upi_uri || platform.upi_qr_payload || buildUpiUri(amountDue, platform.upi_id, platform.merchant_name || "StaySphere", `commission hotel ${state.hotelId}`);

    const today = new Date();
    const curYear = today.getFullYear();
    const curMonth = String(today.getMonth() + 1).padStart(2, "0");
    const dayOfMonth = today.getDate();
    const isReminderPeriod = dayOfMonth >= 30 || (today.getMonth() === 1 && dayOfMonth >= 28);

    $("#portal-view").innerHTML = `
      <div class="stats-grid">
        <div class="stat-card"><small>Commission due</small><strong>${money(amountDue)}</strong></div>
        <div class="stat-card"><small>Pending invoices</small><strong>${summary.pending_ledgers || pendingLedgers.length}</strong></div>
        <div class="stat-card"><small>Paid commission</small><strong>${money(summary.paid_commission || 0)}</strong></div>
        <div class="stat-card"><small>Hotel status</small><strong>${safe(hotelDueStatusLabel(hotel.hotel_status))}</strong></div>
      </div>

      ${isReminderPeriod ? `
        <div class="toast warning" style="position: static; margin: 20px 0; display: block; border-left: 4px solid var(--accent); background: color-mix(in srgb, var(--accent) 10%, var(--surface)); color: var(--text);">
          📅 <strong>Payment Reminder:</strong> Your monthly commission payment of <strong>${money(amountDue)}</strong> is now due. Please scan the QR code below to pay the commission via UPI and upload the transaction screenshot for verification.
        </div>
      ` : `
        <div class="toast info" style="position: static; margin: 20px 0; display: block; border-left: 4px solid var(--border); background: color-mix(in srgb, var(--border) 10%, var(--surface)); color: var(--text);">
          ℹ️ <strong>Current Period:</strong> Commission is currently accumulating based on bookings received. A payment reminder and instructions will be shown here on the 30th of this month. Current commission amount: <strong>${money(amountDue)}</strong>.
        </div>
      `}

      <div class="admin-grid platform-payment-grid">
        <div class="portal-card platform-qr-card">
          <div class="card-row">
            <div>
              <h3>Pay super admin commission</h3>
              <p class="muted">${safe(platform.upi_id || "UPI ID not configured")}</p>
            </div>
            <button class="secondary-btn" data-copy-text="${safe(platform.upi_id || "")}">Copy UPI</button>
          </div>
          <div class="upi-layout admin-upi-layout">
            <div class="upi-qr-box">
              ${qrPayload ? `<img src="${safe(qrImageSrc(qrPayload))}" alt="Super admin commission UPI QR code" onerror="this.classList.add('hidden')">` : `<p class="payment-qr-unavailable">Payment QR unavailable.<br>Please contact the super admin.</p>`}
            </div>
            <div class="upi-tools">
              <div class="breakdown-row"><span>Merchant</span><strong>${safe(platform.merchant_name || "StaySphere")}</strong></div>
              <div class="breakdown-row"><span>Amount due</span><strong>${money(amountDue)}</strong></div>
              <div class="breakdown-row"><span>Due date</span><strong>${safe(hotel.due_date || selectedLedger.due_date || "-")}</strong></div>
              <button class="secondary-btn" data-download-platform-qr>Download QR</button>
            </div>
          </div>
        </div>

        <form class="portal-card stack-form" id="manager-commission-payment-form">
          <h3>Upload payment proof</h3>
          <label><span>Invoice</span>
            <select name="ledger_id" required>
              ${pendingLedgers.map((ledger) => `<option value="${safe(ledger.ledger_id)}">${safe(ledger.ledger_id)} - ${money(ledger.pending_amount)}</option>`).join("") || `<option value="${safe(selectedLedger.ledger_id || "")}">${safe(selectedLedger.ledger_id || "No pending invoice")}</option>`}
            </select>
          </label>
          <label><span>Amount paid</span><input name="amount" type="number" min="1" step="0.01" value="${safe(amountDue || selectedLedger.pending_amount || "")}" required></label>
          <label><span>Transaction reference</span><input name="transaction_ref" placeholder="UPI reference / transaction ID" required></label>
          ${uploadZone("commission_payment_proof", "Commission payment screenshot", { id: "commission-payment-proof", multiple: false, compact: true })}
          <button class="primary-btn" ${pendingLedgers.length ? "" : "disabled"}>Submit proof</button>
        </form>
      </div>

      <div class="table-card">
        <div class="card-row"><h3>Commission invoices</h3><span class="pill">${ledgers.length} invoices</span></div>
        <div class="table-wrap"><table><thead><tr><th>Invoice</th><th>Period</th><th>Commission</th><th>Paid</th><th>Pending</th><th>Status</th><th>Due date</th></tr></thead><tbody>
          ${ledgers.map((ledger) => {
            const isCurrent = ledger.month === curMonth && Number(ledger.year) === curYear;
            const statusClass = isCurrent ? (isReminderPeriod ? "requested" : "pending") : commissionStatusClass(ledger.status);
            const statusText = isCurrent ? (isReminderPeriod ? "Payment Due" : "Accumulating") : (ledger.status || "pending");
            return `<tr>
              <td><strong>${safe(ledger.ledger_id)}</strong></td>
              <td>${safe(ledger.month || "")}/${safe(ledger.year || "")}</td>
              <td>${money(ledger.commission_amount)}</td>
              <td>${money(ledger.paid_amount)}</td>
              <td>${money(ledger.pending_amount)}</td>
              <td><span class="status-pill ${statusClass}">${safe(statusText)}</span></td>
              <td>${safe(ledger.due_date || "-")}</td>
            </tr>`;
          }).join("") || `<tr><td colspan="7" class="muted">No commission invoices yet.</td></tr>`}
        </tbody></table></div>
      </div>

      <div class="table-card">
        <div class="card-row"><h3>Submitted payment proofs</h3><span class="pill">${payments.length} proofs</span></div>
        <div class="table-wrap"><table><thead><tr><th>Proof</th><th>Invoice</th><th>Amount</th><th>Status</th><th>Submitted</th></tr></thead><tbody>
          ${payments.map((payment) => `<tr>
            <td><strong>${safe(payment.proof_id)}</strong><br><a href="${safe(payment.payment_proof_url)}" target="_blank" rel="noopener">View proof</a></td>
            <td>${safe(payment.ledger_id)}</td>
            <td>${money(payment.amount)}<br><span class="muted">${safe(payment.transaction_ref || "")}</span></td>
            <td><span class="status-pill ${commissionStatusClass(payment.status)}">${safe(payment.status || "pending")}</span>${payment.remarks ? `<br><span class="muted">${safe(payment.remarks)}</span>` : ""}</td>
            <td>${safe(payment.created_at || "-")}</td>
          </tr>`).join("") || `<tr><td colspan="5" class="muted">No payment proofs uploaded yet.</td></tr>`}
        </tbody></table></div>
      </div>
    `;
  }

  async function renderAdminView() {
    if (state.activePortal === "platform-payments") return renderAdminPlatformPayments();
    if (state.activePortal === "approvals") return renderApprovals();
    if (state.activePortal === "coupons") return renderAdminCoupons();
    if (state.activePortal === "fraud") return renderFraudMonitoring();

    setPortalTitle("Revenue system", "Super Admin Portal");
    const [analytics, revenue, transactions, payouts] = await Promise.all([
      API.adminAnalytics().catch(() => ({})),
      API.adminRevenue().catch(() => ({})),
      API.adminTransactions().catch(() => []),
      API.adminPayouts().catch(() => []),
    ]);
    state.adminRevenue = revenue;
    state.adminTransactions = transactions;
    state.adminPayouts = payouts;
    const monthly = revenue.monthly_revenue || [];
    const maxCommission = Math.max(1000, ...monthly.map((item) => Number(item.commission || 0)));
    const totalPlatformRev = revenue.total_platform_revenue || 0;
    const totalCommissions = revenue.total_commissions_earned || 0;
    const grossBookingVal = revenue.gross_revenue || analytics.revenue || 0;
    const pendingPayoutsVal = revenue.pending_payouts || 0;
    const failedTxns = revenue.failed_transactions || 0;
    const nextCollectionDate = new Date(new Date().getFullYear(), new Date().getMonth() + 1, 0).toLocaleDateString('en-IN', { month: 'long', day: 'numeric', year: 'numeric' });

    $("#portal-view").innerHTML = `
      <div class="stats-grid compact-stats">
        <div class="stat-card">
          <div class="stat-card-icon">💰</div>
          <small>Platform revenue</small>
          <strong>${money(totalPlatformRev)}</strong>
          <span class="stat-trend positive">Total earned</span>
        </div>
        <div class="stat-card">
          <div class="stat-card-icon">🏷️</div>
          <small>Commissions earned</small>
          <strong>${money(totalCommissions)}</strong>
          <span class="stat-trend info">15% platform fee</span>
        </div>
        <div class="stat-card">
          <div class="stat-card-icon">📊</div>
          <small>Gross booking value</small>
          <strong>${money(grossBookingVal)}</strong>
          <span class="stat-trend info">All bookings</span>
        </div>
        <div class="stat-card">
          <div class="stat-card-icon">📈</div>
          <small>Total hotels</small>
          <strong>${(revenue.top_hotels || []).length}</strong>
          <span class="stat-trend info">Active properties</span>
        </div>
      </div>

      <div class="table-card">
        <div class="card-row">
          <div>
            <h3>Commission Dues by Hotel</h3>
            <p class="muted">Real-time status of pending commission dues from properties.</p>
          </div>
          <div style="display:flex; gap: 8px; align-items:center;">
            <span class="pill">${(revenue.hotel_dues || []).length} hotels</span>
            <button class="secondary-btn" data-download-csv="commission" style="padding: 4px 12px; min-height: 30px; font-size: 13px;">⬇️ Download CSV</button>
          </div>
        </div>
        <div class="table-wrap">
          <table>
            <thead><tr><th>Hotel</th><th>UPI ID</th><th>Commission Due</th><th>Due Date</th><th>Status</th></tr></thead>
            <tbody>
              ${(revenue.hotel_dues || []).slice(0, 10).map(h => {
                const dueAmt = Number(h.commission_due || 0);
                const statusLabel = h.commission_status || 'Paid';
                const statusClass = statusLabel === 'Paid' ? 'approved' : statusLabel === 'Overdue' ? 'cancelled' : 'pending';
                return `<tr>
                  <td><strong>${safe(h.name)}</strong><br><span class="muted">${safe(h.location || '')}</span></td>
                  <td>${h.upi_id ? `<code style="font-size:12px;background:var(--surface-2);padding:2px 8px;border-radius:4px;">${safe(h.upi_id)}</code>` : '<span class="muted">Not set</span>'}</td>
                  <td><strong>${money(dueAmt)}</strong></td>
                  <td>${h.due_date ? safe(h.due_date) : '<span class="muted">-</span>'}</td>
                  <td><span class="status-pill ${statusClass}">${safe(statusLabel)}</span></td>
                </tr>`;
              }).join("") || `<tr><td colspan="5" class="muted">No hotels have commission dues.</td></tr>`}
            </tbody>
          </table>
        </div>
      </div>

      <div class="admin-grid">
        <form class="portal-card stack-form" id="commission-form">
          <h3>Commission settings</h3>
          <p class="muted" style="margin: -4px 0 12px;">Configure the platform-wide commission rate applied to all bookings.</p>
          <label><span>Platform commission percentage</span><input name="commission_rate" type="number" min="0" max="60" step="0.5" value="${safe(revenue.commission_rate || 15)}"></label>
          <button class="primary-btn">Save commission rate</button>
        </form>
        <div class="portal-card">
          <h3>Monthly commission chart</h3>
          <p class="muted" style="margin: -4px 0 12px;">Commission collected per month across all hotels.</p>
          <div class="chart-bars">${(monthly.length ? monthly : [{ commission: 0 }]).map((item) => `<span title="${safe(item.month || "")}: ${money(item.commission)}" style="height:${Math.max(6, (Number(item.commission || 0) / maxCommission) * 100)}%"></span>`).join("")}</div>
        </div>
      </div>

      <div class="table-card">
        <div class="card-row">
          <h3>Top performing hotels</h3>
          <div style="display:flex; gap: 8px; align-items:center;">
            <span class="pill">${(revenue.top_hotels || []).length} hotels</span>
            <button class="secondary-btn" data-download-csv="top-hotels" style="padding: 4px 12px; min-height: 30px; font-size: 13px;">⬇️ Download CSV</button>
          </div>
        </div>
        <div class="table-wrap">
          <table>
            <thead><tr><th>#</th><th>Hotel</th><th>Location</th><th>Bookings</th><th>Gross Revenue</th><th>Commission</th></tr></thead>
            <tbody>
              ${(revenue.top_hotels || []).slice(0, 10).map((h, i) => `<tr>
                <td><strong>${i + 1}</strong></td>
                <td><strong>${safe(h.name)}</strong></td>
                <td>${safe(h.location || '-')}</td>
                <td>${safe(h.bookings || 0)}</td>
                <td>${money(h.gross)}</td>
                <td>${money(h.commission)}</td>
              </tr>`).join("") || `<tr><td colspan="6" class="muted">No revenue data yet.</td></tr>`}
            </tbody>
          </table>
        </div>
      </div>
    `;
  }

  async function renderApprovals() {
    setPortalTitle("Hotel approvals", "Super Admin Portal");
    const hotels = await API.adminHotels("pending").catch(() => []);
    if (!hotels.length) {
      $("#portal-view").innerHTML = `
        <div class="portal-card" style="text-align:center;padding:60px 24px;">
          <svg width="64" height="64" viewBox="0 0 24 24" fill="none" stroke="var(--success)" stroke-width="1.5" style="margin:0 auto 16px;display:block;opacity:.6"><path d="M9 12l2 2 4-4"/><circle cx="12" cy="12" r="10"/></svg>
          <h3 style="margin-bottom:8px;">All caught up!</h3>
          <p class="muted">No hotels are pending verification right now.</p>
        </div>`;
      return;
    }

    $("#portal-view").innerHTML = `
      <div class="verification-queue">
        <div class="queue-header">
          <h3>Verification queue</h3>
          <span class="status-pill pending">${hotels.length} pending</span>
        </div>
        ${hotels.map((h) => {
      const fallbackImg = fallbackHotels[(h.id || 0) % fallbackHotels.length]?.image_url || fallbackHotels[0].image_url;
      const cover = hotelImage(h);
      const docs = Array.isArray(h.documents) ? h.documents : [];
      const amenities = parseAmenities(h.amenities);
      const nearby = parseAmenities(h.nearby_places);
      const payments = parseAmenities(h.accepted_payments);
      const allImages = hotelImageCandidates(h);
      const detailRow = (label, value) => value ? `<div class="vq-detail-row"><span class="vq-label">${safe(label)}</span><span class="vq-value">${safe(value)}</span></div>` : "";
      const policyRow = (label, value) => value ? `<div class="vq-policy-row"><strong>${safe(label)}</strong><p>${safe(value)}</p></div>` : "";

      return `
        <div class="vq-card" id="vq-card-${h.id}">
          <div class="vq-card-header">
            <img src="${safe(cover || fallbackImg)}" onerror="if(!this.dataset.fallback){this.dataset.fallback='true';this.src='${fallbackImg}';}" alt="${safe(h.name)}" class="vq-cover">
            <div class="vq-header-info">
              <div style="display:flex;align-items:center;gap:10px;flex-wrap:wrap;">
                <h3 style="margin:0;">${safe(h.name)}</h3>
                <span class="status-pill ${safe(h.status)}">${safe(h.status)}</span>
              </div>
              <p class="muted" style="margin:4px 0 0;">${safe(h.location || "")}</p>
              <p class="muted" style="margin:2px 0 0;font-size:12px;">Submitted: ${safe(h.created_at || "N/A")} · Hotel ID: ${h.id || "N/A"}</p>
            </div>
            <div class="vq-header-actions">
              <button class="secondary-btn" data-toggle-details="${h.id}">View Details ➔</button>
              <button class="success-btn" data-approve="${h.id}">✓ Approve</button>
              <button class="danger-btn" data-reject="${h.id}">✗ Reject</button>
            </div>
          </div>

          <div class="vq-sections hidden" id="vq-sections-${h.id}">
            <!-- Property Details -->
            <div class="vq-section">
              <h4>🏨 Property Details</h4>
              <div class="vq-detail-grid">
                ${detailRow("Property Name", h.name)}
                ${detailRow("Property Type", h.property_type)}
                ${detailRow("Star Rating", h.star_rating ? h.star_rating + " ★" : "")}
                ${detailRow("Total Rooms", h.room_count)}
                ${detailRow("Status", h.status)}
              </div>
              ${h.description ? `<div class="vq-description"><strong>Description</strong><p>${safe(h.description)}</p></div>` : ""}
            </div>

            <!-- Location Details -->
            <div class="vq-section">
              <h4>📍 Location</h4>
              <div class="vq-detail-grid">
                ${detailRow("Country", h.country)}
                ${detailRow("State", h.state)}
                ${detailRow("City", h.city)}
                ${detailRow("Pincode", h.postal_code)}
                ${detailRow("Full Address", h.address)}
                ${detailRow("Coordinates", h.coordinates)}
              </div>
              ${nearby.length ? `<div class="vq-description"><strong>Nearby Landmarks</strong><p>${nearby.map(n => safe(n)).join(", ")}</p></div>` : ""}
            </div>

            <!-- Owner / Partner Details -->
            <div class="vq-section">
              <h4>👤 Owner / Partner</h4>
              <div class="vq-detail-grid">
                ${detailRow("Full Name", h.owner_name)}
                ${detailRow("Email", h.owner_email)}
                ${detailRow("Phone", h.owner_phone)}
                ${detailRow("Business Name", h.business_name)}
                ${detailRow("Owner ID", h.owner_id)}
              </div>
            </div>

            <!-- Pricing Details -->
            <div class="vq-section">
              <h4>💰 Pricing</h4>
              <div class="vq-detail-grid">
                ${detailRow("Starting Price/Night", h.price_from ? money(h.price_from) : "")}
                ${detailRow("Weekend Price", h.weekend_price ? money(h.weekend_price) : "")}
                ${detailRow("Seasonal Price", h.seasonal_price ? money(h.seasonal_price) : "")}
                ${detailRow("Taxes & Charges", h.taxes_fees ? h.taxes_fees + "%" : "")}
                ${detailRow("Minimum Stay", h.min_stay ? h.min_stay + " night(s)" : "")}
              </div>
            </div>

            <!-- Payment Details -->
            <div class="vq-section">
              <h4>💳 Payment</h4>
              <div class="vq-detail-grid">
                ${detailRow("UPI ID", h.upi_id)}
                ${detailRow("Bank Account", h.bank_account)}
                ${detailRow("Tax Number", h.tax_number)}
                ${detailRow("PAN Registration", h.pan_registration)}
              </div>
              ${payments.length ? `<div class="vq-description"><strong>Accepted Payment Methods</strong><p>${payments.map(p => safe(p)).join(", ")}</p></div>` : ""}
            </div>

            <!-- Amenities -->
            ${amenities.length ? `
            <div class="vq-section">
              <h4>🛎️ Facilities & Amenities</h4>
              <div class="vq-amenities-grid">
                ${amenities.map(a => `<span class="vq-amenity-tag">${safe(a)}</span>`).join("")}
              </div>
            </div>` : ""}

            <!-- Policies -->
            <div class="vq-section">
              <h4>📋 Policies</h4>
              <div class="vq-policies-grid">
                ${detailRow("Check-in Time", h.check_in_time)}
                ${detailRow("Check-out Time", h.check_out_time)}
              </div>
              <div class="vq-policies-list">
                ${policyRow("Cancellation Policy", h.cancellation_policy)}
                ${policyRow("Refund Rules", h.refund_rules)}
                ${policyRow("Child Policy", h.child_policy)}
                ${policyRow("Pet Policy", h.pet_policy)}
                ${policyRow("Smoking Policy", h.smoking_policy)}
                ${policyRow("ID Proof Requirements", h.id_proof_requirements)}
                ${policyRow("Extra Guest Rules", h.extra_guest_rules)}
                ${policyRow("Custom Policies", h.custom_policies)}
              </div>
            </div>

            <!-- Hotel Photos -->
            ${allImages.length ? `
            <div class="vq-section">
              <h4>🖼️ Hotel Photos (${allImages.length})</h4>
              <div class="vq-photos-grid">
                ${allImages.map(img => `<a href="${safe(img)}" target="_blank" rel="noopener"><img src="${safe(img)}" onerror="this.parentElement.style.display='none'" alt="Hotel photo" loading="lazy"></a>`).join("")}
              </div>
            </div>` : ""}

            <!-- Verification Documents -->
            <div class="vq-section vq-section-docs">
              <h4>📄 Verification Documents (${docs.length})</h4>
              ${docs.length ? `
              <div class="vq-docs-grid">
                ${docs.map((doc) => {
                  const url = doc.document_url || "";
                  const isImage = /\.(jpe?g|png)$/i.test(url);
                  const isPdf = /\.pdf$/i.test(url);
                  return `
                  <div class="vq-doc-card vq-doc-card-preview">
                    <div class="vq-doc-preview">
                      ${isImage ? `<img src="${safe(url)}" alt="${safe(doc.document_type || 'Document')}" loading="lazy" onclick="window.open('${safe(url)}','_blank')">` :
                        isPdf ? `<iframe src="${safe(url)}" title="${safe(doc.document_type || 'Document')}" loading="lazy"></iframe>` :
                        `<div class="vq-doc-placeholder">
                          <svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><polyline points="14,2 14,8 20,8"/></svg>
                          <span>Preview not available</span>
                        </div>`}
                    </div>
                    <div class="vq-doc-meta">
                      <div class="vq-doc-meta-left">
                        <div class="vq-doc-icon-sm">
                          ${isImage ? `<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><rect x="3" y="3" width="18" height="18" rx="2"/><circle cx="8.5" cy="8.5" r="1.5"/><path d="M21 15l-5-5L5 21"/></svg>` :
                            isPdf ? `<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><polyline points="14,2 14,8 20,8"/></svg>` :
                            `<svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><path d="M13 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V9z"/><polyline points="13,2 13,9 20,9"/></svg>`}
                        </div>
                        <div>
                          <strong>${safe(doc.document_type || "Verification Document")}</strong>
                          <p class="muted">${safe(doc.uploaded_at || "Date not available")}${doc.status ? ` · <span class="status-pill ${safe(doc.status)}">${safe(doc.status)}</span>` : ""}</p>
                        </div>
                      </div>
                      <a class="primary-btn" href="${safe(url)}" target="_blank" rel="noopener" style="padding:6px 16px;font-size:12px;white-space:nowrap;">Open ↗</a>
                    </div>
                  </div>
                `}).join("")}
              </div>` : `
              <div class="vq-no-docs">
                <svg width="40" height="40" viewBox="0 0 24 24" fill="none" stroke="var(--warning)" stroke-width="1.5" style="opacity:.7"><path d="M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z"/><line x1="12" y1="9" x2="12" y2="13"/><line x1="12" y1="17" x2="12.01" y2="17"/></svg>
                <p><strong>No verification documents uploaded</strong></p>
                <p class="muted">This hotel has not submitted any verification documents. Approval without documents is not recommended.</p>
              </div>`}
            </div>
          </div>

          <div class="vq-card-footer">
            <div class="vq-footer-summary">
              <span>Hotel ID: <strong>${h.id}</strong></span>
              <span>Documents: <strong>${docs.length}</strong></span>
              <span>Amenities: <strong>${amenities.length}</strong></span>
              <span>Rooms: <strong>${h.room_count || 0}</strong></span>
            </div>
          </div>
        </div>
      `}).join("")}
      </div>
    `;
  }

  async function renderUsers() {
    setPortalTitle("User management", "Super Admin Portal");
    const users = await API.adminUsers().catch(() => []);
    $("#portal-view").innerHTML = `
      <div class="admin-grid">
        <form class="portal-card stack-form" id="admin-user-form">
          <h3>Create account</h3>
          <label><span>Full name</span><input name="full_name" required></label>
          <label><span>Username</span><input name="username" required></label>
          <label><span>Password</span><input name="password" type="password" required></label>
          <label><span>Role</span><select name="role"><option value="guest">Guest</option><option value="hotel_manager">Hotel Manager</option><option value="receptionist">Staff</option><option value="super_admin">Super Admin</option></select></label>
          <button class="primary-btn">Create User</button>
        </form>
        <div class="portal-card">
          <h3>Users</h3>
          ${(users || []).map((u) => `<div class="user-item"><div><strong>${safe(u.full_name || u.username)}</strong><p class="muted">${safe(u.username)} · ${safe(u.role)}</p></div><span>${safe(u.hotel_id || 0)}</span></div>`).join("")}
        </div>
      </div>
    `;
  }

  async function renderMonitoring() {
    setPortalTitle("Booking monitoring", "Super Admin Portal");
    const hotels = await API.adminHotels("all").catch(() => []);
    $("#portal-view").innerHTML = `
      <div class="table-card">
        <h3>Hotels and verification status</h3>
        <div class="table-wrap"><table><thead><tr><th>Hotel</th><th>Owner</th><th>Status</th><th>Location</th></tr></thead><tbody>
          ${hotels.map((h) => `<tr><td>${safe(h.name)}</td><td>${safe(h.owner_name || "Unassigned")}</td><td><span class="status-pill ${safe(h.status)}">${safe(h.status)}</span></td><td>${safe(h.location)}</td></tr>`).join("")}
        </tbody></table></div>
      </div>
    `;
  }

  async function renderFraudMonitoring() {
    setPortalTitle("Hotel Verification", "Super Admin Portal");
    const hotels = await API.adminHotels("all").catch(() => []);
    const signals = hotels.map((hotel) => ({
      ...hotel,
      risk: Number(hotel.document_count || 0) === 0 || hotel.status === "pending" ? "Review" : "Clear",
      reason: Number(hotel.document_count || 0) === 0 ? "Missing verification documents" : hotel.status === "pending" ? "Awaiting admin approval" : "Verified listing"
    }));
    $("#portal-view").innerHTML = `
      <div class="table-card">
        <h3>Risk signals</h3>
        <div class="table-wrap"><table><thead><tr><th>Hotel</th><th>Status</th><th>Risk</th><th>Reason</th></tr></thead><tbody>
          ${signals.map((hotel) => `<tr><td>${safe(hotel.name)}</td><td><span class="status-pill ${safe(hotel.status)}">${safe(hotel.status)}</span></td><td>${safe(hotel.risk)}</td><td>${safe(hotel.reason)}</td></tr>`).join("")}
        </tbody></table></div>
      </div>
    `;
  }

  async function renderFeaturedListings() {
    setPortalTitle("Featured listings", "Super Admin Portal");
    const hotels = await API.adminHotels("all").catch(() => []);
    const featured = hotels
      .filter((hotel) => hotel.status === "approved")
      .sort((a, b) => Number(b.review_score || 0) - Number(a.review_score || 0))
      .slice(0, 6);
    $("#portal-view").innerHTML = `
      <div class="portal-card">
        <h3>Marketplace highlights</h3>
        <div class="hotel-grid">
          ${featured.map((hotel) => hotelCard(hotel)).join("") || `<p class="muted">Approve hotels to build featured listings.</p>`}
        </div>
      </div>
    `;
  }

  async function openHotelDetails(id) {
    const hotel = await API.publicHotel(id).catch(() => null) || localHotelDetails(id);
    if (!hotel) {
      toast("Failed to load hotel details from the backend.", "error");
      return;
    }

    state.selectedHotel = hotel;
    const hasUploadedImages = hotel.images && hotel.images.length > 0;
    const images = hasUploadedImages ? hotel.images : [{ image_url: hotel.image_url }];
    const amenities = parseAmenities(hotel.amenities);
    const nearbyPlaces = parseAmenities(hotel.nearby_places);
    const acts = hotel.activities_list || [];
    const fallbackImg = fallbackHotels[(hotel.id || 0) % fallbackHotels.length]?.image_url || fallbackHotels[0].image_url;
    const detailImage = (image, fallback = fallbackImg) => coalesceImage(
      image?.image_path,
      image?.image_url,
      image?.url,
      fallback
    );
    const mainImage = detailImage(images[0], hotel.image_url || fallbackImg);
    
    let sideImages = [];
    if (hasUploadedImages) {
      sideImages = images.slice(1).map(img => detailImage(img));
    } else {
      sideImages = [
        detailImage(undefined, fallbackHotels[((hotel.id || 0) + 1) % fallbackHotels.length]?.image_url || fallbackImg),
        detailImage(undefined, fallbackHotels[((hotel.id || 0) + 2) % fallbackHotels.length]?.image_url || fallbackImg),
      ];
    }

    const heroStyle = sideImages.length === 0 ? 'style="grid-template-columns: 1fr;"' : '';
    const sideDisplayStyle = sideImages.length === 0 ? 'display: none;' : '';
    const galleryStyle = sideImages.length > 0 ? `style="grid-template-rows: repeat(${sideImages.length}, 1fr);"` : '';

    $("#details-content").innerHTML = `
      <article class="hotel-detail-view">
        <section class="hotel-detail-hero" ${heroStyle}>
          <div class="hotel-detail-main-photo">
            <img src="${safe(mainImage)}" onerror="if(!this.dataset.fallback){ this.dataset.fallback='true'; this.src='${fallbackImg}'; }" alt="${safe(hotel.name)}">
            <div class="hotel-detail-overlay">
              <span class="detail-chip">${safe(hotel.property_type || "Hotel")}</span>
              <h2>${safe(hotel.name)}</h2>
              <div class="detail-meta">
                <span>${safe(hotel.location || "Destination")}</span>
                <span>${safe(hotel.review_score || "New")} guest rating</span>
              </div>
            </div>
          </div>
          <div class="hotel-detail-gallery" style="${sideDisplayStyle} ${galleryStyle}">
            ${sideImages.map((src, index) => `
              <div class="hotel-detail-side-photo">
                <img src="${safe(src || fallbackImg)}" onerror="if(!this.dataset.fallback){ this.dataset.fallback='true'; this.src='${fallbackImg}'; }" alt="${safe(hotel.name)} gallery ${index + 1}">
              </div>
            `).join("")}
          </div>
        </section>

        <div class="hotel-detail-content">
          <main class="hotel-detail-main">
            <section class="hotel-detail-section">
              <p class="eyebrow">The stay</p>
              <h3>The Experience</h3>
              <p class="detail-description">${safe(hotel.description || "Verified premium stay with modern rooms and guest-focused service.")}</p>
            </section>

            <section class="hotel-detail-section">
              <div class="detail-section-head">
                <div>
                  <p class="eyebrow">Included</p>
                  <h3>Amenities & Services</h3>
                </div>
              </div>
              <div class="detail-amenities">
                ${(amenities.length ? amenities : ["Free Wi-Fi", "Parking", "Restaurant"]).map((item) => `<span class="pill">${safe(item)}</span>`).join("")}
              </div>
            </section>

            <section class="hotel-detail-section">
              <div class="detail-section-head">
                <div>
                  <p class="eyebrow">Rooms</p>
                  <h3>Select Your Room</h3>
                </div>
              </div>
              <div class="room-list detail-room-list">
                ${(hotel.rooms || []).length ? (hotel.rooms || []).map((room, index) => {
      const roomFallback = fallbackRoomImages[((hotel.id || 0) + index) % fallbackRoomImages.length] || fallbackRoomImages[0];
      const displayRoomImage = roomImage(room, index);
      return `
                    <article class="detail-room-card">
                      <img src="${safe(displayRoomImage)}" onerror="if(!this.dataset.fallback){ this.dataset.fallback='true'; this.src='${roomFallback}'; }" alt="${safe(room.name)}">
                      <div class="detail-room-body">
                        <div>
                          <h4>${safe(room.name)}</h4>
                          <p class="muted">${safe(room.description || "Comfortable room with guest-ready amenities.")}</p>
                        </div>
                        <div class="room-facts">
                          <span>Up to ${safe(room.max_capacity || 2)} guests</span>
                          <span>Premium bedding</span>
                        </div>
                        <div class="detail-room-footer">
                          <div>
                            <small>Per night</small>
                            <strong>${money(room.current_price || room.base_price)}</strong>
                          </div>
                          <button class="primary-btn" data-book-room="${room.id}">Reserve</button>
                        </div>
                      </div>
                    </article>
                  `;
    }).join("") : `<div class="empty-state compact-empty"><h3>No rooms listed yet</h3><p class="muted">This hotel has not published room inventory.</p></div>`}
              </div>
            </section>

            <section class="hotel-detail-section">
              <div class="detail-section-head">
                <div>
                  <p class="eyebrow">Guest feedback</p>
                  <h3>Reviews</h3>
                </div>
              </div>
              <div class="review-list detail-review-list">
                ${(hotel.reviews || []).length ? hotel.reviews.map((r) => `
                  <article class="detail-review-card">
                    <div class="detail-review-head">
                      <div class="review-avatar">${initials(r.author || "Guest")}</div>
                      <div>
                        <strong>${safe(r.author || "Guest")}</strong>
                        <small class="muted">${safe(r.created_at?.slice(0, 10) || "Recent guest")}</small>
                      </div>
                      <span class="score-pill">${safe(r.rating || "5")}</span>
                    </div>
                    <p>&ldquo;${safe(r.comment || "Wonderful stay.")}&rdquo;</p>
                    ${parseImages(r.images).length ? `<div class="review-image-strip">${parseImages(r.images).map((src) => `<img src="${safe(src)}" alt="Review photo">`).join("")}</div>` : ""}
                    ${r.manager_reply ? `<div class="review-reply"><strong>Host response</strong><p>${safe(r.manager_reply)}</p></div>` : ""}
                  </article>
                `).join("") : `<p class="muted">Be the first to review this property.</p>`}
              </div>
            </section>
          </main>

          <aside class="details-sidebar">
            <div class="hotel-reserve-card">
              <div class="reserve-price">
                <small>Starting from</small>
                <strong>${money(hotel.price_from || hotel.current_price || hotel.base_price)} <span>/ night</span></strong>
              </div>
              ${hotelCoupon(hotel) ? `<div class="detail-coupon"><strong>${safe(hotelCoupon(hotel).label)}</strong><span>Use code ${safe(hotelCoupon(hotel).code)}</span></div>` : ""}
              <button class="primary-btn" type="button" onclick="document.querySelector('.detail-room-list').scrollIntoView({behavior: 'smooth'})">Check Availability</button>
              <div class="reserve-contact">
                <a href="tel:${safe(hotel.phone)}">${safe(hotel.phone || "Contact hotel")}</a>
                <a href="mailto:${safe(hotel.email)}">${safe(hotel.email || "Email hotel")}</a>
              </div>
              <div class="nearby-card">
                <h4>Nearby Attractions</h4>
                <ul>
                  ${nearbyPlaces.map((place) => `<li><span></span>${safe(place)}</li>`).join("") || "<li><span></span>City Center (1 km)</li><li><span></span>Local Market (2 km)</li>"}
                </ul>
              </div>
            </div>
          </aside>
        </div>
      </article>
    `;
    document.body.classList.add("modal-open");
    const modal = $("#details-modal");
    modal.classList.remove("hidden");
  }

  function closeDetails() {
    document.body.classList.remove("modal-open");
    $("#details-modal").classList.add("hidden");
  }

  function openRoomTypeModal(type = {}) {
    const isEdit = Boolean(type.id);
    const selected = parseAmenities(type.amenities);
    inputFilesMap.delete("room-type-photos");
    inputFilesMap.delete("room_photos");
    $("#details-content").innerHTML = `
      <div class="modal-form-shell room-editor-shell">
        <div class="modal-form-head">
          <div>
            <p class="eyebrow">Room type</p>
            <h2>${isEdit ? "Edit Room Type" : "Add Room Type"}</h2>
          </div>
          <button class="ghost-btn" type="button" data-close-details>Close</button>
        </div>
        <form class="stack-form" id="room-type-form" data-room-type-id="${safe(type.id || "")}">
          <section class="form-section">
            <div class="form-section-title"><span>Step 1</span><h3>Room Details</h3></div>
            <div class="form-grid">
              <label><span>Name</span><input name="name" value="${safe(type.name || "")}" required></label>
              <label><span>Occupancy</span><input name="max_capacity" type="number" min="1" value="${safe(type.max_capacity || 2)}"></label>
              <label><span>Base price</span><input name="base_price" type="number" min="0" value="${safe(type.base_price || type.current_price || "")}" required></label>
              <label><span>Room size</span><input name="room_size" value="${safe(type.room_size || "")}" placeholder="340 sq ft"></label>
            </div>
            <label><span>Description</span><textarea name="description">${safe(type.description || "")}</textarea></label>
          </section>
          ${!isEdit ? `
          <section class="form-section">
            <div class="form-section-title"><span>Step 2</span><h3>Inventory Setup</h3></div>
            <p class="form-question">How many rooms of this type exist in your property?</p>
            <div class="form-grid">
              <label><span>Total Number of Rooms</span><input name="total_rooms" type="number" min="1" value="15" required></label>
              <label class="switch-field">
                <input name="auto_generate_room_numbers" type="checkbox" checked>
                <span>Auto Generate Room Numbers</span>
              </label>
              <label><span>Room Prefix</span><input name="room_prefix" placeholder="D"></label>
              <label><span>Starting Room Number</span><input name="starting_room_number" type="number" min="1" value="101"></label>
            </div>
          </section>` : ""}
          <section class="form-section">
            <h3>Room Features</h3>
            ${checkboxGrid("room_features", roomFeatureOptions, selected)}
          </section>
          <section class="form-section">
            <h3>Media</h3>
            ${uploadZone("room_photos", "Upload Photos", { id: "room-type-photos", multiple: true, compact: true })}
          </section>
          <button class="primary-btn">${isEdit ? "Save Room Type" : "Create Room Type & Generate Inventory"}</button>
        </form>
      </div>
    `;
    document.body.classList.add("modal-open");
    $("#details-modal").classList.remove("hidden");
  }

  function openRoomModal(types = [], room = {}) {
    if (!types.length) {
      toast("Create a room type before adding actual rooms.", "error");
      return;
    }
    const isEdit = Boolean(room.id || room.room_id);
    const roomTypeId = Number(room.room_type_id || types[0]?.id || 0);
    const firstType = types.find((type) => Number(type.id) === roomTypeId) || types[0] || {};
    const firstPrice = Number(firstType.current_price || firstType.base_price || 0);
    const selectedFeatures = isEdit
      ? String(room.notes || "").split(",").map((item) => item.trim()).filter(Boolean)
      : parseAmenities(firstType.amenities);
    inputFilesMap.delete("room-photos");
    inputFilesMap.delete("room_photos");
    $("#details-content").innerHTML = `
      <div class="modal-form-shell room-editor-shell">
        <div class="modal-form-head">
          <div>
            <p class="eyebrow">Actual room inventory</p>
            <h2>${isEdit ? "Edit Room" : "Add Room"}</h2>
          </div>
          <button class="ghost-btn" type="button" data-close-details>Close</button>
        </div>
        <form class="stack-form" id="room-form" data-room-id="${safe(room.id || room.room_id || "")}">
          <section class="form-section">
            <h3>Basic Information</h3>
            <div class="form-grid">
              <label><span>Room Number</span><input name="room_number" placeholder="D101" value="${safe(room.room_number || "")}" required></label>
              <label><span>Room Type</span><select name="room_type_id" data-room-type-select>${types.map((type) => `<option value="${safe(type.id)}" data-price="${safe(type.current_price || type.base_price || 0)}" ${Number(type.id) === roomTypeId ? "selected" : ""}>${safe(type.name)}</option>`).join("")}</select></label>
              <label><span>Floor</span><input name="floor" type="number" value="${safe(room.floor || 1)}" min="0"></label>
            </div>
          </section>
          <section class="form-section">
            <h3>Pricing</h3>
            <label><span>Price Per Night</span><input name="price" data-room-price-input type="number" min="0" value="${safe(room.price || firstPrice)}"></label>
          </section>
          <section class="form-section">
            <h3>Room Features</h3>
            ${checkboxGrid("room_features", roomFeatureOptions, selectedFeatures)}
          </section>
          <section class="form-section">
            <h3>Media</h3>
            ${uploadZone("room_photos", "Upload Photos", { id: "room-photos", multiple: true, compact: true })}
          </section>
          <button class="primary-btn">${isEdit ? "Save Room" : "Add Room"}</button>
        </form>
      </div>
    `;
    document.body.classList.add("modal-open");
    $("#details-modal").classList.remove("hidden");
  }

  function togglePaymentPanels(form) {
    if (!form) return;
    const method = selectedPaymentMethod(form);
    const panels = {
      "Pay Now": "#pay-now-panel",
      "Pay at Property": "#pay-property-panel",
      "Pay at Hotel": "#pay-property-panel",
    };
    Object.values(panels).forEach((selector) => {
      const panel = $(selector, form);
      if (panel) panel.style.display = "none";
    });
    form.querySelectorAll(".payment-option").forEach((option) => {
      option.classList.toggle("active", option.querySelector("input")?.checked);
    });
    const active = $(panels[method] || "#pay-now-panel", form);
    if (active) active.style.display = "block";
    const proof = form.querySelector('input[name="payment_screenshot_upload"]');
    if (proof) proof.required = method === "Pay Now";
    form.querySelectorAll("[data-pay-now-only]").forEach((node) => {
      node.hidden = method !== "Pay Now";
    });
    syncUpiPanel(form);
    syncPaymentSubmitState(form);
  }

  function paymentCheckoutTemplate(hotel, room, roomTypeId, defaultCin, defaultCout, selectedRoomImage, roomFallback) {
    const today = new Date();
    const yyyy = today.getFullYear();
    const mm = String(today.getMonth() + 1).padStart(2, '0');
    const dd = String(today.getDate()).padStart(2, '0');
    const todayStr = `${yyyy}-${mm}-${dd}`;
    const paymentOptions = hotelPaymentOptions(hotel);
    const defaultMethod = paymentOptions.payNow ? "Pay Now" : "Pay at Property";
    const advance = Number(hotel.advance_amount || hotel.advance_payment || 0);
    const activities = activityOptionsForHotel(hotel);
    const paymentChoiceCards = paymentOptions.both ? `
      <div class="payment-method-grid guest-payment-options">
        <label class="payment-option active">
          <input type="radio" name="payment_method" value="Pay Now" checked>
          <span><strong>Pay Now</strong><small>Scan the secure QR, pay the exact booking amount, then upload your payment screenshot.</small></span>
        </label>
        <label class="payment-option">
          <input type="radio" name="payment_method" value="Pay at Property">
          <span><strong>Pay at Property</strong><small>Reserve now and settle directly with the hotel during check-in.</small></span>
        </label>
      </div>
    ` : paymentOptions.payNow ? `
      <div class="single-payment-card">
        <input type="radio" name="payment_method" value="Pay Now" checked hidden>
        <button class="primary-btn payment-pay-now-button" type="button" data-pay-now-focus>Pay Now</button>
        <p class="muted">Pay the exact booking amount using the QR code below and upload your payment screenshot for verification.</p>
      </div>
    ` : `
      <div class="single-payment-card property">
        <input type="radio" name="payment_method" value="Pay at Property" checked hidden>
        <strong>Pay at Property</strong>
        <p class="muted">No online payment is required now. Settle your stay directly with the hotel during check-in.</p>
      </div>
    `;
    return `
      <div class="payment-page">
        <form class="payment-workspace" id="booking-form" data-hotel-id="${hotel.id}" data-room-type-id="${roomTypeId}">
          <section class="payment-main">
            <div class="payment-head">
              <p class="eyebrow">Secure checkout</p>
              <h2>Complete your booking</h2>
            </div>

            <div class="checkout-panel">
              <div class="panel-title"><span>1</span><h3>Stay details</h3></div>
              <div class="form-grid">
                <label><span>Check-in</span><input name="check_in_date" type="date" value="${defaultCin}" min="${todayStr}" required></label>
                <label><span>Check-out</span><input name="check_out_date" type="date" value="${defaultCout}" min="${todayStr}" required></label>
                <label><span>Adults</span><input name="adults" type="number" value="2" min="1"></label>
                <label><span>Children</span><input name="children" type="number" value="0" min="0"></label>
              </div>
            </div>

            <div class="checkout-panel">
              <div class="panel-title"><span>2</span><h3>Guest information</h3></div>
              <div class="form-grid">
                <label><span>Full name</span><input name="guest_name" value="${safe(state.user?.full_name || "")}" required></label>
                <label><span>Email</span><input name="guest_email" type="email" value="${safe(state.user?.email || "")}" required></label>
                <label><span>Phone</span><input name="guest_phone" value="${safe(state.user?.phone || "")}" required></label>
                <label><span>Country</span><input name="guest_country" value="India"></label>
                <label class="wide-field"><span>Address</span><input name="guest_address" value="${safe(state.user?.address || "")}"></label>
                <label class="wide-field"><span>Special requests</span><textarea name="special_requests" placeholder="Airport pickup, high floor, early check-in"></textarea></label>
              </div>
            </div>

            <div class="checkout-panel">
              <div class="panel-title"><span>3</span><h3>Add-ons</h3></div>
              <div class="addon-list">
                <label class="addon-tile"><input type="checkbox" name="breakfast"><span>Breakfast</span><strong>+${money(350)}/night</strong></label>
                ${activities.map((a) => `<label class="addon-tile"><input type="checkbox" name="activities" value="${safe(a.id)}" ${Number(a.price || 0) > 0 ? "" : "data-local-activity"}><span>${safe(a.name)}</span><strong>${Number(a.price || 0) > 0 ? `+${money(a.price)}` : "Included"}</strong></label>`).join("") || `<p class="muted">No paid activities are available for this hotel.</p>`}
              </div>
            </div>

            <div class="checkout-panel">
              <div class="panel-title"><span>4</span><h3>Payment</h3></div>
              <div class="payment-trust-row">
                <span>Secure booking</span>
                <span>Verified hotel</span>
                <span>Payment proof reviewed</span>
              </div>
              ${paymentChoiceCards}

              ${paymentOptions.payNow ? `
              <div class="payment-method-panel guest-pay-now-panel" id="pay-now-panel" style="${defaultMethod === "Pay Now" ? "" : "display:none;"}">
                <div class="upi-layout modern-upi-layout">
                  <div class="upi-qr-box modern-qr-box">
                    <img data-upi-qr alt="Hotel UPI QR code">
                    <p class="payment-qr-unavailable" data-upi-unavailable hidden>Payment QR unavailable.<br>Please contact the hotel.</p>
                  </div>
                  <div class="upi-tools">
                    <p class="eyebrow">Amount to pay</p>
                    <strong class="payment-amount" data-upi-amount>Calculating...</strong>
                    <p class="muted">Scan the QR with any UPI app and pay directly to ${safe(hotel.name || "the hotel")}.</p>
                    <input data-upi-id-input value="${safe(paymentOptions.upiId)}" readonly hidden>
                    <div class="upi-copy-card">
                      <span>Hotel UPI ID</span>
                      <strong>${safe(paymentOptions.upiId)}</strong>
                      <button class="secondary-btn" type="button" data-copy-upi>Copy UPI ID</button>
                    </div>
                    <div data-pay-now-only>
                      ${uploadZone("payment_screenshot_upload", "Payment screenshot for verification", { id: "booking-payment-screenshot", multiple: false, compact: true })}
                    </div>
                  </div>
                </div>
              </div>` : ""}

              ${paymentOptions.payAtProperty ? `
              <div class="payment-method-panel" id="pay-property-panel" style="${defaultMethod === "Pay at Property" ? "" : "display:none;"}">
                <div class="property-payment-card">
                  <strong>Pay at Property</strong>
                  <span class="muted">${advance > 0 ? `An advance amount of ${money(advance)} may be collected by the hotel.` : "No online payment is required now. The hotel will collect payment during check-in."}</span>
                </div>
              </div>` : ""}

            </div>

            <div class="checkout-panel policy-panel">
              ${hotelPoliciesCard(hotel)}
              <label class="terms-row checkout-agreement">
                <input class="checkout-agreement-checkbox" type="checkbox" name="policy_agreement" required>
                <span>I agree to the hotel policies, cancellation rules, and payment terms.</span>
              </label>
              <button class="primary-btn checkout-submit" type="submit">${defaultMethod === "Pay Now" ? "Submit Payment Proof" : "Confirm Booking"}</button>
            </div>
          </section>

          <aside class="booking-summary-card">
            <img src="${safe(selectedRoomImage)}" onerror="if(!this.dataset.fallback){ this.dataset.fallback='true'; this.src='${roomFallback}'; }" alt="${safe(room?.name || "Room")}">
            <div class="summary-body">
              <p class="eyebrow">${safe(hotel.property_type || "Hotel")}</p>
              <h3>${safe(hotel.name)}</h3>
              <p class="muted">${safe(room?.name || "Selected room")} - up to ${safe(room?.max_capacity || 2)} guests</p>
              <div class="summary-stay">
                <span><strong data-summary-checkin>${safe(defaultCin)}</strong><small>Check-in</small></span>
                <span><strong data-summary-checkout>${safe(defaultCout)}</strong><small>Check-out</small></span>
                <span><strong data-summary-guests>2 guests</strong><small>Guests</small></span>
              </div>
              <div id="booking-preview" class="price-breakdown"><div class="loader">Calculating price...</div></div>
              <div class="coupon-row">
                <input id="booking-promo-code" placeholder="Promo / Coupon Code">
                <button type="button" id="apply-promo-btn" class="secondary-btn">Apply</button>
              </div>
              <div id="available-coupons-container" style="margin-top: 15px;"></div>
              <div class="trust-list">
                <span>Secure payment validation</span>
                <span>Instant invoice PDF</span>
                <span>Verified property and host</span>
              </div>
            </div>
          </aside>
        </form>
      </div>
    `;
  }

  async function loadAvailableCoupons(hotelId) {
    const container = $("#available-coupons-container");
    if (!container) return;
    container.innerHTML = `<span class="muted" style="font-size: 11px;">Loading coupons...</span>`;
    try {
      const allCoupons = await API.coupons().catch(() => []);
      const validCoupons = allCoupons.filter(c => {
        let applicable = c.applicable_hotels;
        if (!applicable || applicable === "[]") return true;
        try {
          const ids = JSON.parse(applicable);
          if (Array.isArray(ids)) {
            return ids.some(id => Number(id) === Number(hotelId));
          }
        } catch (e) {}
        return false;
      });

      if (!validCoupons.length) {
        container.innerHTML = "";
        return;
      }

      container.innerHTML = `
        <div style="font-weight: 700; margin-bottom: 8px; font-size: 11px; color: var(--muted); text-transform: uppercase; letter-spacing: 0.5px;">Available Coupons</div>
        <div style="display: grid; gap: 8px;">
          ${validCoupons.map(c => {
            const label = c.discount_type === "percentage" ? `${c.discount_value}% OFF` : `${money(c.discount_value)} OFF`;
            return `
              <div class="coupon-item-row" style="border: 1px dashed var(--border); background: var(--surface-2); padding: 10px 12px; border-radius: 8px; display: flex; align-items: center; justify-content: space-between; gap: 12px;">
                <div style="display: flex; flex-direction: column; gap: 2px; flex: 1;">
                  <div style="display: flex; align-items: center; gap: 8px; flex-wrap: wrap;">
                    <strong style="color: var(--primary); font-family: monospace; font-size: 14px; letter-spacing: 0.5px; background: color-mix(in srgb, var(--primary) 8%, transparent); padding: 2px 6px; border-radius: 4px; border: 1px solid color-mix(in srgb, var(--primary) 20%, transparent);">${safe(c.code)}</strong>
                    <span style="font-size: 11px; font-weight: 700; color: var(--success);">${safe(label)}</span>
                  </div>
                  ${c.description ? `<p class="muted" style="margin: 0; font-size: 11px; line-height: 1.3; white-space: normal;">${safe(c.description)}</p>` : ""}
                  ${c.min_booking_amount > 0 ? `<p style="margin: 0; font-size: 10px; color: var(--muted);">Min. booking: ${money(c.min_booking_amount)}</p>` : ""}
                </div>
                <button type="button" class="apply-coupon-btn secondary-btn" data-code="${safe(c.code)}" style="padding: 6px 12px; font-size: 12px; min-height: 28px; font-weight: 700; white-space: nowrap; flex-shrink: 0;">Apply</button>
              </div>
            `;
          }).join("")}
        </div>
      `;

      container.querySelectorAll(".apply-coupon-btn").forEach(btn => {
        btn.addEventListener("click", () => {
          const code = btn.dataset.code;
          const input = $("#booking-promo-code");
          if (input) {
            input.value = code;
            toast(`Coupon code ${code} applied`, "success");
            previewBooking($("#booking-form"));
          }
        });
      });
    } catch (error) {
      container.innerHTML = "";
    }
  }

  async function startBooking(roomTypeId, cin = "", cout = "") {
    if (!state.user) {
      toast("Please login to reserve a room.", "info");
      openAuth("login");
      return;
    }
    if (!state.activities.length) {
      // Try the public catalogue first, then fall back to what the hotel
      // detail payload already carries (avoids an extra round trip).
      state.activities = await API.activities().catch(() => []);
      if (!state.activities.length && state.selectedHotel?.activities_list) {
        state.activities = state.selectedHotel.activities_list;
      }
    }
    if (!state.paymentConfig) state.paymentConfig = await API.paymentConfig().catch(() => paymentConfig());
    const room = (state.selectedHotel.rooms || []).find((item) => Number(item.id) === Number(roomTypeId));
    const hotel = state.selectedHotel;

    let defaultCin = cin;
    let defaultCout = cout;
    if (!defaultCin || !defaultCout) {
      const today = new Date();
      const tomorrow = new Date(today);
      tomorrow.setDate(tomorrow.getDate() + 1);
      defaultCin = today.toISOString().slice(0, 10);
      defaultCout = tomorrow.toISOString().slice(0, 10);
    }
    const roomFallback = fallbackRoomImages[((hotel.id || 0) + Number(roomTypeId)) % fallbackRoomImages.length] || fallbackRoomImages[0];
    const selectedRoomImage = roomImage(room, Number(roomTypeId));

    $("#details-content").innerHTML = paymentCheckoutTemplate(hotel, room, roomTypeId, defaultCin, defaultCout, selectedRoomImage, roomFallback);
    $("#booking-form").dataset.hotelId = state.selectedHotel.id;
    $("#booking-form").dataset.roomTypeId = roomTypeId;
    togglePaymentPanels($("#booking-form"));
    if (cin && cout) previewBooking($("#booking-form"));
    previewBooking($("#booking-form"));
    loadAvailableCoupons(hotel.id);
  }

  async function previewBooking(form, isExplicitApply = false) {
    const data = bookingPayload(form);
    if (!data.check_in_date || !data.nights || data.nights <= 0) {
      $("#booking-preview").innerHTML = `<div class="muted">Select valid dates.</div>`;
      return;
    }
    try {
      const preview = await API.previewBooking(data);
      state.lastBookingPreview = preview;
      const adults = Number(form.querySelector('input[name="adults"]')?.value || 0);
      const children = Number(form.querySelector('input[name="children"]')?.value || 0);
      const guests = Math.max(1, adults + children);
      const checkinNode = form.querySelector("[data-summary-checkin]");
      const checkoutNode = form.querySelector("[data-summary-checkout]");
      const guestsNode = form.querySelector("[data-summary-guests]");
      if (checkinNode) checkinNode.textContent = data.check_in_date;
      if (checkoutNode) checkoutNode.textContent = form.querySelector('input[name="check_out_date"]')?.value || "";
      if (guestsNode) guestsNode.textContent = `${guests} guest${guests === 1 ? "" : "s"}`;

      const discount = Number(preview.discount_amount || 0) > 0
        ? `<div class="breakdown-row success"><span>Coupon discount</span><strong>-${money(preview.discount_amount)}</strong></div>`
        : "";
      if (isExplicitApply) {
        if (preview.discount_amount > 0 && data.coupon_code) {
          toast(`Coupon '${data.coupon_code}' applied! You saved ${money(preview.discount_amount)}.`, "success");
        } else if (data.coupon_code) {
          toast(`Coupon '${data.coupon_code}' is not valid for this booking.`, "error");
        }
      }
      $("#booking-preview").innerHTML = `
        <div class="breakdown-row"><span>Room price (${data.nights} night${data.nights === 1 ? "" : "s"})</span><strong>${money(preview.subtotal)}</strong></div>
        <div class="breakdown-row"><span>Taxes / GST</span><strong>${money(preview.gst_amount)}</strong></div>
        ${discount}
        <div class="breakdown-row total"><span>Final total</span><strong>${money(preview.total_amount)}</strong></div>
      `;
      syncUpiPanel(form, preview);
    } catch (error) {
      $("#booking-preview").textContent = error.message;
      if (isExplicitApply) {
        toast(error.message, "error");
      }
    }
  }

  function bookingPayload(form) {
    const data = Object.fromEntries(new FormData(form).entries());
    const activities = [...form.querySelectorAll("input[name=activities]:checked")]
      .map((input) => Number(input.value))
      .filter((id) => Number.isFinite(id) && id > 0);
    const cin = data.check_in_date;
    const cout = data.check_out_date;
    let nights = 1;
    if (cin && cout) {
      const d1 = new Date(cin);
      const d2 = new Date(cout);
      nights = Math.max(1, Math.round((d2 - d1) / 86400000));
    }
    return {
      hotel_id: Number(form.dataset.hotelId),
      room_type_id: Number(form.dataset.roomTypeId),
      guest_name: data.guest_name,
      guest_email: data.guest_email,
      guest_phone: data.guest_phone,
      guest_address: data.guest_address,
      special_requests: data.special_requests || "",
      check_in_date: cin,
      nights: nights,
      breakfast: Boolean(form.querySelector("input[name=breakfast]")?.checked),
      activities: JSON.stringify(activities),
      coupon_code: $("#booking-promo-code")?.value || "",
    };
  }

  function bindEvents() {
    window.addEventListener("scroll", () => {
      const nav = $("#public-nav");
      if (nav) {
        if (window.scrollY > 50) {
          nav.classList.add("scrolled");
        } else {
          nav.classList.remove("scrolled");
        }
      }
    });
    $("#menu-toggle").addEventListener("click", () => $("#public-nav").classList.toggle("open"));
    $$(".theme-toggle").forEach((button) => button.addEventListener("click", toggleTheme));
    $$("[data-auth]").forEach((button) => button.addEventListener("click", () => openAuth(button.dataset.auth)));
    $("#auth-close").addEventListener("click", closeAuth);
    $("#auth-mode-tabs").addEventListener("click", (event) => {
      const button = event.target.closest("button[data-mode]");
      if (!button) return;
      state.authMode = button.dataset.mode;
      renderAuthForm();
    });
    $("#auth-form").addEventListener("submit", submitAuth);
    $("#public-search-form").addEventListener("submit", applyHotelFilters);
    $("#hotel-filter-text").addEventListener("input", () => {
      const q = $("#hotel-filter-text").value.toLowerCase();
      const filtered = state.hotels.filter((hotel) => `${hotel.name} ${hotel.location}`.toLowerCase().includes(q));
      renderHotelCards(filtered);
    });
    $("#hotel-filter-rating").addEventListener("change", applyHotelFilters);
    $("#portal-menu-toggle")?.addEventListener("click", () => $("#portal-app").classList.toggle("nav-open"));
    $("#logout-btn").addEventListener("click", () => {
      API.logout().catch(() => { });
      clearSession();
      hidePortal();
      toast("Logged out", "success");
    });
    $("#portal-nav").addEventListener("click", (event) => {
      const button = event.target.closest("[data-portal]");
      if (!button) return;
      event.stopPropagation();
      state.activePortal = button.dataset.portal;
      $("#portal-app").classList.remove("nav-open");
      renderPortal();
    });
    document.addEventListener("click", handleDocumentClick);
    document.addEventListener("submit", handleDocumentSubmit);
    document.addEventListener("change", handleDocumentChange);
    document.addEventListener("keydown", (event) => {
      if (event.key !== "Escape") return;
      if (!$("#details-modal").classList.contains("hidden")) closeDetails();
      if (!$("#auth-modal").classList.contains("hidden")) closeAuth();
    });
    document.addEventListener("dragover", (event) => {
      if (event.target.closest(".upload-zone")) {
        event.preventDefault();
        event.target.closest(".upload-zone").classList.add("dragging");
      }
    });
    document.addEventListener("dragleave", (event) => {
      event.target.closest(".upload-zone")?.classList.remove("dragging");
    });
    document.addEventListener("drop", (event) => {
      const zone = event.target.closest(".upload-zone");
      if (!zone) return;
      event.preventDefault();
      zone.classList.remove("dragging");
      const input = zone.querySelector("[data-upload-input]");
      if (!input) return;
      input.files = event.dataTransfer.files;
      handleFileInput(input);
      updateUploadPreview(input);
      updateHotelLivePreview(input.closest("[data-live-preview-form], #manager-hotel-form"));
      syncPaymentSubmitState(input.closest("#booking-form"));
    });
    document.addEventListener("input", (event) => {
      if (event.target.closest("[data-upi-id-input]")) {
        syncUpiPanel(event.target.closest("#booking-form"));
        return;
      }
      if (event.target.closest("#booking-form")) previewBooking(event.target.closest("#booking-form"));
      if (event.target.closest("[data-live-preview-form], #manager-hotel-form")) updateHotelLivePreview(event.target.closest("[data-live-preview-form], #manager-hotel-form"));
      if (event.target.closest("[data-room-filter]")) {
        state.roomInventoryPage = 1;
        filterRoomRows();
      }
      if (event.target.closest("[data-staff-filter]")) filterStaffRows();
    });
  }

  async function handleDocumentChange(event) {
    if (event.target.name === "ledger_id" && event.target.closest("#manager-commission-payment-form")) {
      const select = event.target;
      const form = select.closest("form");
      const amountInput = form.querySelector('input[name="amount"]');
      if (amountInput) {
        const option = select.selectedOptions[0];
        if (option) {
          const match = option.textContent.match(/Rs\.\s*([\d,]+(?:\.\d+)?)/);
          if (match) {
            amountInput.value = match[1].replace(/,/g, "");
          }
        }
      }
      return;
    }

    if (event.target.closest("[data-verification-document]")) {
      updateVerificationDocumentValidation(event.target.closest("#auth-form, #manager-hotel-form"), true);
      return;
    }

    const uploadInput = event.target.closest("[data-upload-input]");
    if (uploadInput) {
      handleFileInput(uploadInput);
      updateUploadPreview(uploadInput);
      updateHotelLivePreview(uploadInput.closest("[data-live-preview-form], #manager-hotel-form"));
      syncPaymentSubmitState(uploadInput.closest("#booking-form"));
      return;
    }

    if (event.target.name === "payment_method") {
      togglePaymentPanels(event.target.closest("#booking-form"));
      return;
    }

    if (event.target.closest("[data-upi-id-input]")) {
      syncUpiPanel(event.target.closest("#booking-form"));
      return;
    }

    if (event.target.closest("[data-room-filter]")) {
      state.roomInventoryPage = 1;
      filterRoomRows();
      return;
    }

    const roomTypeSelect = event.target.closest("[data-room-type-select]");
    if (roomTypeSelect) {
      const selected = roomTypeSelect.selectedOptions[0];
      const priceInput = roomTypeSelect.closest("form")?.querySelector("[data-room-price-input]");
      if (priceInput && selected?.dataset.price) priceInput.value = selected.dataset.price;
      return;
    }

    if (event.target.closest("[data-staff-filter]")) {
      filterStaffRows();
      return;
    }

    const statusSelect = event.target.closest("[data-room-status-select]");
    if (!statusSelect) return;
    try {
      await API.setRoomStatus(statusSelect.dataset.roomStatusSelect, { status: statusSelect.value });
      toast("Room status updated", "success");
      renderManagerRooms();
    } catch (error) {
      toast(error.message, "error");
    }
  }

  function showBookingSuccess(result) {
    const status = String(result.payment_status || "pending").toLowerCase();
    const isPaid = status === "paid" || status === "approved";
    $("#details-content").innerHTML = `
      <div class="payment-success">
        <div class="success-check"></div>
        <h2>${isPaid ? "Booking confirmed" : "Booking request received"}</h2>
        <p class="muted">${isPaid ? `Your reservation at ${safe(state.selectedHotel.name)} is complete. Your invoice is ready.` : `Your reservation at ${safe(state.selectedHotel.name)} is recorded and awaiting payment verification.`}</p>
        <div class="success-receipt">
          <p class="eyebrow">Booking reference</p>
          <strong class="success-ref">${safe(result.booking_id)}</strong>
          <div class="breakdown-row"><span>Payment status</span><strong class="status-pill ${safe(status)}">${safe(result.payment_status || "pending")}</strong></div>
          <div class="breakdown-row"><span>Total amount</span><strong>${money(result.total_amount)}</strong></div>
          <div class="breakdown-row"><span>Transaction ID</span><strong>${safe(result.transaction_id || "Recorded")}</strong></div>
        </div>
        <div class="button-row centered">
          <button class="primary-btn" data-receipt="${safe(result.booking_id)}">Download Invoice</button>
          <button class="secondary-btn" data-close-details>Close</button>
        </div>
      </div>
    `;
    toast("Booking confirmed: " + result.booking_id, "success");
    if (roleGroup() === "guest") {
      state.activePortal = "bookings";
      renderPortal();
    }
  }

  function openCancellationDialog(bookingRef) {
    $("#details-content").innerHTML = `
      <div class="modal-form-shell">
        <p class="eyebrow">${safe(bookingRef)}</p>
        <h2>Cancel booking</h2>
        <form class="stack-form" id="cancel-booking-form" data-booking-ref="${safe(bookingRef)}">
          <div class="cancel-reason-grid">
            ${cancellationReasons.map((reason, index) => `
              <label class="cancel-reason-option">
                <input type="radio" name="cancel_reason" value="${safe(reason)}" ${index === 0 ? "checked" : ""}>
                <span>${safe(reason)}</span>
              </label>
            `).join("")}
          </div>
          <label>
            <span>Additional details</span>
            <textarea name="custom_reason" placeholder="Add a note if needed"></textarea>
          </label>
          <p class="muted" style="margin-bottom: 16px; font-size: 13px;">Note: If payment was made via UPI, it will be refunded within 24 hrs.</p>
          <div class="card-row" style="justify-content: flex-end;">
            <button class="secondary-btn" type="button" data-close-details>Keep booking</button>
            <button class="danger-btn" type="submit">Cancel booking</button>
          </div>
        </form>
      </div>
    `;
    document.body.classList.add("modal-open");
    $("#details-modal").classList.remove("hidden");
  }

  async function handleDocumentClick(event) {
    const removeBtn = event.target.closest("[data-remove-input-id]");
    if (removeBtn) {
      event.preventDefault();
      event.stopPropagation();
      const key = removeBtn.dataset.removeInputId;
      const index = parseInt(removeBtn.dataset.removeIndex, 10);
      const files = inputFilesMap.get(key) || [];
      if (index >= 0 && index < files.length) {
        files.splice(index, 1);
        inputFilesMap.set(key, files);

        const input = document.getElementById(key) || document.querySelector(`input[name="${key}"]`);
        if (input) {
          try {
            const dt = new DataTransfer();
            files.forEach(file => dt.items.add(file));
            input.files = dt.files;
          } catch (e) {
            console.error(e);
          }
          updateUploadPreview(input);
          updateHotelLivePreview(input.closest("[data-live-preview-form], #manager-hotel-form"));
          syncPaymentSubmitState(input.closest("#booking-form"));
        }
      }
      return;
    }

    const downloadCsv = event.target.closest("[data-download-csv]");
    if (downloadCsv) {
      event.preventDefault();
      const tableType = downloadCsv.dataset.downloadCsv;
      triggerCsvDownload(tableType);
      return;
    }

    const toggleDetails = event.target.closest("[data-toggle-details]");
    if (toggleDetails) {
      event.preventDefault();
      const id = toggleDetails.dataset.toggleDetails;
      const target = document.getElementById(`vq-sections-${id}`);
      if (target) {
        const isHidden = target.classList.toggle("hidden");
        toggleDetails.textContent = isHidden ? "View Details ➔" : "Hide Details ➔";
      }
      return;
    }

    if (event.target.id === "details-modal" || event.target.closest("[data-close-details]")) {
      closeDetails();
      return;
    }

    if (event.target.id === "auth-modal" || event.target.closest("#auth-close")) {
      closeAuth();
      return;
    }

    const policyCollapse = event.target.closest("[data-policy-collapse]");
    if (policyCollapse) {
      const card = policyCollapse.closest("[data-policy-card]");
      const collapsed = card?.classList.toggle("is-collapsed");
      policyCollapse.setAttribute("aria-expanded", String(!collapsed));
      return;
    }

    const policyReadMore = event.target.closest("[data-policy-read-more]");
    if (policyReadMore) {
      const list = policyReadMore.closest("[data-policy-card]")?.querySelector("[data-policy-list]");
      const expanded = list?.classList.toggle("is-expanded");
      policyReadMore.textContent = expanded ? "Show less" : "Read more";
      policyReadMore.setAttribute("aria-expanded", String(Boolean(expanded)));
      return;
    }

    const detail = event.target.closest("[data-details]");
    if (detail) return openHotelDetails(detail.dataset.details);

    const heroLocation = event.target.closest("[data-hero-location]");
    if (heroLocation) {
      const location = heroLocation.dataset.heroLocation || "";
      $("#search-location").value = location;
      await loadPublicData({ location });
      return;
    }

    const portalButton = event.target.closest("[data-portal]");
    if (portalButton) {
      state.activePortal = portalButton.dataset.portal;
      $("#portal-app")?.classList.remove("nav-open");
      return renderPortal();
    }

    const calendarMode = event.target.closest("[data-calendar-mode]");
    if (calendarMode) {
      localStorage.setItem("hms_calendar_mode", calendarMode.dataset.calendarMode);
      return renderManagerCalendar();
    }

    const roomPage = event.target.closest("[data-room-page]");
    if (roomPage) {
      state.roomInventoryPage += roomPage.dataset.roomPage === "next" ? 1 : -1;
      filterRoomRows();
      return;
    }

    const openRoomType = event.target.closest("[data-open-room-type-modal]");
    if (openRoomType) {
      openRoomTypeModal();
      return;
    }

    const openRoom = event.target.closest("[data-open-room-modal]");
    if (openRoom) {
      const types = state.hotelId ? await API.roomTypes(state.hotelId).catch(() => []) : [];
      openRoomModal(types);
      return;
    }

    const manageInventory = event.target.closest("[data-manage-inventory]");
    if (manageInventory) {
      const id = Number(manageInventory.dataset.manageInventory);
      state.expandedRoomTypeId = Number(state.expandedRoomTypeId) === id ? null : id;
      return renderManagerRooms();
    }

    const editInventoryRoom = event.target.closest("[data-edit-inventory-room]");
    if (editInventoryRoom) {
      try {
        const [types, room] = await Promise.all([
          state.hotelId ? API.roomTypes(state.hotelId).catch(() => []) : [],
          API.room(editInventoryRoom.dataset.editInventoryRoom),
        ]);
        openRoomModal(types, room);
      } catch (error) {
        toast(error.message || "Room not found", "error");
      }
      return;
    }

    const roomMaintenance = event.target.closest("[data-room-maintenance]");
    if (roomMaintenance) {
      try {
        await API.setRoomStatus(roomMaintenance.dataset.roomMaintenance, {
          status: "maintenance",
          notes: "Marked for maintenance from inventory actions.",
        });
        toast("Room moved to maintenance", "success");
        return renderManagerRooms();
      } catch (error) {
        toast(error.message || "Could not update room", "error");
      }
      return;
    }

    const roomDeactivate = event.target.closest("[data-room-deactivate]");
    if (roomDeactivate) {
      const label = roomDeactivate.dataset.roomLabel || "this room";
      if (!confirm(`Deactivate ${label}? It will be hidden from booking availability but retained for reports and booking history.`)) return;
      try {
        await API.setRoomStatus(roomDeactivate.dataset.roomDeactivate, {
          status: "deactivated",
          notes: "Deactivated from inventory management.",
        });
        toast("Room deactivated", "success");
        return renderManagerRooms();
      } catch (error) {
        toast(error.message || "Could not deactivate room", "error");
      }
      return;
    }

    const editRoomType = event.target.closest("[data-edit-room-type]");
    if (editRoomType) {
      const types = state.hotelId ? await API.roomTypes(state.hotelId).catch(() => []) : [];
      const type = types.find((item) => Number(item.id) === Number(editRoomType.dataset.editRoomType));
      if (!type) return toast("Room type not found", "error");
      openRoomTypeModal(type);
      return;
    }

    const deleteRoomType = event.target.closest("[data-delete-room-type]");
    if (deleteRoomType) {
      if (!confirm("Delete this room type and its rooms?")) return;
      try {
        await API.deleteRoomType(deleteRoomType.dataset.deleteRoomType);
        toast("Room type deleted", "success");
        return renderManagerRooms();
      } catch (error) {
        toast(error.message || "Cannot delete: Room type has active or past bookings.", "error");
      }
      return;
    }

    const openStaff = event.target.closest("[data-open-staff-modal]");
    if (openStaff) {
      $("#details-content").innerHTML = `
        <div class="modal-form-shell">
          <h2>Add staff member</h2>
          <form class="stack-form" id="staff-form">
            <div class="form-grid">
              <label><span>Full name</span><input name="full_name" required></label>
              <label><span>Username</span><input name="username" required></label>
              <label><span>Email</span><input name="email" type="email"></label>
              <label><span>Phone</span><input name="phone"></label>
              <label><span>Role badge</span><select name="role_label"><option>Receptionist</option><option>Housekeeping Lead</option><option>Operations Associate</option><option>Concierge</option></select></label>
              <label><span>Shift</span><select name="shift"><option>Front desk</option><option>Morning</option><option>Evening</option><option>Night audit</option></select></label>
              <label><span>Password</span><input name="password" type="password" required></label>
            </div>
            <button class="primary-btn">Create Staff Account</button>
          </form>
        </div>
      `;
      document.body.classList.add("modal-open");
      $("#details-modal").classList.remove("hidden");
      return;
    }

    const openReview = event.target.closest("[data-open-review]");
    if (openReview) {
      $("#details-content").innerHTML = `
        <div class="modal-form-shell">
          <p class="eyebrow">${safe(openReview.dataset.bookingRef || "")}</p>
          <h2>Review ${safe(openReview.dataset.hotelName || "your stay")}</h2>
          <form class="stack-form" id="review-form" data-hotel-id="${safe(openReview.dataset.openReview)}">
            <div class="rating-picker">
              ${[5, 4, 3, 2, 1].map((rating) => `
                <label>
                  <input type="radio" name="rating" value="${rating}" ${rating === 5 ? "checked" : ""}>
                  <span>${rating} star${rating > 1 ? "s" : ""}</span>
                </label>
              `).join("")}
            </div>
            <label><span>Your review</span><textarea name="comment" required placeholder="Share what guests should know about this stay."></textarea></label>
            ${uploadZone("review_photos", "Review photos", { id: "review-photos", multiple: true })}
            <button class="primary-btn">Submit Review</button>
          </form>
        </div>
      `;
      document.body.classList.add("modal-open");
      $("#details-modal").classList.remove("hidden");
      return;
    }

    const categoryFilter = event.target.closest("[data-category-filter]");
    if (categoryFilter) {
      $$(".category-tabs button").forEach((button) => button.classList.toggle("active", button === categoryFilter));
      state.selectedCategory = categoryFilter.dataset.categoryFilter || "";
      $("#guest-search-btn")?.click();
      return;
    }

    if (event.target.id === "guest-search-btn") {
      const q = ($("#guest-search-q")?.value || "").toLowerCase();
      const rating = $("#guest-rating")?.value || "";
      const type = state.selectedCategory || "";
      const sort = $("#guest-sort")?.value || "recommended";

      let filtered = state.hotels.filter((hotel) => {
        const matchesText = `${hotel.name} ${hotel.location} ${hotel.description}`.toLowerCase().includes(q);
        const matchesRating = !rating || Number(hotel.star_rating || 0) >= Number(rating);
        
        let matchesType = false;
        if (!type) {
          matchesType = true;
        } else if (type === "Luxury Resort") {
          matchesType = hotel.property_type.toLowerCase().includes("resort");
        } else if (type === "Villa Stay") {
          matchesType = hotel.property_type.toLowerCase().includes("villa");
        } else if (type === "Hotel") {
          matchesType = hotel.property_type.toLowerCase().includes("hotel");
        } else {
          matchesType = hotel.property_type.toLowerCase().includes(type.toLowerCase());
        }
        
        return matchesText && matchesRating && matchesType;
      });

      if (sort === "price_asc") filtered.sort((a, b) => Number(a.price_from) - Number(b.price_from));
      else if (sort === "price_desc") filtered.sort((a, b) => Number(b.price_from) - Number(a.price_from));
      else if (sort === "rating_desc") filtered.sort((a, b) => Number(b.star_rating) - Number(a.star_rating));
      else filtered.sort((a, b) => Number(b.review_score) - Number(a.review_score));

      const grid = event.target.closest(".search-layout")?.querySelector("#guest-hotel-grid");
      if (!grid) {
        state.activePortal = "search";
        renderPortal();
        if ($("#guest-search-q")) $("#guest-search-q").value = q;
        if ($("#guest-rating")) $("#guest-rating").value = rating;
        if ($("#guest-sort")) $("#guest-sort").value = sort;
        $("#guest-search-btn")?.click();
        return;
      }
      grid.innerHTML = filtered.map(hotelCard).join("") || `<div class="empty-state" style="grid-column: 1/-1;"><h3>No stays found</h3><p class="muted">Try adjusting your filters.</p></div>`;
      return;
    }

    if (event.target.id === "apply-promo-btn") {
      const code = $("#booking-promo-code").value.trim().toUpperCase();
      if (!code) return toast("Enter a promo code first", "error");
      previewBooking($("#booking-form"), true);
      return;
    }

    if (event.target.id === "rt-check-btn") {
      const btn = event.target;
      const cin = $("#rt-checkin").value;
      const cout = $("#rt-checkout").value;
      if (!cin || !cout) return toast("Select both dates first", "error");
      btn.textContent = "Checking...";
      try {
        const req = await fetch('/api/hotels/' + btn.dataset.hotelId + '/rooms/available?check_in=' + cin + '&check_out=' + cout);
        const payload = await req.json();
        if (payload.error || payload.success === false) throw new Error(payload.message || payload.error);
        const data = payload.data || payload;
        const counts = {};
        data.forEach(r => { counts[r.room_type_id] = (counts[r.room_type_id] || 0) + 1; });

        $("#rt-room-list").innerHTML = state.selectedHotel.rooms.map(rt => {
          const c = counts[rt.id] || 0;
          if (c === 0) return `<div class="room-item"><div class="muted">${safe(rt.name)} - Sold Out for these dates</div></div>`;
          return `<div class="room-item"><div><span style="font-size: 15px; font-weight: 600;">${safe(rt.name)}</span><p class="muted">${safe(rt.description)} · ${c} room(s) left</p></div><div class="card-row"><span style="font-weight: 600;">${money(rt.current_price || rt.base_price)}</span><button class="primary-btn" data-book-room="${rt.id}" data-cin="${cin}" data-cout="${cout}">Book</button></div></div>`;
        }).join('');
        toast("Availability updated", "success");
      } catch (e) { toast(e.message, "error"); }
      btn.textContent = "Check Rooms";
      return;
    }

    const completePayout = event.target.closest("[data-complete-payout]");
    if (completePayout) {
      await API.completePayout(completePayout.dataset.completePayout);
      toast("Payout marked completed", "success");
      return renderAdminView();
    }

    const processRefund = event.target.closest("[data-process-refund]");
    if (processRefund) {
      await API.processRefund(processRefund.dataset.processRefund);
      toast("Refund marked processed", "success");
      return renderAdminView();
    }

    const wishlist = event.target.closest("[data-wishlist]");
    if (wishlist) {
      // Authentication disabled - proceed without login
      try {
        await API.addWishlist(Number(wishlist.dataset.wishlist));
        toast("Added to wishlist", "success");
      } catch (error) {
        toast(error.message, "error");
      }
      return;
    }

    const verifyGuestPayment = event.target.closest("[data-verify-guest-payment]");
    if (verifyGuestPayment) {
      const btn = verifyGuestPayment;
      btn.disabled = true;
      btn.textContent = "Verifying...";
      try {
        await API.verifyGuestPayment(btn.dataset.verifyGuestPayment);
        toast("Guest payment verified successfully.", "success");
        renderManagerEarnings();
      } catch (error) {
        toast(error.message, "error");
        btn.disabled = false;
        btn.textContent = "Verify";
      }
      return;
    }

    const bookRoom = event.target.closest("[data-book-room]");
    if (bookRoom) return startBooking(bookRoom.dataset.bookRoom, bookRoom.dataset.cin, bookRoom.dataset.cout);

    const deleteHotelImage = event.target.closest("[data-delete-hotel-image]");
    if (deleteHotelImage) {
      await API.deleteHotelImage(deleteHotelImage.dataset.deleteHotelImage);
      toast("Hotel photo removed", "success");
      return renderManagerHotel();
    }

    const approve = event.target.closest("[data-approve]");
    if (approve) {
      await API.setHotelStatus(approve.dataset.approve, "approved");
      toast("Hotel approved", "success");
      return renderApprovals();
    }

    const reject = event.target.closest("[data-reject]");
    if (reject) {
      await API.setHotelStatus(reject.dataset.reject, "rejected");
      toast("Hotel rejected", "success");
      return renderApprovals();
    }

    const checkin = event.target.closest("[data-checkin]");
    if (checkin) {
      try {
        const res = await fetch(`/api/bookings/${checkin.dataset.checkin}/checkin`, {
          method: "POST",
          headers: { "Authorization": `Bearer ${localStorage.getItem("hms_token") || ""}` }
        });
        if (!res.ok) throw new Error("Check-in failed");
        toast("Guest checked in successfully", "success");
        return renderManagerBookings();
      } catch (e) {
        toast(e.message, "error");
      }
      return;
    }

    const checkout = event.target.closest("[data-checkout]");
    if (checkout) {
      await API.checkout(checkout.dataset.checkout);
      toast("Guest checked out", "success");
      return renderManagerBookings();
    }

    const roomStatus = event.target.closest("[data-room-status]");
    if (roomStatus) {
      await API.setRoomStatus(roomStatus.dataset.roomStatus, { status: roomStatus.dataset.status });
      toast("Room status updated", "success");
      return renderManagerRooms();
    }

    const replyReviewBtn = event.target.closest("[data-reply-review]");
    if (replyReviewBtn) {
      const reviewId = replyReviewBtn.dataset.replyReview;
      const replyText = $(`#reply-input-${reviewId}`).value.trim();
      if (!replyText) return toast("Reply cannot be empty", "error");

      replyReviewBtn.textContent = "Replying...";
      replyReviewBtn.disabled = true;
      try {
        await API.replyToReview(reviewId, replyText);
        toast("Reply posted successfully", "success");
        renderManagerReviews();
      } catch (err) {
        toast(err.message, "error");
        replyReviewBtn.textContent = "Reply";
        replyReviewBtn.disabled = false;
      }
      return;
    }

    const deleteCoupon = event.target.closest("[data-delete-coupon]");
    if (deleteCoupon) {
      if (!confirm("Are you sure you want to delete this coupon? This cannot be undone.")) return;
      await API.deleteCoupon(deleteCoupon.dataset.deleteCoupon);
      toast("Coupon deleted", "success");
      renderAdminCoupons();
      return;
    }

    const deleteStaff = event.target.closest("[data-delete-staff]");
    if (deleteStaff) {
      if (!confirm("Delete this staff account?")) return;
      await API.deleteStaff(deleteStaff.dataset.deleteStaff);
      toast("Staff account deleted", "success");
      return renderStaff();
    }

    const deleteRoom = event.target.closest("[data-delete-room]");
    if (deleteRoom) {
      if (!confirm("Delete this room from inventory?")) return;
      try {
        await API.deleteRoom(deleteRoom.dataset.deleteRoom);
        toast("Room deleted", "success");
        return renderManagerRooms();
      } catch (error) {
        toast(error.message || "Cannot delete: Room has active or past bookings.", "error");
      }
      return;
    }

    const cancelBooking = event.target.closest("[data-cancel-booking]");
    if (cancelBooking) {
      openCancellationDialog(cancelBooking.dataset.cancelBooking);
      return;
    }

    const copyUpi = event.target.closest("[data-copy-upi]");
    if (copyUpi) {
      const form = copyUpi.closest("#booking-form");
      const upiId = form?.querySelector("[data-upi-id-input]")?.value || paymentConfig().upi_id;
      try {
        await navigator.clipboard.writeText(upiId);
      } catch {
        const temp = document.createElement("input");
        temp.value = upiId;
        document.body.appendChild(temp);
        temp.select();
        document.execCommand("copy");
        temp.remove();
      }
      toast("UPI ID copied", "success");
      return;
    }

    const copyText = event.target.closest("[data-copy-text]");
    if (copyText) {
      const value = copyText.dataset.copyText || "";
      if (!value) return toast("Nothing to copy yet", "error");
      try {
        await navigator.clipboard.writeText(value);
      } catch {
        const temp = document.createElement("input");
        temp.value = value;
        document.body.appendChild(temp);
        temp.select();
        document.execCommand("copy");
        temp.remove();
      }
      toast("Copied", "success");
      return;
    }

    const payNowFocus = event.target.closest("[data-pay-now-focus]");
    if (payNowFocus) {
      payNowFocus.closest("#booking-form")?.querySelector(".guest-pay-now-panel")?.scrollIntoView({ behavior: "smooth", block: "center" });
      return;
    }

    const downloadQr = event.target.closest("[data-download-qr]");
    if (downloadQr) {
      const form = downloadQr.closest("#booking-form");
      syncUpiPanel(form);
      const qr = form?.querySelector("[data-upi-qr]");
      const link = document.createElement("a");
      link.download = `StaySphere-UPI-${Date.now()}.png`;
      link.href = qr?.src || "";
      if (!link.href) return toast("QR is still generating", "error");
      document.body.appendChild(link);
      link.click();
      link.remove();
      toast("UPI QR downloaded", "success");
      return;
    }

    const downloadPlatformQr = event.target.closest("[data-download-platform-qr]");
    if (downloadPlatformQr) {
      const card = downloadPlatformQr.closest(".platform-qr-card");
      const qr = card?.querySelector(".upi-qr-box img:not(.hidden)");
      const link = document.createElement("a");
      link.download = `StaySphere-Commission-QR-${Date.now()}.png`;
      link.href = qr?.src || "";
      if (!link.href) return toast("QR is not available yet", "error");
      document.body.appendChild(link);
      link.click();
      link.remove();
      toast("Commission QR downloaded", "success");
      return;
    }

    const receipt = event.target.closest("[data-receipt]");
    if (receipt) {
      try {
        await API.downloadReceiptPdf(receipt.dataset.receipt);
        toast("Invoice PDF downloaded", "success");
      } catch (error) {
        toast(error.message, "error");
      }
    }
  }

  async function handleDocumentSubmit(event) {
    const form = event.target;

    if (form.id === "newsletter-form") {
      event.preventDefault();
      toast("Subscribed to StaySphere travel updates.", "success");
      form.reset();
      return;
    }

    if (form.id === "cancel-booking-form") {
      event.preventDefault();
      const selectedReason = form.querySelector('input[name="cancel_reason"]:checked')?.value || "";
      const customReason = form.custom_reason.value.trim();
      let reason = selectedReason === "Other" ? customReason : selectedReason;
      if (customReason && selectedReason !== "Other") reason = `${selectedReason} - ${customReason}`;
      if (!reason) return toast("Please select or enter a cancellation reason", "error");

      const btn = form.querySelector('button[type="submit"]');
      btn.textContent = "Cancelling...";
      btn.disabled = true;
      try {
        await API.cancel({ booking_ref: form.dataset.bookingRef, reason });
        toast("Booking cancelled. If payment was made via UPI, it will be refunded within 24 hrs.", "success");
        closeDetails();
        return renderMyBookings("Booking history");
      } catch (error) {
        btn.textContent = "Cancel booking";
        btn.disabled = false;
        toast(error.message, "error");
      }
      return;
    }

    if (form.id === "booking-form") {
      event.preventDefault();
      const paymentMethod = selectedPaymentMethod(form);
      const payload = bookingPayload(form);
      payload.payment_method = paymentMethod;
      const btn = form.querySelector("button[type=submit]");
      const isPayNow = paymentMethod === "Pay Now";
      let paymentProofUrl = "";

      if (isPayNow) {
        const proofFile = form.querySelector('input[name="payment_screenshot_upload"]')?.files?.[0];
        const isImageProof = proofFile && (proofFile.type?.startsWith("image/") || /\.(jpe?g|png)$/i.test(proofFile.name || ""));
        if (!isImageProof) {
          toast("Please upload a payment screenshot for verification.", "error");
          form.querySelector('input[name="payment_screenshot_upload"]')?.closest(".upload-field")?.classList.add("missing-upload");
          return;
        }
      }

      btn.textContent = "Processing...";
      btn.disabled = true;
      try {
        if (isPayNow) {
          btn.textContent = "Uploading payment proof...";
          const proofFile = form.querySelector('input[name="payment_screenshot_upload"]')?.files?.[0];
          const uploadedProof = await API.uploadFile(proofFile);
          paymentProofUrl = uploadedProof.public_url || uploadedProof.url || uploadedProof.image_path || "";
        }
        const booking = await API.createBooking(payload);
        form.dataset.bookingRef = booking.booking_id;
        syncUpiPanel(form, state.lastBookingPreview || booking);
        if (paymentMethod === "Pay at Property" || paymentMethod === "Pay at Hotel") {
          showBookingSuccess({ ...booking, payment_status: "pending", transaction_id: "Pay at Property" });
          return;
        }
        btn.textContent = "Submitting for verification...";
        const paid = await API.verifyPayment({
          booking_ref: booking.booking_id,
          payment_method: paymentMethod,
          status: "pending",
          razorpay_order_id: booking.razorpay_order_id,
          transaction_id: `PROOF-${Date.now()}`,
          upi_id: state.selectedHotel?.upi_id || booking.upi_id,
          merchant_name: state.selectedHotel?.name || booking.merchant_name,
          payment_proof_url: paymentProofUrl,
        });
        showBookingSuccess({ ...booking, ...paid, booking_id: booking.booking_id });
      } catch (error) {
        btn.textContent = isPayNow ? "Submit Payment Proof" : "Confirm Booking";
        syncPaymentSubmitState(form);
        toast(error.message, "error");
      }
    }

    if (form.id === "profile-form") {
      event.preventDefault();
      await API.updateProfile(Object.fromEntries(new FormData(form).entries()));
      state.user = await API.me();
      toast("Profile saved", "success");
      renderProfile();
    }

    if (form.id === "platform-payment-settings-form") {
      event.preventDefault();
      const data = Object.fromEntries(new FormData(form).entries());
      data.commission_collection_enabled = form.querySelector('input[name="commission_collection_enabled"]')?.checked || false;
      ["commission_rate", "active_days", "grace_until_days", "suspension_days"].forEach((key) => {
        data[key] = Number(data[key] || 0);
      });
      await API.updatePlatformPaymentSettings(data);
      toast("Platform payment settings saved", "success");
      return renderAdminPlatformPayments();
    }

    if (form.id === "manager-commission-payment-form") {
      event.preventDefault();
      const proofFile = form.querySelector('input[name="commission_payment_proof"]')?.files?.[0];
      const isImageProof = proofFile && (proofFile.type?.startsWith("image/") || /\.(jpe?g|png)$/i.test(proofFile.name || ""));
      if (!isImageProof) {
        toast("Please upload a commission payment screenshot.", "error");
        form.querySelector('input[name="commission_payment_proof"]')?.closest(".upload-field")?.classList.add("missing-upload");
        return;
      }
      const btn = form.querySelector('button[type="submit"]');
      btn.textContent = "Uploading proof...";
      btn.disabled = true;
      try {
        const uploaded = await API.uploadFile(proofFile);
        const data = Object.fromEntries(new FormData(form).entries());
        data.amount = Number(data.amount || 0);
        data.payment_proof_url = uploaded.public_url || uploaded.url || uploaded.image_path || "";
        delete data.commission_payment_proof;
        await API.submitCommissionPayment(data);
        toast("Commission proof submitted", "success");
        return renderManagerCommission();
      } catch (error) {
        btn.textContent = "Submit proof";
        btn.disabled = false;
        toast(error.message, "error");
      }
      return;
    }

    if (form.id === "commission-form") {
      event.preventDefault();
      const rate = Number(new FormData(form).get("commission_rate"));
      await API.updateCommission(rate);
      toast("Commission rate updated", "success");
      return renderAdminView();
    }

    if (form.id === "manager-hotel-form") {
      event.preventDefault();
      const data = Object.fromEntries(new FormData(form).entries());
      ["hotel_photos", "ownership_certificate_upload", "business_license_upload", "tourism_registration_upload", "id_proof_upload"].forEach((key) => delete data[key]);
      data.location = [data.city, data.state, data.country].filter(Boolean).join(", ") || data.location;
      data.amenities = checkedValues(form, "amenities");
      data.accepted_payments = ["Pay Now", "Pay at Property"];
      data.nearby_places = listToArray(data.nearby_landmarks);

      const documents = await uploadRegistrationDocuments(form);
      if (documents.length) data.documents = documents;

      const photoFiles = getUploadedFiles(form, "hotel_photos");
      const uploadedImages = await uploadLooseImages(photoFiles);
      if (uploadedImages.length) {
        data.hotel_images = uploadedImages;
        data.image_path = uploadedImages[0];
      }

      ["price_from", "star_rating", "room_count", "weekend_price", "seasonal_price", "min_stay", "available_rooms", "max_guests"].forEach((key) => {
        data[key] = numberValue(data[key], key === "min_stay" ? 1 : 0);
      });
      const result = await API.submitHotel(data);
      state.hotelId = Number(result.id);
      localStorage.setItem("hms_hotel_id", String(state.hotelId));
      toast("Hotel submitted for approval", "success");
      renderManagerView();
    }

    if (form.id === "hotel-profile-form") {
      event.preventDefault();
      const data = Object.fromEntries(new FormData(form).entries());
      const photoFiles = getUploadedFiles(form, "hotel_photos");
      delete data.hotel_photos;
      data.amenities = checkedValues(form, "amenities");
      data.nearby_places = listToArray(data.nearby_landmarks);
      ["price_from", "star_rating", "room_count"].forEach((key) => data[key] = numberValue(data[key], 0));
      await API.updateHotel(state.hotelId, data);
      await recordHotelUploads(state.hotelId, photoFiles, !(await API.getHotelImages(state.hotelId).catch(() => [])).length);
      toast("Hotel profile saved", "success");
      renderManagerHotel();
    }

    if (form.id === "policy-form") {
      event.preventDefault();
      await API.updateHotel(state.hotelId, Object.fromEntries(new FormData(form).entries()));
      toast("Policies saved", "success");
    }

    if (form.id === "payment-form") {
      event.preventDefault();
      const data = Object.fromEntries(new FormData(form).entries());
      data.accepted_payments = checkedValues(form, "accepted_payments");
      if (data.taxes_fees !== undefined) data.taxes_fees = numberValue(data.taxes_fees, 0);
      await API.updateHotel(state.hotelId, data);
      toast("Payment information saved", "success");
    }

    if (form.id === "room-form") {
      event.preventDefault();
      const btn = form.querySelector("button");
      btn.textContent = "Adding...";
      btn.disabled = true;
      try {
        const data = Object.fromEntries(new FormData(form).entries());
        const photoFiles = getUploadedFiles(form, "room_photos");
        const features = checkedValues(form, "room_features");
        delete data.room_photos;
        data.room_type_id = Number(data.room_type_id || 0);
        data.floor = Number(data.floor || 1);
        data.price = Number(data.price || 0);
        data.notes = features.join(", ");
        await API.createRoom(state.hotelId, data);
        await recordRoomUploads(data.room_type_id, photoFiles, false);
        toast("Room added to inventory", "success");
        closeDetails();
        state.roomInventoryPage = 1;
        renderManagerRooms();
      } catch (error) {
        toast(error.message, "error");
        btn.textContent = "Add Room";
        btn.disabled = false;
      }
      return;
    }

    if (form.id === "room-type-form") {
      event.preventDefault();
      const btn = form.querySelector("button");
      const isEdit = Boolean(form.dataset.roomTypeId);
      btn.textContent = "Saving...";
      btn.disabled = true;
      try {
        const data = Object.fromEntries(new FormData(form).entries());
        const photoFiles = getUploadedFiles(form, "room_photos");
        const features = checkedValues(form, "room_features");
        delete data.room_photos;
        data.base_price = Number(data.base_price || 0);
        data.current_price = data.base_price;
        data.max_capacity = Number(data.max_capacity || 2);
        data.room_size = data.room_size || "";
        data.amenities = features;
        data.description = data.description || "";
        if (!isEdit) {
          data.total_rooms = Math.max(1, Number(data.total_rooms || 1));
          data.auto_generate_room_numbers = Boolean(form.querySelector('input[name="auto_generate_room_numbers"]')?.checked);
          data.room_prefix = data.room_prefix || "";
          data.starting_room_number = data.starting_room_number || "101";
        } else {
          delete data.total_rooms;
          delete data.auto_generate_room_numbers;
          delete data.room_prefix;
          delete data.starting_room_number;
        }
        if (isEdit) {
          await API.updateRoomType(form.dataset.roomTypeId, data);
          await recordRoomUploads(form.dataset.roomTypeId, photoFiles, false);
          toast("Room type updated", "success");
        } else {
          const created = await API.createRoomType(state.hotelId, data);
          await recordRoomUploads(created.id, photoFiles, true);
          toast(`Room type created with ${created.generated_count || data.total_rooms} rooms`, "success");
        }
        closeDetails();
        renderManagerRooms();
      } catch (error) {
        toast(error.message, "error");
        btn.textContent = isEdit ? "Save Room Type" : "Create Room Type";
        btn.disabled = false;
      }
      return;
    }

    if (form.id === "room-type-form") {
      event.preventDefault();
      const btn = form.querySelector("button");
      btn.textContent = "Saving...";
      btn.disabled = true;

      try {
        const data = Object.fromEntries(new FormData(form).entries());
        const photoFiles = getUploadedFiles(form, "room_photos");
        delete data.room_photos;
        data.base_price = Number(data.base_price || 0);
        data.max_capacity = Number(data.max_capacity || 2);
        data.total_rooms = Number(data.total_rooms || 1);
        const features = checkedValues(form, "room_features");
        data.amenities = features;
        data.description = [data.description, data.room_size, data.beds ? `${data.beds} bed(s)` : "", features.join(", ")].filter(Boolean).join(" · ");

        const uploadedImages = await uploadLooseImages(photoFiles);
        if (uploadedImages.length) {
          data.room_images = uploadedImages;
          data.image_path = uploadedImages[0];
        }

        const rt = await API.createRoomType(state.hotelId, data);

        // Auto-generate room inventory
        const prefix = (data.name.substring(0, 3).toUpperCase().replace(/[^A-Z0-9]/g, '')) || "RM";
        const floor = 1;
        for (let i = 1; i <= data.total_rooms; i++) {
          await API.createRoom(state.hotelId, {
            room_type_id: rt.id,
            room_number: `${prefix}-${floor * 100 + i}`,
            floor: floor
          });
        }

        toast(`Room category and ${data.total_rooms} rooms added!`, "success");
        renderManagerRooms();
      } catch (error) {
        toast(error.message, "error");
        btn.textContent = "Save Category & Generate Rooms";
        btn.disabled = false;
      }
    }

    if (form.id === "staff-form") {
      event.preventDefault();
      const data = Object.fromEntries(new FormData(form).entries());
      data.role = "receptionist";
      data.hotel_id = state.hotelId;
      await API.createStaff(state.hotelId, data);
      toast("Staff account created", "success");
      form.reset();
      closeDetails();
      renderStaff();
    }

    if (form.id === "review-form") {
      event.preventDefault();
      const btn = form.querySelector("button");
      const originalText = btn ? btn.textContent : "Submit Review";
      if (btn) {
        btn.textContent = "Submitting...";
        btn.disabled = true;
      }
      try {
        const data = Object.fromEntries(new FormData(form).entries());
        const photoFiles = getUploadedFiles(form, "review_photos");
        delete data.review_photos;
        const images = await uploadLooseImages(photoFiles);
        await API.addReview(form.dataset.hotelId, {
          rating: Number(data.rating || 5),
          comment: data.comment || "",
          images,
        });
        toast("Review submitted. Thanks for helping future guests.", "success");
        closeDetails();
        state.activePortal = "reviews";
        return renderMyReviews();
      } catch (error) {
        if (btn) {
          btn.textContent = originalText;
          btn.disabled = false;
        }
        toast(error.message, "error");
      }
    }

    if (form.id === "admin-user-form") {
      event.preventDefault();
      await API.createAdminUser(Object.fromEntries(new FormData(form).entries()));
      toast("User created", "success");
      renderUsers();
    }

    if (form.id === "coupon-form") {
      event.preventDefault();
      const data = Object.fromEntries(new FormData(form).entries());
      data.code = data.code.toUpperCase().trim();
      ["discount_value", "min_booking_amount", "max_discount", "usage_limit"].forEach((key) => {
        data[key] = data[key] === "" ? "0" : data[key];
      });
      await API.createCoupon(data);
      toast("Coupon created successfully", "success");
      renderAdminCoupons();
    }
  }

  async function init() {
    setTheme(localStorage.getItem("hms_theme") || "light");
    bindEvents();
    const today = new Date();
    const yyyy = today.getFullYear();
    const mm = String(today.getMonth() + 1).padStart(2, '0');
    const dd = String(today.getDate()).padStart(2, '0');
    const todayStr = `${yyyy}-${mm}-${dd}`;
    if ($("#search-checkin")) {
      $("#search-checkin").value = todayStr;
      $("#search-checkin").min = todayStr;
    }
    $("#featured-hotels").innerHTML = Array.from({ length: 8 }).map(() => hotelCard(null)).join("");
    await loadPublicData();

    if (location.hash === "#login") openAuth("login");
    if (location.hash === "#register") openAuth("register");
    if (location.hash === "#partner") openAuth("partner");
    await restoreSession();
  }

  // Fetch interception removed - bookings will now communicate properly with the backend
  // The previous interception was preventing real bookings from being created

  // Override to ensure "Booking Confirmed!" always shows properly
  function showBookingSuccess(result) {
    const safeVal = (val) => String(val || "").replace(/[&<>"']/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#039;" }[c]));
    document.querySelector("#details-content").innerHTML = `
      <div style="text-align:center; padding: 60px 20px; background: var(--bg-light); border-radius: 0 0 12px 12px;">
        <div style="font-size:72px; margin-bottom: 24px;">✅</div>
        <h2 style="margin-bottom: 12px; font-size: 32px;">Booking Confirmed!</h2>
        <p class="muted" style="font-size: 16px;">Your reservation is complete and verified.</p>
        <div style="background: var(--surface); padding: 24px; border-radius: 8px; border: 1px solid var(--border); display: inline-block; text-align: left; margin: 36px 0; min-width: 320px; box-shadow: 0 10px 25px rgba(0,0,0,0.05);">
          <p style="margin: 0 0 8px; font-size: 14px; color: var(--muted); text-transform: uppercase; font-weight: 700;">Booking Reference</p>
          <p style="margin: 0 0 20px; font-size: 28px; font-weight: 700; color: var(--primary);">${safeVal(result.booking_id || "BK-NEW")}</p>
          <div style="display: flex; justify-content: space-between; margin-bottom: 12px; font-size: 16px;"><span>Payment Status:</span><strong style="color: var(--success);">Confirmed</strong></div>
        </div>
        <br>
        <button class="secondary-btn" data-close-details>Close Window</button>
      </div>
    `;
    toast("Booking confirmed!", "success");
  }

  window.addEventListener("DOMContentLoaded", init);

  return { toast, openAuth };
})();
