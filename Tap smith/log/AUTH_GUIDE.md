# Tap Smith 백엔드 인증 가이드

## 개요

백엔드는 **API 문서 기준**으로 구현되었습니다. 클라이언트(프론트엔드)는 이 문서의 API 사양을 따라 개발하면 됩니다.

## 인증 흐름

### 1. 초기 접속 (최초 1회)

클라이언트가 Telegram WebApp에서 실행될 때:

```javascript
// 클라이언트 (JavaScript/React)
const initData = window.Telegram.WebApp.initData;

const response = await fetch('http://localhost:3000/v1/auth/telegram', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ initData })
});

const { payload } = await response.json();
const { accessToken, user } = payload;

// 토큰 저장 (localStorage 또는 상태관리)
localStorage.setItem('accessToken', accessToken);
```

### 2. 이후 모든 API 요청

Authorization 헤더에 토큰을 포함:

```javascript
// 클라이언트
const accessToken = localStorage.getItem('accessToken');

const response = await fetch('http://localhost:3000/v1/user/info', {
  method: 'GET',
  headers: {
    'Content-Type': 'application/json',
    'Authorization': `Bearer ${accessToken}`
  }
});

const data = await response.json();
```

## API 엔드포인트

### POST /v1/auth/telegram
Telegram WebApp initData를 검증하고 Access Token을 발급합니다.

**Request**
```bash
curl -X POST http://localhost:3000/v1/auth/telegram \
  -H "Content-Type: application/json" \
  -d '{"initData":"query_id=123&user=%7B%22id%22%3A456%7D&auth_date=1234567890&hash=abcd1234..."}'
```

**Response (Success)**
```json
{
  "ok": true,
  "result": "OK",
  "payload": {
    "mode": "telegram",
    "user": {
      "id": "123456789",
      "username": "john_doe",
      "firstName": "John",
      "lastName": "Doe"
    },
    "accessToken": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
  }
}
```

**Response (Error)**
```json
{
  "ok": false,
  "result": "AUTH_ERROR",
  "message": "Telegram verification failed: Invalid initData: hash verification failed"
}
```

### POST /v1/auth/dev
개발 환경에서 테스트용 토큰을 발급합니다. (`ALLOW_INSECURE_USER_ID_HEADER=true`일 때만 가능)

**Request**
```bash
curl -X POST http://localhost:3000/v1/auth/dev \
  -H "Content-Type: application/json" \
  -d '{"userId":"dev_user_1"}'
```

**Response (Success)**
```json
{
  "ok": true,
  "result": "OK",
  "payload": {
    "mode": "dev",
    "user": {
      "id": "dev_user_1"
    },
    "accessToken": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
  }
}
```

### Bearer Token 검증

모든 API 요청(인증 제외)은 다음 형식의 Authorization 헤더가 필수입니다:

```
Authorization: Bearer <accessToken>
```

**유효하지 않은 경우 (401 Unauthorized)**
```json
{
  "ok": false,
  "result": "UNAUTHORIZED",
  "message": "Token verification failed: Invalid token signature"
}
```

## 환경변수 설정

`.env` 파일에 다음을 설정해야 합니다:

```env
# Telegram Bot Token
TELEGRAM_BOT_TOKEN=your_bot_token_from_botfather

# JWT Secret Key (강력한 무작위 문자열 권장)
JWT_SECRET=your_super_secret_jwt_key_that_is_very_long_and_random

# 개발 모드 활성화 (dev 엔드포인트 사용 가능)
ALLOW_INSECURE_USER_ID_HEADER=true
```

## JWT 토큰 구조

토큰은 표준 JWT(JSON Web Token) 형식입니다:

```
Header.Payload.Signature
```

**Payload 예시**
```json
{
  "userId": "123456789",
  "mode": "telegram",
  "iat": 1703080000,
  "exp": 1705758400
}
```

- `userId`: 사용자 고유 ID
- `mode`: 인증 방식 (telegram 또는 dev)
- `iat`: 토큰 발급 시간 (Unix timestamp)
- `exp`: 토큰 만료 시간 (Unix timestamp) - 발급 후 30일

## 개발 모드 (테스트)

개발할 때는 Telegram 없이 테스트할 수 있습니다:

```bash
# .env 설정
ALLOW_INSECURE_USER_ID_HEADER=true

# 테스트 로그인
curl -X POST http://localhost:3000/v1/auth/dev \
  -H "Content-Type: application/json" \
  -d '{"userId":"test_user"}'
```

응답에서 받은 `accessToken`으로 모든 API를 테스트할 수 있습니다.

## 보안 고려사항

### ✅ 클라이언트
- 토큰을 안전하게 저장 (HttpOnly 쿠키 권장, localStorage는 XSS에 취약)
- 모든 API 요청에 Authorization 헤더 포함
- 토큰 만료 시 재인증 처리

### ✅ 서버
- JWT_SECRET을 환경변수로 관리 (코드에 하드코딩 금지)
- Telegram Bot Token 안전하게 보관
- HTTPS 사용 권장 (프로덕션)
- 토큰 서명 검증 필수

### ⚠️ 주의사항
- `ALLOW_INSECURE_USER_ID_HEADER=true`는 개발 환경에서만 사용
- 프로덕션에서는 반드시 `false`로 설정
- Telegram Bot Token이 유출되면 즉시 변경

## 오류 처리

### 인증 오류 (400 Bad Request)
```json
{
  "ok": false,
  "result": "AUTH_ERROR",
  "message": "Telegram verification failed: ..."
}
```

**원인**:
- initData 누락
- initData 형식 오류
- hash 검증 실패
- user 데이터 누락

### 인증 헤더 오류 (401 Unauthorized)
```json
{
  "ok": false,
  "result": "UNAUTHORIZED",
  "message": "Authorization header is required"
}
```

**원인**:
- Authorization 헤더 누락
- Bearer 형식 오류
- 토큰 서명 검증 실패
- 토큰 만료

## 테스트

### 단위 테스트
```bash
npm test -- test/auth.service.test.js
```

### 통합 테스트
```bash
npm test -- test/auth.integration.test.js
```

### 전체 테스트
```bash
npm test
```

## 구현 체크리스트

클라이언트 개발 시:

- [ ] Telegram WebApp initData 수집
- [ ] `/v1/auth/telegram` 호출하여 accessToken 획득
- [ ] accessToken 로컬 저장
- [ ] 모든 API 요청에 `Authorization: Bearer {token}` 헤더 추가
- [ ] 401 응답 시 재인증 처리
- [ ] 토큰 만료 시간 확인 및 갱신 로직 (필요시)

## FAQ

### Q: 토큰이 만료되면 어떻게 하나요?
A: 재인증이 필요합니다. `/v1/auth/dev` (개발) 또는 `/v1/auth/telegram` (프로덕션)를 다시 호출하여 새 토큰을 발급받으세요.

### Q: 토큰을 어디에 저장해야 하나요?
A: HttpOnly 쿠키 (가장 안전) > 메모리 상태 > localStorage (XSS 취약)

### Q: Telegram initData는 어디서 얻나요?
A: `window.Telegram.WebApp.initData` (클라이언트 코드 참조)

### Q: 프로덕션에서도 /auth/dev를 사용할 수 있나요?
A: 아니요. `ALLOW_INSECURE_USER_ID_HEADER=false`로 설정하면 자동으로 차단됩니다.

---

**API 문서**: [docs/API.md](../docs/API.md)
**스키마 문서**: [docs/schema.md](../docs/schema.md)
