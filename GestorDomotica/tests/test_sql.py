#!/usr/bin/env python3
"""Ejecuta contra SQLite real TODAS las sentencias de src/db/Sql.h (esquema + consultas)."""
import re, sqlite3, sys, pathlib

src = (pathlib.Path(__file__).parent.parent / "src/db/Sql.h").read_text(encoding="utf-8")
S = {}
for name, body in re.findall(r'(k\w+)\s*=\s*R"SQL\((.*?)\)SQL";', src, re.S):
    S[name] = body
for name, body in re.findall(r'(k\w+)\s*=\s*\n?\s*"((?:[^"\\]|\\.)*)";', src):
    S[name] = body

fails = 0
total = 0
def check(cond, msg):
    global fails, total
    total += 1
    if not cond:
        fails += 1
        print("FALLO:", msg)

db = sqlite3.connect(":memory:")
db.execute("PRAGMA foreign_keys = ON")
schema = [k for k in S if k.startswith("kCreate")]
check(len(schema) == 10, f"se esperaban 10 sentencias de esquema, hay {len(schema)}")
for k in schema:
    db.execute(S[k])
for k in schema:   # idempotente (IF NOT EXISTS)
    db.execute(S[k])

def q(stmt, /, **p):
    return db.execute(S[stmt], p).fetchall()

# Usuarios (UNIQUE sin distinguir mayúsculas)
q("kInsertUser", username="Nicolas", salt="s", password_hash="h", created_at=1)
check(q("kCountUsers") == [(1,)], "contar usuarios")
check(q("kSelectUser", username="nicolas")[0][1] == "Nicolas", "búsqueda NOCASE de usuario")
try:
    q("kInsertUser", username="NICOLAS", salt="s", password_hash="h", created_at=1)
    check(False, "usuario duplicado debió fallar")
except sqlite3.IntegrityError:
    check(True, "")

# Dispositivos
def dev(name, typ="light", state="off", w=60.0, h=5.0, lvl=0, room="Sala"):
    q("kInsertDevice", name=name, room=room, type=typ, state=state, power_w=w, daily_hours=h, level=lvl)
    return db.execute("SELECT last_insert_rowid()").fetchone()[0]
a = dev("Luz sala", w=100)
b = dev("persiana", "blind")
check([r[1] for r in q("kSelectDevices")] == ["Luz sala", "persiana"], "orden NOCASE")
check(q("kSelectDevice", id=a)[0][3] == "light", "select device")
check(q("kDeviceNameExists", name="LUZ SALA", exclude_id=0) == [(1,)], "nombre existe (NOCASE)")
check(q("kDeviceNameExists", name="luz sala", exclude_id=a) == [(0,)], "nombre existe excluyendo id")
for bad in [dict(typ="toaster"), dict(state="roto"), dict(w=-1.0), dict(h=25.0), dict(lvl=101)]:
    try:
        dev("x" + str(bad), **bad)
        check(False, f"CHECK debió rechazar {bad}")
    except sqlite3.IntegrityError:
        check(True, "")
try:
    dev("LUZ SALA")
    check(False, "nombre duplicado debió fallar")
except sqlite3.IntegrityError:
    check(True, "")
q("kUpdateDevice", id=a, name="Luz sala", room="Cocina", type="light", state="on", power_w=100.0, daily_hours=5.0, level=100)
check(q("kSelectDevice", id=a)[0][2:5] == ("Cocina", "light", "on"), "update device")

# Eventos (device_id NULL permitido; filtros; orden descendente; LIMIT)
for i, (dv, kind) in enumerate([(a, "ENCENDIDO"), (b, "APAGADO"), (a, "APAGADO"), (None, "ELIMINACION")]):
    q("kInsertEvent", ts=100 + i, device_id=dv, device_name="n", kind=kind, message="m", username="u")
check(len(q("kSelectEvents", device_id=0, kind="", limit=100)) == 4, "eventos todos")
check([r[1] for r in q("kSelectEvents", device_id=0, kind="", limit=100)] == [103, 102, 101, 100], "orden DESC")
check(len(q("kSelectEvents", device_id=a, kind="", limit=100)) == 2, "filtro por dispositivo")
check(len(q("kSelectEvents", device_id=0, kind="APAGADO", limit=100)) == 2, "filtro por tipo")
check(len(q("kSelectEvents", device_id=a, kind="APAGADO", limit=100)) == 1, "filtro combinado")
check(len(q("kSelectEvents", device_id=0, kind="", limit=2)) == 2, "LIMIT")
check(q("kSelectEvents", device_id=0, kind="", limit=1)[0][2] == 0, "device_id NULL -> 0")

# Sesiones y consumo medido
q("kOpenSession", device_id=a, ts=1000)
q("kCloseSession", ts=4600, device_id=a)             # 3600 s
q("kOpenSession", device_id=a, ts=10000)             # abierta
q("kOpenSession", device_id=b, ts=0)
q("kCloseSession", ts=500, device_id=b)              # fuera de la ventana
u = {r[0]: r for r in q("kUsage", now=12000, **{"from": 900})}
check(u[a][3] == 3600 + 2000, f"uso de luz = {u[a][3]}")
check(u[b][3] == 0, "uso fuera de ventana = 0")
u = {r[0]: r for r in q("kUsage", now=12000, **{"from": 2000})}   # recorta la 1.ª sesión: 2000..4600
check(u[a][3] == 2600 + 2000, f"uso recortado = {u[a][3]}")
c = dev("sin sesiones")
check({r[0]: r[3] for r in q("kUsage", now=12000, **{"from": 0})}[c] == 0, "dispositivo sin sesiones -> 0")
q("kCloseSession", ts=11000, device_id=a)
check(len(db.execute("SELECT 1 FROM sessions WHERE end_ts IS NULL").fetchall()) == 0, "cerrar sesión")

# Rutinas
q("kInsertRoutine", name="Noche", minute_of_day=1380, action="off", days_mask=127, enabled=1)
r = db.execute("SELECT last_insert_rowid()").fetchone()[0]
q("kInsertRoutineDevice", routine_id=r, device_id=a)
q("kInsertRoutineDevice", routine_id=r, device_id=a)   # duplicado ignorado
q("kInsertRoutineDevice", routine_id=r, device_id=b)
check(q("kSelectRoutineDevices") == [(r, a), (r, b)], "dispositivos de rutina")
q("kUpdateRoutine", id=r, name="Noche 2", minute_of_day=1400, action="on", days_mask=31, enabled=0)
check(q("kSelectRoutines")[0] == (r, "Noche 2", 1400, "on", 31, 0), "update rutina")
try:
    q("kInsertRoutine", name="x", minute_of_day=1440, action="on", days_mask=1, enabled=1)
    check(False, "minuto inválido debió fallar")
except sqlite3.IntegrityError:
    check(True, "")
q("kDeleteRoutineDevices", routine_id=r)
check(q("kSelectRoutineDevices") == [], "borrar dispositivos de rutina")
q("kInsertRoutineDevice", routine_id=r, device_id=a)

# Borrar dispositivo: eventos conservados con NULL, sesiones y rutina_dispositivos en cascada
q("kDeleteDevice", id=a)
check(q("kSelectDevice", id=a) == [], "dispositivo borrado")
check(len(q("kSelectEvents", device_id=0, kind="", limit=100)) == 4, "eventos se conservan")
check(all(row[2] == 0 for row in q("kSelectEvents", device_id=0, kind="", limit=100) if row[0] in (1, 3)), "eventos huérfanos con id 0")
check(db.execute("SELECT COUNT(*) FROM sessions WHERE device_id=?", (a,)).fetchone()[0] == 0, "cascada sesiones")
check(q("kSelectRoutineDevices") == [], "cascada routine_devices")
q("kDeleteRoutine", id=r)
check(q("kSelectRoutines") == [], "borrar rutina")

# Configuración (upsert)
check(q("kGetSetting", key="tariff") == [], "setting inexistente")
q("kSetSetting", key="tariff", value="850")
q("kSetSetting", key="tariff", value="900")
check(q("kGetSetting", key="tariff") == [("900",)], "setting reemplazado")

print(f"{total - fails}/{total} comprobaciones SQL correctas")
sys.exit(1 if fails else 0)
