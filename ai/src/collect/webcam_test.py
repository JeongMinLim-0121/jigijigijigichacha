import cv2

def main():
    # 0번 카메라(기본 웹캠)
    cap = cv2.VideoCapture(0)

    if not cap.isOpened():
        print("[ERROR] 웹캠을 열 수 없습니다.")
        return

    print("[INFO] 웹캠 시작됨.  'q' 키를 누르면 종료됩니다.")

    while True:
        ret, frame = cap.read()

        if not ret:
            print("[ERROR] 프레임을 읽을 수 없습니다.")
            break

        # 화면 표시
        cv2.imshow("Webcam Test", frame)

        # 1ms 대기 -> q 누르면 종료
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    # 자원 해제
    cap.release()
    cv2.destroyAllWindows()
    print("[INFO] 웹캠 종료됨.")

if __name__ == "__main__":
    main()
