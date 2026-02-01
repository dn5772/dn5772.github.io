+++
title = "Tap Smith (탭 스미스) — 게임 개발 기획서 (v1.4)"
categories = ["game-dev/tap-smith"]
date = 2026-02-01T00:00:00Z
draft = false
tags = ["게임개발", "강화시스템", "시뮬레이션", "RPG", "밸런싱", "Tap Smith"]
+++

# Tap Smith (탭 스미스) — 게임 개발 기획서 (v1.4)

## 1. 개요
- **장르**: 하드코어 강화 시뮬레이션 / 방치형(Idle) RPG 매니지먼트
- **핵심 목표**: 실패 시 하락 없는 '직진형(No Back)' 강화 시스템에서, 파괴의 공포를 이겨내고 최고의 무기를 만들어 비싸게 파는 것.
- **개발 단계**: MVP (핵심 루프 구현 및 밸런싱)

---

## 2. 핵심 메커니즘: 직진형 강화 (Enchantment Logic)
**"뒤로 가는 건 없다. 성공하거나, 제자리거나, 혹은 사라지거나."**

| 결과 | 현상 | 상태 변화 | 비고 |
|------|------|-----------|------|
| **성공 (Success)** | 강화 성공 | Level +1 | 가치 상승, 수식언 변경 |
| **실패 (Fail)** | 강화 실패 | Level 유지 | 골드만 소모 |
| **파괴 (Break)** | 무기 소멸 | Weapon 삭제 | '영혼의 가루' 획득 (구제책) |

---

## 3. 게임 루프 (Core Loop)
MVP 버전에서는 전투 콘텐츠를 제외하고 **경제 순환**에 집중합니다.

1. **제작/구매**: 기본 무기를 상자에서 확률로 뽑기
2. **강화 (Gamble)**: 골드를 소모하여 강화 시도.
3. **의사 결정 (The Choice)**:
   - **Stop (판매)**: 현재 단계에서 만족하고 판매하여 확정 골드 획득. (다음 강화를 위한 시드머니 확보)
   - **Go (도전)**: 상위 등급을 노리고 강화 버튼 클릭. (성공 시 가치 폭등, 파괴 시 원금 손실)
4. **재투자**: 판매한 골드로 더 많은 무기를 사거나, 가루를 모아 확률 보조제 구매.

---

## 4. 밸런스 및 경제 시스템 (상세)

### 4.1. 기댓값(Expected Value) 산출 공식
각 강화 단계의 가치는 **"파괴 시 0강부터 다시 복구해야 하는 리스크 비용"**을 포함하여 계산합니다.

$$EV_{next} = EV_{current} + \frac{Cost + (P_{break} \times EV_{current})}{P_{success}}$$

이 공식을 기반으로 판매 가격은 `EV * (0.6 ~ 0.8)` 수준으로 책정하여, 유저가 **"직접 띄울까, 그냥 살까"**를 고민하게 만듭니다.

### 4.2. 구간별 상세 테이블
*(테스트 단계에서는 모든 수치를 조정 가능하도록 Config화 필수)*

#### **Phase 1: 안전 성장기 (+0 ~ +9)**
**특징**: 파괴 없음. 클릭하는 재미와 빠른 성장 학습.

| 레벨 | 비용(Gold) | 성공(%) | 유지(%) | 파괴(%) | 판매가(Gold) | 상태 |
|------|-----------|---------|---------|---------|-------------|------|
| 0→1 | 100 | 100% | 0% | 0% | 200 | [일반] |
| 1→2 | 150 | 100% | 0% | 0% | 400 | |
| 2→3 | 250 | 90% | 10% | 0% | 800 | |
| 3→4 | 400 | 80% | 20% | 0% | 1,500 | |
| 4→5 | 600 | 70% | 30% | 0% | 2,500 | [준수] 진입 |
| ... | ... | ... | ... | ... | ... | |
| 9→10 | 4,500 | 40% | 60% | 0% | 25,000 | |

#### **Phase 2: 리스크 도입기 (+10 ~ +19)**
**특징**: 파괴 확률 등장. "여기서 팔까?" 하는 유혹 시작.

| 레벨 | 비용(Gold) | 성공(%) | 유지(%) | 파괴(%) | 판매가(Gold) | 상태 |
|------|-----------|---------|---------|---------|-------------|------|
| 10→11 | 7,000 | 35% | 60% | 5% | 40,000 | [희귀] |
| 11→12 | 10,000 | 30% | 60% | 10% | 65,000 | |
| 14→15 | 30,000 | 20% | 60% | 20% | 250,000 | [장인] 진입 |
| 19→20 | 160,000 | 10% | 50% | 40% | 3,000,000 | 대박 구간 |

#### **Phase 3: 하드코어 (+20 ~ +29)**
**특징**: 파괴 확률 > 성공 확률. 무기가 삭제될 확률이 더 높음. 가격이 기하급수적으로 폭등.

| 레벨 | 비용(Gold) | 성공(%) | 유지(%) | 파괴(%) | 판매가(Gold) | 상태 |
|------|-----------|---------|---------|---------|-------------|------|
| 20→21 | 250,000 | 8% | 42% | 50% | 6,000,000 | [영웅] |
| 21→22 | 350,000 | 7% | 43% | 50% | 12,000,000 | |
| 24→25 | 1,000,000 | 5% | 35% | 60% | 1억 | [고대] 진입 |
| 29→30 | 10,000,000 | 1% | 29% | 70% | 30억 | 서버 알림 |

---

## 5. 수식언 및 연출 시스템 (Prefix)
강화 단계에 따라 무기의 **이름**과 **프로필 이펙트**가 변경됩니다.

| 구간 | 등급 | 수식언 예시 | 시각 효과 (Visual) |
|------|------|------------|-------------------|
| +0 ~ +4 | 일반 | 녹슨, 평범한, 낡은 | 없음 |
| +5 ~ +9 | 준수 | 날카로운, 튼튼한 | 흰색 테두리 |
| +10 ~ +14 | 희귀 | 명장의, 빛나는 | 푸른색 잔상 |
| +15 ~ +19 | 장인 | 불꽃의, 냉기의 | 파티클 이펙트 (약) |
| +20 ~ +24 | 영웅 | 파괴의, 영혼의 | 보라색 오라 (강) |
| +25 ~ +29 | 고대 | 심연의, 천상의 | 검은색 연기/암전 |
| +30 ~ | 전설 | 용살자의, 세계를 먹어치운 | 화면 전체 진동 및 황금빛 |

---

## 6. 개발 및 구현 스펙 (Technical)

### 6.1. 데이터베이스 (Schema Summary)

#### **Users (유저 정보)**
```sql
CREATE TABLE users (
  id INT PRIMARY KEY AUTO_INCREMENT,
  telegram_id BIGINT UNIQUE NOT NULL COMMENT 'Telegram User ID',
  username VARCHAR(100) COMMENT 'Telegram 사용자명',
  gold BIGINT DEFAULT 10000 COMMENT '보유 골드 (시작: 10,000)',
  soul_dust INT DEFAULT 0 COMMENT '영혼의 가루 (파괴 시 획득)',
  highest_level INT DEFAULT 0 COMMENT '역대 최고 강화 레벨 (업적/탑 등반용)',
  total_enchants INT DEFAULT 0 COMMENT '총 강화 시도 횟수',
  total_breaks INT DEFAULT 0 COMMENT '총 파괴 횟수',
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  INDEX idx_telegram_id (telegram_id),
  INDEX idx_highest_level (highest_level)
) COMMENT='유저 기본 정보 및 재화 관리';
```

#### **Weapons (무기 정보)**
```sql
CREATE TABLE weapons (
  id INT PRIMARY KEY AUTO_INCREMENT,
  user_id INT NOT NULL COMMENT '소유자',
  weapon_type ENUM('SWORD', 'AXE', 'SPEAR', 'BOW') DEFAULT 'SWORD' COMMENT '무기 종류',
  level INT DEFAULT 0 COMMENT '현재 강화 레벨',
  prefix VARCHAR(50) COMMENT '수식언 (녹슨, 빛나는, 용살자의 등)',
  status ENUM('ACTIVE', 'SOLD', 'BROKEN') DEFAULT 'ACTIVE' COMMENT '무기 상태',
  base_cost INT NOT NULL COMMENT '구매 원가 (환불/보상 계산용)',
  total_invested BIGINT DEFAULT 0 COMMENT '총 투자 골드 (구매비 + 강화비)',
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  sold_at TIMESTAMP NULL COMMENT '판매 시각',
  FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
  INDEX idx_user_status (user_id, status),
  INDEX idx_level (level)
) COMMENT='무기 인벤토리 및 상태 관리';
```

#### **Enchant_Logs (강화 기록)**
```sql
CREATE TABLE enchant_logs (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  user_id INT NOT NULL COMMENT '강화 시도자',
  weapon_id INT NOT NULL COMMENT '대상 무기',
  level_before INT NOT NULL COMMENT '강화 전 레벨',
  level_after INT NOT NULL COMMENT '강화 후 레벨',
  result ENUM('SUCCESS', 'FAIL', 'BREAK') NOT NULL COMMENT '결과',
  cost BIGINT NOT NULL COMMENT '소모된 골드',
  success_rate DECIMAL(5,2) COMMENT '성공 확률 (%)',
  break_rate DECIMAL(5,2) COMMENT '파괴 확률 (%)',
  random_seed VARCHAR(100) COMMENT '난수 시드 (검증용)',
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
  INDEX idx_user_time (user_id, created_at),
  INDEX idx_weapon (weapon_id),
  INDEX idx_result (result)
) COMMENT='강화 시도 이력 (CS 및 통계 분석용)';
```

#### **Enchant_Config (강화 확률 설정)**
```sql
CREATE TABLE enchant_config (
  level INT PRIMARY KEY COMMENT '강화 레벨 (0→1, 1→2, ...)',
  cost BIGINT NOT NULL COMMENT '강화 비용 (Gold)',
  success_rate DECIMAL(5,2) NOT NULL COMMENT '성공 확률 (%)',
  fail_rate DECIMAL(5,2) NOT NULL COMMENT '유지 확률 (%)',
  break_rate DECIMAL(5,2) NOT NULL COMMENT '파괴 확률 (%)',
  sell_price BIGINT NOT NULL COMMENT '판매 가격',
  tier ENUM('COMMON', 'DECENT', 'RARE', 'MASTER', 'HERO', 'ANCIENT', 'LEGENDARY') COMMENT '등급',
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  CHECK (success_rate + fail_rate + break_rate = 100)
) COMMENT='레벨별 강화 확률 및 가격표 (운영 조정 가능)';
```

#### **Transactions (재화 거래 내역)**
```sql
CREATE TABLE transactions (
  id BIGINT PRIMARY KEY AUTO_INCREMENT,
  user_id INT NOT NULL COMMENT '유저',
  type ENUM('WEAPON_BUY', 'WEAPON_SELL', 'ENCHANT', 'ITEM_BUY') NOT NULL COMMENT '거래 유형',
  gold_change BIGINT NOT NULL COMMENT '골드 증감 (+ or -)',
  gold_after BIGINT NOT NULL COMMENT '거래 후 골드',
  reference_id INT COMMENT '관련 무기/아이템 ID',
  description VARCHAR(255) COMMENT '거래 설명',
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
  INDEX idx_user_time (user_id, created_at),
  INDEX idx_type (type)
) COMMENT='골드 거래 내역 (감사 추적용)';
```

#### **Prefixes (수식언 풀)**
```sql
CREATE TABLE prefixes (
  id INT PRIMARY KEY AUTO_INCREMENT,
  tier ENUM('COMMON', 'DECENT', 'RARE', 'MASTER', 'HERO', 'ANCIENT', 'LEGENDARY') NOT NULL,
  prefix VARCHAR(50) NOT NULL COMMENT '수식언 텍스트',
  weight INT DEFAULT 1 COMMENT '추첨 가중치 (높을수록 자주 등장)',
  UNIQUE KEY uk_tier_prefix (tier, prefix)
) COMMENT='등급별 수식언 데이터';
```

### 6.2. 핵심 API (Endpoints)

#### **인증 (Authentication)**
- `POST /auth/telegram` - Telegram OAuth 로그인/회원가입
  - **Request**: `{ init_data: string }`
  - **Response**: `{ user: { id, username, gold }, token: string }`

#### **무기 관리 (Weapons)**
- `GET /weapons` - 보유 중인 무기 목록 조회 (ACTIVE만)
  - **Response**: `{ weapons: [{ id, type, level, prefix, sell_price }] }`

- `POST /weapons/buy` - 새 무기 구매 (0강)
  - **Request**: `{ weapon_type: 'SWORD' | 'AXE' | 'SPEAR' | 'BOW' }`
  - **Response**: `{ weapon: { id, level: 0, cost }, user: { gold } }`
  - **Error**: `{ error: 'INSUFFICIENT_GOLD' }` (골드 부족 시)

- `POST /weapons/:id/enchant` - **핵심 로직: 강화 시도**
  - **Request**: `{ weapon_id: number }`
  - **Response**:
    ```json
    {
      "result": "SUCCESS" | "FAIL" | "BREAK",
      "weapon": {
        "id": 123,
        "level_before": 10,
        "level_after": 11,
        "prefix": "빛나는",
        "sell_price": 65000
      },
      "user": {
        "gold": 950000,
        "soul_dust": 0
      },
      "log": {
        "success_rate": 30.00,
        "break_rate": 10.00,
        "cost": 10000
      }
    }
    ```
  - **Error Cases**:
    - `{ error: 'WEAPON_NOT_FOUND' }`
    - `{ error: 'INSUFFICIENT_GOLD' }`
    - `{ error: 'MAX_LEVEL_REACHED' }`

- `POST /weapons/:id/sell` - 무기 판매
  - **Response**: `{ gold_gained: 65000, user: { gold } }`

#### **경제 (Economy)**
- `GET /economy/config` - 전체 강화 확률표 조회 (클라이언트 UI용)
  - **Response**: `{ levels: [{ level, cost, success_rate, sell_price, tier }] }`

- `GET /economy/stats` - 유저 통계
  - **Response**:
    ```json
    {
      "total_enchants": 1523,
      "total_breaks": 42,
      "highest_level": 18,
      "break_rate": 2.76,
      "total_invested": 5420000,
      "total_earned": 8200000,
      "net_profit": 2780000
    }
    ```

#### **랭킹 (Leaderboard)**
- `GET /leaderboard/highest` - 최고 레벨 달성 랭킹 (Top 100)
  - **Response**: `{ rankings: [{ rank, username, level, created_at }] }`

- `GET /leaderboard/gold` - 보유 골드 랭킹
  - **Response**: `{ rankings: [{ rank, username, gold }] }`

### 6.3. 보안 고려사항
- **Server Authority**: 모든 확률 계산과 재화 처리는 반드시 서버에서 수행.
- **Concurrency**: 따닥(Double Tap) 방지를 위한 트랜잭션 처리 필수.

---

## 7. 업데이트 로드맵 (Post-MVP)
현재 단계에서는 포함하지 않으나, 확장성을 위해 미리 기획된 콘텐츠입니다.

- **무한의 탑 (v1.5 예정)**: 보유한 무기의 '최고 레벨'을 전투력으로 환산하여 등반. 방치형 보상 획득.
- **보스 레이드 (v1.6 예정)**: 클릭커 요소 결합. 제한 시간 내 타격(탭)하여 보스 처치.
- **무기 파견 (v1.7 예정)**: 사용하지 않는 고강화 무기를 파견 보내 재료 수급.