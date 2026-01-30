import cv2
import numpy as np
import requests
import time

ESP32_CAM_URL = "http://10.172.15.234/cam-mid.jpg"
ESP32_POST_URL = "http://10.172.15.234//receive_color"

# 🔹 حدود الألوان في HSV
yellow_lower = np.array([5, 97, 45])
yellow_upper = np.array([51, 255, 255])

blue_lower = np.array([90, 100, 50])
blue_upper = np.array([130, 255, 255])

green_lower = np.array([35, 70, 50])
green_upper = np.array([85, 255, 255])

ROI_SIZE = 200
last_sent_color = None
last_detected_color = None
confirm_count = 0
confirm_threshold = 10

send_delay = 1.0  # بالثواني بين الإرسال
last_send_time = time.time()

def get_frame_from_esp32():
    try:
        response = requests.get(ESP32_CAM_URL, timeout=2)
        img_array = np.asarray(bytearray(response.content), dtype=np.uint8)
        return cv2.imdecode(img_array, -1)
    except:
        return None

def send_color_to_esp32(color):
    global last_sent_color, last_send_time
    now = time.time()
    if color != last_sent_color and now - last_send_time > send_delay:
        try:
            requests.post(ESP32_POST_URL, data=color, timeout=1)
            last_sent_color = color
            last_send_time = now
            print(f"تم إرسال اللون إلى ESP32: {color}")
        except:
            print("فشل إرسال اللون إلى ESP32")

def detect_color_in_roi(frame):
    height, width, _ = frame.shape
    center_x, center_y = width // 2, height // 2
    x1, y1 = center_x - ROI_SIZE // 2, center_y - ROI_SIZE // 2
    x2, y2 = center_x + ROI_SIZE // 2, center_y + ROI_SIZE // 2

    roi = frame[y1:y2, x1:x2]
    hsv_roi = cv2.cvtColor(roi, cv2.COLOR_BGR2HSV)

    # 🔹 إضافة اللون الأخضر إلى قائمة الألوان
    masks = {
        "Yellow": cv2.inRange(hsv_roi, yellow_lower, yellow_upper),
        "Blue": cv2.inRange(hsv_roi, blue_lower, blue_upper),
        "Green": cv2.inRange(hsv_roi, green_lower, green_upper)
    }

    detected_color = "Unknown"
    for color, mask in masks.items():
        if cv2.countNonZero(mask) > 100:
            detected_color = color
            break

    cv2.rectangle(frame, (x1, y1), (x2, y2), (255, 255, 255), 2)
    cv2.putText(frame, detected_color, (x1, y1 - 10),
                cv2.FONT_HERSHEY_SIMPLEX, 0.9, (255, 255, 255), 2)

    return detected_color, frame

while True:
    frame = get_frame_from_esp32()
    if frame is not None:
        color, frame = detect_color_in_roi(frame)

        if color == last_detected_color:
            confirm_count += 1
        else:
            confirm_count = 1
            last_detected_color = color

        if confirm_count >= confirm_threshold and color != last_sent_color:
            send_color_to_esp32(color)

        cv2.imshow("ESP32 ROI Color Detection", frame)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cv2.destroyAllWindows()
