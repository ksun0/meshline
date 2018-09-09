import random
import requests
import json
import time

phone_numbers = ["123", "456"]
messages = ["Hi"]
locations = ["10 20", "50 10"]

output = {}

for i in phone_numbers:
    for j in phone_numbers:
        if i != j:
            for k in messages:
                key = i + " " + str(random.randint(0, 20)) + " " + j
                value = {"type": "message", "data": k}
                output[key] = value

print(output)

for k, v in output.items():
    req = requests.post("http://192.168.203.1/send", data={"key": k, "value": json.dumps(v)})
    print("done")
    print(req, k, v)
