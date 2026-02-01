---
title: "Tap Smith 개발 일지 #4 - 일일 미션 시스템 설계"
date: 2026-02-05T00:00:00Z
draft: false
tags: ["게임개발", "Tap Smith", "일일미션", "데이터베이스", "기획", "백엔드"]
categories: ["game-dev/tap-smith"]
---

# Tap Smith 개발 일지 #4 - 일일 미션 시스템 설계

Tap Smith에 일일 미션 시스템 추가하려고 계획 중임. 
유저들이 매일 게임에 접속해서 재미있게 플레이하려면 미션은 필수인 듯. 
오늘은 그 설계를 정리해봤음!

## 📋 현재 상태
- ✅ gameService와 missionsService는 테이블 없이도 작동하도록 이미 구현됨
- ✅ 출석 기능은 골드 보상만 작동 (테이블 없어도 OK)
- ⏳ 일일 미션 테이블 및 로직은 아직 구현 안 됨

## 🎯 일일 미션 시스템 설계

### 1️⃣ 데이터베이스 스키마 추가

#### `master_daily_missions` (마스터 데이터)
```sql
CREATE TABLE master_daily_missions (
  mission_id INT AUTO_INCREMENT PRIMARY KEY,
  mission_name VARCHAR(50) NOT NULL,
  description VARCHAR(200),
  mission_type ENUM('ENCHANT', 'DRAW', 'SELL', 'REACH_LEVEL') NOT NULL,
  target_count INT NOT NULL,  -- 필요한 횟수 (예: 강화 5회, 뽑기 3회)
  reward_gold BIGINT NOT NULL,
  is_active TINYINT(1) DEFAULT 1
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

**예시 데이터:**
```sql
INSERT INTO master_daily_missions (mission_name, description, mission_type, target_count, reward_gold) VALUES
('강화 5회', '무기를 강화 5회 수행하기', 'ENCHANT', 5, 500),
('뽑기 3회', '상자에서 3회 뽑기', 'DRAW', 3, 300),
('무기 판매하기', '무기를 1개 판매하기', 'SELL', 1, 200),
('레벨 도달', '무기를 15강 이상으로 강화하기', 'REACH_LEVEL', 15, 1000);
```

#### `user_daily_missions` (유저별 일일 미션 상태)
```sql
CREATE TABLE user_daily_missions (
  id BIGINT AUTO_INCREMENT PRIMARY KEY,
  user_id BIGINT UNSIGNED NOT NULL,
  mission_id INT NOT NULL,
  current_progress INT DEFAULT 0,  -- 현재 진행도
  completed TINYINT(1) DEFAULT 0,
  reward_claimed TINYINT(1) DEFAULT 0,
  checked_at DATE NOT NULL,
  claimed_at DATETIME NULL,
  INDEX idx_user_date (user_id, checked_at),
  CONSTRAINT fk_udm_user FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE,
  CONSTRAINT fk_udm_mission FOREIGN KEY (mission_id) REFERENCES master_daily_missions(mission_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

### 2️⃣ 미션 타입들

- **ENCHANT**: 강화 횟수 달성
- **DRAW**: 상자 뽑기 횟수
- **SELL**: 무기 판매 횟수  
- **REACH_LEVEL**: 특정 강화 레벨 도달

### 3️⃣ 구현 계획

1. 매일 자정에 새로운 미션 할당
2. 게임 액션마다 진행도 업데이트
3. 완료된 미션은 보상 수령 가능
4. 보상은 골드로 지급

## 💡 왜 일일 미션?

- 유저 리텐션 향상 (매일 접속 유도)
- 게임 플레이 다양화
- 추가 골드 수입원

아직 구현 전이지만, 설계 단계부터 공유하니 재밌는 듯. 
다음 일지에서는 실제 구현 과정 올릴 예정임!

#TapSmith #일일미션 #데이터베이스 #기획