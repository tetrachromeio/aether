import requests
from aeon.utils.config_utils import write_global_config

API_BASE_URL = "http://localhost:8080/api"  # Replace with your server URL

def login_user(email: str, password: str):
    try:
        # Send login request to the server
        response = requests.post(f"{API_BASE_URL}/login", json={"email": email, "password": password})
        response.raise_for_status()  # Raise an error for bad responses (4xx, 5xx)
        data = response.json()

        if data["status"] == "success":
            print("Login successful!")
            write_global_config({"session": data["token"]})  # Store the session token
        else:
            print(f"Login failed: {data.get('message', 'Unknown error')}")
    except requests.exceptions.RequestException as e:
        print(f"Error during login: {e}")

def register_user(email: str, password: str):
    try:
        # Send registration request to the server
        response = requests.post(f"{API_BASE_URL}/register", json={"email": email, "password": password})
        response.raise_for_status()  # Raise an error for bad responses (4xx, 5xx)
        data = response.json()

        if data["status"] == "success":
            print("Registration successful!")
            write_global_config({"session": data["token"]})  # Store the session token
        else:
            print(f"Registration failed: {data.get('message', 'Unknown error')}")
    except requests.exceptions.RequestException as e:
        print(f"Error during registration: {e}")