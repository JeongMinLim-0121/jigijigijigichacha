-- 로봇 목록
CREATE TABLE robot (
  robot_id       BIGINT PRIMARY KEY AUTO_INCREMENT,
  name           VARCHAR(50) UNIQUE,
  model          VARCHAR(50),
  can_bus        TINYINT NOT NULL DEFAULT 0,
  created_at     TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 미션(작업) 헤더
CREATE TABLE mission (
  mission_id     BIGINT PRIMARY KEY AUTO_INCREMENT,
  robot_id       BIGINT NULL,
  title          VARCHAR(100),
  priority       TINYINT NOT NULL DEFAULT 5,
  state          ENUM('queued','assigned','running','paused','done','failed','canceled') NOT NULL DEFAULT 'queued',
  created_at     TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  started_at     DATETIME NULL,
  finished_at    DATETIME NULL,
  CONSTRAINT fk_mission_robot FOREIGN KEY (robot_id) REFERENCES robot(robot_id)
);

-- 미션 단계(경유지/행동 단위)
CREATE TABLE mission_step (
  step_id        BIGINT PRIMARY KEY AUTO_INCREMENT,
  mission_id     BIGINT NOT NULL,
  seq            INT NOT NULL,
  action         ENUM('goto','pick','place','wait','custom') NOT NULL,
  waypoint_id    BIGINT NULL,
  param_json     JSON NULL,
  state          ENUM('pending','running','done','failed') NOT NULL DEFAULT 'pending',
  created_at     TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  CONSTRAINT fk_step_mission FOREIGN KEY (mission_id) REFERENCES mission(mission_id),
  UNIQUE (mission_id, seq)
);

-- 경유지(창고 포인트)
CREATE TABLE waypoint (
  waypoint_id    BIGINT PRIMARY KEY AUTO_INCREMENT,
  name           VARCHAR(50) UNIQUE,
  x              DOUBLE NOT NULL,
  y              DOUBLE NOT NULL,
  theta          DOUBLE NOT NULL DEFAULT 0.0,
  zone           VARCHAR(50) NULL
);

-- 로봇 텔레메트리(주기 상태) : 시간파티셔닝 전제
CREATE TABLE telemetry (
  ts             DATETIME NOT NULL,
  robot_id       BIGINT NOT NULL,
  x              DOUBLE, y DOUBLE, theta DOUBLE,
  lin_vel        DOUBLE, ang_vel DOUBLE,
  battery_pct    TINYINT,  -- 0~100
  estop          TINYINT,  -- 0/1
  cpu_temp_c     DOUBLE,
  extra_json     JSON,
  PRIMARY KEY (ts, robot_id),
  INDEX idx_tel_robot_ts (robot_id, ts DESC),
  CONSTRAINT fk_tel_robot FOREIGN KEY (robot_id) REFERENCES robot(robot_id)
) /* PARTITION BY RANGE (TO_DAYS(ts)) (...) */;
-- 실제 운영 시: 일/주 단위 RANGE 파티션 생성

-- 장애물/이벤트 로그
CREATE TABLE obstacle_event (
  event_id       BIGINT PRIMARY KEY AUTO_INCREMENT,
  ts             DATETIME NOT NULL,
  robot_id       BIGINT NOT NULL,
  type           ENUM('lidar','vision','ultrasonic','manual') NOT NULL,
  x DOUBLE, y DOUBLE, note VARCHAR(200),
  CONSTRAINT fk_obs_robot FOREIGN KEY (robot_id) REFERENCES robot(robot_id),
  INDEX idx_obs_robot_ts (robot_id, ts DESC)
);

-- 비상정지/안전 이벤트
CREATE TABLE estop_event (
  event_id       BIGINT PRIMARY KEY AUTO_INCREMENT,
  ts             DATETIME NOT NULL,
  robot_id       BIGINT NOT NULL,
  source         ENUM('button','software','remote') NOT NULL,
  reason         VARCHAR(200),
  CONSTRAINT fk_estop_robot FOREIGN KEY (robot_id) REFERENCES robot(robot_id),
  INDEX idx_estop_robot_ts (robot_id, ts DESC)
);

-- CAN 로그(필요 시 별 DB 또는 압축 테이블 권장)
CREATE TABLE can_log (
  ts             DATETIME NOT NULL,
  robot_id       BIGINT NOT NULL,
  iface          VARCHAR(10) NOT NULL, -- can0, can1
  can_id         INT UNSIGNED NOT NULL,
  dlc            TINYINT NOT NULL,
  data_hex       VARBINARY(16) NOT NULL,
  PRIMARY KEY (ts, robot_id, can_id, iface),
  INDEX idx_can_robot_ts (robot_id, ts DESC)
) /* PARTITION BY RANGE (TO_DAYS(ts)) */;
