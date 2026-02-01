# Tap Smith API Documentation (v1.4)

## 목차
1. [개요 및 공통 사항](#개요-및-공통-사항)
2. [공통 응답 포맷](#공통-응답-포맷)
3. [유저 및 초기화](#유저-및-초기화)
4. [상점 및 뽑기](#상점-및-뽑기)
5. [강화 (Core Loop)](#강화-core-loop)
6. [인벤토리 및 관리](#인벤토리-및-관리)
7. [에러 코드](#에러-코드-정의)

---

## 개요 및 공통 사항

### Base URL
```
https://api.tapsmith.com/v1  (프로덕션)
http://localhost:3000/v1     (개발)
```

### Protocol & Format
- **Protocol**: HTTP/1.1 (HTTPS 권장)
- **Data Format**: JSON
- **Character Set**: UTF-8

### 인증 방식
모든 API 요청 헤더에 다음을 포함해야 합니다:
```
Authorization: Bearer {Access_Token}
```

**토큰 획득 방법**:
최초 접속시에
- Telegram WebApp `initData`를 `/auth/telegram`으로 검증 후 **서버가 Access Token 발급**
- 또는 개발 환경에서 `/auth/dev`로 테스트 토큰 발급

---

## 공통 응답 포맷

### 성공 응답 (State)
```json
{
  "ok": true,
  "result": "OK",
  "state": { /* user state */ },
  "computed": { /* computed balance */ }
}
```

### 성공 응답 (Action)
```json
{
  "ok": true,
  "result": "ACTION",
  "payload": { /* action-specific fields */ }
}
```

### 에러 응답
```json
{
  "ok": false,
  "result": "ERROR_TYPE",
  "message": "Error description",
  "payload": { /* optional context */ }
}
```

### 응답 필드 설명
| 필드 | 타입 | 설명 |
|------|------|------|
| `ok` | BOOLEAN | 성공 여부 |
| `result` | STRING | 결과 코드 (OK/ACTION/ERROR_TYPE 등) |
| `state` | OBJECT | 상태 응답에서만 포함 |
| `computed` | OBJECT | 상태 응답에서만 포함 |
| `payload` | OBJECT | 액션/에러 응답에서 포함 가능 |
| `message` | STRING | 에러 설명 또는 참고 메시지 |

---

## 유저 및 초기화

### 2-1. 유저 정보 조회
게임 접속 시 유저의 재화 및 상태를 동기화합니다.

**Method**: `GET`  
**Endpoint**: `/user/info`

**Request Headers**
```
Authorization: Bearer {Access_Token}
```

**Request Body**
```
(없음 - Header Only)
```

**Response 200 (Success)**
```json
{
  "ok": true,
  "result": "OK",
  "payload": {
    "user_id": 1001,
    "nickname": "SmithMaster",
    "gold": 50000,           // 현재 보유 골드
    "soul_dust": 120,        // 보유 영혼의 가루
    "max_reached_lv": 25     // 최고 기록 레벨 (랭킹용)
  }
}
```

**Notes**
- `gold`, `soul_dust`는 게임 진행도 초기화의 핵심
- `max_reached_lv`는 이전 세션에서 달성한 최고 레벨 추적

---

### 2-2. 마스터 데이터 조회 (선택)
게임 밸런스 데이터(확률, 비용, 무기 정보, 등급)를 클라이언트에 캐싱하기 위해 호출합니다.  
*실제로는 CDN 정적 파일이나 별도 버전 관리 시스템 사용 권장*

**Method**: `GET`  
**Endpoint**: `/master/data`

**Request Headers**
```
Authorization: Bearer {Access_Token}
```

**Response 200 (Success)**
```json
{
  "ok": true,
  "result": "OK",
  "payload": {
    "version": "1.4",
    "weapons": [
      { "base_id": 1, "name": "롱소드", "rarity": "COMMON", "base_attack": 10, "icon_res_id": "sword_01" },
      { "base_id": 2, "name": "도끼", "rarity": "COMMON", "base_attack": 12, "icon_res_id": "axe_01" }
    ],
    "enchant_balance": [
      {
        "level": 0,
        "cost_gold": 100,
        "prob_success": 10000,
        "prob_maintain": 0,
        "prob_break": 0,
        "sell_price": 200,
        "dust_gain": 0
      },
      {
        "level": 10,
        "cost_gold": 7000,
        "prob_success": 3500,
        "prob_maintain": 6000,
        "prob_break": 500,
        "sell_price": 40000,
        "dust_gain": 100
      }
    ],
    "grades": [
      {
        "grade_id": 1,
        "min_level": 0,
        "max_level": 4,
        "grade_name": "일반",
        "prefix_pool": ["녹슨", "평범한", "낡은"],
        "visual_effect": "effect_none"
      }
    ],
    "gacha_boxes": [
      { "box_id": 1, "name": "낡은 나무 상자", "price_gold": 1000, "description": "일반 등급 무기", "is_active": true }
    ]
  }
}
```

**Notes**
- 대용량 데이터이므로 클라이언트에서 로컬 캐싱 권장
- 버전 변경 시에만 재요청하여 네트워크 효율성 확보

---

## 상점 및 뽑기

### 3-1. 상자 목록 조회
구매 가능한 무기 뽑기 상자 목록을 불러옵니다.

**Method**: `GET`  
**Endpoint**: `/shop/boxes`

**Request Headers**
```
Authorization: Bearer {Access_Token}
```

**Response 200 (Success)**
```json
{
  "ok": true,
  "result": "OK",
  "payload": {
    "boxes": [
      {
        "box_id": 1,
        "name": "낡은 나무 상자",
        "price_gold": 1000,
        "description": "일반 등급 무기가 주로 나옵니다."
      },
      {
        "box_id": 2,
        "name": "빛나는 강철 상자",
        "price_gold": 50000,
        "description": "희귀 등급 이상을 노려보세요."
      }
    ]
  }
}
```

---

### 3-2. 상자 뽑기 (무기 획득)
골드를 소모하여 확률에 따라 무기(0강)를 획득합니다.

**Method**: `POST`  
**Endpoint**: `/shop/draw`

**Request Headers**
```
Authorization: Bearer {Access_Token}
Content-Type: application/json
```

**Request Body**
```json
{
  "box_id": 1
}
```

**Response 200 (Success)**
```json
{
  "ok": true,
  "result": "DRAW",
  "payload": {
    "result": "SUCCESS",
    "earned_weapon": {
      "weapon_uid": 5501,     // [중요] 새로 발급된 인벤토리 고유 ID
      "base_id": 3,           // 무기 종류 ID (Master 참조)
      "name": "롱소드",
      "rarity": "COMMON",
      "level": 0
    },
    "updated_gold": 49000     // 차감 후 잔액
  }
}
```

**Error Cases**

| Result | Message | 설명 |
|--------|---------|------|
| NO_GOLD | Not enough gold | 골드가 부족하여 상자를 구매할 수 없음 |
| INVENTORY_FULL | Inventory full | 인벤토리 슬롯이 가득 찼음 (최대 보유 무기 수 초과) |

**Notes**
- `weapon_uid`: 각 무기의 고유 ID (강화/판매 시 이 ID 사용)
- `base_id`: 마스터 데이터의 무기 종류를 나타냄
- 모든 무기는 0강으로 획득

---

## 강화 (Core Loop)

### 4-1. 강화 시도
**게임의 핵심 로직입니다.** 성공/유지/파괴 결과가 서버에서 난수를 기반으로 결정됩니다.

**Method**: `POST`  
**Endpoint**: `/enchantment/enchant`

**Request Headers**
```
Authorization: Bearer {Access_Token}
Content-Type: application/json
```

**Request Body**
```json
{
  "weapon_uid": 5501
}
```

**Response 200 (Success)**
```json
{
  "ok": true,
  "result": "ENCHANT",
  "payload": {
    "result": "SUCCESS",      // SUCCESS(성공), FAIL(유지), BREAK(파괴) 중 하나
    "weapon_uid": 5501,
    "old_level": 9,
    "new_level": 10,          // 결과 레벨 (파괴 시 null)
    "cost_used": 4500,        // 소모된 골드
    "updated_gold": 44500,    // 강화 후 잔액
    "reward_dust": 0,         // 파괴되었을 경우 획득한 영혼의 가루 양 (성공/유지 시 0)
    "new_prefix": "빛나는"    // 새 레벨에 맞는 수식언 (성공 시만 반환)
  }
}
```

**Result 상세 설명**

| Result | 상태 | 레벨 변화 | 비고 |
|--------|------|-----------|------|
| **SUCCESS** | 강화 성공 | +1 | 새 수식언 적용, UI 업데이트 |
| **FAIL** | 강화 실패 | 유지 | 골드만 소모, 레벨 변화 없음 |
| **BREAK** | 무기 파괴 | 삭제 | weapon_uid 무효화, soul_dust 획득 |

**Client 처리 로직**
```javascript
switch(response.payload.result) {
  case "SUCCESS":
    // 레벨업 연출 (파티클, 사운드)
    // UI: 레벨 표시 업데이트, 새 수식언 표시
    break;
  case "FAIL":
    // 실패 연출 (화면 흔들림)
    // UI: 레벨 유지 표시
    break;
  case "BREAK":
    // 파괴 연출 (유리 깨지는 이펙트)
    // UI: 인벤토리에서 무기 삭제 처리
    // UI: 영혼의 가루 획득 팝업
    break;
}
```

**Error Cases**

| Result | Message | 설명 |
|--------|---------|------|
| NO_GOLD | Not enough gold | 강화 비용을 감당할 골드 부족 |
| ITEM_NOT_FOUND | Item not found | 해당 weapon_uid가 존재하지 않음 (이미 파괴/판매됨 또는 따닥) |
| MAX_LEVEL | Max level reached | 더 이상 강화할 수 없는 최고 레벨에 도달 |
| SERVER_ERROR | Server error | 서버 내부 오류 (트랜잭션 실패 등) |

**Security Notes**
- ✅ **서버 권한**: 모든 확률 계산 및 난수 생성은 반드시 서버에서 수행
- ✅ **따닥 방지**: weapon_uid 검증 + 버전 체크로 중복 요청 방지
- ✅ **감시 로그**: 모든 강화 시도는 `log_enchant_history` 테이블에 기록됨 (CS 대응용)

---

## 인벤토리 및 관리

### 5-1. 보유 무기 목록 조회
현재 보유 중인 모든 무기를 조회합니다.

**Method**: `GET`  
**Endpoint**: `/inventory/list`

**Request Headers**
```
Authorization: Bearer {Access_Token}
```

**Query Parameters** (선택)
```
?sort=level_desc      // level_desc, level_asc, time_asc, time_desc
&filter=locked        // locked, unlocked, all (기본값: all)
```

**Response 200 (Success)**
```json
{
  "ok": true,
  "result": "OK",
  "payload": {
    "weapons": [
      {
        "weapon_uid": 5501,
        "base_id": 3,
        "level": 10,
        "is_locked": false,
        "name": "롱소드",
        "rarity": "COMMON",
        "prefix": "빛나는",
        "sell_price": 65000,
        "obtained_at": "2025-01-15T10:30:00Z"
      },
      {
        "weapon_uid": 5420,
        "base_id": 10,
        "level": 21,
        "is_locked": true,
        "name": "드래곤 슬레이어",
        "rarity": "LEGENDARY",
        "prefix": "영혼의",
        "sell_price": 12000000,
        "obtained_at": "2025-01-10T08:15:00Z"
      }
    ],
    "inventory_count": 2,
    "inventory_max": 50
  }
}
```

**Notes**
- `weapon_uid`: 강화/판매 시 사용할 고유 ID
- `sell_price`: 현재 레벨에 따른 판매 가격 (마스터 데이터 기반)
- `inventory_max`: 향후 용량 시스템 추가 시 참고용

---

### 5-2. 무기 잠금 / 잠금 해제
실수로 고강화 무기를 판매하는 것을 방지하기 위한 토글 기능입니다.

**Method**: `POST`  
**Endpoint**: `/inventory/lock`

**Request Headers**
```
Authorization: Bearer {Access_Token}
```

---

## 인증

### POST /auth/telegram
Verifies Telegram WebApp `initData` and returns the Telegram user id.

**Body**
```json
{ "initData": "<telegram init data>" }
```

**Response 200**
```json
{
  "ok": true,
  "result": "OK",
  "payload": {
    "mode": "telegram",
    "user": { "id": "123456789", "username": "alice" }
  }
}
```

### POST /auth/dev
Dev-only login using a local user id (requires `ALLOW_INSECURE_USER_ID_HEADER=true`).

**Body**
```json
{ "userId": "dev_user_1" }
```

**Response 200**
```json
{
  "ok": true,
  "result": "OK",
  "payload": {
    "mode": "dev",
    "user": { "id": "dev_user_1" }
  }
}
```

---

## Game State

### GET /game/state
Returns current user state and computed balance data.

**Response 200**
```json
{
  "ok": true,
  "result": "OK",
  "state": {
    "gold": 0,
    "best": 0,
    "soulDust": 0,
    "weapon": { "level": 0, "typeIndex": 0 },
    "invites": 0,
    "dailyMissionProgress": 0,
    "dailyMissionCompleted": false,
    "lastCheckinDate": null,
    "updatedAt": 0
  },
  "computed": {
    "weaponName": "Worn +0 Sword",
    "enchantCost": 18,
    "sellPrice": 12,
    "chances": { "success": 1, "fail": 0, "destroy": 0 },
    "canCheckin": true
  }
}
```

---

## Economy

### POST /economy/sell
Sells the current weapon and issues a new base weapon.

**Response 200**
```json
{
  "ok": true,
  "result": "SELL",
  "payload": {
    "soldPrice": 120,
    "goldDelta": 120,
    "gold": 220,
    "weapon": { "level": 0, "typeIndex": 2 }
  }
}
```

---

## Missions

### POST /missions/checkin
Claims daily check-in reward.

**Response 200**
```json
{
  "ok": true,
  "result": "CHECKIN",
  "payload": { "goldDelta": 250, "gold": 500 }
}
```

---

### POST /missions/daily/claim
Claims daily mission reward when completed.

**Response 200**
```json
{
  "ok": true,
  "result": "DAILY_CLAIM",
  "payload": { "goldDelta": 500, "gold": 1000 }
}
```

---

## Friends

### POST /friends/invite
Registers a friend invite and grants reward.

**Response 200**
```json
{
  "ok": true,
  "result": "INVITE",
  "payload": { "goldDelta": 120, "gold": 620, "invites": 1 }
}
```

---

## Health Check

### GET /health
Health check.

**Response 200**
```json
{ "status": "ok" }
```

---

## 에러 코드 정의

| Result | Message | 설명 |
|--------|---------|------|
| NO_GOLD | Not enough gold | 골드가 부족함 |
| INVENTORY_FULL | Inventory full | 인벤토리 슬롯이 가득 찼음 |
| ITEM_NOT_FOUND | Item not found | 대상 무기를 찾을 수 없음 |
| MAX_LEVEL | Max level reached | 더 이상 강화할 수 없는 최고 레벨 |
| ALREADY_CHECKED_IN | Already checked in today | 오늘은 이미 출석 완료 |
| DAILY_NOT_READY | Daily mission not completed | 일일 미션이 완료되지 않음 |
| SERVER_ERROR | Server error | 서버 내부 오류 |

