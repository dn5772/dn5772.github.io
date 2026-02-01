# Tap Smith 데이터베이스 스키마 문서

## 개요
MySQL/MariaDB 기반 직진형 강화 시스템을 위한 스키마입니다.  
**핵심 설계 원칙**: 마스터 데이터 분리 + 트랜잭션 안전성 + 로그 추적

---

## 테이블 구조

### 1. `users` — 유저 계정 및 재화
**용도**: 유저 기본 정보 및 재화(골드, 영혼의 가루) 관리

| 컬럼 | 타입 | 설명 |
|------|------|------|
| `user_id` | BIGINT UNSIGNED AUTO_INCREMENT | 유저 고유 ID (PK) |
| `nickname` | VARCHAR(32) NOT NULL | 유저 닉네임 |
| `gold` | BIGINT UNSIGNED | 보유 골드 (기본: 0) |
| `soul_dust` | INT UNSIGNED | 영혼의 가루 - 파괴 시 획득 (기본: 0) |
| `max_reached_lv` | INT | 도달한 최고 강화 레벨 (랭킹용) |
| `version` | INT UNSIGNED | 낙관적 락(Optimistic Lock) 버전 관리 |
| `created_at` | DATETIME | 계정 생성 일시 |
| `updated_at` | DATETIME | 마지막 수정 일시 |

**주요 특징**:
- `version` 필드: 동시성 제어 (따닥 방지, 트랜잭션 충돌 감지)
- 골드와 영혼의 가루 분리 관리로 경제 안정성 확보

**인덱스**: PRIMARY KEY (`user_id`)

---

### 2. `master_weapons` — 무기 원본 (마스터 데이터)
**용도**: 게임에서 사용하는 모든 무기의 기본 정보 (읽기 전용)

| 컬럼 | 타입 | 설명 |
|------|------|------|
| `base_id` | INT UNSIGNED AUTO_INCREMENT | 무기 원본 ID (PK) |
| `name` | VARCHAR(32) NOT NULL | 무기 이름 (e.g., "긴 검", "도끼") |
| `rarity` | ENUM | 태생 등급 (COMMON, RARE, EPIC, LEGENDARY) |
| `base_attack` | INT | 기본 공격력 (추후 전투 시스템 확장용) |
| `description` | TEXT | 무기 설명 |
| `icon_res_id` | VARCHAR(64) | 클라이언트 아이콘 리소스 ID |

**주요 특징**:
- 마스터 데이터: 관리자만 수정 가능
- 유저 무기는 이 테이블을 참조하여 중복 제거

**인덱스**: PRIMARY KEY (`base_id`)

---

### 3. `user_weapons` — 유저 무기 인벤토리
**용도**: 유저가 소유한 각 무기 인스턴스 관리

| 컬럼 | 타입 | 설명 |
|------|------|------|
| `weapon_uid` | BIGINT UNSIGNED AUTO_INCREMENT | 무기 인스턴스 고유 ID (PK) |
| `user_id` | BIGINT UNSIGNED NOT NULL | 소유자 유저 ID (FK) |
| `base_id` | INT UNSIGNED NOT NULL | 무기 원본 ID (FK) |
| `current_level` | INT UNSIGNED | 현재 강화 레벨 (기본: 0) |
| `is_locked` | BOOLEAN | 잠금 상태 (TRUE: 판매 불가) |
| `obtained_at` | DATETIME | 획득 일시 |

**주요 특징**:
- `weapon_uid`: 각 무기를 고유하게 추적 (파괴/판매 시 삭제)
- `base_id`: 무기 종류 정보는 마스터에서 조회
- `is_locked`: 중요한 무기 보호 기능

**인덱스**:
- PRIMARY KEY (`weapon_uid`)
- `idx_user_weapons_sort` (`user_id`, `current_level` DESC) — 인벤토리 조회 최적화

**외래키**:
- FK `fk_uw_user` → `users.user_id` (ON DELETE CASCADE)
- FK `fk_uw_base` → `master_weapons.base_id`

---

### 4. `master_enchant_balance` — 강화 밸런스 (마스터 데이터)
**용도**: 레벨별 강화 확률, 비용, 판매가 관리

| 컬럼 | 타입 | 설명 |
|------|------|------|
| `level` | INT NOT NULL | 현재 레벨 (e.g., 0→1, 1→2) (PK) |
| `cost_gold` | BIGINT NOT NULL | 강화 시도 비용 (골드) |
| `prob_success` | INT NOT NULL | 성공 확률 (만분율: 10000 = 100%) |
| `prob_maintain` | INT NOT NULL | 유지 확률 (만분율) |
| `prob_break` | INT NOT NULL | 파괴 확률 (만분율) |
| `sell_price` | BIGINT NOT NULL | 현재 레벨에서의 판매 가격 |
| `dust_gain` | INT NOT NULL | 파괴 시 획득하는 영혼의 가루 양 |

**주요 특징**:
- 만분율 기반 확률 관리 (정수 연산으로 부동소수점 오차 제거)
- `prob_success + prob_maintain + prob_break = 10000`
- `sell_price`: EV (기댓값) × 0.6~0.8 반영
- 강화 중간에 변경해도 로그 추적 가능

**예시 데이터**:
```
level=9  (9→10): cost=4500, success=40%, maintain=60%, break=0%, sell_price=25000, dust=0
level=10 (10→11): cost=7000, success=35%, maintain=60%, break=5%, sell_price=40000, dust=100
```

**인덱스**: PRIMARY KEY (`level`)

---

### 5. `master_grade_visual` — 등급 및 연출 (마스터 데이터)
**용도**: 강화 구간별 칭호, 수식언 풀, 이펙트 관리

| 컬럼 | 타입 | 설명 |
|------|------|------|
| `grade_id` | INT AUTO_INCREMENT | 등급 ID (PK) |
| `min_level` | INT NOT NULL | 구간 시작 레벨 |
| `max_level` | INT NOT NULL | 구간 끝 레벨 |
| `grade_name` | VARCHAR(20) NOT NULL | 등급 명칭 (e.g., "희귀", "영웅") |
| `prefix_pool` | JSON | 수식어 목록 JSON (e.g., ["빛나는", "명장의"]) |
| `visual_effect` | VARCHAR(50) | 클라이언트 이펙트 리소스 키 |

**예시 데이터**:
```
grade_id=1: min_level=0, max_level=4, grade_name="일반", 
            prefix_pool=["녹슨", "평범한", "낡은"],
            visual_effect="effect_none"

grade_id=3: min_level=10, max_level=14, grade_name="희귀",
            prefix_pool=["명장의", "빛나는"],
            visual_effect="effect_blue_glow"
```

**인덱스**: PRIMARY KEY (`grade_id`)

---

### 6. `master_gacha_boxes` — 뽑기 상자 정보 (마스터 데이터)
**용도**: 상점에서 판매하는 상자 종류 및 가격

| 컬럼 | 타입 | 설명 |
|------|------|------|
| `box_id` | INT UNSIGNED AUTO_INCREMENT | 상자 ID (PK) |
| `name` | VARCHAR(32) NOT NULL | 상자 이름 (e.g., "초급 상자", "고급 상자") |
| `price_gold` | BIGINT NOT NULL | 구매 가격 (골드) |
| `description` | VARCHAR(100) | 상자 설명 |
| `is_active` | BOOLEAN | 상점 노출 여부 (TRUE: 노출) |

**예시 데이터**:
```
box_id=1: name="기초 상자", price_gold=1000, is_active=TRUE
box_id=2: name="프리미엄 상자", price_gold=10000, is_active=TRUE
```

**인덱스**: PRIMARY KEY (`box_id`)

---

### 7. `master_gacha_contents` — 상자 구성품 (마스터 데이터)
**용도**: 상자별 획득 가능 무기 및 확률(가중치)

| 컬럼 | 타입 | 설명 |
|------|------|------|
| `id` | INT UNSIGNED AUTO_INCREMENT | 항목 ID (PK) |
| `box_id` | INT UNSIGNED NOT NULL | 상자 ID (FK) |
| `weapon_base_id` | INT UNSIGNED NOT NULL | 나올 무기 ID (FK) |
| `weight` | INT UNSIGNED NOT NULL | 가중치 (확률 계산용) |

**주요 특징**:
- 확률 = `weight / SUM(box의 모든 weight)` × 100%
- 가중치 변경으로 쉽게 확률 조정 가능

**예시 데이터**:
```
box_id=1, weapon_base_id=1, weight=60  (긴 검 60%)
box_id=1, weapon_base_id=2, weight=30  (도끼 30%)
box_id=1, weapon_base_id=3, weight=10  (창 10%)
```

**인덱스**:
- PRIMARY KEY (`id`)
- `idx_gacha_box` (`box_id`) — 상자별 아이템 리스트 조회

**외래키**:
- FK `fk_gc_box` → `master_gacha_boxes.box_id` (ON DELETE CASCADE)
- FK `fk_gc_weapon` → `master_weapons.base_id`

---

### 8. `log_enchant_history` — 강화 기록 (로그)
**용도**: 모든 강화 시도 및 결과 기록 (CS 대응, 공정성 검증)

| 컬럼 | 타입 | 설명 |
|------|------|------|
| `log_id` | BIGINT UNSIGNED AUTO_INCREMENT | 로그 ID (PK) |
| `user_id` | BIGINT UNSIGNED NOT NULL | 강화 시도자 |
| `weapon_uid` | BIGINT UNSIGNED NOT NULL | 대상 무기 UID (참조만, 파괴 시 무기 삭제됨) |
| `before_level` | INT NOT NULL | 시도 전 강화 레벨 |
| `result` | ENUM | 결과 (SUCCESS, FAIL, BREAK) |
| `cost_used` | BIGINT NOT NULL | 사용한 골드 |
| `created_at` | DATETIME | 강화 시도 일시 |

**주요 특징**:
- **불변 기록**: 삽입만 가능, 수정/삭제 불가 (트리거로 보호)
- `weapon_uid`는 ForeignKey 아님 (파괴 시 무기가 삭제되므로)
- CS 분쟁 해결, 부정 행위 감시용

**인덱스**:
- PRIMARY KEY (`log_id`)
- `idx_log_user_date` (`user_id`, `created_at`) — 유저별 최근 로그 조회 최적화

---

## 데이터 흐름 (Core Loop)

### 1️⃣ 무기 구매 (Gacha)
```
master_gacha_boxes/contents (마스터)
    ↓ (가중치 기반 선택)
master_weapons (마스터에서 무기 정보 조회)
    ↓ (새 인스턴스 생성)
user_weapons (유저 인벤토리 추가, level=0)
    ↓ (골드 차감)
users (gold -= price)
```

### 2️⃣ 강화 시도 (Enchant)
```
user_weapons (weapon_uid 선택)
    ↓ (현재 레벨 조회)
master_enchant_balance (확률/비용 조회)
    ↓ (서버에서 난수 생성 및 결과 판정)
    → SUCCESS: user_weapons.level += 1
    → FAIL: 변화 없음
    → BREAK: user_weapons 삭제, users.soul_dust += dust_gain
    ↓ (모든 시도 기록)
log_enchant_history (불변 로그 삽입)
    ↓ (재화 변경)
users (gold -= cost, soul_dust 증가 가능)
```

### 3️⃣ 무기 판매 (Sell)
```
user_weapons (weapon_uid 선택)
    ↓ (현재 레벨 확인)
master_enchant_balance (판매 가격 조회)
    ↓ (무기 삭제)
user_weapons (DELETE WHERE weapon_uid = ?)
    ↓ (골드 지급)
users (gold += sell_price)
```

---

## 설계 원칙

### ✅ 마스터 데이터 분리
- 게임 로직에 영향을 주는 데이터는 `master_*` 테이블에 중앙화
- 강화 확률, 판매가, 무기 정보 등 모두 마스터에서 조회
- **장점**: 운영 중 게임 밸런스 조정 시 DB만 수정 (코드 배포 불필요)

### ✅ 트랜잭션 안전성
- **낙관적 락** (`users.version`): 동시 요청 시 골드 오류 방지
- **Atomicity**: 각 강화 시도는 단일 트랜잭션 (무기 변경 + 골드 차감 + 로그 기록)
- **따닥 방지**: version 체크로 중복 처리 차단

### ✅ 감사 추적 (Audit Log)
- `log_enchant_history`: 모든 강화 시도 기록
- CS 분쟁, 부정 행위 감시에 필수
- 확률 검증: 로그에서 `result` 분포 확인

### ✅ 확장성
- 마스터 데이터 추가 가능 (신규 무기, 상자, 등급)
- 로그 쿼리 성능: 날짜/유저별 인덱싱으로 고속 조회

---

## 주의사항

### ⚠️ 확률 계산 (중요)
```python
# 서버에서 난수 생성 (클라이언트 조작 불가)
random_num = randint(1, 10000)

if random_num <= prob_success:
    result = "SUCCESS"
elif random_num <= prob_success + prob_maintain:
    result = "FAIL"
else:
    result = "BREAK"
```

### ⚠️ 파괴 시 무기 삭제
```sql
-- 무기 삭제 (log_enchant_history는 weapon_uid 참조만)
DELETE FROM user_weapons WHERE weapon_uid = ?;
-- 로그는 유지되어 파괴 기록 남음
```

### ⚠️ 버전 충돌 처리
```sql
-- 강화 비용 차감 시
UPDATE users 
SET gold = gold - ?, version = version + 1 
WHERE user_id = ? AND version = ?;

-- 영향받은 행이 0이면 버전 충돌 → 재시도
```

---

## 향후 확장

### v1.5+ 추가 테이블 (예정)
- `user_stats`: 유저별 통계 (최고 레벨 도달, 누적 파괴 등)
- `master_items`: 영혼의 가루로 구매 가능한 아이템 (확률 부스터)
- `log_gacha_history`: 뽑기 로그 (레어 무기 추적용)
- `log_transaction`: 재화 거래 내역 (감사)

---

## ERD (개념도)

```
┌─────────────────┐
│     users       │
│ (계정 & 재화)   │
└────────┬────────┘
         │ 1:N
         │
    ┌────▼─────────────────┐
    │  user_weapons        │
    │ (인벤토리)           │
    └────┬──────────┬──────┘
         │ N:1      │ N:1
         │          │
    ┌────▼──┐   ┌──▼────────────────┐
    │master │   │master_enchant_    │
    │weapons│   │balance (확률표)   │
    └───────┘   └───────────────────┘
         │
    ┌────▼──────────────────┐
    │master_grade_visual    │
    │(등급 & 연출)          │
    └───────────────────────┘

┌──────────────────────┐
│master_gacha_boxes    │
└──────────┬───────────┘
           │ 1:N
           │
    ┌──────▼──────────────────┐
    │master_gacha_contents    │
    │(상자 구성품)             │
    └──────┬──────────────────┘
           │ N:1
           │
    ┌──────▼──────┐
    │master_      │
    │weapons      │
    └─────────────┘

┌─────────────────────────┐
│log_enchant_history      │
│(강화 로그 - 불변)      │
└─────────────────────────┘
```
