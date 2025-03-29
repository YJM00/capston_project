import requests

url = "http://localhost:5000/update_book_status"
data = {
    "location": "3F-A-1-F",
    "scanned_barcodes": ["123", "234", "345", "456", "567"]
}

response = requests.post(url, json=data)

print(response.status_code)
print(response.json())
