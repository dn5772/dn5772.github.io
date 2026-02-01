# 일일 미션 시스템 구현 계획 (TODO)

## 📋 현재 상태
- ✅ gameService와 missionsService는 테이블 없이도 작동하도록 구현됨
- ✅ 출석 기능은 골드 보상만 작동 (테이블 없어도 OK)
- ⏳ 일일 미션 테이블 및 로직은 아직 구현 X

---

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

#### `user_daily_checkins` (일일 출석 기록)
```sql
CREATE TABLE user_daily_checkins (
  id BIGINT AUTO_INCREMENT PRIMARY KEY,
  user_id BIGINT UNSIGNED NOT NULL,
  checked_at DATE NOT NULL,
  reward_gold INT NOT NULL DEFAULT 250,
  checked_timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY uk_user_date (user_id, checked_at),
  CONSTRAINT fk_udc_user FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

---

## 🔧 구현 방식 (2가지 옵션)

### 옵션 1️⃣: **간단한 방식 (권장: 초기 배포용)**
**특징**: 고정된 미션 타입만 지원
- 매일 같은 미션만 반복 (강화, 뽑기, 판매, 레벨 도달)
- 보상도 고정됨
- 구현이 단순하고 빠름
- 유저 경험은 약간 단조로움

**구현 난이도**: ⭐⭐ (낮음)

```javascript
// missionsService.js - 초기화 로직
async initializeDailyMissions(userId, db) {
  // 매일 자동으로 user_daily_missions 생성
  // 기본 4개 미션을 모두에게 할당
}
```

---

### 옵션 2️⃣: **고급 방식 (나중에 개선)**
**특징**: 동적 미션 시스템
- 매일 4개 미션 중 3개를 랜덤 선택
- 주간/월간 특수 미션 추가 가능
- 난이도 조절 가능
- 유저가 매일 다른 미션을 경험

**구현 난이도**: ⭐⭐⭐⭐ (높음)

```javascript
// 매일 자정마다 스케줄러 실행
// 1. 모든 유저의 전날 미션 완료 여부 확인
// 2. 마스터 미션 중 3-4개를 랜덤으로 선택
// 3. 각 유저의 user_daily_missions 갱신
```

---

## 📊 미션 진행 추적 방식

### 현재 문제점:
- enchant, drawBox, sellWeapon 함수에서 미션 진행도 업데이트 안 함
- 미션 완료 여부는 수동으로만 체크

### 개선 방안:

#### 1️⃣ **옵션 A: 에이전트 업데이트 (추천)**
각 게임 로직 함수에서 미션 진행도 자동 업데이트:

```javascript
// enchantmentService.js - enchant() 함수 마지막에 추가
async enchant(userId, weaponUid, db) {
  // ... 기존 강화 로직
  
  // 미션 진행도 업데이트
  await this._updateMissionProgress(userId, 'ENCHANT', db);
}

// 미션 진행도 업데이트
async _updateMissionProgress(userId, missionType, db) {
  try {
    const today = new Date().toISOString().split('T')[0];
    await db.query(
      `UPDATE user_daily_missions 
       SET current_progress = current_progress + 1,
           completed = IF(current_progress + 1 >= (
             SELECT target_count FROM master_daily_missions WHERE mission_type = ?
           ), 1, 0)
       WHERE user_id = ? AND checked_at = ? AND mission_id IN (
         SELECT mission_id FROM master_daily_missions WHERE mission_type = ?
       )`,
      [missionType, userId, today, missionType]
    );
  } catch (e) {
    // 미션 진행도 업데이트 실패해도 게임은 계속 진행
  }
}
```

#### 2️⃣ **옵션 B: 명시적 클레임 (간단함)**
API에서 미션 상태만 조회하고, 완료는 별도 엔드포인트:

```javascript
// POST /v1/missions/daily/check
// 현재 미션 상태 확인하고 수동으로 체크
// 클라이언트가 게임 플레이 후 호출
```

---

## 🛠️ 구현 순서

### Phase 1: 기본 구조 (3-4일)
```
1. 마스터 테이블 추가 (master_daily_missions)
2. 유저 미션 테이블 추가 (user_daily_missions)
3. 일일 출석 테이블 추가 (user_daily_checkins)
4. 초기 데이터 삽입 스크립트 작성
```

### Phase 2: 서비스 로직 (2-3일)
```
1. missionsService.js 확장
   - initializeDailyMissions() - 유저 첫 로그인 시 미션 초기화
   - claimDailyReward() - 미션 보상 수령
   - getMissionProgress() - 미션 진행도 조회

2. enchantmentService, shopService, economyService 수정
   - 각 함수 마지막에 미션 진행도 업데이트
```

### Phase 3: 자동화 (1-2일)
```
1. 서버 시작 시 일일 미션 초기화 배치
2. 매일 자정(또는 서버 재시작)에 미션 갱신
3. 미션 완료 알림 로직 (옵션)
```

---

## 🎮 클라이언트 통신 예시

### GET /v1/missions/daily
**응답:**
```json
{
  "ok": true,
  "result": "OK",
  "payload": {
    "missions": [
      {
        "mission_id": 1,
        "name": "강화 5회",
        "description": "무기를 강화 5회 수행하기",
        "target": 5,
        "current": 2,
        "reward": 500,
        "completed": false,
        "progress_percentage": 40
      },
      {
        "mission_id": 2,
        "name": "뽑기 3회",
        "current": 0,
        "target": 3,
        "reward": 300,
        "completed": false,
        "progress_percentage": 0
      }
    ],
    "daily_reward_claimed": false,
    "checkin_reward_claimed": true
  }
}
```

### POST /v1/missions/daily/claim
**요청:**
```json
{ "mission_id": 1 }
```

**응답:**
```json
{
  "ok": true,
  "result": "MISSION_CLAIM",
  "payload": {
    "mission_id": 1,
    "reward_gold": 500,
    "gold": 150000
  }
}
```

---

## ⚠️ 중요 고려사항

1. **시간대 처리**
   - 일일 미션은 KST(한국 시간) 기준 자정 초기화
   - 또는 서버 시간대로 통일

2. **트랜잭션 관리**
   - 미션 진행도 업데이트는 게임 로직 실패에 영향 X
   - try-catch로 감싸서 에러 무시

3. **성능 최적화**
   - 매일 모든 유저의 미션 갱신은 배치 처리
   - 또는 첫 로그인 시 자동 갱신

4. **테스트 전략**
   - 시간을 조작하는 테스트 데이터 환경 필요
   - 미션 완료 자동 판정 로직 테스트 필수

---

## 📝 다음 작업 체크리스트

- [ ] schema.sql에 3개 테이블 추가
- [ ] init.js에 마스터 데이터 삽입 로직 추가
- [ ] missionsService.js 확장
- [ ] enchantmentService, shopService 수정 (미션 진행도 업데이트)
- [ ] 테스트 케이스 작성
- [ ] API 문서 업데이트 (API.md)

---

## 💡 추천 구현 방식

**현재 단계**: 옵션 1️⃣ (간단한 방식) + 옵션 B (명시적 클레임)

**이유**:
1. 빠르게 기본 게임 플레이 완성 가능
2. 나중에 복잡한 로직으로 마이그레이션 용이
3. 초기 서비스는 간단할수록 좋음

**나중 개선**: Phase 2에서 옵션 A로 자동화 가능
