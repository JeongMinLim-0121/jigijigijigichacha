import cv2
import os
from datetime import datetime

def main():
    # 현재 파일 기준으로 data/raw 폴더 경로 만들기
    base_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    save_dir = os.path.join(base_dir, "ai", "data", "raw")

    os.makedirs(save_dir, exist_ok=True)
    print(f"[INFO] 저장 경로: {save_dir}")

    cap = cv2.VideoCapture(0)

    if not cap.isOpened():
        print("[ERROR] 웹캠을 열 수 없습니다.")
        return

    print("[INFO] 웹캠 시작됨.")
    print("[INFO] 사용법: 스페이스바 = 사진 저장 / q = 종료")

    img_count = 0

    while True:
        ret, frame = cap.read()
        if not ret:
            print("[ERROR] 프레임을 읽을 수 없습니다.")
            break

        # 화면에 텍스트 안내 띄우기
        msg = "SPACE: capture  |  Q: quit"
        cv2.putText(frame, msg, (10, 30),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)

        cv2.imshow("Collect Line Data", frame)

        key = cv2.waitKey(1) & 0xFF

        # 스페이스바: 이미지 저장
        if key == 32:  # space
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
            filename = f"frame_{timestamp}.png"
            save_path = os.path.join(save_dir, filename)

            cv2.imwrite(save_path, frame)
            img_count += 1
            print(f"[SAVE] #{img_count} -> {save_path}")

        # q: 종료
        elif key == ord('q'):
            print("[INFO] 수집 종료.")
            break

    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
