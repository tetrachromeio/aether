import os
import toml
import hashlib
import json
from pathlib import Path
from typing import Dict, Tuple

MODELS_DIR = Path("src/models")
SCHEMA_SNAPSHOT_DIR = Path("aeon_modules/schema")
MIGRATIONS_DIR = Path("aeon_modules/migrations")

def hash_model(model_data: dict) -> str:
    return hashlib.sha256(json.dumps(model_data, sort_keys=True).encode()).hexdigest()

def load_toml_models(directory: Path) -> Dict[str, dict]:
    models = {}
    for file in directory.glob("*.toml"):
        data = toml.load(file)
        model_name = data["model"]["name"]
        models[model_name] = data
    return models

def load_snapshots() -> Dict[str, dict]:
    if not SCHEMA_SNAPSHOT_DIR.exists():
        return {}
    snapshot_file = SCHEMA_SNAPSHOT_DIR / "schema_defs.json"
    if snapshot_file.exists():
        with open(snapshot_file) as f:
            return json.load(f)
    return {}

def save_snapshots(defs: Dict[str, dict]):
    SCHEMA_SNAPSHOT_DIR.mkdir(parents=True, exist_ok=True)
    snapshot_file = SCHEMA_SNAPSHOT_DIR / "schema_defs.json"
    with open(snapshot_file, "w") as f:
        json.dump(defs, f, indent=4)

def map_type(field_type: str) -> str:
    return {
        "int": "INTEGER",
        "string": "TEXT",
        "datetime": "TIMESTAMP",
        "bool": "BOOLEAN",
        "float": "REAL"
    }.get(field_type, "TEXT")

def generate_create_table_sql(table_name: str, fields: dict) -> str:
    lines = []
    for name, props in fields.items():
        line = f"    {name} {map_type(props['type'])}"
        if props.get("primary_key"):
            line += " PRIMARY KEY"
        if not props.get("nullable", True):
            line += " NOT NULL"
        if props.get("unique"):
            line += " UNIQUE"  # SQLite supports inline UNIQUE, so this is fine
        lines.append(line)
    sql = f"CREATE TABLE IF NOT EXISTS {table_name} (\n" + ",\n".join(lines) + "\n);"
    return sql

def compare_fields(old: dict, new: dict) -> dict:
    added = {}
    dropped = {}
    altered = {}
    nullability_changed = {}
    unique_added = []
    unique_dropped = []

    old_keys = set(old.keys())
    new_keys = set(new.keys())

    for col in new_keys - old_keys:
        added[col] = new[col]

    for col in old_keys - new_keys:
        dropped[col] = old[col]

    for col in old_keys & new_keys:
        o = old[col]
        n = new[col]

        # type changed?
        if o["type"] != n["type"]:
            altered[col] = {"old_type": o["type"], "new_type": n["type"]}

        # nullability changed?
        old_nullable = o.get("nullable", True)
        new_nullable = n.get("nullable", True)
        if old_nullable != new_nullable:
            nullability_changed[col] = {"old": old_nullable, "new": new_nullable}

        # unique added/dropped
        old_unique = o.get("unique", False)
        new_unique = n.get("unique", False)
        if old_unique != new_unique:
            if new_unique:
                unique_added.append(col)
            else:
                unique_dropped.append(col)

    return {
        "added": added,
        "dropped": dropped,
        "altered": altered,
        "nullability_changed": nullability_changed,
        "unique_added": unique_added,
        "unique_dropped": unique_dropped,
    }

def generate_alter_table_sql(table_name: str, old_fields: dict, new_fields: dict, renames: dict = None) -> str:
    """
    SQLite limitations:
    - Supports ADD COLUMN only (simple columns)
    - Does NOT support DROP COLUMN, ALTER COLUMN TYPE, ALTER NULLABILITY
    - Does NOT support ADD/DROP named constraints
    To change more, you must recreate the table.

    This function will:
    - Add new columns with ADD COLUMN
    - Detect changes requiring table rebuild and return None if so
    """
    renames = renames or {}
    diff = compare_fields(old_fields, new_fields)

    # Renames are NOT supported in SQLite ALTER TABLE; full rebuild needed
    if renames:
        return None  # Signal to caller that full rebuild needed

    # If any dropped, altered, nullability changed, or unique constraint changes detected
    # SQLite cannot do these with ALTER TABLE, so full rebuild needed
    if diff["dropped"] or diff["altered"] or diff["nullability_changed"] or diff["unique_added"] or diff["unique_dropped"]:
        return None  # Signal to caller full rebuild needed

    # Only add columns with ADD COLUMN (allowed)
    stmts = []
    for col, props in diff["added"].items():
        col_def = f"{col} {map_type(props['type'])}"
        if not props.get("nullable", True):
            col_def += " NOT NULL DEFAULT ''"  # SQLite requires default for NOT NULL new columns
        elif "default" in props:
            col_def += f" DEFAULT {props['default']}"
        stmts.append(f"ALTER TABLE {table_name} ADD COLUMN {col_def};")

    return "\n".join(stmts)

def makemigrations(renames_map: Dict[str, Dict[str, str]] = None):
    renames_map = renames_map or {}
    current_models = load_toml_models(MODELS_DIR)
    previous_defs = load_snapshots()
    new_defs = {}
    migrations = []

    for model_name, data in current_models.items():
        table_name = data["model"].get("table", model_name.lower())
        fields = data["fields"]
        new_defs[model_name] = fields

        old_fields = previous_defs.get(model_name)

        if old_fields is None:
            # New model - create table
            sql = generate_create_table_sql(table_name, fields)
            migrations.append((model_name, sql))
        else:
            rename_cols = renames_map.get(model_name, {})
            alter_sql = generate_alter_table_sql(table_name, old_fields, fields, rename_cols)
            if alter_sql is None:
                # Need full table rebuild migration
                # Generate SQL for rebuilding table:
                # 1. Create new temp table with new schema
                # 2. Copy data over with column mapping (accounting for renames)
                # 3. Drop old table
                # 4. Rename new table
                temp_table = f"{table_name}__new"
                new_create_sql = generate_create_table_sql(temp_table, fields)

                # Build columns mapping for copy: from old to new columns,
                # account for renames by reversing renames dict
                old_to_new = {v: k for k, v in rename_cols.items()}
                copy_columns = []
                for col in fields.keys():
                    old_col = old_to_new.get(col, col)
                    if old_col in old_fields:
                        copy_columns.append(f"{old_col} AS {col}" if old_col != col else col)
                    else:
                        # New column, use NULL or default
                        copy_columns.append(f"NULL AS {col}")

                copy_cols_str = ", ".join(copy_columns)

                sql = f"""
BEGIN TRANSACTION;
{new_create_sql}
INSERT INTO {temp_table} ({', '.join(fields.keys())})
SELECT {copy_cols_str} FROM {table_name};
DROP TABLE {table_name};
ALTER TABLE {temp_table} RENAME TO {table_name};
COMMIT;
""".strip()
                migrations.append((model_name, sql))
            elif alter_sql.strip():
                migrations.append((model_name, alter_sql))

    if migrations:
        MIGRATIONS_DIR.mkdir(exist_ok=True)
        mig_number = len(list(MIGRATIONS_DIR.glob("*.sql"))) + 1
        mig_filename = f"{mig_number:04d}_auto.sql"
        with open(MIGRATIONS_DIR / mig_filename, "w") as f:
            for name, sql in migrations:
                f.write(f"-- Migration for {name}\n{sql}\n\n")
        print(f"Created migration: {mig_filename}")
        save_snapshots(new_defs)
    else:
        print("No changes detected.")
