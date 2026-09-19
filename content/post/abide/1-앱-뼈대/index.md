---
title: "ABIDE 개발 일지 #1 - Flutter 앱 뼈대를 한 번에 올렸음"
description: "화면 14개, 커밋 48개. Clean Architecture 3레이어로 시작한 성경 묵상 앱"
slug: "abide-1"
date: 2026-03-10T22:10:00+09:00
draft: false
tags: ["ABIDE", "Flutter", "CleanArchitecture", "Riverpod", "앱개발", "디자인시스템"]
categories: ["app/abide"]
---

# ABIDE 개발 일지 #1 - Flutter 앱 뼈대를 한 번에 올렸음

**ABIDE(말씀 안에 거하다)** — AI랑 같이 성경을 묵상하는 앱을 만들기 시작했음.

요약을 던져주는 게 아니라 **소크라테스식으로 되물으면서** 스스로 생각하게 만드는 게 목표임.
"이 구절이 무슨 뜻이냐"를 알려주는 앱은 이미 많은데, "당신한테 이게 왜 걸리냐"를 묻는 건 없었음.

첫날에 Flutter 앱 뼈대를 통째로 올렸음. 커밋 48개가 전부 `22:09~22:10` 에 찍혀 있음.

## 🧱 3레이어로 갈랐음

Clean Architecture 그대로 감.

```
lib/
├── core/           테마 · 라우터 · API 클라이언트 · 로컬라이제이션 · 공용 위젯
├── domain/         엔티티 + 리포지토리 인터페이스 (추상)
├── data/           데이터소스(local/remote) + 리포지토리 구현
└── presentation/   Riverpod 프로바이더 + 화면
```

**의존 방향이 한쪽임.** `presentation → domain ← data`.
`domain` 은 Flutter도 모르고 HTTP도 모름. 순수 다트 객체랑 인터페이스만 있음.

이렇게 한 이유가 있음. 이 앱은 성경 본문을 **로컬 에셋에서** 읽고, 묵상 대화는 **서버 SSE에서** 받음.
두 데이터소스의 성격이 완전히 다른데, 화면이 그걸 다 알고 있으면 나중에 못 바꿈.

```
domain/repositories/bible_repository.dart      ← 인터페이스
data/repositories/bible_repository_impl.dart   ← 로컬 에셋에서 읽음
data/datasources/local/bible_parser.dart       ← JSON 파싱 + 인덱싱
```

화면은 `BibleRepository` 만 알면 됨. 나중에 본문을 서버에서 받기로 해도 화면은 안 바뀜.

## 🎨 디자인 시스템부터 깔았음

화면보다 테마를 먼저 만들었음. 커밋 순서가 그럼.

```
feat(core/theme): add Deep Forest colour palette
feat(core/theme): add typography scale and text style definitions
feat(core/theme): compose full ThemeData (light/dark) from colours and typography
feat(core/widgets): add shared UI components (glassmorphism cards, tactile buttons, logo)
```

**Deep Forest + 글래스모피즘**으로 잡음. 묵상 앱이라 차분해야 하는데, 그렇다고 밋밋하면 안 씀.
짙은 녹색 계열에 반투명 카드를 얹는 식으로 갔음.

색 → 타이포 → ThemeData → 공용 위젯 순서로 쌓은 게 나중에 편했음.
**화면마다 색을 직접 쓰는 일이 없어서** [#6]({{< ref "/post/abide/6-성경" >}})에서 설정 시트 같은 걸 추가할 때 토큰만 가져다 쓰면 됐음.

## 📱 화면 14개

| 화면 | 하는 일 |
|---|---|
| **Compass** | 홈. 오늘의 기분 기반 구절 추천 + 빠른 진입 |
| **Bible Reader** | 성경 본문. 장 이동, 구절 선택 |
| **Mate (묵상)** | SSE 스트리밍 AI 대화. 소크라테스식 질문 |
| **DeepLens** | 원어 분석, 역사적 배경, 상호참조 |
| **Ask** | 자유 질문. 성경 근거를 붙인 답 |
| **Echo** | 익명 커뮤니티 피드. 묵상 나눔 |
| **Journey** | 묵상 캘린더 + 월별 통계 |
| **Profile** | 연속 기록, 설정, 테마, 언어 |

여기에 스플래시 · 온보딩 · 로그인 · 프로필 설정 · 하단 탭 셸까지.

## 🌊 SSE 클라이언트를 처음부터 넣었음

```
feat(data): add SSE client for real-time AI response streaming
feat(presentation): add meditation session provider (SSE stream lifecycle management)
```

묵상 대화가 이 앱의 핵심인데, **한 번에 답이 다 나오면 안 됨.**
AI가 생각하는 동안 화면이 멈춰 있으면 대화가 아니라 로딩임.

그래서 처음부터 Server-Sent Events로 잡았음. Dio 기반으로 직접 만들었고, 스트림 생명주기(구독·취소·에러·재연결)는 Riverpod 프로바이더가 들고 있게 함.

이게 나중에 [#7]({{< ref "/post/abide/7-스트리밍" >}})에서 크게 확장됨.

## 🌏 로컬라이제이션도 첫날에

```
feat(core): add Korean/English localisation string maps and locale provider
```

한국어가 1차인데 영어 문자열 맵을 같이 깔아뒀음.
나중에 붙이려면 화면 14개를 다 뒤져야 하니까, 비어 있더라도 구조는 먼저 만들어두는 게 나음.

…라고 생각했는데 **이게 절반만 맞았음.** 앱 UI는 이걸로 됐는데, **AI가 생성하는 텍스트**는 전혀 대비가 안 돼 있었음.
그 대가를 [#8]({{< ref "/post/abide/8-다국어" >}})에서 치름.

## 🛡️ 전역 에러 바운더리

```
feat: add main.dart entrypoint with global error boundary and app bootstrap
feat(core): add centralised error handler and user feedback utilities
```

`main.dart` 에서 에러를 한 군데로 모음. 네트워크 실패든 파싱 실패든 사용자한테 보이는 건 한 형식.

묵상 중에 빨간 Flutter 에러 화면이 뜨면 그날 묵상은 끝임. 이건 기능이 아니라 **분위기 문제**라서 처음부터 넣었음.

## 🎯 느낀 점

48커밋을 한 번에 올린 건데, **커밋을 잘게 쪼갠 게 나중에 읽기 좋았음.**
`feat(core/theme)`, `feat(domain)`, `feat(data)`, `feat(ui)` 로 레이어를 접두사에 박아두니까 나중에 "이때 뭘 깔았지"가 로그만 봐도 보임.

그리고 **화면보다 테마랑 라우터를 먼저 만든 순서**가 맞았음.
반대로 갔으면 화면 14개에 색이랑 네비게이션이 제각각 박혔을 것.

다음은 서버임. 앱은 있는데 부를 데가 없음.

#ABIDE #Flutter #CleanArchitecture #Riverpod #앱개발
