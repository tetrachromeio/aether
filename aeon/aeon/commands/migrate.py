import sqlite3
import toml
from pathlib import Path

AEON_CONFIG = Path("aeon.toml")
MIGRATIONS_DIR = Path("aeon_modules/migrations")

def get_sqlite_path() -> str:
    config = toml.load(AEON_CONFIG)
    return config["database"]["path"]

def ensure_migrations_table(conn: sqlite3.Connection):
    conn.execute("""
        CREATE TABLE IF NOT EXISTS __migrations__ (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            filename TEXT NOT NULL UNIQUE,
            applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    """)

def get_applied_migrations(conn: sqlite3.Connection) -> set:
    cur = conn.execute("SELECT filename FROM __migrations__")
    return {row[0] for row in cur.fetchall()}

def apply_migration(conn: sqlite3.Connection, migration_file: Path):
    with open(migration_file, "r") as f:
        sql = f.read()
    conn.executescript(sql)
    conn.execute("INSERT INTO __migrations__ (filename) VALUES (?)", (migration_file.name,))

def migrate():
    db_path = get_sqlite_path()
    conn = sqlite3.connect(db_path)

    try:
        with conn:
            ensure_migrations_table(conn)
            applied = get_applied_migrations(conn)
            all_migrations = sorted(MIGRATIONS_DIR.glob("*.sql"))
            new_migrations = [m for m in all_migrations if m.name not in applied]

            if not new_migrations:
                print("No new migrations to apply.")
                return

            for mig in new_migrations:
                print(f"Applying migration: {mig.name}")
                apply_migration(conn, mig)

            print(f"✅ Applied {len(new_migrations)} new migration(s).")

    except sqlite3.Error as e:
        print(f"❌ Migration failed: {e}")
        print("Rolling back...")
        conn.rollback()
    finally:
        conn.close()
