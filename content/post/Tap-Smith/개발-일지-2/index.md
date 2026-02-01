+++
title = "Tap Smith 개발 일지 #2 - 백엔드 인증 시스템 구현"
date = 2026-02-03T00:00:00Z
draft = false
tags = ["게임개발", "Tap Smith", "인증", "백엔드", "Telegram", "JWT"]
categories = ["game-dev/tap-smith"]
+++

# Tap Smith 개발 일지 #2 - 백엔드 인증 시스템 구현

요즘 Tap Smith 백엔드 인증 시스템 구현 완료했음.
Telegram Mini App이라 그런지 인증 방식이 좀 독특한데, 그 과정이랑 코드 예시 공유해봄!

## 🔐 인증 방식 개요

Tap Smith은 Telegram WebApp으로 실행되니까, Telegram의 OAuth를 활용했음.
사용자가 Telegram으로 로그인하면 별도의 절차 없이 자동으로 게임에 접속할 수 있도록 설계함.

백엔드는 일단 API 문서 기준으로 만들고, 프론트엔드는 그걸 따라오도록 개발하고 있음.

## 🚀 인증 흐름

### 1. 첫 접속 (한 번만)

Telegram WebApp에서 앱이 실행될 때:

```javascript
// 프론트엔드 (React/JS)
const initData = window.Telegram.WebApp.initData;

const response = await fetch('http://localhost:3000/v1/auth/telegram', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ initData })
});

const { payload } = await response.json();
const { accessToken, user } = payload;

// 토큰 저장 (localStorage나 상태 관리에)
localStorage.setItem('accessToken', accessToken);
```

### 2. 그 다음부터 모든 API 요청

Authorization 헤더에 토큰 넣어서 보내면 됨:

```javascript
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

## 📋 주요 API 엔드포인트

### POST /v1/auth/telegram
- Telegram 인증 수행
- Request: `{ initData: string }`
- Response: `{ ok: true, payload: { accessToken, user } }`

### GET /v1/user/info  
- 사용자 정보 조회
- Headers: `Authorization: Bearer <token>`

### POST /v1/auth/refresh
- 토큰 만료 시 갱신용

## 🔧 구현 팁

1. **토큰 만료 처리**: JWT 토큰 만료되면 자동으로 refresh 해주는 로직 추가함.
2. **보안**: initData 검증은 필수. Telegram에서 제공하는 해시 검증 로직 직접 구현했음.
3. **에러 처리**: 토큰이 없거나 잘못된 경우에 대한 예외 처리는 꼼꼼하게.

## 🎯 느낀 점

Telegram Mini App 인증은 처음 해봤는데 생각보다 간단해서 놀람.
그래도 역시 문서화가 제일 중요하다는 걸 다시 한번 느낌..
담번엔 프론트엔드 연동하면서 겪은 일들 더 자세히 써보겠음!

#TapSmith #백엔드 #인증 #Telegram #JWT