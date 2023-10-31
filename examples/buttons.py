#! /bin/python3

import subprocess
import json

def show_dialog(dialog):
    res = subprocess.run(["tanto", "stdin"], stdout=subprocess.PIPE, input=json.dumps(dialog).encode())
    return json.loads(res.stdout.decode())

DIALOG = {
    "type": "window",
    "width": 500,
    "height": 400,
    "fixed": True,

    "body": {
        "type": "column",

        "items": [
            {"id": "mybutton1", "type": "button", "text": "Click 1"},
            {"id": "mybutton2", "type": "button", "text": "Click 2"},
            {"id": "mybutton3", "type": "button", "text": "Click 3"}
        ]
    }
}

response = show_dialog(DIALOG)
print(f"Event '{response['type']}' received from '{response['from']}'")
