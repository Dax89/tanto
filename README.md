Tantō
========
Tanto provides an easy way to create GUI-enabled scripts without installing external libraries/bindings.<br>
It's communication protocol is based on JSON.

|GTK3                        | Qt5/6                   |
|:-------------------------:|:-------------------------:|
|![Screenshot_31102023_183712](https://github.com/Dax89/tanto/assets/1503603/a55d832a-4bf3-4143-98f8-8e2907ef0be9)|![Screenshot_31102023_183659](https://github.com/Dax89/tanto/assets/1503603/d5ee36b5-bbd6-4f84-bd28-d07f847525c9)|

Supported Types
-----

|Name                      | Type      | Description             |
:-------------------------:|:---------:|:------------------------|
|window                    | Container | Top level Window        |
|text                      | Widget    | Text label              |
|input                     | Widget    | Single/Multi line text input  |
|number                    | Widget    | Numeric-only input            |
|image                     | Widget    | An image viewer (URLs are supported too)  |
|button                    | Widget    | Clickable button  |
|check                     | Widget    | Checkbox          |
|list                      | Widget    | ListView (with optional model support) |
|tree                      | Widget    | TreeView (with optional model support) |
|tabs                      | Container | Tab View |
|row                       | Layout    | Aligns items horizontally |
|column                    | Layout    | Aligns items vertically |
|grid                      | Layout    | Aligns items in a grid MxN |
|from                      | Layout    | Creates N-rows of text + widget pairs |

Command Line Arguments
-----
```
Usage:
  tanto stdin [--debug] [--backend=ARG]
  tanto load <filename> [--debug] [--backend=ARG]
  tanto message <title> <text> [(info|question|warning|error)] [--debug] [--backend=ARG]
  tanto confirm <title> <text> [(info|question|warning|error)] [--debug] [--backend=ARG]
  tanto input <title> [text] [value] [--debug] [--backend=ARG]
  tanto password <title> [text] [--debug] [--backend=ARG]
  tanto selectdir [title] [dir] [--debug] [--backend=ARG]
  tanto loadfile [title] [filter] [dir] [--debug] [--backend=ARG]
  tanto savefile [title] [filter] [dir] [--debug] [--backend=ARG]
  tanto list [--debug]
  tanto --version
  tanto --help

Options:
  -h --help        Show this screen
  -v --version     Show version
  -d --debug       Debug mode
  -b --backend=ARG Select backend
```


A Simple Example
-----
```python

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
```

If we click `mybutton2`, Tanto sends a JSON object like this:
```json
{
    "from": "mybutton2",
    "type": "clicked"
}
```
