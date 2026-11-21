CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL,
    full_name TEXT NOT NULL,
    email TEXT NOT NULL DEFAULT '',
    phone TEXT NOT NULL DEFAULT '',
    role TEXT NOT NULL DEFAULT 'guest',
    hotel_id INTEGER NOT NULL DEFAULT 0,
    is_locked INTEGER NOT NULL DEFAULT 0,
    loyalty_points INTEGER NOT NULL DEFAULT 0,
    failed_attempts INTEGER NOT NULL DEFAULT 0,
    last_login TEXT,
    created_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);

CREATE TABLE IF NOT EXISTS hotels (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    hotel_name TEXT NOT NULL DEFAULT '',
    name TEXT NOT NULL,
    location TEXT NOT NULL,
    address TEXT NOT NULL DEFAULT '',
    phone TEXT NOT NULL DEFAULT '',
    email TEXT NOT NULL DEFAULT '',
    website TEXT NOT NULL DEFAULT '',
    owner_id INTEGER NOT NULL DEFAULT 0,
    description TEXT NOT NULL DEFAULT '',
    amenities TEXT NOT NULL DEFAULT '[]',
    image TEXT NOT NULL DEFAULT '',
    image_url TEXT NOT NULL DEFAULT '',
    property_type TEXT NOT NULL DEFAULT 'Hotel',
    price REAL NOT NULL DEFAULT 0.0,
    price_from REAL NOT NULL DEFAULT 0.0,
    rating REAL NOT NULL DEFAULT 5.0,
    star_rating REAL NOT NULL DEFAULT 5.0,
    review_score REAL NOT NULL DEFAULT 9.0,
    status TEXT NOT NULL DEFAULT 'pending',
    is_active INTEGER NOT NULL DEFAULT 1,
    country TEXT NOT NULL DEFAULT '',
    state TEXT NOT NULL DEFAULT '',
    city TEXT NOT NULL DEFAULT '',
    postal_code TEXT NOT NULL DEFAULT '',
    coordinates TEXT NOT NULL DEFAULT '',
    business_name TEXT NOT NULL DEFAULT '',
    room_count INTEGER NOT NULL DEFAULT 0,
    weekend_price REAL NOT NULL DEFAULT 0.0,
    seasonal_price REAL NOT NULL DEFAULT 0.0,
    taxes_fees REAL NOT NULL DEFAULT 0.0,
    min_stay INTEGER NOT NULL DEFAULT 1,
    check_in_time TEXT NOT NULL DEFAULT '',
    check_out_time TEXT NOT NULL DEFAULT '',
    cancellation_policy TEXT NOT NULL DEFAULT '',
    refund_rules TEXT NOT NULL DEFAULT '',
    child_policy TEXT NOT NULL DEFAULT '',
    pet_policy TEXT NOT NULL DEFAULT '',
    smoking_policy TEXT NOT NULL DEFAULT '',
    id_proof_requirements TEXT NOT NULL DEFAULT '',
    extra_guest_rules TEXT NOT NULL DEFAULT '',
    custom_policies TEXT NOT NULL DEFAULT '',
    bank_account TEXT NOT NULL DEFAULT '',
    tax_number TEXT NOT NULL DEFAULT '',
    pan_registration TEXT NOT NULL DEFAULT '',
    accepted_payments TEXT NOT NULL DEFAULT '[]',
    upi_id TEXT NOT NULL DEFAULT '',
    upi_qr_code TEXT NOT NULL DEFAULT '',
    commission_due REAL NOT NULL DEFAULT 0.0,
    due_date TEXT NOT NULL DEFAULT '',
    days_overdue INTEGER NOT NULL DEFAULT 0,
    commission_status TEXT NOT NULL DEFAULT 'Paid',
    hotel_status TEXT NOT NULL DEFAULT 'Active',
    created_at TEXT NOT NULL DEFAULT (datetime('now','localtime')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);

CREATE TABLE IF NOT EXISTS hotel_images (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    hotel_id INTEGER NOT NULL REFERENCES hotels(id) ON DELETE CASCADE,
    image_path TEXT NOT NULL DEFAULT '',
    image_url TEXT NOT NULL DEFAULT '',
    is_primary INTEGER NOT NULL DEFAULT 0,
    created_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);

CREATE TABLE IF NOT EXISTS hotel_documents (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    hotel_id INTEGER NOT NULL REFERENCES hotels(id) ON DELETE CASCADE,
    document_type TEXT NOT NULL DEFAULT 'verification',
    document_url TEXT NOT NULL,
    status TEXT NOT NULL DEFAULT 'pending',
    uploaded_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);

CREATE TABLE IF NOT EXISTS room_types (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    hotel_id INTEGER NOT NULL REFERENCES hotels(id) ON DELETE CASCADE,
    name TEXT NOT NULL,
    description TEXT NOT NULL DEFAULT '',
    room_size TEXT NOT NULL DEFAULT '',
    base_price REAL NOT NULL DEFAULT 0.0,
    current_price REAL NOT NULL DEFAULT 0.0,
    max_capacity INTEGER NOT NULL DEFAULT 2,
    amenities TEXT NOT NULL DEFAULT '[]',
    total_rooms INTEGER NOT NULL DEFAULT 0,
    created_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);

CREATE TABLE IF NOT EXISTS room_images (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    room_type_id INTEGER NOT NULL REFERENCES room_types(id) ON DELETE CASCADE,
    image_url TEXT NOT NULL,
    is_primary INTEGER NOT NULL DEFAULT 0,
    created_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);

CREATE TABLE IF NOT EXISTS rooms (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    hotel_id INTEGER NOT NULL REFERENCES hotels(id) ON DELETE CASCADE,
    room_type_id INTEGER NOT NULL REFERENCES room_types(id) ON DELETE CASCADE,
    room_number TEXT NOT NULL,
    floor INTEGER NOT NULL DEFAULT 1,
    status TEXT NOT NULL DEFAULT 'available',
    notes TEXT NOT NULL DEFAULT '',
    last_updated TEXT NOT NULL DEFAULT (datetime('now','localtime')),
    created_at TEXT NOT NULL DEFAULT (datetime('now','localtime')),
    UNIQUE(hotel_id, room_number)
);

CREATE TABLE IF NOT EXISTS inventory (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    hotel_id INTEGER NOT NULL REFERENCES hotels(id) ON DELETE CASCADE,
    room_id INTEGER NOT NULL REFERENCES rooms(id) ON DELETE CASCADE,
    room_type_id INTEGER NOT NULL REFERENCES room_types(id) ON DELETE CASCADE,
    status TEXT NOT NULL DEFAULT 'available',
    price REAL NOT NULL DEFAULT 0.0,
    last_synced TEXT NOT NULL DEFAULT (datetime('now','localtime')),
    UNIQUE(room_id)
);

CREATE TABLE IF NOT EXISTS staff (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL UNIQUE REFERENCES users(id) ON DELETE CASCADE,
    hotel_id INTEGER NOT NULL REFERENCES hotels(id) ON DELETE CASCADE,
    role_label TEXT NOT NULL DEFAULT 'Receptionist',
    shift TEXT NOT NULL DEFAULT 'Front desk',
    status TEXT NOT NULL DEFAULT 'active',
    avatar_url TEXT NOT NULL DEFAULT '',
    created_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);

CREATE TABLE IF NOT EXISTS bookings (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    booking_id TEXT NOT NULL UNIQUE,
    hotel_id INTEGER NOT NULL REFERENCES hotels(id) ON DELETE RESTRICT,
    room_id INTEGER NOT NULL REFERENCES rooms(id) ON DELETE RESTRICT,
    room_type_id INTEGER NOT NULL DEFAULT 0,
    room_number TEXT NOT NULL DEFAULT '',
    room_type_name TEXT NOT NULL DEFAULT '',
    guest_name TEXT NOT NULL,
    guest_email TEXT NOT NULL DEFAULT '',
    guest_phone TEXT NOT NULL,
    guest_address TEXT NOT NULL DEFAULT '',
    check_in_date TEXT NOT NULL,
    check_out_date TEXT NOT NULL,
    nights INTEGER NOT NULL DEFAULT 1,
    breakfast INTEGER NOT NULL DEFAULT 0,
    activities_json TEXT NOT NULL DEFAULT '[]',
    room_cost REAL NOT NULL DEFAULT 0.0,
    activity_cost REAL NOT NULL DEFAULT 0.0,
    breakfast_cost REAL NOT NULL DEFAULT 0.0,
    subtotal REAL NOT NULL DEFAULT 0.0,
    gst_amount REAL NOT NULL DEFAULT 0.0,
    total_amount REAL NOT NULL DEFAULT 0.0,
    coupon_code TEXT NOT NULL DEFAULT '',
    discount_amount REAL NOT NULL DEFAULT 0.0,
    payment_method TEXT NOT NULL DEFAULT 'Pay at hotel',
    payment_status TEXT NOT NULL DEFAULT 'pending',
    commission_rate REAL NOT NULL DEFAULT 15.0,
    commission_amount REAL NOT NULL DEFAULT 0.0,
    hotel_amount REAL NOT NULL DEFAULT 0.0,
    transaction_id TEXT NOT NULL DEFAULT '',
    status TEXT NOT NULL DEFAULT 'active',
    created_by INTEGER NOT NULL DEFAULT 1,
    created_at TEXT NOT NULL DEFAULT (datetime('now','localtime')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);

CREATE TABLE IF NOT EXISTS payments (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    booking_id INTEGER NOT NULL REFERENCES bookings(id) ON DELETE CASCADE,
    booking_ref TEXT NOT NULL DEFAULT '',
    hotel_id INTEGER NOT NULL DEFAULT 0,
    user_id INTEGER NOT NULL DEFAULT 0,
    provider TEXT NOT NULL DEFAULT '',
    payment_method TEXT NOT NULL DEFAULT '',
    amount REAL NOT NULL DEFAULT 0.0,
    status TEXT NOT NULL DEFAULT 'pending',
    transaction_ref TEXT NOT NULL DEFAULT '',
    upi_id TEXT NOT NULL DEFAULT '',
    merchant_name TEXT NOT NULL DEFAULT 'StaySphere',
    commission_rate REAL NOT NULL DEFAULT 15.0,
    commission_amount REAL NOT NULL DEFAULT 0.0,
    hotel_amount REAL NOT NULL DEFAULT 0.0,
    transaction_date TEXT NOT NULL DEFAULT (datetime('now','localtime')),
    created_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);

CREATE TABLE IF NOT EXISTS app_settings (
    key TEXT PRIMARY KEY,
    value TEXT NOT NULL DEFAULT '',
    updated_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);

CREATE TABLE IF NOT EXISTS hotel_payouts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    payout_id TEXT NOT NULL UNIQUE,
    hotel_id INTEGER NOT NULL DEFAULT 0,
    booking_id INTEGER NOT NULL DEFAULT 0,
    booking_ref TEXT NOT NULL DEFAULT '',
    gross_amount REAL NOT NULL DEFAULT 0.0,
    commission_amount REAL NOT NULL DEFAULT 0.0,
    payout_amount REAL NOT NULL DEFAULT 0.0,
    status TEXT NOT NULL DEFAULT 'pending',
    scheduled_at TEXT NOT NULL DEFAULT (datetime('now','localtime','+7 days')),
    paid_at TEXT NOT NULL DEFAULT '',
    transaction_ref TEXT NOT NULL DEFAULT '',
    created_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);

CREATE TABLE IF NOT EXISTS monthly_commission_ledger (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    ledger_id TEXT NOT NULL UNIQUE,
    hotel_id INTEGER NOT NULL DEFAULT 0,
    month TEXT NOT NULL DEFAULT '',
    year INTEGER NOT NULL DEFAULT 0,
    gross_revenue REAL NOT NULL DEFAULT 0.0,
    commission_rate REAL NOT NULL DEFAULT 15.0,
    commission_amount REAL NOT NULL DEFAULT 0.0,
    pending_amount REAL NOT NULL DEFAULT 0.0,
    paid_amount REAL NOT NULL DEFAULT 0.0,
    status TEXT NOT NULL DEFAULT 'pending',
    due_date TEXT NOT NULL DEFAULT '',
    paid_at TEXT NOT NULL DEFAULT '',
    transaction_ref TEXT NOT NULL DEFAULT '',
    notes TEXT NOT NULL DEFAULT '',
    created_at TEXT NOT NULL DEFAULT (datetime('now','localtime')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);

CREATE TABLE IF NOT EXISTS commission_payment_proofs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    proof_id TEXT NOT NULL UNIQUE,
    ledger_id TEXT NOT NULL DEFAULT '',
    hotel_id INTEGER NOT NULL DEFAULT 0,
    amount REAL NOT NULL DEFAULT 0.0,
    upi_id TEXT NOT NULL DEFAULT '',
    payment_proof_url TEXT NOT NULL DEFAULT '',
    transaction_ref TEXT NOT NULL DEFAULT '',
    status TEXT NOT NULL DEFAULT 'pending',
    remarks TEXT NOT NULL DEFAULT '',
    submitted_by INTEGER NOT NULL DEFAULT 0,
    reviewed_by INTEGER NOT NULL DEFAULT 0,
    reviewed_at TEXT NOT NULL DEFAULT '',
    created_at TEXT NOT NULL DEFAULT (datetime('now','localtime')),
    updated_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);

CREATE TABLE IF NOT EXISTS commission_audit_log (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    entity_type TEXT NOT NULL DEFAULT 'commission',
    entity_id TEXT NOT NULL DEFAULT '',
    hotel_id INTEGER NOT NULL DEFAULT 0,
    actor_id INTEGER NOT NULL DEFAULT 0,
    action TEXT NOT NULL DEFAULT '',
    event_payload TEXT NOT NULL DEFAULT '{}',
    created_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);

CREATE TABLE IF NOT EXISTS refund_requests (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    refund_id TEXT NOT NULL UNIQUE,
    booking_id INTEGER NOT NULL DEFAULT 0,
    booking_ref TEXT NOT NULL DEFAULT '',
    payment_id INTEGER NOT NULL DEFAULT 0,
    user_id INTEGER NOT NULL DEFAULT 0,
    hotel_id INTEGER NOT NULL DEFAULT 0,
    amount REAL NOT NULL DEFAULT 0.0,
    reason TEXT NOT NULL DEFAULT '',
    status TEXT NOT NULL DEFAULT 'requested',
    created_at TEXT NOT NULL DEFAULT (datetime('now','localtime')),
    processed_at TEXT NOT NULL DEFAULT ''
);

CREATE TABLE IF NOT EXISTS wallet_credits (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL DEFAULT 0,
    booking_id INTEGER NOT NULL DEFAULT 0,
    amount REAL NOT NULL DEFAULT 0.0,
    entry_type TEXT NOT NULL DEFAULT 'credit',
    notes TEXT NOT NULL DEFAULT '',
    created_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);

CREATE TABLE IF NOT EXISTS booking_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    booking_id INTEGER NOT NULL REFERENCES bookings(id) ON DELETE CASCADE,
    booking_ref TEXT NOT NULL DEFAULT '',
    user_id INTEGER NOT NULL DEFAULT 0,
    hotel_id INTEGER NOT NULL DEFAULT 0,
    event_type TEXT NOT NULL DEFAULT 'created',
    event_payload TEXT NOT NULL DEFAULT '{}',
    created_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);

CREATE TABLE IF NOT EXISTS coupons (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    code TEXT NOT NULL UNIQUE,
    description TEXT NOT NULL DEFAULT '',
    discount_type TEXT NOT NULL DEFAULT 'percentage',
    discount_value REAL NOT NULL DEFAULT 0.0,
    min_booking_amount REAL NOT NULL DEFAULT 0.0,
    max_discount REAL,
    valid_to TEXT,
    is_active INTEGER NOT NULL DEFAULT 1,
    usage_limit INTEGER,
    usage_count INTEGER NOT NULL DEFAULT 0,
    applicable_hotels TEXT NOT NULL DEFAULT '[]',
    created_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);

CREATE TABLE IF NOT EXISTS reviews (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL REFERENCES users(id),
    hotel_id INTEGER NOT NULL REFERENCES hotels(id),
    rating INTEGER NOT NULL DEFAULT 5,
    comment TEXT NOT NULL DEFAULT '',
    images TEXT NOT NULL DEFAULT '[]',
    manager_reply TEXT NOT NULL DEFAULT '',
    replied_at TEXT NOT NULL DEFAULT '',
    created_at TEXT NOT NULL DEFAULT (datetime('now','localtime'))
);

CREATE INDEX IF NOT EXISTS idx_hotel_images_hotel ON hotel_images(hotel_id);
CREATE INDEX IF NOT EXISTS idx_room_images_type ON room_images(room_type_id);
CREATE INDEX IF NOT EXISTS idx_inventory_hotel_status ON inventory(hotel_id, status);
CREATE INDEX IF NOT EXISTS idx_staff_hotel ON staff(hotel_id, status);
CREATE INDEX IF NOT EXISTS idx_bookings_hotel ON bookings(hotel_id);
CREATE INDEX IF NOT EXISTS idx_bookings_ref ON bookings(booking_id);
CREATE INDEX IF NOT EXISTS idx_payments_booking ON payments(booking_id);
CREATE INDEX IF NOT EXISTS idx_payments_status ON payments(status);
CREATE INDEX IF NOT EXISTS idx_payouts_hotel ON hotel_payouts(hotel_id);
CREATE INDEX IF NOT EXISTS idx_payouts_status ON hotel_payouts(status);
CREATE INDEX IF NOT EXISTS idx_commission_proofs_ledger ON commission_payment_proofs(ledger_id);
CREATE INDEX IF NOT EXISTS idx_commission_proofs_status ON commission_payment_proofs(status);
CREATE INDEX IF NOT EXISTS idx_commission_audit_hotel ON commission_audit_log(hotel_id);
CREATE INDEX IF NOT EXISTS idx_refunds_booking ON refund_requests(booking_id);
CREATE INDEX IF NOT EXISTS idx_coupons_code ON coupons(code);
