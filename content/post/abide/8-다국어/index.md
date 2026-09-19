---
title: "ABIDE 개발 일지 #8 - AI가 생성하는 말은 번역이 안 됨"
description: "언어 파라미터를 DB부터 프롬프트까지 관통시킨 하루. 그리고 한 달 회고"
slug: "abide-8"
date: 2026-04-10T19:21:00+09:00
draft: false
tags: ["ABIDE", "다국어", "i18n", "회고", "LangGraph", "Flutter"]
categories: ["app/abide"]
---

# ABIDE 개발 일지 #8 - AI가 생성하는 말은 번역이 안 됨

[#1]({{< ref "/post/abide/1-앱-뼈대" >}}) 첫날에 한국어/영어 문자열 맵을 깔아뒀음. 나중에 고생 안 하려고.

**절반만 맞았음.**

버튼이랑 라벨은 그걸로 됐는데, 이 앱에서 사용자가 보는 텍스트의 대부분은 **AI가 그 자리에서 만들어내는 것**임.
묵상 질문, 딥렌즈 분석, 묵상 노트, 에러 메시지, 무드 문구. 문자열 맵에 있을 리가 없음.

4월 10일 하루에 이걸 전부 뚫었음.

## 🧵 언어가 통과해야 하는 경로

```
Flutter 설정
  → Core API (preferred_language)
    → AI 서버 요청 스키마
      → MeditationState
        → LangGraph 노드들
          → 프롬프트 선택
            → 생성된 텍스트
```

커밋이 이 순서 그대로임.

```
00:02  (infra)  feat: add language column to deep_lens_cache for i18n support
00:42  (ai)     feat(ai): add language field to schemas and MeditationState
01:19  (ai)     feat(ai): propagate language parameter through meditation graph
01:50  (ai)     feat(ai): bilingual error messages, mood messages, and note title
01:52  (ai)     feat(ai): use language-aware prompt helpers for ask and deep-lens
01:58  (core)   feat(core): inject preferred_language into AI server requests
01:59  (flutter)feat(flutter): sync language change to user profile and bible version
02:25  (ai)     fix(ai): make DeepLens cache language-aware
18:57  (ai)     fix(ai): remove remaining hardcoded Korean strings
19:21  (core)   fix(core): include verse_text in DeepLens cache hit response
```

**서브모듈 네 개를 한 번에 건드림.** [#4]({{< ref "/post/abide/4-인증" >}})의 세션 스냅샷 때랑 같은 모양인데, 이번엔 더 넓음.

## 🗄️ 캐시가 언어를 몰랐음

02:25 커밋이 이 작업에서 제일 아팠음.

[#7]({{< ref "/post/abide/7-스트리밍" >}})에서 DeepLens 구절 **62,189개를 미리 계산해서** DB에 박아뒀음.
그 캐시 키에 **언어가 없었음.**

```
전:  verse_ref                  → 분석 결과
후:  verse_ref + language       → 분석 결과
```

영어 사용자가 구절을 열면 **한국어 분석이 그대로 나옴.** 캐시는 히트하니까 에러도 안 남.

인프라에 컬럼을 추가하고(`00:02`) 조회 키를 바꿨음. [#7]({{< ref "/post/abide/7-스트리밍" >}})에서 자랑했던 사전 계산이 **언어 하나 늘자 절반만 쓸모 있게** 된 셈임.

**캐시 키는 "결과가 달라지는 모든 입력"을 담아야** 하는데, 그때는 언어가 입력인 줄 몰랐음.

## 🇰🇷 하드코딩된 한국어가 계속 나왔음

01:50에 에러 메시지·무드 문구·노트 제목을 이중화했는데, 그날 저녁에 이게 또 나옴.

```
18:57  fix(ai): remove remaining hardcoded Korean strings
```

17시간 뒤에 **남은 게 또 발견됨.**

노드가 일곱 개고([#3]({{< ref "/post/abide/3-에이전트" >}})) 각각이 폴백 문구, 기본 제목, 예외 메시지를 들고 있음.
한 군데씩 찾아서 고치는 방식이라 **놓친 게 있는지 알 방법이 없었음.**

문자열 맵을 처음부터 서버에도 뒀으면 "맵에 없는 리터럴"을 린트로 잡을 수 있었을 텐데, 사후에 하려니까 눈으로 찾는 수밖에 없었음.

## 🔤 프롬프트도 언어별로

```
03-27 16:41  feat: DEEP_LENS_PROMPT_EN 추가 및 성경 구절 분석 가이드라인 정의
04-10 01:52  feat(ai): use language-aware prompt helpers for ask and deep-lens
```

**"영어로 답해줘"를 프롬프트에 한 줄 붙이는 걸로는 안 됐음.**

딥렌즈는 원어(히브리어/헬라어) 분석이 들어가는데, 한국어 독자용 설명이랑 영어 독자용 설명이 **내용 자체가 달라야** 함.
한국어 성경 번역 관습을 설명하는 부분이 영어 사용자한테는 의미가 없음.

그래서 프롬프트를 통째로 따로 만들고, 언어에 따라 고르는 헬퍼를 씀.

## 📊 한 달, 커밋 181개

```
apps/abide       84커밋  (Flutter)
servers/ai       29커밋  (Python/FastAPI)
servers/core     11커밋  (Rust/Axum)
apps/admin-web   10커밋  (Next.js)
servers/admin     5커밋
infra             5커밋
the_seeker       37커밋  (오케스트레이션)
                ─────
                181커밋   3/10 ~ 4/10
```

루트 37커밋 중 **15개가 "Sync all submodules"** 임.

## 💭 배운 것

### 다국어는 UI 문자열이 아니라 데이터 파이프라인임

[#1]({{< ref "/post/abide/1-앱-뼈대" >}})에서 문자열 맵을 먼저 깐 건 잘한 건데, **그게 다국어의 10%였음.**

나머지 90%는 언어 값이 Flutter → Core → AI → 그래프 상태 → 프롬프트 → 캐시 키까지 **끊기지 않고 흘러가게** 만드는 일이었음.
중간에 한 군데만 빠져도 [#7]({{< ref "/post/abide/7-스트리밍" >}})의 캐시처럼 조용히 틀린 결과가 나옴.

다시 한다면 **요청 스키마에 `language` 를 첫날에 넣었을 것.** 값이 하나뿐이더라도.

### LLM 판정에는 코드 가드가 붙어야 함

[#3]({{< ref "/post/abide/3-에이전트" >}})의 wrap_up 무한 반복 카운터, [#7]({{< ref "/post/abide/7-스트리밍" >}})의 Observer 상한·델타 캡·단조성·거절 가드.
전부 같은 얘기임. **흐름은 LLM이 잡아도 "그만"과 "한계"는 코드가 세야 함.**

### "일단 되게"는 반드시 다시 옴

SharedPreferences → secure storage([#4]({{< ref "/post/abide/4-인증" >}})), `.env` 커밋 → gitignore, 릴리즈 빌드 디버그 로그([#6]({{< ref "/post/abide/6-성경" >}})).
[정담]({{< ref "/post/정담/7-보안" >}}) 때랑 완전히 같은 패턴이 다른 프로젝트에서 또 나왔음.

### 옵션 하나가 조합을 곱으로 늘림

[#6]({{< ref "/post/abide/6-성경" >}})의 성경 버전 추가가 UI 버그 11개를, 여기 언어 추가가 캐시 재설계를 불렀음.
**기능 추가 비용은 그 기능이 아니라 기존 것과의 곱에서 나옴.**

## 🕳️ 안 끝낸 것들

- **신학 RAG 시딩 미완료.** 청크 129개는 만들었는데 `seed_theology_vectors.py` 를 안 씀. [#3]({{< ref "/post/abide/3-에이전트" >}})의 RAG가 반쪽임
- **DeepLens 사전 계산이 KRV+NIV 2개 버전**뿐. [#6]({{< ref "/post/abide/6-성경" >}})에서 KJV/ASV를 넣었는데 캐시는 안 따라감
- 스크린샷이 README에 아직 "Coming soon"

## 🎯 지금

4월 10일 이후로 멈춰 있음.

[seam]({{< ref "/post/seam/8-마무리" >}})이 사흘 몰아치고 멈춘 거였다면, 이건 한 달을 붙들고 **실제로 돌아가는 데까지** 갔음.
앱이 있고, 서버 네 개가 뜨고, 62,189개 구절이 DB에 들어 있고, 로그인해서 묵상을 할 수 있음.

돌아온다면 신학 RAG 시딩부터 할 것 같음. [#3]({{< ref "/post/abide/3-에이전트" >}})에서 만들다 만 게 제일 걸림.

#ABIDE #다국어 #i18n #회고 #LangGraph
