"""
Demonstration of the GazeTracking library.
Check the README.md for complete documentation.
"""

import cv2
from gaze_tracking import GazeTracking
import socket

target_ip = "localhost"
target_port = 8080
buffer_size = 4096

# 1.ソケットオブジェクトの作成
tcp_client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# 2.サーバに接続
tcp_client.connect((target_ip, target_port))

gaze = GazeTracking()
webcam = cv2.VideoCapture(0)
x = 0
y = 0

while True:
    # We get a new frame from the webcam
    _, frame = webcam.read()

    # We send this frame to GazeTracking to analyze it
    gaze.refresh(frame)

    frame = gaze.annotated_frame()
    text = ""

    if gaze.is_blinking():
        text = "Blinking"
    elif gaze.is_right():
        text = "Looking right"
    elif gaze.is_left():
        text = "Looking left"
    elif gaze.is_center():
        text = "Looking center"

    cv2.putText(frame, text, (90, 60),
                cv2.FONT_HERSHEY_DUPLEX, 1.6, (147, 58, 31), 2)

    left_pupil = gaze.pupil_left_coords()
    right_pupil = gaze.pupil_right_coords()

    try:
        x = 1-(left_pupil[0]+right_pupil[0])/640
        y = (left_pupil[1]+right_pupil[1])/480-1
    except:
        pass
    print(x, y)

    # 3.サーバにデータを送信
    tcp_client.send(bytearray(str(x) + " " + str(y) + "\n", 'utf-8'))

    cv2.putText(frame, "Left pupil:  " + str(left_pupil), (90, 130),
                cv2.FONT_HERSHEY_DUPLEX, 0.9, (147, 58, 31), 1)
    cv2.putText(frame, "Right pupil: " + str(right_pupil), (90, 165),
                cv2.FONT_HERSHEY_DUPLEX, 0.9, (147, 58, 31), 1)

    cv2.imshow("Demo", frame)

    if cv2.waitKey(1) == 27:
        break
