from flask import Flask, request, jsonify, render_template, redirect, url_for
import json, os, re, paramiko

app = Flask(__name__)

BOOKS_FILE = "books.json"
EXPECTED_BARCODES_FILE = "expected_barcodes.json"
RASPBERRY_PI_IP = "192.168.137.82"
RASPBERRY_PI_USER = "pi"
RASPBERRY_PI_PASSWORD = "raspberry"
SCAN_COMMAND = "python3 /home/pi/YJM/YOLOV5_motor_Code.py"

IP_JSON_PATH = 'ips.json'

if os.getenv("FLASK_ENV") == "development":
    app.config["DEBUG"] = True
else:
    app.config["DEBUG"] = False

def load_ips():
    with open(IP_JSON_PATH, 'r') as f:
        data = json.load(f)
    return data['raspberry']

@app.route('/add_ip', methods=['POST'])
def add_ip():
    new_ip = request.form['new_ip']
    
    with open(IP_JSON_PATH, 'r') as f:
        data = json.load(f)
        
    data['raspberry'] = new_ip
    
    with open(IP_JSON_PATH, 'w') as f:
              json.dump(data,f)
              
    return redirect('/')

def is_consonant_only(char):
    """ 주어진 한글 글자가 자음만 있는지 여부를 반환 """
    return char and "ㄱ" <= char <= "ㅎ"  # 한글 자음 범위

def parse_call_number(call_number):
    """ 청구기호를 정렬하기 위한 파싱 함수 """
    match = re.match(r"(\d+)\s*([\u3131-ㅎ])?(\d+)?([\u3131-힣]*)?", call_number)
    if match:
        first_num = int(match.group(1))  # 첫 번째 숫자
        first_char = match.group(2) if match.group(2) else ""  # 첫 번째 문자
        second_num = int(match.group(3)) if match.group(3) else 0  # 두 번째 숫자
        last_char = match.group(4) if match.group(4) else ""  # 마지막 문자
        last_char_priority = (0 if is_consonant_only(last_char) else 1, last_char)  # 자음만 있는 경우(0), 자음+모음(1)
        return (first_num, first_char, second_num, last_char_priority)
    return (float('inf'), "", float('inf'), (1, ""))  # 잘못된 형식 방지

def sort_books_by_call_number():
    """ book.json을 읽고, 같은 location끼리 정렬하여 position을 1부터 다시 부여 후 저장 """
    if not os.path.exists(BOOKS_FILE):
        return {"error": "book.json not found"}

    # JSON 파일 로드
    with open(BOOKS_FILE, "r", encoding="utf-8") as f:
        books = json.load(f)

    # location별 그룹화
    books_by_location = {}
    for book in books:
        location = book["location"]
        if location not in books_by_location:
            books_by_location[location] = []
        books_by_location[location].append(book)

    # 새로운 books 리스트 생성 (정렬 후 position 초기화)
    sorted_books = []
    for location in sorted(books_by_location.keys()):  # location을 정렬된 순서대로 처리
        book_list = books_by_location[location]

        # 청구기호 기준 정렬
        sorted_location_books = sorted(book_list, key=lambda x: parse_call_number(x["call_number"]))

        # 각 location 별로 position을 1부터 할당
        for i, book in enumerate(sorted_location_books, start=1):
            book["position"] = i
            sorted_books.append(book)  # 최종 리스트에 추가

    # 업데이트된 JSON 저장
    with open(BOOKS_FILE, "w", encoding="utf-8") as f:
        json.dump(sorted_books, f, indent=4, ensure_ascii=False)

    print("✅ Books sorted and positions updated on startup")
    return {"message": "Books sorted and positions updated"}


# JSON 파일에서 책 정보 불러오기
def load_books():
    return json.load(open(BOOKS_FILE, "r", encoding="utf-8")) if os.path.exists(BOOKS_FILE) else []

books = load_books()

# JSON 파일에 책 정보 저장하기
def save_books():
    json.dump(books, open(BOOKS_FILE, "w", encoding="utf-8"), ensure_ascii=False, indent=4)

# 📌 서버 부팅 시 expected_barcodes.json 생성
expected_barcodes = {}

def generate_expected_barcodes():
    global expected_barcodes

    location_dict = {}
    for book in books:
        location = book["location"]
        barcode = book["barcode"]
        position = book["position"]

        if location not in location_dict:
            location_dict[location] = []

        location_dict[location].append((barcode, position))

    # 정렬 후 저장 (position 기준)
    expected_barcodes = {
        location: [barcode for barcode, _ in sorted(barcodes, key=lambda x: x[1])]
        for location, barcodes in location_dict.items()
    }

    # JSON 파일로 저장
    with open(EXPECTED_BARCODES_FILE, "w", encoding="utf-8") as file:
        json.dump(expected_barcodes, file, ensure_ascii=False, indent=4)

    print("✅ expected_barcodes.json 업데이트 완료!")
    
def update_book_status():
    global books, expected_barcodes
    
    data = request.get_json()
    print("📌 수신된 데이터:", data)  # 디버깅 출력
    
    if not data:
        return jsonify({"error": "잘못된 데이터입니다."}), 400
    
    location = list(data.keys())[0]
    scanned_barcodes = data[location]
    
    print("📌 Location:", location)
    print("📌 Scanned Barcodes:", scanned_barcodes)
    print("📌 Expected Barcodes:", expected_barcodes)

    if location not in expected_barcodes:
        return jsonify({"error": "잘못된 책장 위치입니다."}), 400


sort_books_by_call_number()
# 📌 서버 부팅 시 expected_barcodes.json 생성
generate_expected_barcodes()

@app.route('/')
def home():
    
    return render_template('index.html', books=books)


@app.route('/books')
def books_page():
    return render_template('books.html', books=books)

@app.route('/book_shelf')
def book_shelf():
    grouped_shelves = {}  # location별로 그룹화된 책 저장
    
    # 책 상태와 그룹화를 처리
    for book in books:
        if book['location'] in ['3F-1-A-1-f', '3F-1-A-2-f', '3F-1-A-3-f','3F-1-A-4-f', '3F-1-A-5-f', '3F-1-A-6-f' , '3F-1-A-7-f', '3F-1-A-8-f', '3F-1-B-1-f', '3F-1-B-2-f','3F-1-B-3-f','3F-1-B-4-f','3F-1-B-5-f','3F-1-B-6-f','3F-1-B-7-f','3F-1-B-8-f',]:
            status_label = ""
            color_class = ""

            if book.get("available", False):
                status_label = "책 있음"
                color_class = "available"
            elif not book.get("available", False) and not book.get("misplaced", False) and not book.get("wrong-location", False):
                status_label = "찾을 수 없음"
                color_class = "not-available"
            elif book.get("misplaced", False):
                status_label = "순서 잘못됨"
                color_class = "misplaced"
            elif book.get("wrong-location", False):
                status_label = "잘못 배치됨"
                color_class = "wrong-location"

            # 책을 location별로 그룹화
            if book['location'] not in grouped_shelves:
                grouped_shelves[book['location']] = []

            grouped_shelves[book['location']].append({
                'title': book['title'],
                'barcode': book['barcode'],
                'image_url': book.get('image_url', 'default.jpg'),
                'status_label': status_label,
                'color_class': color_class
            })

    return render_template('book_shelf.html', grouped_shelves=grouped_shelves)


# 스캔을 시작하는 API
@app.route('/scan', methods=['POST'])
def scan_book():
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())  # 호스트 키 자동 추가
    try:
        ssh.connect(RASPBERRY_PI_IP, username=RASPBERRY_PI_USER, password=RASPBERRY_PI_PASSWORD)
        
        VENV_PATH = "/home/pi/yolov5_env"  # 가상환경 경로 확인!
        CODE_1 = "/home/pi/YOLOV5_motor_Code.py"
        CODE_2 = "/home/pi/scanner.py"  # 실행할 두 번째 코드

        # 첫 번째 코드 실행 (터미널 1)
        command1 = f"DISPLAY=:0 lxterminal --command='bash -c \"source {VENV_PATH}/bin/activate && python {CODE_1}; exec bash\"'"

        # 두 번째 코드 실행 (터미널 2)
        command2 = f"DISPLAY=:0 lxterminal --command='bash -c \"source {VENV_PATH}/bin/activate && python {CODE_2}; exec bash\"'"

        # 두 개의 터미널을 실행
        ssh.exec_command(command1)
        ssh.exec_command(command2)

        return jsonify({"success": True, "message": "두 개의 코드 실행 성공"})

    except Exception as e:
        return jsonify({"success": False, "message": "서버 오류", "error": str(e)})


# 책 상태 업데이트 API
@app.route('/update_book_status', methods=['POST'])
def update_book_status():
    global books, expected_barcodes
    
    data = request.get_json()
    scanned = data  # 클라이언트에서 전송한 스캔 데이터 (예: {"3F-A-1-F": ["123", "234", "345", "456", "567"]})
    
    # 스캔된 칸바코드가 있는지 확인
    if not scanned:
        return jsonify({"error": "잘못된 데이터입니다."}), 400
    
    location = list(scanned.keys())[0]  # 예: "3F-A-1-F"
    scanned_barcodes = scanned[location]  # 예: ["123", "234", "345", "456", "567"]
    
    # expected_barcodes.json에서 예상 바코드 목록 가져오기
    if location not in expected_barcodes:
        return jsonify({"error": "잘못된 책장 위치입니다."}), 400

    
    expected_barcodes_list = expected_barcodes[location]  # 예: ["123", "234", "345", "456", "567"]
    expected_set = set(expected_barcodes_list)  # 빠른 검색을 위한 집합 변환
    scanned_set = set(scanned_barcodes)  # 스캔된 바코드 집합 변환
    
    # 1. expected에는 있지만 scanned에는 없는 바코드 찾기 (책이 없는 경우)
    missing_barcodes = expected_set - scanned_set
    
    # 2. expected에는 없지만 scanned에 있는 바코드 찾기 (잘못된 위치)
    wrong_location_barcodes = scanned_set - expected_set
    
    # wrong-location에 해당하는 바코드는 제거
    scanned_barcodes = [b for b in scanned_barcodes if b not in wrong_location_barcodes]
    
    # 3. 버퍼에 분배
    buffers = []  # 여러 개의 버퍼 (각각의 버퍼는 순서대로 스캔된 바코드들)
    
    # 첫 번째 바코드는 첫 번째 버퍼에 넣기
    current_buffer = [scanned_barcodes[0]]
    
    # 나머지 바코드는 이전 바코드와의 순서를 비교하여 버퍼에 나누어 넣기
    for i in range(1, len(scanned_barcodes)):
        prev_barcode = scanned_barcodes[i - 1]
        curr_barcode = scanned_barcodes[i]
        
        # prev_barcode가 current_buffer의 마지막 바코드보다 앞선 순서일 경우
        if expected_barcodes_list.index(curr_barcode) > expected_barcodes_list.index(prev_barcode):
            current_buffer.append(curr_barcode)
        else:
            # 새로운 버퍼를 시작
            buffers.append(current_buffer)
            current_buffer = [curr_barcode]
    
    # 마지막 버퍼도 추가
    buffers.append(current_buffer)
    
    # 가장 큰 버퍼를 available로 간주
    largest_buffer = max(buffers, key=len)
    
    available_barcodes = []
    misplaced_barcodes = []
    
    for buffer in buffers:
        if buffer == largest_buffer:
            available_barcodes.extend(buffer)  # 가장 큰 버퍼의 바코드는 available
        else:
            misplaced_barcodes.extend(buffer)  # 나머지 버퍼의 바코드는 misplaced
    
    # 책 상태 업데이트
    for book in books:
        if book["barcode"] in available_barcodes:
            book["available"] = True
            book["misplaced"] = False
            book["wrong-location"] = False
        elif book["barcode"] in misplaced_barcodes:
            book["available"] = False
            book["misplaced"] = True
            book["wrong-location"] = False
        elif book["barcode"] in wrong_location_barcodes:
            book["available"] = False
            book["misplaced"] = False
            book["wrong-location"] = True
        elif book["barcode"] in missing_barcodes:
            book["available"] = False
            book["misplaced"] = False
            book["wrong-location"] = False
    
    save_books()  # 변경된 책 상태 저장
    
    return jsonify({
        "available": available_barcodes,
        "misplaced": misplaced_barcodes,
        "wrong-location": list(wrong_location_barcodes),
        "not-available": list(missing_barcodes)
    }), 200




@app.route('/get_books', methods=['GET'])
def get_books():
    return jsonify(books)

@app.route('/search')
def search_books():
    query = request.args.get('query', '').lower()
    books = load_books()
    filtered_books = [book for book in books if query in book['title'].lower() or query in book['author'].lower()]
    return jsonify(filtered_books)

if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5000)
