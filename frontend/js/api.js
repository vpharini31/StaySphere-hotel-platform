const API = (() => {
  const BASE = "";

  function token() {
    return localStorage.getItem("hms_token") || "";
  }

  async function request(method, path, body, auth = true) {
    const headers = { "Content-Type": "application/json" };
    if (auth && token()) headers.Authorization = "Bearer " + token();
    const options = { method, headers };
    if (body !== undefined) options.body = JSON.stringify(body);

    const response = await fetch(BASE + path, options);
    const text = await response.text();
    let payload = {};
    try {
      payload = text ? JSON.parse(text) : {};
    } catch {
      throw new Error(text || "Invalid server response");
    }
    if (!response.ok || payload.success === false) {
      throw new Error(payload.message || "Request failed");
    }
    return payload.data;
  }

  async function uploadRequest(path, formData, auth = true) {
    const headers = {};
    if (auth && token()) headers.Authorization = "Bearer " + token();

    const response = await fetch(BASE + path, { method: "POST", headers, body: formData });
    const text = await response.text();
    let payload = {};
    try {
      payload = text ? JSON.parse(text) : {};
    } catch {
      throw new Error(text || "Invalid server response");
    }
    if (!response.ok || payload.success === false) {
      throw new Error(payload.message || "Upload failed");
    }
    return payload.data;
  }

  async function downloadFile(path, filename, auth = true) {
    const headers = {};
    if (auth && token()) headers.Authorization = "Bearer " + token();
    const response = await fetch(BASE + path, { headers });
    if (!response.ok) {
      const text = await response.text();
      throw new Error(text || "Download failed");
    }
    const blob = await response.blob();
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.href = url;
    link.download = filename;
    document.body.appendChild(link);
    link.click();
    link.remove();
    URL.revokeObjectURL(url);
  }

  function query(params) {
    const search = new URLSearchParams();
    Object.entries(params || {}).forEach(([key, value]) => {
      if (value !== undefined && value !== null && value !== "") search.set(key, value);
    });
    const qs = search.toString();
    return qs ? "?" + qs : "";
  }

  return {
    request,
    login: (username, password) => request("POST", "/api/auth/login", { username, password }, false),
    register: (data) => request("POST", "/api/auth/register", data, false),
    logout: () => request("POST", "/api/auth/logout", {}),
    me: () => request("GET", "/api/auth/me"),
    updateProfile: (data) => request("PUT", "/api/auth/profile", data),
    loyalty: () => request("GET", "/api/auth/loyalty"),

    publicHotels: (params) => request("GET", "/api/public/hotels" + query(params), undefined, false),
    publicHotel: (id) => request("GET", "/api/public/hotels/" + id, undefined, false),
    getHotels: () => request("GET", "/getHotels", undefined, false),
    getHotelImages: (id) => request("GET", "/getHotelImages/" + id, undefined, false),
    destinations: () => request("GET", "/api/public/destinations", undefined, false),

    hotels: () => request("GET", "/api/hotels"),
    hotel: (id) => request("GET", "/api/hotels/" + id),
    getHotel: (id) => request("GET", "/api/hotels/" + id),
    updateHotel: (id, data) => request("PUT", "/api/hotels/" + id, data),
    submitHotel: (data) => request("POST", "/api/manager/hotels", data),
    uploadHotelImage: (id, data) => request("POST", `/api/hotels/${id}/images`, data),
    uploadHotel: (hotelId, files, isPrimary = false) => {
      const formData = new FormData();
      formData.append("hotel_id", hotelId);
      formData.append("is_primary", isPrimary ? "1" : "0");
      [...files].forEach((file) => formData.append("images", file));
      return uploadRequest("/uploadHotel", formData);
    },
    uploadRoom: (roomTypeId, files, isPrimary = false) => {
      const formData = new FormData();
      formData.append("room_type_id", roomTypeId);
      formData.append("is_primary", isPrimary ? "1" : "0");
      [...files].forEach((file) => formData.append("images", file));
      return uploadRequest("/api/uploadRoom", formData);
    },
    uploadFile: (file) => {
      const formData = new FormData();
      formData.append("file", file);
      return uploadRequest("/api/upload", formData);
    },
    uploadHotelDocument: (id, data) => request("POST", `/api/hotels/${id}/documents`, data),
    deleteHotelImage: (id) => request("DELETE", `/api/hotels/images/${id}`),
    createStaff: (hotelId, data) => request("POST", `/api/hotels/${hotelId}/staff`, data),
    staff: (hotelId) => request("GET", `/api/hotels/${hotelId}/staff`),
    deleteStaff: (id) => request("DELETE", `/api/staff/${id}`),

    roomTypes: (hotelId) => request("GET", `/api/hotels/${hotelId}/room-types`),
    getRoomTypes: (hotelId) => request("GET", `/api/hotels/${hotelId}/room-types`),
    createRoomType: (hotelId, data) => request("POST", `/api/hotels/${hotelId}/room-types`, data),
    updateRoomType: (id, data) => request("PUT", `/api/room-types/${id}`, data),
    deleteRoomType: (id) => request("DELETE", `/api/room-types/${id}`),
    uploadRoomImage: (id, data) => request("POST", `/api/room-types/${id}/images`, data),
    rooms: (hotelId) => request("GET", `/api/hotels/${hotelId}/rooms`),
    getRooms: (hotelId) => request("GET", `/api/hotels/${hotelId}/rooms`),
    inventory: (hotelId, params = {}) => request("GET", `/api/hotels/${hotelId}/inventory` + query(params)),
    createRoom: (hotelId, data) => request("POST", `/api/hotels/${hotelId}/rooms`, data),
    room: (id) => request("GET", `/api/rooms/${id}`),
    updateRoom: (id, data) => request("PUT", `/api/rooms/${id}`, data),
    setRoomStatus: (id, data) => request("PUT", `/api/rooms/${id}/status`, data),
    deleteRoom: (id) => request("DELETE", `/api/rooms/${id}`),

    previewBooking: (data) => request("POST", "/api/bookings/preview", data),
    createBooking: (data) => request("POST", "/api/bookings", data),
    bookings: (hotelId, status = "all") => request("GET", `/api/bookings?hotel_id=${hotelId}&status=${status}`),
    myBookings: () => request("GET", "/api/me/bookings"),
    coupons: () => request("GET", "/api/coupons"),
    checkout: (ref) => request("POST", `/api/bookings/${ref}/checkout`, {}),
    paymentConfig: () => request("GET", "/api/payments/config"),
    upiQr: (data) => request("POST", "/api/payments/upi-qr", data),
    createPaymentOrder: (data) => request("POST", "/api/payments/create-order", data),
    verifyPayment: (data) => request("POST", "/api/payments/verify", data),
    paymentStatus: (ref) => request("GET", `/api/payments/${encodeURIComponent(ref)}/status`),
    cancel: (data) => request("POST", "/api/cancellations", data),
    bill: (ref) => request("GET", "/api/billing/" + ref),
    getBill: (ref) => request("GET", "/api/billing/" + ref),
    receipt: (ref) => request("GET", "/api/receipts/" + ref),
    getReceipt: (ref) => request("GET", "/api/receipts/" + ref),
    downloadReceiptPdf: (ref) => downloadFile(`/api/receipts/${encodeURIComponent(ref)}/pdf`, `INVOICE-${ref}.pdf`),
    activities: () => request("GET", "/api/activities"),
    getActivities: () => request("GET", "/api/activities"),

    wishlist: () => request("GET", "/api/wishlist"),
    addWishlist: (hotel_id) => request("POST", "/api/wishlist", { hotel_id }),
    removeWishlist: (hotel_id) => request("DELETE", "/api/wishlist/" + hotel_id),
    reviews: (hotelId) => request("GET", `/api/hotels/${hotelId}/reviews`, undefined, false),
    myReviews: () => request("GET", "/api/me/reviews"),
    addReview: (hotelId, data) => request("POST", `/api/hotels/${hotelId}/reviews`, data),
    replyToReview: (id, reply) => request("POST", `/api/reviews/${id}/reply`, { reply }),

    managerAnalytics: (hotelId) => request("GET", "/api/manager/analytics" + query({ hotel_id: hotelId })),
    reportStats: (hotelId) => request("GET", `/api/reports/stats?hotel_id=${hotelId}`),
    revenueTimeline: (hotelId, days = 14) => request("GET", `/api/reports/timeline?hotel_id=${hotelId}&days=${days}`),
    popularRooms: (hotelId) => request("GET", `/api/reports/popular-rooms?hotel_id=${hotelId}`),
    cancellations: (hotelId) => request("GET", `/api/cancellations?hotel_id=${hotelId}`),

    adminHotels: (status = "all") => request("GET", "/api/admin/hotels" + query({ status })),
    setHotelStatus: (id, status) => request("PUT", `/api/admin/hotels/${id}/status`, { status }),
    adminUsers: () => request("GET", "/api/admin/users"),
    createAdminUser: (data) => request("POST", "/api/users", data),
    adminAnalytics: () => request("GET", "/api/admin/analytics"),
    adminRevenue: () => request("GET", "/api/admin/revenue"),
    adminPlatformPayments: () => request("GET", "/api/admin/platform-payments"),
    updatePlatformPaymentSettings: (data) => request("PUT", "/api/admin/platform-payments/settings", data),
    reviewCommissionPayment: (id, data) => request("POST", `/api/admin/commission-payments/${id}/review`, data),
    adminTransactions: (status = "") => request("GET", "/api/admin/transactions" + query({ status })),
    adminPayouts: (status = "") => request("GET", "/api/admin/payouts" + query({ status })),
    updateCommission: (commission_rate) => request("PUT", "/api/admin/commission", { commission_rate }),
    completePayout: (id) => request("POST", `/api/admin/payouts/${id}/complete`, {}),
    adminRefunds: () => request("GET", "/api/admin/refunds"),
    processRefund: (id) => request("POST", `/api/admin/refunds/${id}/process`, {}),
    managerEarnings: (hotelId) => request("GET", "/api/manager/earnings" + query({ hotel_id: hotelId })),
    managerCommission: (hotelId) => request("GET", "/api/manager/commission" + query({ hotel_id: hotelId })),
    submitCommissionPayment: (data) => request("POST", "/api/manager/commission-payments", data),
    verifyGuestPayment: (booking_ref) => request("POST", "/api/manager/verify-guest-payment", { booking_ref }),
    adminCoupons: () => request("GET", "/api/admin/coupons"),
    createCoupon: (data) => request("POST", "/api/admin/coupons", data),
    deleteCoupon: (id) => request("DELETE", `/api/admin/coupons/${id}`),
  };
})();
