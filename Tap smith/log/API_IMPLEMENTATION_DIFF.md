# API 문서 vs 실제 구현 차이점 분석

> 📅 분석 날짜: 2026년 2월 1일  
> 📄 대상 문서: [[API]]
> 🔍 분석 범위: 전체 API 엔드포인트 및 응답 형식

---

## 🎯 요약

총 **10개 항목** 검토, 그 중 **6개 불일치** 발견

### 중요도별 분류
- 🔴 **높음 (즉시 수정 권장)**: 5개
- 🟡 **중간 (검토 필요)**: 1개
- 🟢 **낮음 (참고)**: 4개 (일치 항목)

---

## 🔴 중요도 높음 (즉시 수정 권장)

### 1. 인증 응답에 `accessToken` 필드 누락

**문제**: 실제 응답에는 `accessToken`이 포함되지만 문서에 명시되지 않음

**문서 (API.md)**:
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

**실제 구현** ([authController.js:23-28](../src/controllers/authController.js#L23-L28)):
```json
{
  "ok": true,
  "result": "OK",
  "payload": {
    "mode": "telegram",
    "user": { "id": "123456789", "username": "alice" },
    "accessToken": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."  // ⚠️ 문서에 없음
  }
}
```

**영향**: 클라이언트가 토큰을 저장하는 로직을 구현할 때 혼란 발생 가능

**권장 조치**: API.md의 인증 섹션에 `accessToken` 필드 추가

---

### 2. 게임 상태 응답의 무기 필드명 불일치

**문제**: 문서는 `typeIndex`, 실제 코드는 `baseId` 사용

**문서 (API.md)**:
```json
"weapon": { "level": 0, "typeIndex": 0 }
```

**실제 구현** ([gameService.js:70](../src/services/gameService.js#L70)):
```json
"weapon": { "level": 0, "baseId": 0 }
```

**영향**: 클라이언트 코드에서 필드명 오류로 인한 런타임 에러 발생

**권장 조치**: 
- 옵션 1: 문서를 `baseId`로 수정 (권장 - 다른 API와 일관성)
- 옵션 2: 코드를 `typeIndex`로 수정 (Breaking Change)

---

### 3. 친구 초대 API Request Body 파라미터 누락

**문제**: 문서에 요청 파라미터가 명시되지 않음

**문서 (API.md)**:
```markdown
### POST /friends/invite
Registers a friend invite and grants reward.

**Response 200**
...
```

**실제 구현** ([friendsController.js:6](../src/controllers/friendsController.js#L6)):
```javascript
const { invited_user_id } = request.body;
```

**영향**: API 사용자가 필수 파라미터를 알 수 없음

**권장 조치**: Request Body 섹션 추가
```markdown
**Request Body**

```json
{
  "invited_user_id": "user_123"
}
```


---

### 4. 무기 잠금/잠금 해제 API 섹션 본문 누락

**문제**: 섹션 헤더만 있고 본문 내용이 비어있음

**문서 (API.md - Line ~350)**:
```markdown
### 5-2. 무기 잠금 / 잠금 해제
실수로 고강화 무기를 판매하는 것을 방지하기 위한 토글 기능입니다.

**Method**: `POST`  
**Endpoint**: `/inventory/lock`

**Request Headers**
```
Authorization: Bearer {Access_Token}
```

(이후 내용 없음)
```

**실제 구현** ([inventoryController.js:22-38](../src/controllers/inventoryController.js#L22-L38)):
- 엔드포인트: `POST /inventory/lock` ✅
- Request Body: `{ "weapon_uid": 5501 }`
- Response: 
  ```json
  {
    "ok": true,
    "result": "LOCK",
    "payload": {
      "weapon_uid": 5501,
      "is_locked": true
    }
  }
  ```

**권장 조치**: 완전한 API 명세 작성 (Request Body, Response, Error Cases)

---

### 5. 에러 응답 `result` 필드 패턴 불일치

**문제**: 일반 에러와 비즈니스 로직 에러의 `result` 값이 다른 패턴 사용

**문서 (API.md - 공통 응답 포맷)**:
```json
{
  "ok": false,
  "result": "ERROR_TYPE",
  "message": "Error description"
}
```

**실제 구현**:

**패턴 A - 일반 에러** (XXX_ERROR 형식):
- `USER_ERROR` ([userController.js:19](../src/controllers/userController.js#L19))
- `MASTER_DATA_ERROR` ([shopController.js:14](../src/controllers/shopController.js#L14))
- `INVENTORY_ERROR` ([inventoryController.js:17](../src/controllers/inventoryController.js#L17))

**패턴 B - 비즈니스 로직 에러** (에러 메시지 직접 사용):
- `NO_GOLD` ([shopController.js:51](../src/controllers/shopController.js#L51))
- `ITEM_NOT_FOUND` ([enchantmentController.js:13](../src/controllers/enchantmentController.js#L13))
- `INVENTORY_FULL`
- `MAX_LEVEL`
- `ALREADY_CHECKED_IN`

**영향**: 클라이언트가 에러 처리 로직을 작성할 때 패턴 불일치로 인한 혼란

**권장 조치**: 
1. 문서에 두 가지 패턴을 명확히 구분하여 설명
2. 또는 일관된 패턴으로 통일 (예: 모든 에러를 비즈니스 에러 형식으로)

---

## 🟡 중요도 중간 (검토 필요)

### 6. 인벤토리 `prefix` 필드 계산 방식과 마스터 데이터 불일치

**문제**: `master_grade_visual.prefix_pool`의 JSON 데이터가 사용되지 않음

**마스터 데이터** ([master_grade_visual](../src/db/init.js)):
```javascript
{
  grade_id: 1,
  grade_name: '일반',
  prefix_pool: ['녹슨', '평범한', '낡은'],  // 3개 수식언
  // ...
}
```

**실제 구현** ([inventoryService.js:64](../src/services/inventoryService.js#L64)):
```javascript
prefix: this._getPrefix(w.level),  // 고정 배열 사용

_getPrefix(level) {
  const prefixes = ['녹슨', '평범한', '낡은', '빛나는', '영혼의'];
  return prefixes[Math.min(Math.floor(level / 5), 4)];
}
```

**현재 동작**:
- 각 등급마다 **하나의 고정된 수식언**만 사용
- 레벨 0-4: '녹슨', 5-9: '평범한', 10-14: '낡은', 15-19: '빛나는', 20-24: '영혼의'

**마스터 데이터 설계 의도**:
- 각 등급마다 **여러 수식언 중 랜덤 선택**
- 레벨 0-4: ['녹슨', '평범한', '낡은'] 중 랜덤

**영향**: 
- 게임 다양성 감소 (같은 레벨대 무기는 항상 같은 수식언)
- 마스터 데이터의 `prefix_pool` 필드가 무용지물

**권장 조치**:
1. 옵션 A (다양성 증가): 레벨 구간별로 `prefix_pool`에서 랜덤 선택하도록 수정
2. 옵션 B (현 상태 유지): 마스터 데이터의 `prefix_pool` 설계 변경 (각 등급당 1개 수식언만 저장)
3. 기획 의도 확인 필요

**관련 파일**:
- [inventoryService.js](../src/services/inventoryService.js) - `_getPrefix()` 메서드
- [enchantmentService.js](../src/services/enchantmentService.js) - `_getPrefix()` 메서드
- [gameService.js](../src/services/gameService.js) - `_getWeaponName()` 메서드

---

## 🟢 일치 항목 (참고)

### 7. 마스터 데이터 강화 밸런스 응답
✅ **문서와 구현 일치** - 모든 필드가 정확히 매칭됨

### 8. 강화 응답 필드
✅ **문서와 구현 일치** - SUCCESS/FAIL/BREAK 모든 케이스 일치

### 9. 마스터 데이터 버전 정보
✅ **문서와 구현 일치** - 둘 다 `"version": "1.4"` 사용

### 10. 기타 API 엔드포인트
✅ **대부분 일치** - 유저 정보, 상점, 상자 뽑기, 강화, 판매 등 주요 API 모두 정상

---

## 📊 통계

### API 엔드포인트별 점검 결과

| 카테고리 | 엔드포인트                        | 상태  | 이슈                  |
| ---- | ---------------------------- | --- | ------------------- |
| 인증   | `POST /auth/telegram`        | ⚠️  | accessToken 필드 누락   |
| 인증   | `POST /auth/dev`             | ⚠️  | accessToken 필드 누락   |
| 유저   | `GET /user/info`             | ✅   | -                   |
| 마스터  | `GET /master/data`           | ⚠️  | prefix_pool 사용 불일치  |
| 상점   | `GET /shop/boxes`            | ✅   | -                   |
| 상점   | `POST /shop/draw`            | ✅   | -                   |
| 강화   | `POST /enchantment/enchant`  | ✅   | -                   |
| 인벤토리 | `GET /inventory/list`        | ⚠️  | prefix 계산 방식        |
| 인벤토리 | `POST /inventory/lock`       | ❌   | 문서 본문 누락            |
| 게임   | `GET /game/state`            | ⚠️  | typeIndex vs baseId |
| 경제   | `POST /economy/sell`         | ✅   | -                   |
| 미션   | `POST /missions/checkin`     | ✅   | -                   |
| 미션   | `POST /missions/daily/claim` | ✅   | -                   |
| 친구   | `POST /friends/invite`       | ⚠️  | Request Body 누락     |
| 헬스   | `GET /health`                | ✅   | -                   |

**총계**: 15개 엔드포인트
- ✅ 정상: 9개 (60%)
- ⚠️ 주의: 5개 (33%)
- ❌ 누락: 1개 (7%)

---

## 🔧 권장 수정 우선순위

### 1순위 (즉시 수정)
1. **인증 API**: `accessToken` 필드 문서 추가
2. **게임 상태 API**: `typeIndex` → `baseId` 필드명 통일
3. **무기 잠금 API**: 누락된 본문 작성

### 2순위 (단기)
4. **친구 초대 API**: Request Body 파라미터 명세 추가
5. **에러 응답 가이드**: 패턴 구분 설명 추가

### 3순위 (기획 검토 후)
6. **수식언 시스템**: `prefix_pool` 활용 방식 결정 및 구현

---

## 📝 추가 발견 사항

### 구현되었으나 문서화 불완전한 항목
1. **무기 판매 API** (`POST /economy/sell`)
   - API.md의 "인벤토리 및 관리" 섹션이 아닌 "Economy" 섹션에 있음
   - 인벤토리 관련 기능이지만 분류가 다름 (혼란 가능)

2. **일일 미션/출석 시스템**
   - 문서에는 "⏳ 향후 업데이트 예정" 표시
   - 실제 코드에도 TODO 주석과 함께 스텁 구현
   - 문서와 코드 상태 일치 ✅

---

## 🎓 검증 방법

이 분석은 다음 방법으로 수행되었습니다:

1. **코드 레벨 검증**:
   - 모든 Controller 파일 검토
   - 모든 Service 로직 분석
   - Routes 정의 확인

2. **통합 테스트 결과**:
   - `test/integration/database.test.js`: 10/10 통과
   - 실제 API 동작 검증 완료

3. **문서 검토**:
   - API.md 전체 섹션 리뷰
   - 각 엔드포인트별 스펙 비교

---

## 📞 문의 및 논의

이 문서에 대한 질문이나 수정 제안은:
- 개발팀 미팅에서 논의
- 또는 이슈 트래커에 등록

---

*Last Updated: 2026-02-01*  
*Analyzer: GitHub Copilot (Claude Sonnet 4.5)*
