
import toml
import getpass
from datetime import datetime
import sqlite3
from pathlib import Path

# Paths (project-root relative)
ADMIN_TOML_PATH = Path("src/models/admin.toml")
AEON_TOML_PATH = Path("aeon.toml")

def get_sqlite_path():
    config = toml.load(AEON_TOML_PATH.resolve())
    return config["database"]["path"]

def create_superuser():
    # Read admin.toml
    with open(ADMIN_TOML_PATH.resolve(), 'r') as f:
        config = toml.load(f)
    fields = config.get('fields', {})
    table = config['model']['table']

    # Fields to prompt for
    prompt_fields = []
    for field, props in fields.items():
        if field in ['id', 'active', 'created_at', 'role']:
            continue
        prompt_fields.append(field)

    # Prompt user for each field
    user_data = {}
    for field in prompt_fields:
        if field == 'password_hash':
            password = getpass.getpass('Password: ')
            # TODO: Hash password here if needed
            user_data[field] = password
        else:
            value = input(f'{field.capitalize()}: ')
            user_data[field] = value

    # Set role, active, created_at
    user_data['role'] = 'admin'
    user_data['active'] = True
    user_data['created_at'] = datetime.now().isoformat()

    # Write to database
    db_path = get_sqlite_path()
    conn = sqlite3.connect(db_path)
    columns = ', '.join(user_data.keys())
    placeholders = ', '.join(['?'] * len(user_data))
    values = tuple(user_data.values())
    sql = f"INSERT INTO {table} ({columns}) VALUES ({placeholders})"
    try:
        with conn:
            conn.execute(sql, values)
        print('Superuser created successfully.')
    except sqlite3.Error as e:
        print(f'Error creating superuser: {e}')
    finally:
        conn.close()
