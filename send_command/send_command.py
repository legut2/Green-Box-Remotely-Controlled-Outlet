import http.client
import requests
import threading
import time
import os
import json
from dotenv import load_dotenv

# Load environment variables from the .env file
load_dotenv()

# Global variable for relay command and interval
relay_command = False

# Environment variables
CLIENT_ID = os.getenv("CLIENT_ID")
CLIENT_SECRET = os.getenv("CLIENT_SECRET")
PROJECT_ID = os.getenv("PROJECT_ID")
DEVICE_ID = os.getenv("DEVICE_ID")

# Token management
access_token = None
token_expiry = time.time()

def get_access_token():
    global access_token, token_expiry
    # Check if the current access token is expired or about to expire
    if access_token is None or time.time() > token_expiry - 30:  # Refresh the token 30 seconds before expiry just to be safe
        conn = http.client.HTTPSConnection("notehub.io")
        payload = f"grant_type=client_credentials&client_id={CLIENT_ID}&client_secret={CLIENT_SECRET}"
        headers = {'content-type': "application/x-www-form-urlencoded"}
        conn.request("POST", "/oauth2/token", payload, headers)
        res = conn.getresponse()
        data = res.read().decode("utf-8")
        conn.close()
        
        # Parse the response to get the access token and expiry time
        token_data = json.loads(data)
        access_token = token_data['access_token']
        expires_in = token_data['expires_in']  # Time in seconds until the token expires
        token_expiry = time.time() + expires_in

    return access_token

def send_relay_command():
    global relay_command
    while True:
        try:
            token = get_access_token()  # Function to get a new access token
            headers = {
                'Authorization': f'Bearer {token}',
                'Content-Type': 'application/json',
            }
            # Sending relay command instead of heart rate
            data = json.dumps({"body": {"desired_relay_state": relay_command}})
            response = requests.post(
                f'https://api.notefile.net/v1/projects/app:{PROJECT_ID}/devices/dev:{DEVICE_ID}/notes/data.qi',
                headers=headers,
                data=data,
            )
            print(f"Status Code: {response.status_code}")
            print(f"Response: {response.text}")
        except Exception as e:
            print(f"An error occurred: {e}")
        time.sleep(5)

# Start the thread
thread = threading.Thread(target=send_relay_command)
thread.daemon = True
thread.start()

# Interactive loop for modifying the relay command and interval
try:
    while True:
        new_relay_command = input("Enter new relay command (True/False): ")
        # new_interval = input("Enter new interval (in seconds): ")
        if new_relay_command.lower() in ["true", "false"]:
            relay_command = new_relay_command.lower() == "true"
        # if new_interval.isdigit():
        #     interval = int(new_interval)
except KeyboardInterrupt:
    print("Stopping the script...")
