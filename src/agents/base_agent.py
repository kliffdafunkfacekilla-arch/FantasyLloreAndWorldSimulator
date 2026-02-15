import requests
import json
import time

class SagaAgent:
    def __init__(self, name, role, server_url="http://localhost:8000"):
        self.name = name
        self.role = role
        self.base_url = server_url
        print(f"[{self.role.upper()}] {self.name} initialized.")

    def query_brain(self, endpoint, params=None):
        try:
            response = requests.get(f"{self.base_url}{endpoint}", params=params)
            response.raise_for_status()
            return response.json()
        except requests.exceptions.RequestException as e:
            print(f"[{self.role.upper()}] Connection Error: {e}")
            return None

    def log(self, message):
        print(f"[{self.role.upper()}] {message}")
