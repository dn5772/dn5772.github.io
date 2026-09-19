---
title: "ABIDE 개발 일지 #4 - 토큰을 SharedPreferences에서 꺼냈음"
description: "Google/Apple 로그인, secure storage 마이그레이션, FCM 푸시, 세션 상태 스냅샷"
slug: "abide-4"
date: 2026-03-21T18:16:00+09:00
draft: false
tags: ["ABIDE", "Flutter", "인증", "OAuth", "FCM", "보안"]
categories: ["app/abide"]
---

# ABIDE 개발 일지 #4 - 토큰을 SharedPreferences에서 꺼냈음

[#3]({{< ref "/post/abide/3-에이전트" >}})까지 오면 대화는 되는데, **누가 쓰는지를 모른다.**
3월 15일~21일은 인증이랑 세션을 붙인 기간이다.

## Google / Apple 로그인

```
feat: integrate Google Sign-In with serverClientId and handle new user flow
```

`serverClientId` 가 핵심이다. 이걸 안 넘기면 클라이언트용 토큰만 나와서 **서버가 검증을 못 한다.**
서버에서 구글에 물어보려면 서버 클라이언트 ID로 발급된 ID 토큰이 필요하다.

그리고 [#2]({{< ref "/post/abide/2-백엔드" >}})의 Core 서버에 플래그 하나를 추가했다.

```
Add is_new_user flag to AuthTokenResponse and include oauth_provider in UserProfileResponse
```

처음 가입한 사람이면 **프로필 설정 화면으로** 보내고, 기존 유저면 바로 홈으로 보내야 한다.
앱이 그걸 알 방법이 없어서 로그인 응답에 `is_new_user` 를 얹었다.

클라이언트가 "유저 정보를 한 번 더 조회해서 프로필이 비었는지 보는" 식으로 짜면 왕복이 한 번 더 생기고, 그 사이에 화면이 깜빡이다.

## 토큰을 secure storage로 옮김

15일 마지막 커밋이 이거다.

```
feat: Phase 4-A migrate authentication tokens to secure storage and refactor related logic
```

처음엔 JWT를 `SharedPreferences` 에 넣었다. 쉽고 빠르니까.

근데 `SharedPreferences` 는 **그냥 평문 파일**이다. 안드로이드는 XML, iOS는 plist.
루팅/탈옥된 기기나 백업 파일에서 그대로 읽힌다.

`flutter_secure_storage` 로 옮겼다. 안드로이드는 Keystore, iOS는 Keychain에 들어간다.

[wallet]({{< ref "/post/정담/7-보안" >}}) 때도 그랬는데. **"일단 되게" 하려고 고른 저장소는 거의 항상 나중에 옮기게 된다.**
처음부터 secure storage를 썼어도 코드량 차이는 거의 없었다.

## FCM 푸시

```
feat: Phase 3-E implement FCM service for push notifications and enhance notification settings
```

묵상 앱은 **안 열면 안 쓰게 된다.** 매일 알림이 리텐션의 거의 전부다.

Core 서버에도 FCM 클라이언트를 붙였다.

```
환경 변수 및 미들웨어 개선: FCM 클라이언트 추가, Rate Limiter 구현,
AppConfig의 민감 정보 마스킹, .env 파일 수정 및 .gitignore 업데이트
```

이 커밋에 세 가지가 같이 들어갔는데 전부 운영 준비다.

- **Rate Limiter**. 인증 엔드포인트가 무방비였다
- **민감 정보 마스킹**. 설정을 로그에 찍을 때 키가 그대로 나오고 있었다
- **`.gitignore`**. `.env` 랑 firebase 서비스 계정 키

마지막 게 제일 급했다. 26일에 한 번 더 손본다.

```
chore: ignore .env and firebase service account key
```

**[정담]({{< ref "/post/정담/7-보안" >}})에서 배운 걸 또 반복했다.** 개발 초반에 "일단 돌아가게" 넣은 파일들이 저장소에 들어가 있었다.

## 세션을 DB에 스냅샷으로

21일 커밋이 이 시리즈에서 제일 중요한 것 중 하나다.

```
(ai)    feat: VerseFinder 노드 추가 및 구절 탐색 기능 구현; 세션 상태 DB 저장 및 복원 로직 개선
(core)  세션 생성 요청에 session_type 및 initial_query 필드 추가
(infra) ai_sessions 테이블에 세션 상태 스냅샷 컬럼 및 인덱스 추가
```

서브모듈 셋이 **같은 시각에 같이 움직이다.** 하나의 기능이 세 레포에 걸쳐 있다.

### 왜 스냅샷이 필요했나

묵상 대화는 **길고, 중간에 끊긴다.** 지하철에서 하다가 내리고, 저녁에 다시 열는다.

[#3]({{< ref "/post/abide/3-에이전트" >}})의 LangGraph 상태(지금 어느 노드인지, 깊이 점수가 얼만지, 어떤 구절을 보고 있는지)가 메모리에만 있으면 **서버가 재시작되는 순간 날아간다.**
사용자는 처음부터 다시 해야 한다.

그래서 상태를 통째로 DB 컬럼에 넣고, 이어서 열면 복원하게 했다.

[seam]({{< ref "/post/seam/7-hitl" >}})의 세션 영속이랑 같은 문제인데, 여기는 **모바일이라 훨씬 자주 끊긴다.**

### VerseFinder

`session_type` 이랑 `initial_query` 가 같이 들어간 이유다.

"요즘 불안한데 읽을 구절 있을까" 같은 요청은 묵상 대화가 아니라 **구절 찾기**이다.
같은 그래프인데 진입점이 다르다. 세션 타입으로 갈래를 나눴다.

다음 날 이걸 한 번 더 손본다.

```
세션 생성 시 verse_finder 타입을 meditation으로 변환하여 DB에 저장
```

**타입이 두 개인 건 진입 시점뿐**이고, 구절을 찾고 나면 그냥 묵상으로 이어진다.
DB에는 `meditation` 으로 통일해서 저장하게 바꿨다. 안 그러면 Journey 통계에서 같은 활동이 둘로 갈라진다.

## 느낀 점

**"일단 되게" 저장소는 반드시 다시 옮기게 된다.** SharedPreferences → secure storage, `.env` 커밋 → gitignore.
둘 다 처음부터 제대로 했어도 드는 시간이 비슷했다.

그리고 21일의 3-레포 동시 커밋이 서브모레포의 실제 모습이었다.
기능 하나가 Flutter·Core·AI·infra를 다 건드리는데, **커밋은 네 군데에 따로 남는다.** [#2]({{< ref "/post/abide/2-백엔드" >}})에서 적은 포인터 관리 비용이 여기서 체감된다.

다음은 어드민이다.
