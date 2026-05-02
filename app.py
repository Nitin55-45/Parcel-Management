from flask import Flask, render_template, request, session, redirect, url_for
import subprocess, csv, os, functools, datetime

app = Flask(__name__)
app.secret_key = "parcel_secret_key_007"

# Configuration
ADMIN_ID, ADMIN_PASSWORD, ADMIN_NAME = "admin007", "csk", "Admin"
DELIVERY_ID, DELIVERY_PASSWORD, DELIVERY_CITY = "del123", "india", "Chennai"
USERS_CSV = "users.csv"

def run_exe(cmd, args=None, stdin_input=None):
    exe = [f"{cmd}.exe"] if os.name == "nt" else [f"./{cmd}"]
    if args: exe += args
    try:
        res = subprocess.run(exe, input=stdin_input, capture_output=True, text=True)
        return res.stdout
    except: return ""

def role_required(role=None):
    def decorator(f):
        @functools.wraps(f)
        def wrapped(*args, **kwargs):
            if "user" not in session or (role and session.get("role") != role):
                return redirect(url_for("login"))
            return f(*args, **kwargs)
        return wrapped
    return decorator

def get_hubs():
    output = run_exe("parcel_system", ["hubs"])
    for line in output.splitlines():
        if line.startswith("CITIES:"):
            return sorted(c.strip() for c in line[7:].split(",") if c.strip())
    return []

def parse_parcel(line):
    d = [x.strip() for x in line.split("|")]
    if len(d) < 13: return None
    res = {"tracking_number": d[0], "sender_name": d[1], "sender_contact": d[2], "sender_address": d[3], "sender_city": d[4],
           "receiver_name": d[5], "receiver_contact": d[6], "receiver_address": d[7], "receiver_city": d[8],
           "weight": d[9], "parcel_type": d[10], "special_instructions": d[11], "priority": "0"}
    if len(d) == 13: res["latest_status"] = d[12]
    elif len(d) == 14: res.update({"latest_status": d[12], "priority": d[13]})
    elif len(d) == 15: res.update({"latest_status": d[12], "status_datetime": f"{d[13]} {d[14]}"})
    elif len(d) == 16: res.update({"latest_status": d[12], "status_datetime": f"{d[13]} {d[14]}", "priority": d[15]})
    elif len(d) >= 18: 
        res.update({"date": d[12], "time": d[13], "latest_status": d[14], "status_date": d[15], "status_time": d[16], "priority": d[17]})
        if len(d) >= 19: res["tag"] = "|".join(d[18:])
    return res

def parse_blocks(output, start, end):
    res, inside = [], False
    for line in output.splitlines():
        if line.strip() == start: inside = True
        elif line.strip() == end: inside = False
        elif inside and line.strip(): res.append(line.strip())
    return res

@app.route("/")
@role_required()
def index():
    notifs = []
    if session.get("role") == "user":
        out = run_exe("parcel_system", ["notifs", session["user"]])
        for block in parse_blocks(out, "NOTIF_START", "NOTIF_END"):
            parts = block.split("|")
            if len(parts) >= 5:
                notifs.append({
                    "tracking_number": parts[0], "status": parts[1],
                    "date": parts[2], "time": parts[3], "role": parts[4],
                    "location": parts[5] if len(parts) >= 6 else "N/A"
                })
    return render_template("index.html", role=session.get("role"), user_name=session.get("name"), notifications=notifs)

@app.route("/login", methods=["GET", "POST"])
def login():
    if request.method != "POST": return render_template("login.html")
    u, p, r = request.form.get("username"), request.form.get("password"), request.form.get("role_type")
    if r == "admin" and u == ADMIN_ID and p == ADMIN_PASSWORD:
        session.update(user=u, name=ADMIN_NAME, role="admin", staff_id=u)
    elif r == "delivery" and u == DELIVERY_ID and p == DELIVERY_PASSWORD:
        session.update(user=u, name="Driver", role="delivery", staff_id=u, driver_city=DELIVERY_CITY)
    else:
        users = {}
        if os.path.exists(USERS_CSV):
            with open(USERS_CSV, newline="") as f:
                for row in csv.DictReader(f): users[row["phone"]] = row
        if u in users and users[u]["password"] == p:
            session.update(user=u, name=users[u]["name"], role="user")
        else: return render_template("login.html", error="Invalid credentials")
    return redirect(url_for("index"))

@app.route("/user_signup", methods=["POST"])
def signup():
    n, ph, pw = request.form.get("name"), request.form.get("phone"), request.form.get("password")
    if len(ph) != 10: return render_template("login.html", error="Invalid phone")
    ex = os.path.exists(USERS_CSV)
    with open(USERS_CSV, "a", newline="") as f:
        w = csv.DictWriter(f, fieldnames=["name", "phone", "password"])
        if not ex: w.writeheader()
        w.writerow({"name": n, "phone": ph, "password": pw})
    return render_template("login.html", error="Signup successful")

@app.route("/logout")
def logout(): session.clear(); return redirect(url_for("login"))

@app.route("/register")
@role_required("admin")
def register(): return render_template("register.html")

@app.route("/book", methods=["POST"])
@role_required("admin")
def book():
    f = request.form
    # 4 lines for sender, 4 for receiver, 3 for parcel info, 1 for priority = 12 lines
    stdin = f"{f['senderName']}\n{f['senderAddress']}\n{f['senderCity']}\n{f['senderPhone']}\n{f['receiverName']}\n{f['receiverAddress']}\n{f['receiverCity']}\n{f['receiverPhone']}\n{f['weight']}\n{f['parcelType']}\n{f.get('specialInstructions','') or 'None'}\n{f.get('priority', '0')}\n"
    out = run_exe("parcel_system", ["book"], stdin)
    tr, dt = "N/A", "N/A"
    for l in out.splitlines():
        if "Tracking No :" in l: tr = l.split("Tracking No :")[1].strip()
        if "Date :" in l: dt = l.split("Date :")[1].strip()
    return render_template("confirm.html", 
                           sender_name=f["senderName"], sender_phone=f["senderPhone"], sender_address=f["senderAddress"], sender_city=f["senderCity"],
                           receiver_name=f["receiverName"], receiver_phone=f["receiverPhone"], receiver_address=f["receiverAddress"], receiver_city=f["receiverCity"],
                           weight=f["weight"], parcel_type=f["parcelType"], spl_instruction=f.get("specialInstructions") or "None",
                           priority=int(f.get("priority", 0)), tracking=tr, date=dt)

@app.route("/admin_dashboard")
@role_required("admin")
def admin_dashboard():
    h, s = request.args.get("hub", "").title(), request.args.get("search", "").lower()
    b, i, o, log = [], [], [], []
    if h:
        q_out = run_exe("parcel_system", ["queues", h])
        qs = {"booked": [], "intransit": [], "outdelivery": []}
        for l in q_out.splitlines():
            for k in qs:
                if l.startswith(k.upper() + ":"): qs[k] = [t for t in l.split(":")[1].split(",") if t.strip()]
        all_p = {p["tracking_number"]: p for p in [parse_parcel(l) for l in parse_blocks(run_exe("parcel_system", ["parcellog"]), "PARCEL_START", "PARCEL_END")] if p}
        for k, lst in [("booked", b), ("intransit", i), ("outdelivery", o)]:
            for t in qs[k]:
                if not s or s in t.lower():
                    if t in all_p: lst.append(all_p[t])
        
        out_log = run_exe("parcel_system", ["delivered_today", h])
        for line in parse_blocks(out_log, "DELIV_START", "DELIV_END"):
            parts = line.split("|")
            if len(parts) >= 11:
                log.append({
                    "tracking": parts[0], "sender_name": parts[1], "sender_contact": parts[2], "from_city": parts[3],
                    "receiver_name": parts[4], "receiver_contact": parts[5], "receiver_city": parts[6], 
                    "date": parts[7], "time": parts[8], "type": parts[9], "priority": parts[10]
                })
    today_str = datetime.date.today().strftime("%d %b %Y")
    return render_template("admin_dashboard.html", hubs=get_hubs(), selected_hub=h, search_query=s, booked=b, intransit=i, out_for_delivery=o, log=log, override_msg=request.args.get("override_result"), override_tracking=request.args.get("override_tracking"), today=today_str)

@app.route("/process_batch", methods=["POST"])
@role_required("admin")
def process():
    h, t, sel = request.form.get("current_hub"), request.form.get("target_status"), request.form.getlist("tracking_numbers")
    aid = session.get("staff_id", "ADMIN")
    f_q = {"InTransit": "booked", "Out for Delivery": "intransit", "Delivered": "outdelivery"}.get(t)
    if f_q == "outdelivery":
        for tr in sel: run_exe("parcel_system", ["update", "deliver", tr, aid, h])
    else:
        for _ in range(len(sel) if sel else 1): run_exe("parcel_system", ["process", h, f_q, aid])
    return redirect(url_for("admin_dashboard", hub=h))

@app.route("/admin_override", methods=["POST"])
@role_required("admin")
def override():
    tr = request.form.get("tracking_number", "").strip()
    h, st = request.form.get("current_hub"), request.form.get("status")
    loc, s_out = h, run_exe("parcel_system", ["search", "tracking", tr])
    tag = "Admin_Override_Fwd"
    if s_out.startswith("FOUND"):
        lines = s_out.splitlines()
        if len(lines) > 1:
            d = lines[1].split("|")
            if len(d) >= 13:
                loc = d[4] if st == "Booked" else d[8]
                st_old = d[12]
                r_map = {"booked": 1, "intransit": 2, "out for delivery": 3, "delivered": 4}
                s1, s2 = st.lower(), st_old.lower()
                if r_map.get(s1, 0) < r_map.get(s2, 0):
                    tag = f"BACK|{st_old}|{h}"
    out = run_exe("parcel_system", ["update", "override", tr, st, session.get("staff_id", "ADMIN"), loc, tag])
    msg = f"Moved to {loc} ({st})" if "STATUS_OK" in out else "Override Failed"
    if "BACK|" in tag:
        parts = tag.split('|')
        msg = f"Returned from {parts[1]} ({parts[2]}) to {loc} ({st})"
    return redirect(url_for("admin_dashboard", hub=h, override_result=msg, override_tracking=tr))

@app.route("/search")
@role_required()
def search_page():
    if session.get("role") == "user": return redirect(url_for("my_parcels"))
    return render_template("search.html", role=session.get("role"), hubs=get_hubs())

@app.route("/search_by_tracking", methods=["POST"])
@role_required()
def search_trk():
    tr = request.form.get("tracking_number", "").strip()
    out = run_exe("parcel_system", ["search", "tracking", tr])
    res = []
    if out.startswith("FOUND"):
        lines = out.splitlines()
        if len(lines) > 1:
            d = [x.strip() for x in lines[1].split("|")]
            if len(d) >= 15:
                res.append({"tracking": d[0], "sender_name": d[1], "sender_contact": d[2], "sender_city": d[4], "receiver_name": d[5], "receiver_contact": d[6], "receiver_city": d[8], "weight": d[9], "parcel_type": d[10], "instructions": d[11], "latest_status": d[12], "date": d[13], "time": d[14]})
    return render_template("search_results.html", search_type="Tracking", search_query=tr, results=res)

def parse_search(out):
    res, cur = [], {}
    for l in out.splitlines():
        l = l.strip()
        if l == "RESULT_START": cur = {}
        elif l == "RESULT_END": res.append(cur)
        elif ":" in l:
            k, v = l.split(":", 1)
            k = k.lower().replace(" ", "_").replace("tracking_no", "tracking")
            if k in ["sender", "receiver"]:
                p = v.strip().split(" | ")
                for i, sf in enumerate(["name", "contact", "address", "city"]):
                    if i < len(p): cur[f"{k}_{sf}"] = p[i]
            elif k == "status":
                cur["latest_status"] = v.strip()
            else: cur[k] = v.strip()
    return res

@app.route("/search_by_hub", methods=["POST"])
@role_required("admin")
def search_hub():
    h = request.form.get("hub").title()
    res = parse_search(run_exe("parcel_system", ["search", "location", h]))
    return render_template("search_results.html", search_type="Hub", search_query=h, results=res, from_hub=[r for r in res if r.get("direction")=="FROM"], to_hub=[r for r in res if r.get("direction")=="TO"])

@app.route("/search_by_date", methods=["POST"])
@role_required()
def search_dt():
    f, t = request.form.get("from_date"), request.form.get("to_date")
    res = parse_search(run_exe("parcel_system", ["search", "date", f, t]))
    return render_template("search_results.html", search_type="Date Range", search_query=f"{f} to {t}", results=res)

@app.route("/my_parcels")
@role_required("user")
def my_parcels():
    out = run_exe("parcel_system", ["myparcels", session["user"]])
    s = [p for p in [parse_parcel(l) for l in parse_blocks(out, "SENT_START", "SENT_END")] if p]
    r = [p for p in [parse_parcel(l) for l in parse_blocks(out, "RECV_START", "RECV_END")] if p]
    n = [{"tracking_number": d[0], "status": d[1], "date": d[2], "time": d[3]} for d in [x.split("|") for x in parse_blocks(run_exe("parcel_system", ["notifs", session["user"]]), "NOTIF_START", "NOTIF_END")]]
    return render_template("my_parcels.html", user_name=session.get("name"), phone=session["user"], sent=s, received=r, notifications=n)

@app.route("/delivery_dashboard")
@role_required("delivery")
def delivery():
    h = request.args.get("hub", "").title()
    d, log = [], []
    if h:
        out = run_exe("parcel_system", ["delivery", h])
        d = [p for p in [parse_parcel(l) for l in parse_blocks(out, "PARCEL_START", "PARCEL_END")] if p]
        
        out_log = run_exe("parcel_system", ["delivered_today", h, session.get("staff_id", "")] )
        for line in parse_blocks(out_log, "DELIV_START", "DELIV_END"):
            parts = line.split("|")
            if len(parts) >= 11:
                log.append({
                    "tracking": parts[0], "sender_name": parts[1], "sender_contact": parts[2], "from_city": parts[3],
                    "receiver_name": parts[4], "receiver_contact": parts[5], "receiver_city": parts[6], 
                    "date": parts[7], "time": parts[8], "type": parts[9], "priority": parts[10]
                })
    today_str = datetime.date.today().strftime("%d %b %Y")
    return render_template("delivery_dashboard.html", staff_id=session.get("staff_id"), selected_hub=h, hubs=get_hubs(), deliveries=d, log=log, today=today_str)

@app.route("/mark_delivered", methods=["POST"])
@role_required("delivery")
def deliver():
    h = request.form.get("current_hub")
    for t in request.form.getlist("tracking_numbers"): run_exe("parcel_system", ["update", "deliver", t, session.get("staff_id"), h])
    return redirect(url_for("delivery", hub=h))

@app.route("/parcel_log")
@role_required("admin")
def parcel_log():
    p = [p for p in [parse_parcel(l) for l in parse_blocks(run_exe("parcel_system", ["parcellog"]), "PARCEL_START", "PARCEL_END")] if p]
    return render_template("parcel_log.html", parcels=p)

if __name__ == "__main__": app.run(debug=True)