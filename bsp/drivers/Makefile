# --- Top-level Makefile for all driver modules ---

# 하위 드라이버 폴더들(필요 없는 건 지워도 됨)
SUBDIRS := encoder estop imu ledkey lidar motor power

.PHONY: all clean

all:
	@echo "== Building all driver modules =="
	@for dir in $(SUBDIRS); do \
		echo "[BUILD] $$dir"; \
		$(MAKE) -C $$dir || exit $$?; \
	done
	@echo "✅ All modules built successfully!"

clean:
	@echo "== Cleaning all driver modules =="
	@for dir in $(SUBDIRS); do \
		echo "[CLEAN] $$dir"; \
		$(MAKE) -C $$dir clean || true; \
	done
	@echo "🧹 All modules cleaned!"
