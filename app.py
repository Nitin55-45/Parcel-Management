from flask import Flask, render_template, request, session, redirect, url_for
import subprocess
import csv
import os

app = Flask(__name__)
app.secret_key = "parcel_secret_key_007"

# ─── Constants ────────────────────────────────────────────────
ADMIN_ID       = "admin007"
ADMIN_PASSWORD = "csk"
ADMIN_NAME     = "Admin"
PARCELS_CSV    = "parcels.csv"
USERS_CSV      = "users.csv"
STATUS_CSV     = "status_log.csv"

# Hardcoded delivery partner (replace with delivery.csv later)
DELIVERY_ID       = "del123"
DELIVERY_PASSWORD = "india"
DELIVERY_CITY     = "Chennai"

# ─── C Executable Helper ──────────────────────────────────────
def run_exe(exe_name, args=None, stdin_input=None):
    """Run a C executable with optional args or stdin input."""
    if os.name == "nt":
        cmd = [f"{exe_name}.exe"] + (args or [])
    else:
        cmd = [f"./{exe_name}"] + (args or [])
    try:
        result = subprocess.run(cmd, input=stdin_input, capture_output=True, text=True)
        return result.stdout
    except Exception as e:
        print(f"[C Error] {exe_name}: {e}")
        return ""

# ─── CSV Helpers ──────────────────────────────────────────────

def load_users():
    users = {}
    if not os.path.exists(USERS_CSV):
        return users
    with open(USERS_CSV, newline="", encoding="utf-8") as f:
        for row in csv.DictReader(f):
            users[row["phone"]] = {"name": row["name"], "password": row["password"]}
    return users

def save_user(name, phone, password):
    file_exists = os.path.exists(USERS_CSV)
    with open(USERS_CSV, "a", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=["name", "phone", "password"])
        if not file_exists:
            writer.writeheader()
        writer.writerow({"name": name, "phone": phone, "password": password})

def load_parcels():
    parcels = []
    if not os.path.exists(PARCELS_CSV):
        return parcels
    with open(PARCELS_CSV, newline="", encoding="utf-8") as f:
        for row in csv.DictReader(f):
            first_val = list(row.values())[0] if row else ""
            if first_val.startswith("<<<<") or first_val.startswith(">>>>") or first_val.startswith("===="):
                continue
            parcels.append(row)
    return parcels

def load_status_log():
    logs = []
    if not os.path.exists(STATUS_CSV):
        return logs
    with open(STATUS_CSV, newline="", encoding="utf-8") as f:
        for row in csv.DictReader(f):
            logs.append(row)
    return logs

def get_latest_status(tracking):
    logs = load_status_log()
    entries = [l for l in logs if l.get("tracking_number", "").strip() == tracking.strip()]
    return entries[-1] if entries else None

def get_parcel_by_tracking(tracking):
    for p in load_parcels():
        if p.get("tracking_number", "").strip() == tracking.strip():
            return p
    return None

def get_user_parcels(phone):
    parcels = load_parcels()
    sent, received = [], []
    for p in parcels:
        s = p.get("sender_contact", "").strip().strip('"')
        r = p.get("receiver_contact", "").strip().strip('"')
        trk = p.get("tracking_number", "").strip().strip('"')
        latest = get_latest_status(trk)
        p["latest_status"]   = latest["status"] if latest else "Booked"
        p["status_datetime"] = (latest["date"] + " " + latest["time"]) if latest else ""
        if s == phone:
            sent.append(p)
        if r == phone:
            received.append(p)
    return sent, received

def get_notifications(phone):
    logs    = load_status_log()
    parcels = load_parcels()
    parcel_map = {p.get("tracking_number", "").strip().strip('"'): p for p in parcels}
    notifications = []
    for log in reversed(logs):
        trk = log.get("tracking_number", "").strip()
        p   = parcel_map.get(trk)
        if not p:
            continue
        s = p.get("sender_contact",   "").strip().strip('"')
        r = p.get("receiver_contact", "").strip().strip('"')
        if s == phone:
            log["role"] = "Sender"
            notifications.append(dict(log))
        elif r == phone:
            log["role"] = "Receiver"
            notifications.append(dict(log))
    return notifications[:10]

def login_required(role=None):
    if "user" not in session:
        return False
    if role and session.get("role") != role:
        return False
    return True

# ─── Parse search output from C ───────────────────────────────
def parse_search_output(output):
    results, current = [], {}
    for line in output.splitlines():
        line = line.strip()
        if line == "RESULT_START":
            current = {}
        elif line == "RESULT_END":
            if current:
                results.append(current)
        elif line.startswith("Tracking No:"):
            current["tracking"] = line.split(":", 1)[1].strip()
        elif line.startswith("Sender:"):
            parts = line.split(":", 1)[1].strip().split(" | ")
            current["sender_name"]    = parts[0] if len(parts) > 0 else ""
            current["sender_contact"] = parts[1] if len(parts) > 1 else ""
            current["sender_address"] = parts[2] if len(parts) > 2 else ""
            current["sender_city"]    = parts[3] if len(parts) > 3 else ""
        elif line.startswith("Receiver:"):
            parts = line.split(":", 1)[1].strip().split(" | ")
            current["receiver_name"]    = parts[0] if len(parts) > 0 else ""
            current["receiver_contact"] = parts[1] if len(parts) > 1 else ""
            current["receiver_address"] = parts[2] if len(parts) > 2 else ""
            current["receiver_city"]    = parts[3] if len(parts) > 3 else ""
        elif line.startswith("Weight:"):
            current["weight"] = line.split(":", 1)[1].strip()
        elif line.startswith("Type:"):
            current["parcel_type"] = line.split(":", 1)[1].strip()
        elif line.startswith("Instructions:"):
            current["instructions"] = line.split(":", 1)[1].strip()
        elif line.startswith("Date:"):
            current["date"] = line.split(":", 1)[1].strip()
        elif line.startswith("Time:"):
            current["time"] = line.split(":", 1)[1].strip()
    return results

# ─── Auth Routes ──────────────────────────────────────────────

@app.route("/")
def index():
    if not login_required():
        return redirect(url_for("login"))
    phone = session.get("user")
    notifications = []
    if session.get("role") == "user":
        notifications = get_notifications(phone)
    return render_template("index.html",
                           role=session.get("role"),
                           user_name=session.get("name"),
                           notifications=notifications)

@app.route("/login", methods=["GET", "POST"])
def login():
    if request.method == "POST":
        username  = request.form.get("username", "").strip()
        password  = request.form.get("password", "").strip()
        role_type = request.form.get("role_type", "customer")

        if role_type == "admin":
            if username == ADMIN_ID and password == ADMIN_PASSWORD:
                session["user"]     = ADMIN_ID
                session["name"]     = ADMIN_NAME
                session["role"]     = "admin"
                session["staff_id"] = ADMIN_ID
                return redirect(url_for("index"))
            return render_template("login.html", error="Invalid Admin credentials.")

        elif role_type == "delivery":
            if username == DELIVERY_ID and password == DELIVERY_PASSWORD:
                session["user"]        = username
                session["name"]        = "Driver"
                session["role"]        = "delivery"
                session["staff_id"]    = username
                session["driver_city"] = DELIVERY_CITY
                return redirect(url_for("index"))
            return render_template("login.html", error="Invalid Driver credentials.")

        else:  # customer
            users = load_users()
            if username in users and users[username]["password"] == password:
                session["user"] = username
                session["name"] = users[username]["name"]
                session["role"] = "user"
                return redirect(url_for("index"))
            return render_template("login.html", error="Invalid Customer credentials.")

    return render_template("login.html", error=None)

@app.route("/user_signup", methods=["POST"])
def user_signup():
    name     = request.form.get("name", "").strip()
    phone    = request.form.get("phone", "").strip()
    password = request.form.get("password", "").strip()
    if len(phone) != 10 or not phone.isdigit():
        return render_template("login.html", error="Phone must be exactly 10 digits.")
    users = load_users()
    if phone in users:
        return render_template("login.html", error="Phone number already registered.")
    save_user(name, phone, password)
    return render_template("login.html", error="Registration successful! Please login.")

@app.route("/logout")
def logout():
    session.clear()
    return redirect(url_for("login"))

# ─── Parcel Registration (Admin only) ─────────────────────────

@app.route("/register")
def register():
    if not login_required("admin"):
        return redirect(url_for("login"))
    return render_template("register.html")

@app.route("/book", methods=["POST"])
def book():
    if not login_required("admin"):
        return redirect(url_for("login"))

    sender_name    = request.form["senderName"]
    sender_phone   = request.form["senderPhone"]
    sender_address = request.form["senderAddress"]
    sender_city    = request.form["senderCity"]
    receiver_name    = request.form["receiverName"]
    receiver_phone   = request.form["receiverPhone"]
    receiver_address = request.form["receiverAddress"]
    receiver_city    = request.form["receiverCity"]
    weight          = request.form["weight"]
    parcel_type     = request.form["parcelType"]
    spl_instruction = request.form.get("specialInstructions", "")

    stdin_data = (f"{sender_name}\n{sender_address}\n{sender_city}\n{sender_phone}\n"
                  f"{receiver_name}\n{receiver_address}\n{receiver_city}\n{receiver_phone}\n"
                  f"{weight}\n{parcel_type}\n{spl_instruction}\n")

    output = run_exe("program", stdin_input=stdin_data)

    tracking         = "N/A"
    booking_datetime = "N/A"
    for line in output.splitlines():
        line = line.strip()
        if "Tracking No" in line:
            tracking = line.split(":", 1)[1].strip()
        if "Date" in line:
            booking_datetime = line.split(":", 1)[1].strip()

    return render_template("confirm.html",
                           sender_name=sender_name, sender_phone=sender_phone,
                           sender_address=sender_address, sender_city=sender_city,
                           receiver_name=receiver_name, receiver_phone=receiver_phone,
                           receiver_address=receiver_address, receiver_city=receiver_city,
                           weight=weight, parcel_type=parcel_type,
                           spl_instruction=spl_instruction,
                           date=booking_datetime, tracking=tracking)

# ─── Admin Dashboard (Hub Queue) ──────────────────────────────

@app.route("/admin_dashboard")
def admin_dashboard():
    if not login_required("admin"):
        return redirect(url_for("login"))

    selected_hub  = request.args.get("hub", "").strip()
    search_query  = request.args.get("search", "").strip()
    all_parcels   = load_parcels()

    # Get unique hub list from all sender/receiver cities
    hubs = sorted(set(
        [p.get("sender_city", "").strip() for p in all_parcels] +
        [p.get("receiver_city", "").strip() for p in all_parcels]
    ) - {""})

    booked       = []
    intransit    = []
    out_delivery = []

    # Override search result
    override_result = request.args.get("override_result", "")
    override_tracking = request.args.get("override_tracking", "")

    if selected_hub:
        for p in all_parcels:
            trk    = p.get("tracking_number", "").strip()
            latest = get_latest_status(trk)
            status = latest["status"].strip() if latest else "Booked"
            p["latest_status"] = status

            s_city = p.get("sender_city",   "").strip().lower()
            r_city = p.get("receiver_city", "").strip().lower()
            hub_l  = selected_hub.lower()

            # Apply search filter if provided
            if search_query and search_query.lower() not in trk.lower():
                continue

            if status == "Booked" and s_city == hub_l:
                booked.append(p)
            elif status == "InTransit" and r_city == hub_l:
                intransit.append(p)
            elif status in ("Out for Delivery", "Reached") and r_city == hub_l:
                out_delivery.append(p)

    return render_template("admin_dashboard.html",
                           hubs=hubs,
                           selected_hub=selected_hub,
                           search_query=search_query,
                           booked=booked,
                           intransit=intransit,
                           out_for_delivery=out_delivery,
                           override_result=override_result,
                           override_tracking=override_tracking)

@app.route("/process_batch", methods=["POST"])
def process_batch():
    if not login_required("admin"):
        return redirect(url_for("login"))

    tracking_numbers = request.form.getlist("tracking_numbers")
    target_status    = request.form.get("target_status", "").strip()
    current_hub      = request.form.get("current_hub", "").strip()
    admin_id         = session.get("staff_id", "ADMIN_SYS")

    for trk in tracking_numbers:
        try:
            run_exe("update_main", args=["override", trk, target_status, admin_id, current_hub])
        except Exception as e:
            print(f"[process_batch] Error updating {trk}: {e}")

    return redirect(url_for("admin_dashboard", hub=current_hub))

@app.route("/admin_override", methods=["POST"])
def admin_override():
    """Search bar override action inside admin dashboard."""
    if not login_required("admin"):
        return redirect(url_for("login"))

    tracking   = request.form.get("tracking_number", "").strip()
    new_status = request.form.get("status", "").strip()
    location   = request.form.get("location", "").strip()
    hub        = request.form.get("current_hub", "").strip()
    admin_id   = session.get("staff_id", "ADMIN_SYS")

    output = run_exe("update_main", args=["override", tracking, new_status, admin_id, location])
    success = "STATUS_OK" in output
    result_msg = "✅ Status updated successfully!" if success else "❌ Update failed. Check tracking number."

    return redirect(url_for("admin_dashboard", hub=hub,
                            override_result=result_msg,
                            override_tracking=tracking))

# ─── Search (Admin — all parcels; Customer — own only) ────────

@app.route("/search")
def search():
    role = session.get("role", "")
    if not login_required():
        return redirect(url_for("login"))
    return render_template("search.html", role=role)

@app.route("/search_by_tracking", methods=["POST"])
def search_by_tracking():
    if not login_required():
        return redirect(url_for("login"))

    tracking_number = request.form.get("tracking_number", "").strip()
    output = run_exe("search_main", stdin_input=tracking_number + "\n")

    results_list = []
    lines = output.strip().split("\n")
    if len(lines) >= 2 and lines[0] == "FOUND":
        data = lines[1].split("|")
        if len(data) >= 12:
            results_list.append({
                "tracking":         data[0],
                "sender_name":      data[1],
                "sender_contact":   data[2],
                "sender_address":   data[3],
                "sender_city":      data[4],
                "receiver_name":    data[5],
                "receiver_contact": data[6],
                "receiver_address": data[7],
                "receiver_city":    data[8],
                "weight":           data[9],
                "parcel_type":      data[10],
                "instructions":     data[11],
                "date":             "N/A",
                "time":             "N/A",
            })

    return render_template("search_results.html",
                           search_type="Tracking Number",
                           search_query=tracking_number,
                           results=results_list)

@app.route("/search_by_hub", methods=["POST"])
def search_by_hub():
    if not login_required("admin"):
        return redirect(url_for("login"))

    hub = request.form.get("hub", "").strip()
    output = run_exe("search", args=["location", hub])
    results = parse_search_output(output)

    # Enrich with latest status
    for r in results:
        latest = get_latest_status(r.get("tracking", ""))
        r["latest_status"] = latest["status"] if latest else "Booked"

    return render_template("search_results.html",
                           search_type="Hub",
                           search_query=hub,
                           results=results)

@app.route("/search_by_date", methods=["POST"])
def search_by_date():
    if not login_required():
        return redirect(url_for("login"))

    from_date = request.form.get("from_date", "").strip()
    to_date   = request.form.get("to_date", "").strip()
    output    = run_exe("search", args=["date", from_date, to_date])
    results   = parse_search_output(output)

    for r in results:
        latest = get_latest_status(r.get("tracking", ""))
        r["latest_status"] = latest["status"] if latest else "Booked"

    return render_template("search_results.html",
                           search_type="Date Range",
                           search_query=f"{from_date} to {to_date}",
                           results=results)

# ─── My Parcels (Customer only) ───────────────────────────────

@app.route("/my_parcels")
def my_parcels():
    if not login_required("user"):
        return redirect(url_for("login"))
    phone = session.get("user")
    name  = session.get("name")
    sent, received = get_user_parcels(phone)
    notifications  = get_notifications(phone)
    return render_template("my_parcels.html",
                           user_name=name, phone=phone,
                           sent=sent, received=received,
                           notifications=notifications)

# ─── Delivery Dashboard ───────────────────────────────────────

@app.route("/delivery_dashboard")
def delivery_dashboard():
    if session.get("role") != "delivery":
        return redirect(url_for("login"))

    driver_id = session.get("staff_id", "DRV01")
    selected_hub = request.args.get("hub")

    hub_result = subprocess.run(["./get_hubs_main.exe"], capture_output=True, text=True)
    hubs = [h.strip() for h in hub_result.stdout.strip().splitlines() if h.strip()]

    deliveries = []
    if selected_hub:
        parcels_all = load_parcels()
        for p in parcels_all:
            if p.get("receiver_city", "").strip().lower() == selected_hub.lower():
                latest = get_latest_status(p.get("tracking_number", "").strip())
                status = latest["status"].strip() if latest else "Booked"
                p["latest_status"] = status
                # Show ALL parcels that are not yet delivered
                if status != "Delivered":
                    deliveries.append(p)

    return render_template("delivery_dashboard.html",
                           driver_id=driver_id,
                           selected_hub=selected_hub,
                           hubs=hubs,
                           deliveries=deliveries)

@app.route("/mark_delivered", methods=["POST"])
@app.route("/mark_delivered", methods=["POST"])
def mark_delivered():
    tracking_numbers = request.form.getlist("tracking_numbers")
    current_hub = request.form.get("current_hub")
    driver_id = session.get("staff_id", "DRV01")

    for tracking in tracking_numbers:
        subprocess.run(["./update_main.exe", "deliver", tracking, driver_id, current_hub])

    return redirect(url_for("delivery_dashboard", hub=current_hub))
@app.route("/parcel_log")
def parcel_log():
    if session.get("role") != "admin":
        return redirect(url_for("login"))

    parcels = load_parcels()
    for p in parcels:
        tracking = p.get("tracking_number", "").strip()
        latest = get_latest_status(tracking)
        p["latest_status"] = latest["status"] if latest else "Booked"
        p["status_date"] = latest["date"] if latest else "-"
        p["status_time"] = latest["time"] if latest else "-"

    return render_template("parcel_log.html", parcels=parcels)


# ─── Run ──────────────────────────────────────────────────────

if __name__ == "__main__":
    app.run(debug=True)
